#include "manifest-parser.hpp"
#include <QJsonArray>

namespace qapp {

int ManifestParser::parseSize(const QString &sizes)
{
    if (sizes.isEmpty()) return 0;
    // Parse "192x192" → 192, "any" → 0
    int idx = sizes.indexOf('x');
    if (idx <= 0) return 0;
    bool ok = false;
    int val = sizes.left(idx).toInt(&ok);
    return ok ? val : 0;
}

Result<Manifest> ManifestParser::validate(const QJsonObject &json)
{
    if (json.isEmpty())
        return Result<Manifest>::fail(QStringLiteral("Empty manifest"));

    Manifest m;

    if (json.contains(QStringLiteral("name")) && json[QStringLiteral("name")].isString())
        m.name = json[QStringLiteral("name")].toString();
    if (json.contains(QStringLiteral("short_name")) && json[QStringLiteral("short_name")].isString())
        m.shortName = json[QStringLiteral("short_name")].toString();
    if (json.contains(QStringLiteral("start_url")) && json[QStringLiteral("start_url")].isString())
        m.startUrl = json[QStringLiteral("start_url")].toString();
    if (json.contains(QStringLiteral("scope")) && json[QStringLiteral("scope")].isString())
        m.scope = json[QStringLiteral("scope")].toString();
    if (json.contains(QStringLiteral("theme_color")) && json[QStringLiteral("theme_color")].isString())
        m.themeColor = json[QStringLiteral("theme_color")].toString();
    if (json.contains(QStringLiteral("background_color")) && json[QStringLiteral("background_color")].isString())
        m.backgroundColor = json[QStringLiteral("background_color")].toString();

    // Validate display mode
    if (json.contains(QStringLiteral("display")) && json[QStringLiteral("display")].isString()) {
        m.display = json[QStringLiteral("display")].toString();
        static const QStringList valid = {
            QStringLiteral("fullscreen"), QStringLiteral("standalone"),
            QStringLiteral("minimal-ui"), QStringLiteral("browser")
        };
        if (!valid.contains(m.display))
            return Result<Manifest>::fail(QStringLiteral("Invalid display mode: ") + m.display);
    }

    // Parse id
    if (json.contains(QStringLiteral("id")) && json[QStringLiteral("id")].isString())
        m.id = json[QStringLiteral("id")].toString();

    // Parse display_override array
    if (json.contains(QStringLiteral("display_override")) && json[QStringLiteral("display_override")].isArray()) {
        for (const auto &val : json[QStringLiteral("display_override")].toArray()) {
            if (val.isString())
                m.displayOverride.append(val.toString());
        }
    }

    // Parse shortcuts array
    if (json.contains(QStringLiteral("shortcuts")) && json[QStringLiteral("shortcuts")].isArray()) {
        for (const auto &val : json[QStringLiteral("shortcuts")].toArray()) {
            if (!val.isObject()) continue;
            auto obj = val.toObject();
            ManifestShortcut sc;
            sc.name = obj[QStringLiteral("name")].toString();
            sc.url = obj[QStringLiteral("url")].toString();
            if (sc.name.isEmpty() || sc.url.isEmpty()) continue;
            sc.description = obj[QStringLiteral("description")].toString();
            if (obj.contains(QStringLiteral("icons")) && obj[QStringLiteral("icons")].isArray()) {
                auto iconsArr = obj[QStringLiteral("icons")].toArray();
                if (!iconsArr.isEmpty() && iconsArr[0].isObject())
                    sc.iconSrc = iconsArr[0].toObject()[QStringLiteral("src")].toString();
            }
            m.shortcuts.append(sc);
        }
    }

    // Parse file_handlers array
    if (json.contains(QStringLiteral("file_handlers")) && json[QStringLiteral("file_handlers")].isArray()) {
        for (const auto &val : json[QStringLiteral("file_handlers")].toArray()) {
            if (!val.isObject()) continue;
            auto obj = val.toObject();
            ManifestFileHandler fh;
            fh.action = obj[QStringLiteral("action")].toString();
            if (fh.action.isEmpty()) continue;
            auto acceptObj = obj[QStringLiteral("accept")].toObject();
            for (auto it = acceptObj.begin(); it != acceptObj.end(); ++it)
                fh.accept.append(it.key());
            m.fileHandlers.append(fh);
        }
    }

    // Parse protocol_handlers array
    if (json.contains(QStringLiteral("protocol_handlers")) && json[QStringLiteral("protocol_handlers")].isArray()) {
        for (const auto &val : json[QStringLiteral("protocol_handlers")].toArray()) {
            if (!val.isObject()) continue;
            auto obj = val.toObject();
            ManifestProtocolHandler ph;
            ph.protocol = obj[QStringLiteral("protocol")].toString();
            ph.url = obj[QStringLiteral("url")].toString();
            if (ph.protocol.isEmpty() || ph.url.isEmpty()) continue;
            m.protocolHandlers.append(ph);
        }
    }

    // Parse icons array
    if (json.contains(QStringLiteral("icons")) && json[QStringLiteral("icons")].isArray()) {
        for (const auto &val : json[QStringLiteral("icons")].toArray()) {
            if (!val.isObject()) continue;
            auto obj = val.toObject();
            ManifestIcon icon;
            icon.src = obj[QStringLiteral("src")].toString();
            if (icon.src.isEmpty()) continue; // src is required
            icon.sizes = obj[QStringLiteral("sizes")].toString();
            icon.type = obj[QStringLiteral("type")].toString();
            icon.purpose = obj[QStringLiteral("purpose")].toString();
            m.icons.append(icon);
        }
    }

    return Result<Manifest>::ok(m);
}

QString ManifestParser::pickBestIcon(const QVector<ManifestIcon> &icons, const QUrl &baseUrl)
{
    if (icons.isEmpty()) return {};

    // Prefer 192x192, then largest
    int bestIdx = 0;
    int bestSize = parseSize(icons[0].sizes);
    bool found192 = (bestSize == 192);

    for (int i = 1; i < icons.size(); ++i) {
        int s = parseSize(icons[i].sizes);
        if (!found192 && s == 192) {
            bestIdx = i; bestSize = s; found192 = true;
        } else if (!found192 && s > bestSize) {
            bestIdx = i; bestSize = s;
        }
    }

    QString src = icons[bestIdx].src;
    if (src.startsWith(QStringLiteral("http://")) || src.startsWith(QStringLiteral("https://")))
        return src;
    return baseUrl.resolved(QUrl(src)).toString();
}

} // namespace qapp
