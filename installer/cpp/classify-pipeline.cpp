#include "classify-pipeline.hpp"
#include "url-helpers.hpp"
#include "icon-manager.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>

namespace qapp::installer {

static const QStringList kSwPaths = {
    QStringLiteral("/sw.js"),
    QStringLiteral("/service-worker.js"),
    QStringLiteral("/serviceworker.js"),
};

static const QStringList kFaviconPaths = {
    QStringLiteral("/favicon.ico"),
    QStringLiteral("/apple-touch-icon.png"),
};

static constexpr int kTimeoutMs = 10000;
static constexpr int kProbeTimeoutMs = 5000;

QJsonObject ClassifyResult::toJson() const
{
    QJsonObject classification;
    classification[QStringLiteral("level")] = level;
    classification[QStringLiteral("hasManifest")] = hasManifest;
    classification[QStringLiteral("hasServiceWorker")] = hasServiceWorker;

    QJsonObject metadata;
    metadata[QStringLiteral("name")] = name;
    metadata[QStringLiteral("iconUrl")] = iconUrl;
    metadata[QStringLiteral("displayMode")] = displayMode;
    metadata[QStringLiteral("themeColor")] = themeColor;
    metadata[QStringLiteral("backgroundColor")] = backgroundColor;
    metadata[QStringLiteral("startUrl")] = startUrl;
    metadata[QStringLiteral("scope")] = scope;
    if (!manifestId.isEmpty())
        metadata[QStringLiteral("manifestId")] = manifestId;
    if (!displayOverride.isEmpty()) {
        QJsonArray doArr;
        for (const auto &m : displayOverride) doArr.append(m);
        metadata[QStringLiteral("displayOverride")] = doArr;
    }
    if (!shortcuts.isEmpty())
        metadata[QStringLiteral("shortcuts")] = shortcuts;
    if (!mimeTypes.isEmpty()) {
        QJsonArray mtArr;
        for (const auto &m : mimeTypes) mtArr.append(m);
        metadata[QStringLiteral("mimeTypes")] = mtArr;
    }
    if (!protocolHandlers.isEmpty())
        metadata[QStringLiteral("protocolHandlers")] = protocolHandlers;

    QJsonObject root;
    root[QStringLiteral("classification")] = classification;
    root[QStringLiteral("metadata")] = metadata;
    root[QStringLiteral("finalUrl")] = finalUrl;
    return root;
}

ClassifyPipeline::ClassifyPipeline(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void ClassifyPipeline::start(const QUrl &url)
{
    m_url = url;

    if (!qapp::UrlHelpers::isValidUrl(url.toString())) {
        emit failed(QStringLiteral("Invalid URL"));
        return;
    }

    emit statusChanged(QStringLiteral("Fetching page..."));
    fetchPage();
}

void ClassifyPipeline::startFromData(const QString &html, const QUrl &finalUrl,
                                      bool isHttps, const QJsonObject &manifestJson,
                                      bool swDetected)
{
    Q_UNUSED(isHttps)
    m_url = finalUrl;
    m_html = html;
    m_swDetected = swDetected;

    // Parse HTML
    m_htmlMeta = qapp::HtmlParser::parse(html, finalUrl);

    // Parse manifest if provided
    if (!manifestJson.isEmpty()) {
        auto r = qapp::ManifestParser::validate(manifestJson);
        if (r.success) {
            m_manifest = r.data;
            m_hasManifest = true;
        }
    }

    classify();
    resolveIcon();
}

// --- Step 1: Fetch page HTML ---

void ClassifyPipeline::fetchPage()
{
    QNetworkRequest req(m_url);
    req.setTransferTimeout(kTimeoutMs);
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onPageFetched(reply);
    });
}

void ClassifyPipeline::onPageFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        // Signal errors that may benefit from WebEngine fallback
        if (reply->error() == QNetworkReply::ContentAccessDenied ||
            reply->error() == QNetworkReply::ServiceUnavailableError ||
            reply->error() == QNetworkReply::ConnectionRefusedError ||
            reply->error() == QNetworkReply::TimeoutError ||
            reply->error() == QNetworkReply::OperationCanceledError) {
            err = QStringLiteral("Failed to fetch: ") + err;
        }
        emit failed(err);
        return;
    }

    // Follow redirects — QNetworkAccessManager handles this by default in Qt 6
    m_url = reply->url(); // Update to final URL after redirects
    m_html = QString::fromUtf8(reply->readAll());

    parseHtmlAndContinue();
}

