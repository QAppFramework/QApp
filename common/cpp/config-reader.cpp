#include "config-reader.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace qapp {

Result<QJsonObject> ConfigReader::readJson(const QString &path)
{
    if (path.isEmpty()) {
        return Result<QJsonObject>::fail(QStringLiteral("Path must not be empty"));
    }

    QFile file(path);
    if (!file.exists()) {
        return Result<QJsonObject>::fail(QStringLiteral("File not found: ") + path);
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return Result<QJsonObject>::fail(QStringLiteral("Cannot open file: ") + path);
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return Result<QJsonObject>::fail(
            QStringLiteral("JSON parse error: ") + parseError.errorString());
    }

    if (!doc.isObject()) {
        return Result<QJsonObject>::fail(QStringLiteral("JSON root is not an object"));
    }

    return Result<QJsonObject>::ok(doc.object());
}

Result<bool> ConfigReader::writeJson(const QString &path, const QJsonObject &obj)
{
    if (path.isEmpty()) {
        return Result<bool>::fail(QStringLiteral("Path must not be empty"));
    }

    // Create parent directories if needed
    QFileInfo info(path);
    QDir parentDir = info.dir();
    if (!parentDir.exists()) {
        if (!parentDir.mkpath(QStringLiteral("."))) {
            return Result<bool>::fail(
                QStringLiteral("Cannot create directory: ") + parentDir.path());
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<bool>::fail(QStringLiteral("Cannot open file for writing: ") + path);
    }

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    if (file.write(data) != data.size()) {
        file.close();
        return Result<bool>::fail(QStringLiteral("Write failed: ") + path);
    }

    file.close();
    return Result<bool>::ok(true);
}

} // namespace qapp
