#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace qapp::pwa {

/// OS integration for PWA: taskbar badge + web share.
class PwaOsIntegration : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString desktopFileId READ desktopFileId WRITE setDesktopFileId NOTIFY desktopFileIdChanged)

public:
    explicit PwaOsIntegration(QObject *parent = nullptr);

    QString desktopFileId() const { return m_desktopFileId; }
    void setDesktopFileId(const QString &id);

    /// Set taskbar badge count (0 = clear).
    Q_INVOKABLE void setBadge(int count);
    /// Share content via mailto: compose.
    Q_INVOKABLE void share(const QString &title, const QString &text, const QString &url);
    /// Returns JS polyfill for navigator.setAppBadge/clearAppBadge/share.
    Q_INVOKABLE QString polyfillScript() const;

signals:
    void desktopFileIdChanged();

private:
    QString m_desktopFileId;
};

} // namespace qapp::pwa
