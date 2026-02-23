#include "app-manager.hpp"
#include "url-helpers.hpp"
#include "icon-manager.hpp"
#include "desktop-entry.hpp"
#include "config-reader.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QUrl>

namespace qapp::installer {

AppInstaller::AppInstaller(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void AppInstaller::clearResult()
{
    m_installed = false;
    m_appId.clear();
    m_appName.clear();
    m_desktopPath.clear();
    m_errorMessage.clear();
}

void AppInstaller::installError(const QString &msg)
{
    m_errorMessage = msg;
    m_busy = false;
    emit busyChanged();
    emit resultChanged();
}

// --- Install from URL (classify first, then install) ---

void AppInstaller::install(const QString &url, const QString &customIconPath)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    auto *pipeline = new ClassifyPipeline(this);

    connect(pipeline, &ClassifyPipeline::finished,
            this, [this, pipeline, customIconPath](const ClassifyResult &result) {
        pipeline->deleteLater();
        doInstall(result, customIconPath);
    });

    connect(pipeline, &ClassifyPipeline::failed,
            this, [this, pipeline](const QString &error) {
        pipeline->deleteLater();
        installError(QStringLiteral("Classification failed: ") + error);
    });

    pipeline->start(QUrl(url));
}

// --- Install from pre-classified data ---

void AppInstaller::installFromData(const QString &classifyResultJson, const QString &customIconPath)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    QJsonDocument doc = QJsonDocument::fromJson(classifyResultJson.toUtf8());
    if (!doc.isObject()) {
        installError(QStringLiteral("Invalid classify result JSON"));
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject classification = root[QStringLiteral("classification")].toObject();
    QJsonObject metadata = root[QStringLiteral("metadata")].toObject();

    ClassifyResult result;
    result.level = classification[QStringLiteral("level")].toString();
    result.hasManifest = classification[QStringLiteral("hasManifest")].toBool();
    result.hasServiceWorker = classification[QStringLiteral("hasServiceWorker")].toBool();
    result.name = metadata[QStringLiteral("name")].toString();
    result.iconUrl = metadata[QStringLiteral("iconUrl")].toString();
    result.displayMode = metadata[QStringLiteral("displayMode")].toString();
    result.themeColor = metadata[QStringLiteral("themeColor")].toString();
    result.startUrl = metadata[QStringLiteral("startUrl")].toString();
    result.scope = metadata[QStringLiteral("scope")].toString();
    result.finalUrl = root[QStringLiteral("finalUrl")].toString();

    if (result.finalUrl.isEmpty()) {
        installError(QStringLiteral("Missing finalUrl in classify result"));
        return;
    }

    doInstall(result, customIconPath);
}

// --- Core install logic (shared by install + installFromData) ---

void AppInstaller::doInstall(const ClassifyResult &result, const QString &customIconPath)
{
    m_installResult = result;

    // Generate app ID from final URL
    auto idResult = qapp::UrlHelpers::generateAppId(result.finalUrl);
    if (!idResult.success) {
        installError(QStringLiteral("App ID: ") + idResult.error);
        return;
    }
    m_installAppId = idResult.data;

    // Launch URL: startUrl (if present) → finalUrl
    m_installUrl = result.startUrl.isEmpty() ? result.finalUrl : result.startUrl;

    // Wrapper binary path (same directory as installer)
    m_wrapperPath = QCoreApplication::applicationDirPath() + QStringLiteral("/qapp-ws-wrapper");

    // Determine icon format for path resolution
    QString iconSource = customIconPath.isEmpty() ? result.iconUrl : customIconPath;
    QString iconExt = qapp::IconManager::detectFormat(iconSource);

    // Resolve XDG paths
    auto pathsResult = qapp::XdgPaths::resolveAppPaths(m_installAppId, iconExt);
    if (!pathsResult.success) {
        installError(QStringLiteral("Paths: ") + pathsResult.error);
        return;
    }
    m_installPaths = pathsResult.data;

    // Handle icon source
    if (!customIconPath.isEmpty()) {
        QDir().mkpath(m_installPaths.iconsDir);
        QFile::remove(m_installPaths.iconFile); // QFile::copy doesn't overwrite
        if (!QFile::copy(customIconPath, m_installPaths.iconFile)) {
            installError(QStringLiteral("Icon: Failed to copy ") + customIconPath);
            return;
        }
        finishInstall();
    } else if (result.iconUrl.startsWith(QStringLiteral("data:"))) {
        auto saveResult = qapp::IconManager::saveDataUri(result.iconUrl, m_installPaths.iconFile);
        if (!saveResult.success) {
            installError(QStringLiteral("Icon: ") + saveResult.error);
            return;
        }
        finishInstall();
    } else if (!result.iconUrl.isEmpty()) {
        downloadIcon(result.iconUrl, m_installPaths.iconFile);
    } else {
        // No icon available — generate letter icon
        QString letterIcon = qapp::IconManager::generateLetterIcon(result.name);
        auto svgPaths = qapp::XdgPaths::resolveAppPaths(m_installAppId, QStringLiteral("svg"));
        if (svgPaths.success) m_installPaths = svgPaths.data;
        qapp::IconManager::saveDataUri(letterIcon, m_installPaths.iconFile);
        finishInstall();
    }
}

// --- Async icon download ---

void AppInstaller::downloadIcon(const QString &url, const QString &destPath)
{
    QNetworkRequest req{QUrl{url}};
    req.setTransferTimeout(10000);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, destPath]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QDir().mkpath(QFileInfo(destPath).absolutePath());
            QFile f(destPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                finishInstall();
                return;
            }
        }

        // Download failed — fallback to letter icon
        QString letterIcon = qapp::IconManager::generateLetterIcon(m_installResult.name);
        auto svgPaths = qapp::XdgPaths::resolveAppPaths(m_installAppId, QStringLiteral("svg"));
        if (svgPaths.success) {
            m_installPaths = svgPaths.data;
            qapp::IconManager::saveDataUri(letterIcon, m_installPaths.iconFile);
        }
        finishInstall();
    });
}

