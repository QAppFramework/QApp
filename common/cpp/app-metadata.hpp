#ifndef QAPP_APP_METADATA_HPP
#define QAPP_APP_METADATA_HPP

#include "result.hpp"
#include <QJsonObject>
#include <QString>

namespace qapp {

/// Per-app metadata stored as JSON.
struct AppMetadataData {
    // Required fields
    QString appId;
    QString name;
    QString url;
    QString level;        // "WS", "WAPP", "PWAPP"
    QString iconPath;
    QString desktopPath;
    QString wrapperPath;
    QString installedAt;  // ISO 8601 timestamp

    // Optional manifest fields (empty = not set)
    QString displayMode;  // fullscreen, standalone, minimal-ui, browser
    QString themeColor;
    QString startUrl;
    QString scope;
};

/// App metadata builder and validator.
/// Ports js/src/installer/app/app-metadata.js
class AppMetadata {
public:
    /// Build metadata from input, adding current ISO timestamp.
    /// Validates all fields before returning.
    static Result<AppMetadataData> build(const AppMetadataData &input);

    /// Validate existing metadata from a QJsonObject (e.g. read from file).
    static Result<AppMetadataData> validate(const QJsonObject &obj);

    /// Convert metadata struct to QJsonObject for storage.
    static QJsonObject toJson(const AppMetadataData &data);

private:
    static Result<AppMetadataData> validateFields(const AppMetadataData &data);
};

} // namespace qapp

#endif // QAPP_APP_METADATA_HPP
