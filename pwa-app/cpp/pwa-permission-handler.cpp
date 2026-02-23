#include "pwa-permission-handler.hpp"

namespace qapp::pwa {

PwaPermissionHandler::PwaPermissionHandler(QObject *parent)
    : QObject(parent)
{
}

bool PwaPermissionHandler::shouldGrant(int permissionType) const
{
    QString key = permissionKey(permissionType);
    if (key.isEmpty()) return false;

    QSettings settings;
    // Default: true (installed PWAs get permissions auto-granted)
    return settings.value(QStringLiteral("permissions/") + key, true).toBool();
}

void PwaPermissionHandler::storeDecision(int permissionType, bool granted)
{
    QString key = permissionKey(permissionType);
    if (key.isEmpty()) return;

    QSettings settings;
    settings.setValue(QStringLiteral("permissions/") + key, granted);
}

QString PwaPermissionHandler::permissionKey(int type)
{
    // Maps WebEnginePermission::PermissionType enum values to settings keys
    switch (type) {
    case 1: return QStringLiteral("geolocation");          // Geolocation
    case 2: return QStringLiteral("mediaAudioCapture");    // MediaAudioCapture
    case 3: return QStringLiteral("mediaVideoCapture");    // MediaVideoCapture
    case 4: return QStringLiteral("mediaAudioVideo");      // MediaAudioVideoCapture
    case 5: return QStringLiteral("desktopVideoCapture");  // DesktopVideoCapture
    case 6: return QStringLiteral("desktopAudioVideo");    // DesktopAudioVideoCapture
    case 7: return QStringLiteral("notifications");        // Notifications
    case 8: return QStringLiteral("clipboardReadWrite");   // ClipboardReadWrite
    case 9: return QStringLiteral("localFontsAccess");     // LocalFontsAccess
    default: return {};
    }
}

} // namespace qapp::pwa
