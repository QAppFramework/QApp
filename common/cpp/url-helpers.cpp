#include "url-helpers.hpp"
#include <QRegularExpression>
#include <QUrl>

namespace qapp {

namespace {
// URL pattern: accepts domains, bare hostnames (mimer, localhost), IPs, optional port
const QRegularExpression kUrlPattern(
    QStringLiteral(R"(^https?://[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*(:\d{1,5})?([\w\-._~:/?#\[\]@!$&'()*+,;=%]*)?)"));
} // anonymous namespace

bool UrlHelpers::isValidUrl(const QString &text)
{
    if (text.isEmpty()) {
        return false;
    }
    QRegularExpressionMatch match = kUrlPattern.match(text);
    return match.hasMatch() && match.capturedLength() == text.length();
}

Result<QString> UrlHelpers::generateAppId(const QString &url)
{
    QUrl parsed(url);
    if (!parsed.isValid() || parsed.host().isEmpty()) {
        return Result<QString>::fail(QStringLiteral("Invalid URL"));
    }

    QString scheme = parsed.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        return Result<QString>::fail(QStringLiteral("URL must use http or https protocol"));
    }

    QString hostname = parsed.host().toLower();
    if (hostname.isEmpty()) {
        return Result<QString>::fail(QStringLiteral("URL has no hostname"));
    }

    QString appId = hostname.replace(QLatin1Char('.'), QLatin1Char('-'));

    // Append non-default port (skip 80 for http, 443 for https)
    int port = parsed.port(-1);
    if (port != -1) {
        bool isDefault = (scheme == QStringLiteral("http") && port == 80)
                      || (scheme == QStringLiteral("https") && port == 443);
        if (!isDefault) {
            appId += QLatin1Char('-') + QString::number(port);
        }
    }

    return Result<QString>::ok(appId);
}

Result<QString> UrlHelpers::generateAppIdFromName(const QString &name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return Result<QString>::fail(QStringLiteral("App name must not be empty"));
    }

    // Lowercase, replace non-alphanumeric with hyphens, collapse, trim
    QString lower = trimmed.toLower();

    static const QRegularExpression kNonAlnum(QStringLiteral("[^a-z0-9]+"));
    QString slug = lower.replace(kNonAlnum, QStringLiteral("-"));

    static const QRegularExpression kEdgeHyphens(QStringLiteral("^-+|-+$"));
    slug.replace(kEdgeHyphens, QString());

    if (slug.isEmpty()) {
        return Result<QString>::fail(
            QStringLiteral("App name must contain at least one alphanumeric character"));
    }

    return Result<QString>::ok(slug);
}

} // namespace qapp
