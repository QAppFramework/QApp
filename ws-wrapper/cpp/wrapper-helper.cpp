#include "wrapper-helper.hpp"
#include "config-reader.hpp"
#include "xdg-paths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace qapp::ws {

WrapperHelper::WrapperHelper(QObject *parent)
    : QObject(parent)
{
}

void WrapperHelper::loadMetadata(const QString &appId)
{
    if (appId.isEmpty()) return;

    auto pathsResult = qapp::XdgPaths::resolveAppPaths(appId);
    if (!pathsResult.success) {
        // No valid paths — use browser defaults
        m_metadataLoaded = true;
        emit metadataChanged();
        return;
    }

    auto configResult = qapp::ConfigReader::readJson(pathsResult.data.metadataFile);
    if (!configResult.success) {
        // No metadata file — WS or legacy install, use browser defaults
        m_metadataLoaded = true;
        emit metadataChanged();
        return;
    }

    const QJsonObject &obj = configResult.data;
    m_displayMode = obj.value("displayMode").toString();
    m_themeColor = obj.value("themeColor").toString();
    m_scope = obj.value("scope").toString();
    m_iconPath = obj.value("iconPath").toString();

    // Resolve startUrl
    QString startUrl = obj.value("startUrl").toString();
    if (!startUrl.isEmpty()) {
        m_metadataStartUrl = startUrl;
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

void WrapperHelper::clearAppDataAndRestart(const QString &appId)
{
    if (appId.isEmpty()) return;

    // WebEngine stores profile data under <AppData>/QtWebEngine/<storageName>/
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + "/QtWebEngine/" + appId;

    // Relaunch args: same binary + same arguments
    QString exe = QCoreApplication::applicationFilePath();
    QStringList args = QCoreApplication::arguments().mid(1);

    // Spawn a detached shell that waits for us to exit, deletes data, relaunches
    QString cmd = QString("sleep 0.5 && rm -rf '%1' && exec '%2' %3")
                      .arg(dataPath, exe, args.join("' '").prepend("'").append("'"));

    QProcess::startDetached("bash", {"-c", cmd});
    QCoreApplication::quit();
}

void WrapperHelper::saveAsApp(const QString &url, const QString &name)
{
    if (m_busy) return;

    // Launch the installer binary (sibling to this binary)
    QString installerPath = QCoreApplication::applicationDirPath()
                            + QStringLiteral("/qapp-installer");

    QStringList args = {url};
    if (!name.isEmpty()) {
        args << QStringLiteral("--name") << name;
    }

    bool launched = QProcess::startDetached(installerPath, args);

    if (launched) {
        m_statusMessage = QStringLiteral("Launching installer...");
    } else {
        m_statusMessage = QStringLiteral("Failed to launch installer");
    }
    emit statusChanged();
}

} // namespace qapp::ws
