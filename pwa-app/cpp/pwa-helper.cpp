#include "pwa-helper.hpp"
#include "config-reader.hpp"
#include "xdg-paths.hpp"
#include <QUrl>

namespace qapp::pwa {

PwaHelper::PwaHelper(QObject *parent)
    : QObject(parent)
{
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
    m_displayMode = obj.value("displayMode").toString("standalone");
    m_themeColor = obj.value("themeColor").toString();
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
