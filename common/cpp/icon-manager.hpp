#pragma once
#include "result.hpp"
#include <QString>

namespace qapp {

class IconManager {
public:
    static QString generateLetterIcon(const QString &name);
    static QString defaultIcon();
    static int hashToHue(const QString &str);
    static QString detectFormat(const QString &source);
    static Result<bool> saveDataUri(const QString &dataUri, const QString &destPath);
};

} // namespace qapp
