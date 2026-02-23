#include "pwa-notification-handler.hpp"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

namespace qapp::pwa {

PwaNotificationHandler::PwaNotificationHandler(QObject *parent)
    : QObject(parent)
{
}

void PwaNotificationHandler::setAppName(const QString &name)
{
    if (m_appName != name) {
        m_appName = name;
        emit appNameChanged();
    }
}

void PwaNotificationHandler::setIconPath(const QString &path)
{
    if (m_iconPath != path) {
        m_iconPath = path;
        emit iconPathChanged();
    }
}

void PwaNotificationHandler::show(const QString &title, const QString &body)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));

    QList<QVariant> args;
    args << m_appName;                          // app_name
    args << static_cast<uint>(0);               // replaces_id
    args << m_iconPath;                         // app_icon
    args << title;                              // summary
    args << body;                               // body
    args << QStringList();                      // actions
    args << QVariantMap();                      // hints
    args << static_cast<int>(-1);               // expire_timeout (-1 = server default)

    msg.setArguments(args);
    QDBusConnection::sessionBus().send(msg);
}

} // namespace qapp::pwa
