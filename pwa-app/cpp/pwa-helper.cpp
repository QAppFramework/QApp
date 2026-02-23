#include "pwa-helper.hpp"
#include "config-reader.hpp"
#include "xdg-paths.hpp"
#include <QJsonArray>
#include <QNetworkInformation>
#include <QUrl>

namespace qapp::pwa {

PwaHelper::PwaHelper(QObject *parent)
    : QObject(parent)
{
    // Monitor network reachability
    if (QNetworkInformation::loadDefaultBackend()) {
        auto *netInfo = QNetworkInformation::instance();
        if (netInfo) {
            m_isOnline = (netInfo->reachability() == QNetworkInformation::Reachability::Online);
            connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
                    [this](QNetworkInformation::Reachability r) {
                bool online = (r == QNetworkInformation::Reachability::Online);
                if (online != m_isOnline) {
                    m_isOnline = online;
                    emit onlineChanged();
                }
            });
        }
    }
}

void PwaHelper::loadMetadata(const QString &appId)
{
    if (appId.isEmpty()) return;

    auto pathsResult = qapp::XdgPaths::resolveAppPaths(appId);
    if (!pathsResult.success) {
        m_metadataLoaded = true;
        emit metadataChanged();
        return;
    }

    auto configResult = qapp::ConfigReader::readJson(pathsResult.data.metadataFile);
    if (!configResult.success) {
        m_metadataLoaded = true;
        emit metadataChanged();
        return;
    }

    const QJsonObject &obj = configResult.data;
    // Resolve display_override: first supported mode wins, else fall back to display
    static const QStringList supportedModes = {
        QStringLiteral("fullscreen"), QStringLiteral("standalone"),
        QStringLiteral("minimal-ui"), QStringLiteral("browser")
    };
    QString resolvedDisplay;
    if (obj.contains(QStringLiteral("displayOverride")) && obj[QStringLiteral("displayOverride")].isArray()) {
        for (const auto &v : obj[QStringLiteral("displayOverride")].toArray()) {
            QString mode = v.toString();
            if (supportedModes.contains(mode)) {
                resolvedDisplay = mode;
                break;
            }
        }
    }
    if (resolvedDisplay.isEmpty())
        resolvedDisplay = obj.value("displayMode").toString("standalone");
    m_displayMode = resolvedDisplay;
    m_themeColor = obj.value("themeColor").toString();
    m_backgroundColor = obj.value("backgroundColor").toString();
    m_iconPath = obj.value("iconPath").toString();
    m_appName = obj.value("name").toString();
    m_scope = obj.value("scope").toString();

    // Resolve startUrl
    QString startUrl = obj.value("startUrl").toString();
    if (!startUrl.isEmpty()) {
        m_startUrl = startUrl;
    }

    // Resolve scope to absolute if relative
    if (!m_scope.isEmpty()) {
        QString baseUrl = obj.value("url").toString();
        if (!baseUrl.isEmpty()) {
            QUrl resolved = QUrl(baseUrl).resolved(QUrl(m_scope));
            m_scope = resolved.toString();
        }
    }

    m_metadataLoaded = true;
    emit metadataChanged();
}

} // namespace qapp::pwa
