#include "app-metadata.hpp"
#include <QDateTime>
#include <QJsonValue>
#include <QStringList>
#include <QUrl>

namespace qapp {

namespace {

const QStringList kValidLevels = {
    QStringLiteral("WS"),
    QStringLiteral("WAPP"),
    QStringLiteral("PWAPP"),
};

const QStringList kValidDisplayModes = {
    QStringLiteral("fullscreen"),
    QStringLiteral("standalone"),
    QStringLiteral("minimal-ui"),
    QStringLiteral("browser"),
};

} // anonymous namespace

Result<AppMetadataData> AppMetadata::validateFields(const AppMetadataData &data)
{
    if (data.appId.isEmpty()) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at appId: Invalid length"));
    }
    if (data.name.isEmpty()) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at name: Invalid length"));
    }

    // Validate URL format
    QUrl url(data.url);
    if (!url.isValid() || data.url.isEmpty()) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at url: Invalid URL"));
    }

    if (!kValidLevels.contains(data.level)) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at level: Invalid input"));
    }

    // iconPath, desktopPath, wrapperPath are required but can be any string
    // (file paths don't need URL validation)

    // Validate installedAt as ISO 8601
    QDateTime dt = QDateTime::fromString(data.installedAt, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at installedAt: Invalid timestamp"));
    }

    // Validate optional displayMode
    if (!data.displayMode.isEmpty() && !kValidDisplayModes.contains(data.displayMode)) {
        return Result<AppMetadataData>::fail(
            QStringLiteral("Validation failed at displayMode: Invalid input"));
    }

    return Result<AppMetadataData>::ok(data);
}

Result<AppMetadataData> AppMetadata::build(const AppMetadataData &input)
{
    AppMetadataData data = input;
    data.installedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    return validateFields(data);
}

Result<AppMetadataData> AppMetadata::validate(const QJsonObject &obj)
{
    AppMetadataData data;
    data.appId = obj.value(QStringLiteral("appId")).toString();
    data.name = obj.value(QStringLiteral("name")).toString();
    data.url = obj.value(QStringLiteral("url")).toString();
    data.level = obj.value(QStringLiteral("level")).toString();
    data.iconPath = obj.value(QStringLiteral("iconPath")).toString();
    data.desktopPath = obj.value(QStringLiteral("desktopPath")).toString();
    data.wrapperPath = obj.value(QStringLiteral("wrapperPath")).toString();
    data.installedAt = obj.value(QStringLiteral("installedAt")).toString();

    // Optional fields
    data.displayMode = obj.value(QStringLiteral("displayMode")).toString();
    data.themeColor = obj.value(QStringLiteral("themeColor")).toString();
    data.backgroundColor = obj.value(QStringLiteral("backgroundColor")).toString();
    data.startUrl = obj.value(QStringLiteral("startUrl")).toString();
    data.scope = obj.value(QStringLiteral("scope")).toString();
    data.manifestId = obj.value(QStringLiteral("manifestId")).toString();
    if (obj.contains(QStringLiteral("displayOverride")) && obj[QStringLiteral("displayOverride")].isArray()) {
        for (const auto &v : obj[QStringLiteral("displayOverride")].toArray())
            if (v.isString()) data.displayOverride.append(v.toString());
    }
    if (obj.contains(QStringLiteral("shortcuts")) && obj[QStringLiteral("shortcuts")].isArray())
        data.shortcuts = obj[QStringLiteral("shortcuts")].toArray();
    if (obj.contains(QStringLiteral("mimeTypes")) && obj[QStringLiteral("mimeTypes")].isArray()) {
        for (const auto &v : obj[QStringLiteral("mimeTypes")].toArray())
            if (v.isString()) data.mimeTypes.append(v.toString());
    }
    if (obj.contains(QStringLiteral("protocolHandlers")) && obj[QStringLiteral("protocolHandlers")].isArray())
        data.protocolHandlers = obj[QStringLiteral("protocolHandlers")].toArray();

    return validateFields(data);
}

QJsonObject AppMetadata::toJson(const AppMetadataData &data)
{
    QJsonObject obj;
    obj[QStringLiteral("appId")] = data.appId;
    obj[QStringLiteral("name")] = data.name;
    obj[QStringLiteral("url")] = data.url;
    obj[QStringLiteral("level")] = data.level;
    obj[QStringLiteral("iconPath")] = data.iconPath;
    obj[QStringLiteral("desktopPath")] = data.desktopPath;
    obj[QStringLiteral("wrapperPath")] = data.wrapperPath;
    obj[QStringLiteral("installedAt")] = data.installedAt;

    // Only include optional fields if set
    if (!data.displayMode.isEmpty()) {
        obj[QStringLiteral("displayMode")] = data.displayMode;
    }
    if (!data.themeColor.isEmpty()) {
        obj[QStringLiteral("themeColor")] = data.themeColor;
    }
    if (!data.backgroundColor.isEmpty()) {
        obj[QStringLiteral("backgroundColor")] = data.backgroundColor;
    }
    if (!data.startUrl.isEmpty()) {
        obj[QStringLiteral("startUrl")] = data.startUrl;
    }
    if (!data.scope.isEmpty()) {
        obj[QStringLiteral("scope")] = data.scope;
    }
    if (!data.manifestId.isEmpty()) {
        obj[QStringLiteral("manifestId")] = data.manifestId;
    }
    if (!data.displayOverride.isEmpty()) {
        QJsonArray arr;
        for (const auto &mode : data.displayOverride) arr.append(mode);
        obj[QStringLiteral("displayOverride")] = arr;
    }
    if (!data.shortcuts.isEmpty()) {
        obj[QStringLiteral("shortcuts")] = data.shortcuts;
    }
    if (!data.mimeTypes.isEmpty()) {
        QJsonArray arr;
        for (const auto &m : data.mimeTypes) arr.append(m);
        obj[QStringLiteral("mimeTypes")] = arr;
    }
    if (!data.protocolHandlers.isEmpty()) {
        obj[QStringLiteral("protocolHandlers")] = data.protocolHandlers;
    }

    return obj;
}

} // namespace qapp
