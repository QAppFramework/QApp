#pragma once
#include "classify-pipeline.hpp"
#include "xdg-paths.hpp"
#include "app-metadata.hpp"
#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;
class QProcess;

namespace qapp::installer {

/// Native app manager — replaces Node.js QProcess bridge.
/// Same QML interface as old AppInstaller (drop-in replacement).
class AppInstaller : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool installed READ installed NOTIFY resultChanged)
    Q_PROPERTY(QString appId READ appId NOTIFY resultChanged)
    Q_PROPERTY(QString appName READ appName NOTIFY resultChanged)
    Q_PROPERTY(QString desktopPath READ desktopPath NOTIFY resultChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY resultChanged)
    Q_PROPERTY(QJsonArray installedApps READ installedApps NOTIFY installedAppsChanged)
    Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateStatusChanged)
    Q_PROPERTY(QJsonArray appUpdates READ appUpdates NOTIFY appUpdatesChanged)
    Q_PROPERTY(bool checkingUpdates READ checkingUpdates NOTIFY checkingUpdatesChanged)

public:
    explicit AppInstaller(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    bool installed() const { return m_installed; }
    QString appId() const { return m_appId; }
    QString appName() const { return m_appName; }
    QString desktopPath() const { return m_desktopPath; }
    QString errorMessage() const { return m_errorMessage; }
    QJsonArray installedApps() const { return m_installedApps; }
    QString updateStatus() const { return m_updateStatus; }
    QJsonArray appUpdates() const { return m_appUpdates; }
    bool checkingUpdates() const { return m_checkingUpdates; }

    Q_INVOKABLE void install(const QString &url, const QString &customIconPath = QString());
    Q_INVOKABLE void installFromData(const QString &classifyResultJson, const QString &customIconPath = QString());
    Q_INVOKABLE void uninstall(const QString &appId);
    Q_INVOKABLE void listApps();
    Q_INVOKABLE void launch(const QString &appId, const QString &url);
    Q_INVOKABLE void updateQApp();
    Q_INVOKABLE void checkUpdates();

signals:
    void busyChanged();
    void resultChanged();
    void installedAppsChanged();
    void updateStatusChanged();
    void appUpdatesChanged();
    void checkingUpdatesChanged();

private:
    void clearResult();
    void doInstall(const ClassifyResult &result, const QString &customIconPath);
    void downloadIcon(const QString &url, const QString &destPath);
    void finishInstall();
    void installError(const QString &msg);
    void processNextUpdate();

    QNetworkAccessManager *m_nam = nullptr;

    // Install/uninstall state
    bool m_busy = false;
    bool m_installed = false;
    QString m_appId;
    QString m_appName;
    QString m_desktopPath;
    QString m_errorMessage;

    // Install intermediate data (used across async steps)
    ClassifyResult m_installResult;
    QString m_installAppId;
    qapp::AppPaths m_installPaths;
    QString m_installUrl;
    QString m_wrapperPath;

    // List state
    QJsonArray m_installedApps;

    // Self-update state
    QString m_updateStatus;
    QProcess *m_updateProcess = nullptr;

    // Check-updates state
    bool m_checkingUpdates = false;
    QJsonArray m_appUpdates;
    QVector<qapp::AppMetadataData> m_updateQueue;
    int m_updateQueueIndex = 0;
    QJsonArray m_updateResults;
};

} // namespace qapp::installer
