#pragma once
#include "result.hpp"
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QUrl>

namespace qapp {

struct ManifestIcon {
    QString src;
    QString sizes;
    QString type;
    QString purpose;
};

struct ManifestShortcut {
    QString name;
    QString url;
    QString description;
    QString iconSrc;
};

struct ManifestFileHandler {
    QString action;          // URL to open when file opened
    QStringList accept;      // MIME types (e.g. "image/png", ".txt")
};

struct ManifestProtocolHandler {
    QString protocol;
    QString url;             // URL template with %s
};

struct Manifest {
    QString name;
    QString shortName;
    QString startUrl;
    QString scope;
    QString display;
    QStringList displayOverride;
    QString id;
    QVector<ManifestIcon> icons;
    QVector<ManifestShortcut> shortcuts;
    QVector<ManifestFileHandler> fileHandlers;
    QVector<ManifestProtocolHandler> protocolHandlers;
    QString themeColor;
    QString backgroundColor;
};

class ManifestParser {
public:
    static Result<Manifest> validate(const QJsonObject &json);
    static QString pickBestIcon(const QVector<ManifestIcon> &icons, const QUrl &baseUrl);

private:
    static int parseSize(const QString &sizes);
};

} // namespace qapp
