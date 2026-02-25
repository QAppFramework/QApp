#pragma once
#include <QString>

// D-Bus: desktop notification (org.freedesktop.Notifications.Notify)
void qappShowNotification(const QString &appName, const QString &iconPath,
                          const QString &title, const QString &body);

// D-Bus: Unity LauncherEntry badge (count + visibility)
void qappSetBadge(const QString &desktopFileId, int count);

// QDesktopServices::openUrl() wrapper
void qappOpenUrl(const QString &url);

// QNetworkInformation: init backend + signal-cached online state
bool qappInitNetworkMonitoring();
bool qappIsNetworkOnline();

// QSettings: read/write bool values (per-app settings)
bool qappReadSettingBool(const QString &key, bool defaultValue);
void qappWriteSettingBool(const QString &key, bool value);

// QStandardPaths::writableLocation(AppDataLocation)
QString qappAppDataLocation();

// WebEngine fallback: load URL in headless Chromium, extract HTML + manifest + SW.
// Returns JSON: {"success":true,"html":"...","finalUrl":"...","isHttps":true,
//                "manifestJson":"...","swDetected":false}
// or:           {"success":false,"error":"..."}
// MUST be called on Qt main thread (WebEngine requirement).
// Blocks via QEventLoop until page loads or 30s timeout.
QString qappWebFetch(const QString &url);
