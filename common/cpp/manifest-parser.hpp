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

struct Manifest {
    QString name;
    QString shortName;
    QString startUrl;
    QString scope;
    QString display;
    QVector<ManifestIcon> icons;
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
