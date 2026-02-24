#include <QTest>
#include <QDateTime>
#include <QJsonObject>
#include "app-metadata.hpp"

class TestAppMetadata : public QObject
{
    Q_OBJECT

private:
    qapp::AppMetadataData makeValidInput()
    {
        qapp::AppMetadataData input;
        input.appId = QStringLiteral("github-com");
        input.name = QStringLiteral("GitHub");
        input.url = QStringLiteral("https://github.com");
        input.level = QStringLiteral("WS");
        input.iconPath = QStringLiteral("/home/user/.local/share/qapp-framework/icons/github-com.png");
        input.desktopPath = QStringLiteral("/home/user/.local/share/applications/qapp-github-com.desktop");
        input.wrapperPath = QStringLiteral("/usr/bin/qapp-ws-wrapper");
        return input;
    }

private slots:
    // --- build tests ---

    void buildAddsTimestamp()
    {
        auto input = makeValidInput();
        QDateTime before = QDateTime::currentDateTimeUtc();

        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);

        QDateTime after = QDateTime::currentDateTimeUtc();

        // installedAt should be a valid ISO timestamp between before and after
        QDateTime installed = QDateTime::fromString(result.data.installedAt, Qt::ISODateWithMs);
        QVERIFY(installed.isValid());
        QVERIFY(installed >= before.addMSecs(-1000)); // 1s tolerance
        QVERIFY(installed <= after.addMSecs(1000));
    }

    void buildPreservesFields()
    {
        auto input = makeValidInput();
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
        QCOMPARE(result.data.appId, QStringLiteral("github-com"));
        QCOMPARE(result.data.name, QStringLiteral("GitHub"));
        QCOMPARE(result.data.url, QStringLiteral("https://github.com"));
        QCOMPARE(result.data.level, QStringLiteral("WS"));
    }

    void buildAcceptsWappLevel()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("WAPP");
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
        QCOMPARE(result.data.level, QStringLiteral("WAPP"));
    }

    void buildAcceptsPwappLevel()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("PWAPP");
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
    }

    void buildRejectsInvalidLevel()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("INVALID");
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("level")));
    }

    void buildWithOptionalManifestFields()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("WAPP");
        input.displayMode = QStringLiteral("standalone");
        input.themeColor = QStringLiteral("#ff5500");
        input.startUrl = QStringLiteral("https://github.com/dashboard");
        input.scope = QStringLiteral("https://github.com/");

        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
        QCOMPARE(result.data.displayMode, QStringLiteral("standalone"));
        QCOMPARE(result.data.themeColor, QStringLiteral("#ff5500"));
        QCOMPARE(result.data.startUrl, QStringLiteral("https://github.com/dashboard"));
        QCOMPARE(result.data.scope, QStringLiteral("https://github.com/"));
    }

    void buildAcceptsValidDisplayModes()
    {
        QStringList modes = {
            QStringLiteral("fullscreen"),
            QStringLiteral("standalone"),
            QStringLiteral("minimal-ui"),
            QStringLiteral("browser"),
        };
        for (const QString &mode : modes) {
            auto input = makeValidInput();
            input.displayMode = mode;
            auto result = qapp::AppMetadata::build(input);
            QVERIFY2(result.success, qPrintable(QStringLiteral("Failed for mode: ") + mode));
        }
    }

    void buildRejectsInvalidDisplayMode()
    {
        auto input = makeValidInput();
        input.displayMode = QStringLiteral("popup");
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("displayMode")));
    }

    void buildEmptyAppIdFails()
    {
        auto input = makeValidInput();
        input.appId = QString();
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("appId")));
    }

    void buildEmptyNameFails()
    {
        auto input = makeValidInput();
        input.name = QString();
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("name")));
    }

    void buildEmptyUrlFails()
    {
        auto input = makeValidInput();
        input.url = QString();
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("url")));
    }

    void buildBackwardCompatibleWithoutOptional()
    {
        // Optional fields can be omitted (empty) — backward compatible
        auto input = makeValidInput();
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
        QVERIFY(result.data.displayMode.isEmpty());
        QVERIFY(result.data.themeColor.isEmpty());
        QVERIFY(result.data.backgroundColor.isEmpty());
        QVERIFY(result.data.startUrl.isEmpty());
        QVERIFY(result.data.scope.isEmpty());
    }

    void buildWithBackgroundColor()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("PWAPP");
        input.backgroundColor = QStringLiteral("#1a1a2e");
        auto result = qapp::AppMetadata::build(input);
        QVERIFY(result.success);
        QCOMPARE(result.data.backgroundColor, QStringLiteral("#1a1a2e"));
    }

    void toJsonBackgroundColorRoundtrip()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("PWAPP");
        input.backgroundColor = QStringLiteral("#ffffff");
        auto buildResult = qapp::AppMetadata::build(input);
        QVERIFY(buildResult.success);

        QJsonObject json = qapp::AppMetadata::toJson(buildResult.data);
        QVERIFY(json.contains(QStringLiteral("backgroundColor")));
        QCOMPARE(json.value("backgroundColor").toString(), QStringLiteral("#ffffff"));

        auto validateResult = qapp::AppMetadata::validate(json);
        QVERIFY(validateResult.success);
        QCOMPARE(validateResult.data.backgroundColor, QStringLiteral("#ffffff"));
    }

    void validateOldMetadataWithoutBackgroundColor()
    {
        // Old metadata without backgroundColor should still validate
        QJsonObject obj;
        obj["appId"] = QStringLiteral("test-app");
        obj["name"] = QStringLiteral("Test");
        obj["url"] = QStringLiteral("https://test.com");
        obj["level"] = QStringLiteral("WAPP");
        obj["iconPath"] = QStringLiteral("/path/icon.png");
        obj["desktopPath"] = QStringLiteral("/path/desktop.desktop");
        obj["wrapperPath"] = QStringLiteral("/usr/bin/qapp-ws-wrapper");
        obj["installedAt"] = QStringLiteral("2026-02-22T14:00:00.000Z");

        auto result = qapp::AppMetadata::validate(obj);
        QVERIFY(result.success);
        QVERIFY(result.data.backgroundColor.isEmpty());
    }

    // --- validate tests ---

    void validateValidJsonObject()
    {
        QJsonObject obj;
        obj["appId"] = QStringLiteral("github-com");
        obj["name"] = QStringLiteral("GitHub");
        obj["url"] = QStringLiteral("https://github.com");
        obj["level"] = QStringLiteral("WS");
        obj["iconPath"] = QStringLiteral("/path/to/icon.png");
        obj["desktopPath"] = QStringLiteral("/path/to/desktop.desktop");
        obj["wrapperPath"] = QStringLiteral("/usr/bin/qapp-ws-wrapper");
        obj["installedAt"] = QStringLiteral("2026-02-22T14:00:00.000Z");

        auto result = qapp::AppMetadata::validate(obj);
        QVERIFY(result.success);
        QCOMPARE(result.data.appId, QStringLiteral("github-com"));
        QCOMPARE(result.data.name, QStringLiteral("GitHub"));
    }

    void validateWithOptionalFields()
    {
        QJsonObject obj;
        obj["appId"] = QStringLiteral("example-com");
        obj["name"] = QStringLiteral("Example");
        obj["url"] = QStringLiteral("https://example.com");
        obj["level"] = QStringLiteral("WAPP");
        obj["iconPath"] = QStringLiteral("/path/icon.png");
        obj["desktopPath"] = QStringLiteral("/path/desktop.desktop");
        obj["wrapperPath"] = QStringLiteral("/usr/bin/qapp-ws-wrapper");
        obj["installedAt"] = QStringLiteral("2026-02-22T14:00:00.000Z");
        obj["displayMode"] = QStringLiteral("standalone");
        obj["themeColor"] = QStringLiteral("#24292e");

        auto result = qapp::AppMetadata::validate(obj);
        QVERIFY(result.success);
        QCOMPARE(result.data.displayMode, QStringLiteral("standalone"));
        QCOMPARE(result.data.themeColor, QStringLiteral("#24292e"));
    }

    void validateMissingFieldFails()
    {
        QJsonObject obj;
        obj["appId"] = QStringLiteral("test");
        // Missing name, url, level, etc.
        auto result = qapp::AppMetadata::validate(obj);
        QVERIFY(!result.success);
    }

    void validateEmptyObjectFails()
    {
        QJsonObject obj;
        auto result = qapp::AppMetadata::validate(obj);
        QVERIFY(!result.success);
    }

    // --- toJson tests ---

    void toJsonRoundtrip()
    {
        auto input = makeValidInput();
        auto buildResult = qapp::AppMetadata::build(input);
        QVERIFY(buildResult.success);

        QJsonObject json = qapp::AppMetadata::toJson(buildResult.data);
        QCOMPARE(json.value("appId").toString(), QStringLiteral("github-com"));
        QCOMPARE(json.value("name").toString(), QStringLiteral("GitHub"));
        QCOMPARE(json.value("level").toString(), QStringLiteral("WS"));

        // Validate roundtrip
        auto validateResult = qapp::AppMetadata::validate(json);
        QVERIFY(validateResult.success);
        QCOMPARE(validateResult.data.appId, buildResult.data.appId);
        QCOMPARE(validateResult.data.name, buildResult.data.name);
    }

    void toJsonOmitsEmptyOptionalFields()
    {
        auto input = makeValidInput();
        auto buildResult = qapp::AppMetadata::build(input);
        QVERIFY(buildResult.success);

        QJsonObject json = qapp::AppMetadata::toJson(buildResult.data);
        QVERIFY(!json.contains(QStringLiteral("displayMode")));
        QVERIFY(!json.contains(QStringLiteral("themeColor")));
        QVERIFY(!json.contains(QStringLiteral("startUrl")));
        QVERIFY(!json.contains(QStringLiteral("scope")));
    }

    void toJsonIncludesSetOptionalFields()
    {
        auto input = makeValidInput();
        input.level = QStringLiteral("WAPP");
        input.displayMode = QStringLiteral("standalone");
        input.themeColor = QStringLiteral("#ff0000");

        auto buildResult = qapp::AppMetadata::build(input);
        QVERIFY(buildResult.success);

        QJsonObject json = qapp::AppMetadata::toJson(buildResult.data);
        QVERIFY(json.contains(QStringLiteral("displayMode")));
        QVERIFY(json.contains(QStringLiteral("themeColor")));
        QCOMPARE(json.value("displayMode").toString(), QStringLiteral("standalone"));
        QCOMPARE(json.value("themeColor").toString(), QStringLiteral("#ff0000"));
    }
};

QTEST_MAIN(TestAppMetadata)
#include "test-app-metadata.moc"
