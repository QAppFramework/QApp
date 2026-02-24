#include <QTest>
#include <QJsonArray>
#include "manifest-parser.hpp"

class TestManifestParser : public QObject {
    Q_OBJECT
private slots:
    void emptyObject() {
        auto r = qapp::ManifestParser::validate(QJsonObject());
        QVERIFY(!r.success);
        QCOMPARE(r.error, "Empty manifest");
    }

    void minimalManifest() {
        QJsonObject json;
        json["name"] = "My App";
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.name, "My App");
    }

    void fullManifest() {
        QJsonObject json;
        json["name"] = "Full App";
        json["short_name"] = "Full";
        json["start_url"] = "/start";
        json["scope"] = "/";
        json["display"] = "standalone";
        json["theme_color"] = "#ff0000";
        json["background_color"] = "#ffffff";
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.name, "Full App");
        QCOMPARE(r.data.shortName, "Full");
        QCOMPARE(r.data.startUrl, "/start");
        QCOMPARE(r.data.scope, "/");
        QCOMPARE(r.data.display, "standalone");
        QCOMPARE(r.data.themeColor, "#ff0000");
        QCOMPARE(r.data.backgroundColor, "#ffffff");
    }

    void invalidDisplay() {
        QJsonObject json;
        json["name"] = "Bad Display";
        json["display"] = "invalid-mode";
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(!r.success);
        QVERIFY(r.error.contains("Invalid display mode"));
    }

    void validDisplayModes() {
        QStringList modes = {"fullscreen", "standalone", "minimal-ui", "browser"};
        for (const auto &mode : modes) {
            QJsonObject json;
            json["name"] = "Test";
            json["display"] = mode;
            auto r = qapp::ManifestParser::validate(json);
            QVERIFY2(r.success, qPrintable("Mode should be valid: " + mode));
            QCOMPARE(r.data.display, mode);
        }
    }

    void iconsArray() {
        QJsonArray icons;
        QJsonObject icon1;
        icon1["src"] = "/icon-192.png";
        icon1["sizes"] = "192x192";
        icon1["type"] = "image/png";
        icons.append(icon1);
        QJsonObject icon2;
        icon2["src"] = "/icon-512.png";
        icon2["sizes"] = "512x512";
        icons.append(icon2);

        QJsonObject json;
        json["name"] = "With Icons";
        json["icons"] = icons;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.icons.size(), 2);
        QCOMPARE(r.data.icons[0].src, "/icon-192.png");
        QCOMPARE(r.data.icons[0].sizes, "192x192");
        QCOMPARE(r.data.icons[1].src, "/icon-512.png");
    }

    void iconsSkipEmptySrc() {
        QJsonArray icons;
        QJsonObject icon1;
        icon1["src"] = "";
        icons.append(icon1);
        QJsonObject icon2;
        icon2["src"] = "/valid.png";
        icons.append(icon2);

        QJsonObject json;
        json["name"] = "Test";
        json["icons"] = icons;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.icons.size(), 1);
        QCOMPARE(r.data.icons[0].src, "/valid.png");
    }

    void pickBestIconPrefers192() {
        QVector<qapp::ManifestIcon> icons = {
            {"/icon-512.png", "512x512", "", ""},
            {"/icon-192.png", "192x192", "", ""},
            {"/icon-64.png", "64x64", "", ""},
        };
        auto best = qapp::ManifestParser::pickBestIcon(icons, QUrl("https://example.com"));
        QCOMPARE(best, "https://example.com/icon-192.png");
    }

    void pickBestIconFallsBackToLargest() {
        QVector<qapp::ManifestIcon> icons = {
            {"/icon-64.png", "64x64", "", ""},
            {"/icon-512.png", "512x512", "", ""},
            {"/icon-128.png", "128x128", "", ""},
        };
        auto best = qapp::ManifestParser::pickBestIcon(icons, QUrl("https://example.com"));
        QCOMPARE(best, "https://example.com/icon-512.png");
    }

    void pickBestIconAbsoluteUrl() {
        QVector<qapp::ManifestIcon> icons = {
            {"https://cdn.example.com/icon.png", "192x192", "", ""},
        };
        auto best = qapp::ManifestParser::pickBestIcon(icons, QUrl("https://example.com"));
        QCOMPARE(best, "https://cdn.example.com/icon.png");
    }

    void pickBestIconEmpty() {
        auto best = qapp::ManifestParser::pickBestIcon({}, QUrl("https://example.com"));
        QVERIFY(best.isEmpty());
    }

    void displayOverride() {
        QJsonObject json;
        json["name"] = "Override App";
        json["display"] = "browser";
        QJsonArray doArr;
        doArr.append("window-controls-overlay");
        doArr.append("standalone");
        doArr.append("minimal-ui");
        json["display_override"] = doArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.displayOverride.size(), 3);
        QCOMPARE(r.data.displayOverride[0], "window-controls-overlay");
        QCOMPARE(r.data.displayOverride[1], "standalone");
    }

    void manifestId() {
        QJsonObject json;
        json["name"] = "ID App";
        json["id"] = "/?source=pwa";
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.id, "/?source=pwa");
    }

    void shortcuts() {
        QJsonObject json;
        json["name"] = "Shortcut App";
        QJsonArray scArr;
        QJsonObject sc1;
        sc1["name"] = "New Post";
        sc1["url"] = "/compose";
        sc1["description"] = "Create a new post";
        scArr.append(sc1);
        QJsonObject sc2;
        sc2["name"] = "Settings";
        sc2["url"] = "/settings";
        scArr.append(sc2);
        json["shortcuts"] = scArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.shortcuts.size(), 2);
        QCOMPARE(r.data.shortcuts[0].name, "New Post");
        QCOMPARE(r.data.shortcuts[0].url, "/compose");
        QCOMPARE(r.data.shortcuts[0].description, "Create a new post");
        QCOMPARE(r.data.shortcuts[1].name, "Settings");
    }

    void shortcutsSkipInvalid() {
        QJsonObject json;
        json["name"] = "Test";
        QJsonArray scArr;
        QJsonObject sc1;
        sc1["name"] = ""; // empty name → skip
        sc1["url"] = "/test";
        scArr.append(sc1);
        QJsonObject sc2;
        sc2["name"] = "Valid";
        sc2["url"] = "/valid";
        scArr.append(sc2);
        json["shortcuts"] = scArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.shortcuts.size(), 1);
        QCOMPARE(r.data.shortcuts[0].name, "Valid");
    }

    void fileHandlers() {
        QJsonObject json;
        json["name"] = "Editor";
        QJsonArray fhArr;
        QJsonObject fh;
        fh["action"] = "/open-file";
        QJsonObject accept;
        accept["text/plain"] = QJsonArray({".txt"});
        accept["text/html"] = QJsonArray({".html", ".htm"});
        fh["accept"] = accept;
        fhArr.append(fh);
        json["file_handlers"] = fhArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.fileHandlers.size(), 1);
        QCOMPARE(r.data.fileHandlers[0].action, "/open-file");
        QCOMPARE(r.data.fileHandlers[0].accept.size(), 2);
        QVERIFY(r.data.fileHandlers[0].accept.contains("text/plain"));
        QVERIFY(r.data.fileHandlers[0].accept.contains("text/html"));
    }

    void fileHandlersSkipNoAction() {
        QJsonObject json;
        json["name"] = "Test";
        QJsonArray fhArr;
        QJsonObject fh;
        // missing action → skip
        QJsonObject accept;
        accept["image/png"] = QJsonArray({".png"});
        fh["accept"] = accept;
        fhArr.append(fh);
        json["file_handlers"] = fhArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.fileHandlers.size(), 0);
    }

    void protocolHandlers() {
        QJsonObject json;
        json["name"] = "Chat App";
        QJsonArray phArr;
        QJsonObject ph;
        ph["protocol"] = "web+chat";
        ph["url"] = "/chat?uri=%s";
        phArr.append(ph);
        json["protocol_handlers"] = phArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.protocolHandlers.size(), 1);
        QCOMPARE(r.data.protocolHandlers[0].protocol, "web+chat");
        QCOMPARE(r.data.protocolHandlers[0].url, "/chat?uri=%s");
    }

    void protocolHandlersSkipInvalid() {
        QJsonObject json;
        json["name"] = "Test";
        QJsonArray phArr;
        QJsonObject ph1;
        ph1["protocol"] = "web+test";
        // missing url → skip
        phArr.append(ph1);
        QJsonObject ph2;
        ph2["protocol"] = "web+valid";
        ph2["url"] = "/handle?uri=%s";
        phArr.append(ph2);
        json["protocol_handlers"] = phArr;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QCOMPARE(r.data.protocolHandlers.size(), 1);
        QCOMPARE(r.data.protocolHandlers[0].protocol, "web+valid");
    }

    void nonStringFieldsIgnored() {
        QJsonObject json;
        json["name"] = 42;
        json["short_name"] = true;
        auto r = qapp::ManifestParser::validate(json);
        QVERIFY(r.success);
        QVERIFY(r.data.name.isEmpty());
        QVERIFY(r.data.shortName.isEmpty());
    }
};

QTEST_MAIN(TestManifestParser)
#include "test-manifest-parser.moc"
