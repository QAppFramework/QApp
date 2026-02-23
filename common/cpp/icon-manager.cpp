#include "icon-manager.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace qapp {

int IconManager::hashToHue(const QString &str)
{
    int hash = 0;
    for (QChar ch : str) {
        hash = ((hash << 5) - hash) + ch.unicode();
        hash &= 0x7FFFFFFF; // Keep positive
    }
    return ((hash % 360) + 360) % 360;
}

QString IconManager::generateLetterIcon(const QString &name)
{
    QChar letter = name.isEmpty() ? QChar('?') : name.at(0).toUpper();
    int hue = hashToHue(name.isEmpty() ? QStringLiteral("default") : name);

    QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 192 192'>"
        "<rect width='192' height='192' rx='32' fill='hsl(%1,65%,45%)'/>"
        "<text x='96' y='96' font-size='96' text-anchor='middle' "
        "dominant-baseline='central' fill='white' font-family='sans-serif'>%2</text>"
        "</svg>").arg(hue).arg(letter);

    return QStringLiteral("data:image/svg+xml,") + QUrl::toPercentEncoding(svg);
}

QString IconManager::defaultIcon()
{
    static const QString icon = QStringLiteral(
        "data:image/svg+xml,"
        "%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 192 192'%3E"
        "%3Ccircle cx='96' cy='96' r='80' fill='%23607d8b' stroke='white' stroke-width='4'/%3E"
        "%3Ctext x='96' y='96' font-size='64' text-anchor='middle' "
        "dominant-baseline='central' fill='white' font-family='sans-serif'%3E"
        "%F0%9F%8C%90%3C/text%3E%3C/svg%3E");
    return icon;
}

QString IconManager::detectFormat(const QString &source)
{
    if (source.startsWith(QStringLiteral("data:image/svg"))) return QStringLiteral("svg");
    if (source.startsWith(QStringLiteral("data:image/png"))) return QStringLiteral("png");
    if (source.startsWith(QStringLiteral("data:image/ico"))) return QStringLiteral("ico");
    if (source.startsWith(QStringLiteral("data:image/webp"))) return QStringLiteral("webp");
    if (source.endsWith(QStringLiteral(".svg"))) return QStringLiteral("svg");
    if (source.endsWith(QStringLiteral(".ico"))) return QStringLiteral("ico");
    if (source.endsWith(QStringLiteral(".webp"))) return QStringLiteral("webp");
    return QStringLiteral("png");
}

Result<bool> IconManager::saveDataUri(const QString &dataUri, const QString &destPath)
{
    int commaIdx = dataUri.indexOf(',');
    if (commaIdx < 0)
        return Result<bool>::fail(QStringLiteral("Invalid data URI"));

    QString header = dataUri.left(commaIdx);
    QString payload = dataUri.mid(commaIdx + 1);

    QDir().mkpath(QFileInfo(destPath).absolutePath());
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly))
        return Result<bool>::fail(QStringLiteral("Cannot write: ") + destPath);

    if (header.contains(QStringLiteral(";base64"))) {
        file.write(QByteArray::fromBase64(payload.toUtf8()));
    } else {
        file.write(QUrl::fromPercentEncoding(payload.toUtf8()).toUtf8());
    }
    file.close();
    return Result<bool>::ok(true);
}

} // namespace qapp
