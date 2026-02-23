#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace qapp::pwa {

/// Forwards WebEngine notifications to desktop via freedesktop D-Bus.
class PwaNotificationHandler : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString appName READ appName WRITE setAppName NOTIFY appNameChanged)
    Q_PROPERTY(QString iconPath READ iconPath WRITE setIconPath NOTIFY iconPathChanged)

public:
    explicit PwaNotificationHandler(QObject *parent = nullptr);

    QString appName() const { return m_appName; }
    void setAppName(const QString &name);
    QString iconPath() const { return m_iconPath; }
    void setIconPath(const QString &path);

    /// Send a desktop notification via org.freedesktop.Notifications D-Bus.
    Q_INVOKABLE void show(const QString &title, const QString &body);

signals:
    void appNameChanged();
    void iconPathChanged();

private:
    QString m_appName;
    QString m_iconPath;
};

} // namespace qapp::pwa
