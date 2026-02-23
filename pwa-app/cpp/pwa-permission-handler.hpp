#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QtQml/qqmlregistration.h>

namespace qapp::pwa {

/// Permission handler for PWA apps.
/// Auto-grants permissions for installed PWAs, persists decisions in QSettings.
class PwaPermissionHandler : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PwaPermissionHandler(QObject *parent = nullptr);

    /// Check if a permission type should be granted (default: true for installed PWAs).
    Q_INVOKABLE bool shouldGrant(int permissionType) const;

    /// Store a permission decision persistently.
    Q_INVOKABLE void storeDecision(int permissionType, bool granted);

private:
    static QString permissionKey(int type);
};

} // namespace qapp::pwa