// --- Step 2: Parse HTML ---

void ClassifyPipeline::parseHtmlAndContinue()
{
    emit statusChanged(QStringLiteral("Parsing page..."));
    m_htmlMeta = qapp::HtmlParser::parse(m_html, m_url);

    if (!m_htmlMeta.manifestUrl.isEmpty()) {
        fetchManifest();
    } else {
        detectServiceWorker();
    }
}

// --- Step 3: Fetch manifest ---

void ClassifyPipeline::fetchManifest()
{
    emit statusChanged(QStringLiteral("Fetching manifest..."));
    QNetworkRequest req(QUrl(m_htmlMeta.manifestUrl));
    req.setTransferTimeout(kTimeoutMs);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onManifestFetched(reply);
    });
}

void ClassifyPipeline::onManifestFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            auto r = qapp::ManifestParser::validate(doc.object());
            if (r.success) {
                m_manifest = r.data;
                m_hasManifest = true;
            }
        }
    }
    // Manifest fetch failure is non-fatal — continue without manifest

    detectServiceWorker();
}

// --- Step 4: Detect service worker ---

void ClassifyPipeline::detectServiceWorker()
{
    // Check HTML hint first
    if (m_htmlMeta.swHint) {
        m_swDetected = true;
        classify();
        resolveIcon();
        return;
    }

    // Probe common SW paths
    emit statusChanged(QStringLiteral("Detecting service worker..."));
    probeSwPath(0);
}

void ClassifyPipeline::probeSwPath(int index)
{
    if (index >= kSwPaths.size()) {
        // No SW found via probing
        m_swDetected = false;
        classify();
        resolveIcon();
        return;
    }

    QUrl probeUrl = m_url;
    probeUrl.setPath(kSwPaths[index]);
    probeUrl.setQuery(QString());
    probeUrl.setFragment(QString());

    QNetworkRequest req(probeUrl);
    req.setTransferTimeout(kProbeTimeoutMs);
    auto *reply = m_nam->head(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        onSwProbeDone(reply, index);
    });
}

void ClassifyPipeline::onSwProbeDone(QNetworkReply *reply, int index)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        m_swDetected = true;
        classify();
        resolveIcon();
        return;
    }

    probeSwPath(index + 1);
}

// --- Step 5: Classify ---