// --- Write .desktop + metadata after icon is ready ---

void AppInstaller::finishInstall()
{
    // Generate .desktop entry
    qapp::DesktopEntryInput entryInput;
    entryInput.name = m_installResult.name;
    entryInput.exec = m_wrapperPath + QStringLiteral(" ") +
                      m_installAppId + QStringLiteral(" ") + m_installUrl;
    entryInput.icon = m_installPaths.iconFile;

    auto entryResult = qapp::DesktopEntry::generate(entryInput);
    if (!entryResult.success) {
        installError(QStringLiteral("Desktop entry: ") + entryResult.error);
        return;
    }

    // Write .desktop file
    QDir().mkpath(m_installPaths.applicationsDir);
    QFile desktopFile(m_installPaths.desktopFile);
    if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        installError(QStringLiteral("Cannot write: ") + m_installPaths.desktopFile);
        return;
    }
    desktopFile.write(entryResult.data.toUtf8());
    desktopFile.close();

    // Build metadata
    qapp::AppMetadataData metaInput;
    metaInput.appId = m_installAppId;
    metaInput.name = m_installResult.name;
    metaInput.url = m_installUrl;
    metaInput.level = m_installResult.level;
    metaInput.iconPath = m_installPaths.iconFile;
    metaInput.desktopPath = m_installPaths.desktopFile;
    metaInput.wrapperPath = m_wrapperPath;

    // Optional fields — skip default "browser" for displayMode
    if (!m_installResult.displayMode.isEmpty() &&
        m_installResult.displayMode != QStringLiteral("browser"))
        metaInput.displayMode = m_installResult.displayMode;
    if (!m_installResult.themeColor.isEmpty())
        metaInput.themeColor = m_installResult.themeColor;
    if (!m_installResult.startUrl.isEmpty())
        metaInput.startUrl = m_installResult.startUrl;
    if (!m_installResult.scope.isEmpty())
        metaInput.scope = m_installResult.scope;

    auto metaResult = qapp::AppMetadata::build(metaInput);
    if (!metaResult.success) {
        installError(QStringLiteral("Metadata: ") + metaResult.error);
        return;
    }

    // Write metadata JSON
    auto writeResult = qapp::ConfigReader::writeJson(
        m_installPaths.metadataFile,
        qapp::AppMetadata::toJson(metaResult.data));
    if (!writeResult.success) {
        installError(QStringLiteral("Config: ") + writeResult.error);
        return;
    }

    // Success
    m_appId = m_installAppId;
    m_appName = m_installResult.name;
    m_desktopPath = m_installPaths.desktopFile;
    m_installed = true;
    m_busy = false;
    emit busyChanged();
    emit resultChanged();
}

// --- Uninstall ---

