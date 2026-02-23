#include "pwa-os-integration.hpp"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>

namespace qapp::pwa {

PwaOsIntegration::PwaOsIntegration(QObject *parent)
    : QObject(parent)
{
}

void PwaOsIntegration::setDesktopFileId(const QString &id)
{
    if (m_desktopFileId == id) return;
    m_desktopFileId = id;
    emit desktopFileIdChanged();
}

void PwaOsIntegration::setBadge(int count)
{
    if (m_desktopFileId.isEmpty()) return;

    // Unity LauncherEntry protocol (supported by KDE Plasma, GNOME, Unity)
    QDBusMessage msg = QDBusMessage::createSignal(
        QStringLiteral("/"),
        QStringLiteral("com.canonical.Unity.LauncherEntry"),
        QStringLiteral("Update"));

    msg << QStringLiteral("application://") + m_desktopFileId;
    QVariantMap props;
    props[QStringLiteral("count")] = static_cast<qlonglong>(count);
    props[QStringLiteral("count-visible")] = (count > 0);
    msg << props;

    QDBusConnection::sessionBus().send(msg);
}

void PwaOsIntegration::share(const QString &title, const QString &text, const QString &url)
{
    // Compose mailto: with shared content — universal Linux approach
    QUrl mailtoUrl(QStringLiteral("mailto:"));
    QUrlQuery query;
    if (!title.isEmpty())
        query.addQueryItem(QStringLiteral("subject"), title);

    QString body;
    if (!text.isEmpty()) body = text;
    if (!url.isEmpty()) {
        if (!body.isEmpty()) body += QStringLiteral("\n\n");
        body += url;
    }
    if (!body.isEmpty())
        query.addQueryItem(QStringLiteral("body"), body);

    mailtoUrl.setQuery(query);
    QDesktopServices::openUrl(mailtoUrl);
}

QString PwaOsIntegration::polyfillScript() const
{
    return QStringLiteral(
        "(function(){"
        "if(!navigator.setAppBadge){"
        "navigator.setAppBadge=function(c){"
        "console.info('__QAPP_CMD:badge:'+(c||0));return Promise.resolve();};"
        "}"
        "if(!navigator.clearAppBadge){"
        "navigator.clearAppBadge=function(){"
        "console.info('__QAPP_CMD:badge:0');return Promise.resolve();};"
        "}"
        "if(!navigator.share){"
        "navigator.canShare=function(){return true;};"
        "navigator.share=function(d){"
        "console.info('__QAPP_CMD:share:'+JSON.stringify(d||{}));return Promise.resolve();};"
        "}"
        "})();"
    );
}

} // namespace qapp::pwa
