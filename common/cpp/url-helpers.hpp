#ifndef QAPP_URL_HELPERS_HPP
#define QAPP_URL_HELPERS_HPP

#include "result.hpp"
#include <QString>

namespace qapp {

/// URL validation and app ID generation.
/// Ports js/src/installer/system/url-validator.js + js/src/installer/app/app-id.js
class UrlHelpers {
public:
    /// Check whether a string is a valid HTTP(S) URL.
    /// Uses same regex pattern as the JS version + QML main.qml.
    static bool isValidUrl(const QString &text);

    /// Generate an app ID from a URL string.
    /// Rules: hostname only, dots→hyphens, lowercase, non-default port appended.
    static Result<QString> generateAppId(const QString &url);

    /// Generate an app ID from a custom name string.
    /// Rules: lowercase, non-alphanumeric→hyphens, collapse, trim edges.
    static Result<QString> generateAppIdFromName(const QString &name);
};

} // namespace qapp

#endif // QAPP_URL_HELPERS_HPP