void AppInstaller::uninstall(const QString &appId)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    // Read metadata to find file paths
    auto pathsResult = qapp::XdgPaths::resolveAppPaths(appId);
    if (!pathsResult.success) {
        installError(QStringLiteral("Paths: ") + pathsResult.error);
        return;
    }

    auto metaResult = qapp::ConfigReader::readJson(pathsResult.data.metadataFile);
    if (!metaResult.success) {
        installError(QStringLiteral("App \"") + appId + QStringLiteral("\" is not installed"));
        return;
    }

    auto validated = qapp::AppMetadata::validate(metaResult.data);
    if (!validated.success) {
        installError(QStringLiteral("Corrupt metadata: ") + validated.error);
        return;
    }

    // Delete files (best effort — continue even if individual removes fail)
    QFile::remove(validated.data.desktopPath);
    QFile::remove(validated.data.iconPath);
    QFile::remove(pathsResult.data.metadataFile);

    m_installed = false;
    m_appId.clear();
    m_appName.clear();
    m_desktopPath.clear();
    m_busy = false;
    emit busyChanged();
    emit resultChanged();

    // Auto-refresh the installed apps list
    listApps();
}

// --- List installed apps ---

void AppInstaller::listApps()
{
    // Resolve base paths (using dummy appId just to get directory paths)
    auto pathsResult = qapp::XdgPaths::resolveAppPaths(QStringLiteral("_"));
    if (!pathsResult.success) {
        m_installedApps = QJsonArray();
        emit installedAppsChanged();
        return;
    }

    QDir appsDir(pathsResult.data.appsDir);
    if (!appsDir.exists()) {
        m_installedApps = QJsonArray();
        emit installedAppsChanged();
        return;
    }

    QVector<QJsonObject> appVec;
    const QStringList files = appsDir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString &file : files) {
        auto readResult = qapp::ConfigReader::readJson(appsDir.filePath(file));
        if (!readResult.success) continue;

        auto validated = qapp::AppMetadata::validate(readResult.data);
        if (!validated.success) continue;

        appVec.append(qapp::AppMetadata::toJson(validated.data));
    }

    // Sort by name (case-insensitive)
    std::sort(appVec.begin(), appVec.end(), [](const QJsonObject &a, const QJsonObject &b) {
        return a[QStringLiteral("name")].toString().toLower() <
               b[QStringLiteral("name")].toString().toLower();
    });

    QJsonArray sorted;
    for (const auto &obj : appVec)
        sorted.append(obj);

    m_installedApps = sorted;
    emit installedAppsChanged();
}

// --- Launch wrapper ---

void AppInstaller::launch(const QString &appId, const QString &url)
{
    QString wrapperPath = QCoreApplication::applicationDirPath() + QStringLiteral("/qapp-ws-wrapper");
    QProcess::startDetached(wrapperPath, {appId, url});
}

// --- Self-update (runs bash script, no Node.js) ---

void AppInstaller::updateQApp()
{
    if (m_busy) return;

    m_busy = true;
    m_updateStatus = QStringLiteral("Downloading latest version...");
    emit busyChanged();
    emit updateStatusChanged();

    m_updateProcess = new QProcess(this);

    connect(m_updateProcess, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_busy = false;
        emit busyChanged();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_updateStatus = QStringLiteral("Updated! Restart QApp to use the new version.");
        } else {
            QByteArray errOutput = m_updateProcess->readAllStandardError();
            QString errMsg = QString::fromUtf8(errOutput).trimmed();
            if (errMsg.isEmpty()) {
                m_updateStatus = QStringLiteral("Update failed (exit code ") +
                    QString::number(exitCode) + QStringLiteral(")");
            } else {
                QStringList lines = errMsg.split('\n');
                m_updateStatus = QStringLiteral("Update failed: ") + lines.last().trimmed();
            }
        }

        emit updateStatusChanged();
        m_updateProcess->deleteLater();
        m_updateProcess = nullptr;
    });

    // Stream output for live status updates
    connect(m_updateProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_updateProcess->readAllStandardOutput();
        QString lines = QString::fromUtf8(data).trimmed();
        if (!lines.isEmpty()) {
            QStringList allLines = lines.split('\n');
            for (int i = allLines.size() - 1; i >= 0; --i) {
                QString line = allLines[i].trimmed();
                if (line.startsWith(QStringLiteral("[QApp]"))) {
                    m_updateStatus = line.mid(7).trimmed();
                    emit updateStatusChanged();
                    break;
                }
            }
        }
    });

    QString scriptPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../bin/update.sh");
    m_updateProcess->start(QStringLiteral("bash"), {scriptPath});

    if (!m_updateProcess->waitForStarted(5000)) {
        m_updateStatus = QStringLiteral("Failed to start update process");
        m_busy = false;
        emit busyChanged();
        emit updateStatusChanged();
        m_updateProcess->deleteLater();
        m_updateProcess = nullptr;
    }
}

