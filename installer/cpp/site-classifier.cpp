#include "site-classifier.hpp"
#include "web-fetcher.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace qapp::installer {

SiteClassifier::SiteClassifier(QObject *parent)
    : QObject(parent)
{
}

void SiteClassifier::classify(const QString &url)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    m_pendingUrl = url;
    emit busyChanged();
    setStatusMessage(QStringLiteral("Classifying..."));

    m_pipeline = new ClassifyPipeline(this);

    connect(m_pipeline, &ClassifyPipeline::statusChanged,
            this, &SiteClassifier::setStatusMessage);

    connect(m_pipeline, &ClassifyPipeline::finished,
            this, [this](const ClassifyResult &result) {
        m_pipeline->deleteLater();
        m_pipeline = nullptr;
        m_busy = false;
        emit busyChanged();
        applyResult(result);
        setStatusMessage(QStringLiteral("Classification complete"));
    });

    connect(m_pipeline, &ClassifyPipeline::failed,
            this, [this](const QString &error) {
        m_pipeline->deleteLater();
        m_pipeline = nullptr;

        if (shouldTryWebEngine(error)) {
            qDebug() << "Native classify failed:" << error << "— trying WebEngine fallback";
            startWebEngineFallback();
        } else {
            m_errorMessage = error;
            m_hasResult = true;
            m_busy = false;
            emit busyChanged();
            emit resultChanged();
            setStatusMessage(QStringLiteral("Classification failed"));
        }
    });

    m_pipeline->start(QUrl(url));
}

bool SiteClassifier::shouldTryWebEngine(const QString &error)
{
    return error.contains(QStringLiteral("403")) ||
           error.contains(QStringLiteral("503")) ||
           error.contains(QStringLiteral("Failed to fetch")) ||
           error.contains(QStringLiteral("not reachable")) ||
           error.contains(QStringLiteral("Connection refused")) ||
           error.contains(QStringLiteral("timed out"));
}

void SiteClassifier::startWebEngineFallback()
{
    setStatusMessage(QStringLiteral("Retrying with browser engine..."));

    m_webFetcher = new WebFetcher(this);
    connect(m_webFetcher, &WebFetcher::finished,
            this, &SiteClassifier::onWebFetchFinished);
    m_webFetcher->fetch(QUrl(m_pendingUrl));
}

void SiteClassifier::onWebFetchFinished(bool success, const QString &html,
                                         const QString &finalUrl, bool isHttps,
                                         const QString &manifestJson, bool swDetected,
                                         const QString &error)
{
    m_webFetcher = nullptr; // WebFetcher calls deleteLater() on itself

    if (!success) {
        m_errorMessage = QStringLiteral("WebEngine fallback failed: ") + error;
        m_hasResult = true;
        m_busy = false;
        emit busyChanged();
        emit resultChanged();
        setStatusMessage(QStringLiteral("Classification failed"));
        return;
    }

    // Parse manifest JSON if present
    QJsonObject manifestObj;
    if (!manifestJson.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(manifestJson.toUtf8());
        if (doc.isObject())
            manifestObj = doc.object();
    }

    // Run classify-from-data pipeline
    m_pipeline = new ClassifyPipeline(this);

    connect(m_pipeline, &ClassifyPipeline::statusChanged,
            this, &SiteClassifier::setStatusMessage);

    connect(m_pipeline, &ClassifyPipeline::finished,
            this, [this](const ClassifyResult &result) {
        m_pipeline->deleteLater();
        m_pipeline = nullptr;
        m_busy = false;
        emit busyChanged();
        applyResult(result);
        setStatusMessage(QStringLiteral("Classification complete"));
    });

    connect(m_pipeline, &ClassifyPipeline::failed,
            this, [this](const QString &err) {
        m_pipeline->deleteLater();
        m_pipeline = nullptr;
        m_errorMessage = err;
        m_hasResult = true;
        m_busy = false;
        emit busyChanged();
        emit resultChanged();
        setStatusMessage(QStringLiteral("Classification failed"));
    });

    m_pipeline->startFromData(html, QUrl(finalUrl), isHttps, manifestObj, swDetected);
}

void SiteClassifier::clearResult()
{
    m_hasResult = false;
    m_level.clear();
    m_name.clear();
    m_iconUrl.clear();
    m_displayMode.clear();
    m_themeColor.clear();
    m_startUrl.clear();
    m_hasManifest = false;
    m_hasServiceWorker = false;
    m_errorMessage.clear();
    m_pendingUrl.clear();
    m_classifyResultJson.clear();
}

void SiteClassifier::applyResult(const ClassifyResult &result)
{
    m_level = result.level;
    m_name = result.name;
    m_iconUrl = result.iconUrl;
    m_displayMode = result.displayMode;
    m_themeColor = result.themeColor;
    m_startUrl = result.startUrl;
    m_hasManifest = result.hasManifest;
    m_hasServiceWorker = result.hasServiceWorker;

    m_classifyResultJson = QString::fromUtf8(
        QJsonDocument(result.toJson()).toJson(QJsonDocument::Compact));

    m_hasResult = true;
    emit resultChanged();
}

void SiteClassifier::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

} // namespace qapp::installer
