#include "xdg-paths.hpp"
#include <QStandardPaths>

namespace qapp {

Result<AppPaths> XdgPaths::resolveAppPaths(const QString &appId, const QString &iconExt)
{
    if (appId.isEmpty()) {
        return Result<AppPaths>::fail(QStringLiteral("App ID must not be empty"));
    }

    QString localShare = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString applicationsDir = localShare + QStringLiteral("/applications");
    QString qappBase = localShare + QStringLiteral("/qapp-framework");
    QString iconsDir = qappBase + QStringLiteral("/icons");
    QString appsDir = qappBase + QStringLiteral("/apps");

    AppPaths paths;
    paths.desktopFile = applicationsDir + QStringLiteral("/qapp-") + appId + QStringLiteral(".desktop");
    paths.iconFile = iconsDir + QStringLiteral("/") + appId + QStringLiteral(".") + iconExt;
    paths.metadataFile = appsDir + QStringLiteral("/") + appId + QStringLiteral(".json");
    paths.iconsDir = iconsDir;
    paths.appsDir = appsDir;
    paths.applicationsDir = applicationsDir;

    return Result<AppPaths>::ok(std::move(paths));
}

} // namespace qapp