// --- Check for app updates (re-classify WAPP/PWAPP apps, compare metadata) ---

void AppInstaller::checkUpdates()
{
    if (m_checkingUpdates) return;

    m_checkingUpdates = true;
    emit checkingUpdatesChanged();

    // List apps and filter to WAPP/PWAPP (which have manifests to compare)
    m_updateQueue.clear();
    m_updateQueueIndex = 0;
    m_updateResults = QJsonArray();

    auto pathsResult = qapp::XdgPaths::resolveAppPaths(QStringLiteral("_"));
    if (pathsResult.success) {
        QDir appsDir(pathsResult.data.appsDir);
        if (appsDir.exists()) {
            const QStringList files = appsDir.entryList({QStringLiteral("*.json")}, QDir::Files);
            for (const QString &file : files) {
                auto readResult = qapp::ConfigReader::readJson(appsDir.filePath(file));
                if (!readResult.success) continue;
                auto validated = qapp::AppMetadata::validate(readResult.data);
                if (!validated.success) continue;
                if (validated.data.level == QStringLiteral("WAPP") ||
                    validated.data.level == QStringLiteral("PWAPP")) {
                    m_updateQueue.append(validated.data);
                }
            }
        }
    }

    processNextUpdate();
}

void AppInstaller::processNextUpdate()
{
    if (m_updateQueueIndex >= m_updateQueue.size()) {
        m_appUpdates = m_updateResults;
        m_checkingUpdates = false;
        emit checkingUpdatesChanged();
        emit appUpdatesChanged();
        return;
    }

    int idx = m_updateQueueIndex;
    const auto &app = m_updateQueue[idx];

    auto *pipeline = new ClassifyPipeline(this);

    connect(pipeline, &ClassifyPipeline::finished,
            this, [this, pipeline, idx](const ClassifyResult &result) {
        pipeline->deleteLater();
        const auto &app = m_updateQueue[idx];

        QStringList changes;

        // Name
        if (!result.name.isEmpty() && result.name != app.name)
            changes << QStringLiteral("name");

        // Display mode: "browser" and empty treated as equivalent
        QString storedDisplay = app.displayMode.isEmpty()
            ? QStringLiteral("browser") : app.displayMode;
        QString freshDisplay = result.displayMode.isEmpty()
            ? QStringLiteral("browser") : result.displayMode;
        if (storedDisplay != freshDisplay)
            changes << QStringLiteral("displayMode");

        // Theme color
        if (app.themeColor != result.themeColor)
            changes << QStringLiteral("themeColor");

        // Start URL
        if (app.startUrl != result.startUrl)
            changes << QStringLiteral("startUrl");

        // Scope
        if (app.scope != result.scope)
            changes << QStringLiteral("scope");

        QJsonObject entry;
        entry[QStringLiteral("appId")] = app.appId;
        entry[QStringLiteral("name")] = app.name;
        entry[QStringLiteral("hasUpdate")] = !changes.isEmpty();
        QJsonArray changesArr;
        for (const QString &c : changes) changesArr.append(c);
        entry[QStringLiteral("changes")] = changesArr;

        m_updateResults.append(entry);
        m_updateQueueIndex = idx + 1;
        processNextUpdate();
    });

    connect(pipeline, &ClassifyPipeline::failed,
            this, [this, pipeline, idx](const QString &error) {
        pipeline->deleteLater();
        const auto &app = m_updateQueue[idx];

        QJsonObject entry;
        entry[QStringLiteral("appId")] = app.appId;
        entry[QStringLiteral("name")] = app.name;
        entry[QStringLiteral("hasUpdate")] = false;
        entry[QStringLiteral("changes")] = QJsonArray();
        entry[QStringLiteral("error")] = error;

        m_updateResults.append(entry);
        m_updateQueueIndex = idx + 1;
        processNextUpdate();
    });

    pipeline->start(QUrl(app.url));
}

} // namespace qapp::installer
