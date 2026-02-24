#include <QTest>
#include "xdg-paths.hpp"

class TestXdgPaths : public QObject
{
    Q_OBJECT

private slots:
    void resolveValidAppId()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(QStringLiteral("github-com"));
        QVERIFY(result.success);
        QVERIFY(result.data.desktopFile.endsWith(QStringLiteral("/applications/qapp-github-com.desktop")));
        QVERIFY(result.data.iconFile.endsWith(QStringLiteral("/qapp-framework/icons/github-com.png")));
        QVERIFY(result.data.metadataFile.endsWith(QStringLiteral("/qapp-framework/apps/github-com.json")));
        QVERIFY(result.data.iconsDir.endsWith(QStringLiteral("/qapp-framework/icons")));
        QVERIFY(result.data.appsDir.endsWith(QStringLiteral("/qapp-framework/apps")));
        QVERIFY(result.data.applicationsDir.endsWith(QStringLiteral("/applications")));
    }

    void resolveWithSvgExtension()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(
            QStringLiteral("example-com"), QStringLiteral("svg"));
        QVERIFY(result.success);
        QVERIFY(result.data.iconFile.endsWith(QStringLiteral("/qapp-framework/icons/example-com.svg")));
    }

    void resolveWithIcoExtension()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(
            QStringLiteral("example-com"), QStringLiteral("ico"));
        QVERIFY(result.success);
        QVERIFY(result.data.iconFile.endsWith(QStringLiteral("/qapp-framework/icons/example-com.ico")));
    }

    void resolveAppIdWithPort()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(QStringLiteral("localhost-3000"));
        QVERIFY(result.success);
        QVERIFY(result.data.desktopFile.endsWith(QStringLiteral("/applications/qapp-localhost-3000.desktop")));
        QVERIFY(result.data.iconFile.endsWith(QStringLiteral("/qapp-framework/icons/localhost-3000.png")));
        QVERIFY(result.data.metadataFile.endsWith(QStringLiteral("/qapp-framework/apps/localhost-3000.json")));
    }

    void emptyAppIdFails()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(QString());
        QVERIFY(!result.success);
        QCOMPARE(result.error, QStringLiteral("App ID must not be empty"));
    }

    void pathsUnderLocalShare()
    {
        auto result = qapp::XdgPaths::resolveAppPaths(QStringLiteral("test-app"));
        QVERIFY(result.success);
        // All paths should be under .local/share
        QVERIFY(result.data.desktopFile.contains(QStringLiteral(".local/share")));
        QVERIFY(result.data.iconFile.contains(QStringLiteral(".local/share")));
        QVERIFY(result.data.metadataFile.contains(QStringLiteral(".local/share")));
    }
};

QTEST_MAIN(TestXdgPaths)
#include "test-xdg-paths.moc"
