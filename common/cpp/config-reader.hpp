#ifndef QAPP_CONFIG_READER_HPP
#define QAPP_CONFIG_READER_HPP

#include "result.hpp"
#include <QJsonObject>
#include <QString>

namespace qapp {

/// Generic JSON config file reader/writer.
/// Creates parent directories on write if needed.
class ConfigReader {
public:
    static Result<QJsonObject> readJson(const QString &path);
    static Result<bool> writeJson(const QString &path, const QJsonObject &obj);
};

} // namespace qapp

#endif // QAPP_CONFIG_READER_HPP
