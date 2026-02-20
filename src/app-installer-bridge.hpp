#ifndef APP_INSTALLER_BRIDGE_HPP
#define APP_INSTALLER_BRIDGE_HPP

#include <QObject>
#include <QProcess>
#include <QString>
#include <QtQml/qqmlregistration.h>

class AppInstaller : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool installed READ installed NOTIFY resultChanged)
    Q_PROPERTY(QString appId READ appId NOTIFY resultChanged)
    Q_PROPERTY(QString appName READ appName NOTIFY resultChanged)
    Q_PROPERTY(QString desktopPath READ desktopPath NOTIFY resultChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY resultChanged)

public:
    explicit AppInstaller(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    bool installed() const { return m_installed; }
    QString appId() const { return m_appId; }
    QString appName() const { return m_appName; }
    QString desktopPath() const { return m_desktopPath; }
    QString errorMessage() const { return m_errorMessage; }

    Q_INVOKABLE void install(const QString &url);
    Q_INVOKABLE void uninstall(const QString &appId);

signals:
    void busyChanged();
    void resultChanged();

private slots:
    void onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onUninstallFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void clearResult();

    QProcess *m_process = nullptr;
    bool m_busy = false;
    bool m_installed = false;
    QString m_appId;
    QString m_appName;
    QString m_desktopPath;
    QString m_errorMessage;
};

#endif // APP_INSTALLER_BRIDGE_HPP