void ClassifyPipeline::classify()
{
    emit statusChanged(QStringLiteral("Classifying..."));

    m_result.hasManifest = m_hasManifest;
    m_result.hasServiceWorker = m_swDetected;

    if (m_hasManifest && m_swDetected)
        m_result.level = QStringLiteral("PWAPP");
    else if (m_hasManifest)
        m_result.level = QStringLiteral("WAPP");
    else
        m_result.level = QStringLiteral("WS");

    // Extract metadata with fallback chains
    // Name: manifest.name → manifest.short_name → htmlMeta.title → hostname
    if (!m_manifest.name.isEmpty())
        m_result.name = m_manifest.name;
    else if (!m_manifest.shortName.isEmpty())
        m_result.name = m_manifest.shortName;
    else if (!m_htmlMeta.title.isEmpty())
        m_result.name = m_htmlMeta.title;
    else
        m_result.name = m_url.host();

    // Display mode: manifest.display → "browser"
    m_result.displayMode = m_manifest.display.isEmpty()
        ? QStringLiteral("browser") : m_manifest.display;

    // Theme color: manifest.theme_color → htmlMeta.themeColor
    if (!m_manifest.themeColor.isEmpty())
        m_result.themeColor = m_manifest.themeColor;
    else if (!m_htmlMeta.themeColor.isEmpty())
        m_result.themeColor = m_htmlMeta.themeColor;

    // Background color: manifest.background_color
    if (!m_manifest.backgroundColor.isEmpty())
        m_result.backgroundColor = m_manifest.backgroundColor;

    // Start URL: manifest.start_url (resolved) → null
    if (!m_manifest.startUrl.isEmpty())
        m_result.startUrl = m_url.resolved(QUrl(m_manifest.startUrl)).toString();

    // Scope: manifest.scope (resolved) → null
    if (!m_manifest.scope.isEmpty())
        m_result.scope = m_url.resolved(QUrl(m_manifest.scope)).toString();

    // Manifest id
    if (!m_manifest.id.isEmpty())
        m_result.manifestId = m_manifest.id;

    // display_override
    if (!m_manifest.displayOverride.isEmpty())
        m_result.displayOverride = m_manifest.displayOverride;

    // Shortcuts as JSON array
    if (!m_manifest.shortcuts.isEmpty()) {
        QJsonArray scArr;
        for (const auto &sc : m_manifest.shortcuts) {
            QJsonObject scObj;
            scObj[QStringLiteral("name")] = sc.name;
            scObj[QStringLiteral("url")] = m_url.resolved(QUrl(sc.url)).toString();
            if (!sc.description.isEmpty())
                scObj[QStringLiteral("description")] = sc.description;
            scArr.append(scObj);
        }
        m_result.shortcuts = scArr;
    }

    // File handlers → MIME types
    for (const auto &fh : m_manifest.fileHandlers)
        for (const auto &mime : fh.accept)
            if (!m_result.mimeTypes.contains(mime))
                m_result.mimeTypes.append(mime);

    // Protocol handlers
    if (!m_manifest.protocolHandlers.isEmpty()) {
        QJsonArray phArr;
        for (const auto &ph : m_manifest.protocolHandlers) {
            QJsonObject phObj;
            phObj[QStringLiteral("protocol")] = ph.protocol;
            phObj[QStringLiteral("url")] = m_url.resolved(QUrl(ph.url)).toString();
            phArr.append(phObj);
        }
        m_result.protocolHandlers = phArr;
    }

    // Final URL (after redirects)
    m_result.finalUrl = m_url.toString();

    // Icon: manifest best icon → htmlMeta icons → (probing in resolveIcon)
    if (!m_manifest.icons.isEmpty()) {
        m_result.iconUrl = qapp::ManifestParser::pickBestIcon(m_manifest.icons, m_url);
    } else if (!m_htmlMeta.icons.isEmpty()) {
        m_result.iconUrl = m_htmlMeta.icons[0].first;
    }
}

// --- Step 6: Resolve icon (probe fallbacks if needed) ---

void ClassifyPipeline::resolveIcon()
{
    emit statusChanged(QStringLiteral("Resolving icon..."));

    // If we already have an icon URL, done
    if (!m_result.iconUrl.isEmpty()) {
        emitResult();
        return;
    }

    // Probe favicon paths
    probeFavicon(0);
}

void ClassifyPipeline::probeFavicon(int index)
{
    if (index >= kFaviconPaths.size()) {
        // Try og:image
        if (!m_htmlMeta.ogImage.isEmpty()) {
            m_result.iconUrl = m_htmlMeta.ogImage;
            emitResult();
            return;
        }
        // Generate letter icon as last resort
        m_result.iconUrl = qapp::IconManager::generateLetterIcon(m_result.name);
        emitResult();
        return;
    }

    QUrl probeUrl = m_url;
    probeUrl.setPath(kFaviconPaths[index]);
    probeUrl.setQuery(QString());
    probeUrl.setFragment(QString());

    QNetworkRequest req(probeUrl);
    req.setTransferTimeout(kProbeTimeoutMs);
    auto *reply = m_nam->head(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        onFaviconProbeDone(reply, index);
    });
}

void ClassifyPipeline::onFaviconProbeDone(QNetworkReply *reply, int index)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        // Verify content type is an image
        QString ct = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (ct.startsWith(QStringLiteral("image/")) ||
            ct.contains(QStringLiteral("octet-stream"))) {
            m_result.iconUrl = reply->url().toString();
            emitResult();
            return;
        }
    }

    probeFavicon(index + 1);
}

void ClassifyPipeline::emitResult()
{
    emit statusChanged(QStringLiteral("Classification complete"));
    emit finished(m_result);
}

} // namespace qapp::installer
