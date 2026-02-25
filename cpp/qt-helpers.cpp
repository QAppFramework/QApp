#include "qt-helpers.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkInformation>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <atomic>

// --- D-Bus notification ---

void qappShowNotification(const QString &appName, const QString &iconPath,
                          const QString &title, const QString &body)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));

    QList<QVariant> args;
    args << appName;
    args << static_cast<uint>(0);
    args << iconPath;
    args << title;
    args << body;
    args << QStringList();
    args << QVariantMap();
    args << static_cast<int>(-1);

    msg.setArguments(args);
    QDBusConnection::sessionBus().send(msg);
}

// --- D-Bus badge ---

void qappSetBadge(const QString &desktopFileId, int count)
{
    if (desktopFileId.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createSignal(
        QStringLiteral("/"),
        QStringLiteral("com.canonical.Unity.LauncherEntry"),
        QStringLiteral("Update"));

    msg << QStringLiteral("application://") + desktopFileId;
    QVariantMap props;
    props[QStringLiteral("count")] = static_cast<qlonglong>(count);
    props[QStringLiteral("count-visible")] = (count > 0);
    msg << props;

    QDBusConnection::sessionBus().send(msg);
}

// --- QDesktopServices ---

void qappOpenUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

// --- QNetworkInformation ---

static std::atomic<bool> s_networkOnline{true};

bool qappInitNetworkMonitoring()
{
    if (!QNetworkInformation::loadDefaultBackend())
        return false;

    auto *netInfo = QNetworkInformation::instance();
    if (!netInfo) return false;

    s_networkOnline.store(
        netInfo->reachability() == QNetworkInformation::Reachability::Online);

    QObject::connect(netInfo, &QNetworkInformation::reachabilityChanged,
                     [](QNetworkInformation::Reachability r) {
        s_networkOnline.store(r == QNetworkInformation::Reachability::Online);
    });
    return true;
}

bool qappIsNetworkOnline()
{
    return s_networkOnline.load();
}

// --- QSettings ---

bool qappReadSettingBool(const QString &key, bool defaultValue)
{
    QSettings settings;
    return settings.value(key, defaultValue).toBool();
}

void qappWriteSettingBool(const QString &key, bool value)
{
    QSettings settings;
    settings.setValue(key, value);
}

// --- QStandardPaths ---

QString qappAppDataLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

// --- WebEngine fallback fetch ---

QString qappWebFetch(const QString &url)
{
    auto *profile = new QWebEngineProfile();
    auto *page = new QWebEnginePage(profile);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool done = false;
    QString resultJson;

    auto finish = [&](const QString &json) {
        if (done) return;
        done = true;
        resultJson = json;
        timer.stop();
        loop.quit();
    };

    auto errorJson = [](const QString &msg) -> QString {
        QJsonObject obj;
        obj[QStringLiteral("success")] = false;
        obj[QStringLiteral("error")] = msg;
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    };

    // Timeout after 30s
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        finish(errorJson(QStringLiteral("Page load timed out (30s)")));
    });

    // On load finished: inject JS to extract data
    QObject::connect(page, &QWebEnginePage::loadFinished, [&](bool ok) {
        if (done) return;
        if (!ok) {
            finish(errorJson(QStringLiteral("Page load failed")));
            return;
        }

        // Synchronous JS extraction (mirrors web-fetcher.cpp)
        const QString js = QStringLiteral(R"JS(
            (function() {
                var result = {};
                result.html = document.documentElement.outerHTML;
                result.finalUrl = window.location.href;
                result.isHttps = window.location.protocol === 'https:';
                var manifestLink = document.querySelector('link[rel="manifest"]');
                result.manifestUrl = manifestLink ? manifestLink.href : null;
                try {
                    result.swDetected = !!(navigator.serviceWorker && navigator.serviceWorker.controller);
                } catch (e) {
                    result.swDetected = false;
                }
                result.manifestJson = null;
                if (result.manifestUrl) {
                    try {
                        var xhr = new XMLHttpRequest();
                        xhr.open('GET', result.manifestUrl, false);
                        xhr.send();
                        if (xhr.status === 200) {
                            result.manifestJson = JSON.parse(xhr.responseText);
                        }
                    } catch (e) {}
                }
                return JSON.stringify(result);
            })()
        )JS");

        page->runJavaScript(js, [&](const QVariant &jsResult) {
            if (done) return;

            if (!jsResult.isValid() || jsResult.isNull()) {
                finish(errorJson(QStringLiteral("JS extraction returned null")));
                return;
            }

            const QString jsonStr = jsResult.toString();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

            if (doc.isNull()) {
                finish(errorJson(QStringLiteral("Failed to parse JS result: ") + parseError.errorString()));
                return;
            }

            QJsonObject obj = doc.object();
            QJsonObject out;
            out[QStringLiteral("success")] = true;
            out[QStringLiteral("html")] = obj[QStringLiteral("html")].toString();
            out[QStringLiteral("finalUrl")] = obj[QStringLiteral("finalUrl")].toString();
            out[QStringLiteral("isHttps")] = obj[QStringLiteral("isHttps")].toBool();
            out[QStringLiteral("swDetected")] = obj[QStringLiteral("swDetected")].toBool();

            if (obj[QStringLiteral("manifestJson")].isObject()) {
                out[QStringLiteral("manifestJson")] = QString::fromUtf8(
                    QJsonDocument(obj[QStringLiteral("manifestJson")].toObject())
                        .toJson(QJsonDocument::Compact));
            } else {
                out[QStringLiteral("manifestJson")] = QString();
            }

            finish(QString::fromUtf8(QJsonDocument(out).toJson(QJsonDocument::Compact)));
        });
    });

    timer.start(30000);
    page->load(QUrl(url));
    loop.exec();

    delete page;
    delete profile;

    return resultJson;
}
