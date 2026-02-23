#include "html-parser.hpp"
#include <QRegularExpression>

namespace qapp {

QString HtmlParser::resolveUrl(const QString &href, const QUrl &baseUrl)
{
    if (href.isEmpty()) return {};
    QUrl resolved = baseUrl.resolved(QUrl(href));
    return resolved.isValid() ? resolved.toString() : href;
}

HtmlMeta HtmlParser::parse(const QString &html, const QUrl &baseUrl)
{
    HtmlMeta meta;

    // Manifest: <link rel="manifest" href="..."> (either attribute order)
    static const QRegularExpression manifestRe1(
        R"(<link[^>]*rel=["']manifest["'][^>]*href=["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression manifestRe2(
        R"(<link[^>]*href=["']([^"']+)["'][^>]*rel=["']manifest["'])",
        QRegularExpression::CaseInsensitiveOption);

    auto m = manifestRe1.match(html);
    if (!m.hasMatch()) m = manifestRe2.match(html);
    if (m.hasMatch())
        meta.manifestUrl = resolveUrl(m.captured(1), baseUrl);

    // Title: <title>...</title>
    static const QRegularExpression titleRe(
        R"(<title[^>]*>([^<]*)</title>)",
        QRegularExpression::CaseInsensitiveOption);
    m = titleRe.match(html);
    if (m.hasMatch())
        meta.title = m.captured(1).trimmed();

    // Icons: <link rel="icon|apple-touch-icon|shortcut icon" href="..." sizes="...">
    static const QRegularExpression iconRe1(
        R"(<link[^>]*rel=["'](icon|apple-touch-icon|shortcut icon)["'][^>]*href=["']([^"']+)["'][^>]*/?>)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression iconRe2(
        R"(<link[^>]*href=["']([^"']+)["'][^>]*rel=["'](icon|apple-touch-icon|shortcut icon)["'][^>]*/?>)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression sizesRe(
        R"(sizes=["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption);

    auto it = iconRe1.globalMatch(html);
    while (it.hasNext()) {
        auto im = it.next();
        QString href = resolveUrl(im.captured(2), baseUrl);
        auto sm = sizesRe.match(im.captured(0));
        meta.icons.append({href, sm.hasMatch() ? sm.captured(1) : QString()});
    }
    auto it2 = iconRe2.globalMatch(html);
    while (it2.hasNext()) {
        auto im = it2.next();
        QString href = resolveUrl(im.captured(1), baseUrl);
        auto sm = sizesRe.match(im.captured(0));
        meta.icons.append({href, sm.hasMatch() ? sm.captured(1) : QString()});
    }

    // Theme color: <meta name="theme-color" content="...">
    static const QRegularExpression themeRe1(
        R"(<meta[^>]*name=["']theme-color["'][^>]*content=["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression themeRe2(
        R"(<meta[^>]*content=["']([^"']+)["'][^>]*name=["']theme-color["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = themeRe1.match(html);
    if (!m.hasMatch()) m = themeRe2.match(html);
    if (m.hasMatch())
        meta.themeColor = m.captured(1);

    // OG Image: <meta property="og:image" content="...">
    static const QRegularExpression ogRe1(
        R"(<meta[^>]*property=["']og:image["'][^>]*content=["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression ogRe2(
        R"(<meta[^>]*content=["']([^"']+)["'][^>]*property=["']og:image["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogRe1.match(html);
    if (!m.hasMatch()) m = ogRe2.match(html);
    if (m.hasMatch())
        meta.ogImage = resolveUrl(m.captured(1), baseUrl);

    // Service worker hint
    meta.swHint = html.contains(QStringLiteral("navigator.serviceWorker.register("));

    return meta;
}

} // namespace qapp
