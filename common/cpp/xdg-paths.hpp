#ifndef QAPP_XDG_PATHS_HPP
#define QAPP_XDG_PATHS_HPP

#include "result.hpp"
#include <QString>

namespace qapp {

/// Resolved XDG paths for an installed app.
struct AppPaths {
    QString desktopFile;    // ~/.local/share/applications/qapp-{appId}.desktop
    QString iconFile;       // ~/.local/share/qapp-framework/icons/{appId}.{ext}
    QString metadataFile;   // ~/.local/share/qapp-framework/apps/{appId}.json
    QString iconsDir;       // ~/.local/share/qapp-framework/icons/
    QString appsDir;        // ~/.local/share/qapp-framework/apps/
    QString applicationsDir; // ~/.local/share/applications/
};

/// XDG path resolution for installed apps.
/// Ports js/src/installer/system/xdg-paths.js
class XdgPaths {
public:
    static Result<AppPaths> resolveAppPaths(
        const QString &appId,
        const QString &iconExt = QStringLiteral("png"));
};

} // namespace qapp

#endif // QAPP_XDG_PATHS_HPP
