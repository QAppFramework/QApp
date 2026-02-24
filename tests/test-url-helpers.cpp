#include <QTest>
#include "url-helpers.hpp"

class TestUrlHelpers : public QObject
{
    Q_OBJECT

private slots:
    // --- isValidUrl tests (matches JS url-validator.test.js) ---

    void validHttpsUrl()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://github.com")));
    }

    void validHttpUrl()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("http://example.com")));
    }

    void validUrlWithPath()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://github.com/QAppFramework")));
    }

    void validUrlWithQuery()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://example.com/search?q=test")));
    }

    void validUrlWithFragment()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://example.com/page#section")));
    }

    void validUrlWithSubdomain()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://www.example.com")));
    }

    void validUrlWithMultiPartTld()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://example.co.uk")));
    }

    void validUrlWithTrailingSlash()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://example.com/")));
    }

    void invalidEmptyString()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QString()));
    }

    void validBareHostname()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://mimer:8444")));
    }

    void validLocalhost()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://localhost:3000")));
    }

    void validIpWithPort()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("https://192.168.0.144:8444")));
    }

    void validBareHostnameNoPort()
    {
        QVERIFY(qapp::UrlHelpers::isValidUrl(QStringLiteral("http://nas")));
    }

    void invalidBareDomain()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QStringLiteral("example.com")));
    }

    void invalidFtpProtocol()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QStringLiteral("ftp://example.com")));
    }

    void invalidSpacesInDomain()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QStringLiteral("https://ex ample.com")));
    }

    void invalidBareHttps()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QStringLiteral("https://")));
    }

    void invalidBareHttp()
    {
        QVERIFY(!qapp::UrlHelpers::isValidUrl(QStringLiteral("http://")));
    }

    // --- generateAppId tests (matches JS app-id.test.js) ---

    void appIdFromHttpsUrl()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("https://github.com"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("github-com"));
    }

    void appIdFromHttpUrl()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("http://example.com"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("example-com"));
    }

    void appIdLowercases()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("https://GitHub.COM"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("github-com"));
    }

    void appIdIgnoresPath()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("https://github.com/QAppFramework/QApp"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("github-com"));
    }

    void appIdIgnoresQuery()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("https://example.com/search?q=test"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("example-com"));
    }

    void appIdWithNonDefaultPort()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("http://localhost:3000"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("localhost-3000"));
    }

    void appIdIpAddress()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("http://192.168.1.1:8080"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("192-168-1-1-8080"));
    }

    void appIdStripsDefaultHttpPort()
    {
        // QUrl doesn't report port 80 for http as explicit port
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("http://example.com:80"));
        QVERIFY(result.success);
        // Port 80 is default for http — should not appear in appId
        QCOMPARE(result.data, QStringLiteral("example-com"));
    }

    void appIdStripsDefaultHttpsPort()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("https://example.com:443"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("example-com"));
    }

    void appIdInvalidUrlFails()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("not a url"));
        QVERIFY(!result.success);
    }

    void appIdFtpFails()
    {
        auto result = qapp::UrlHelpers::generateAppId(QStringLiteral("ftp://example.com"));
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("http or https")));
    }

    // --- generateAppIdFromName tests (matches JS app-id.test.js) ---

    void nameIdSimple()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("QAppFramework"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("qappframework"));
    }

    void nameIdWithSpaces()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("My Cool App"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("my-cool-app"));
    }

    void nameIdTrimsWhitespace()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("  Trimmed  "));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("trimmed"));
    }

    void nameIdUnderscoresAndDots()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("app_v2.0"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("app-v2-0"));
    }

    void nameIdNonAsciiRemoved()
    {
        // Non-ASCII characters (Ø) are treated as non-alphanumeric → replaced with hyphens
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("\u00d8rsted Energy"));
        QVERIFY(result.success);
        QCOMPARE(result.data, QStringLiteral("rsted-energy"));
    }

    void nameIdEmptyFails()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QString());
        QVERIFY(!result.success);
    }

    void nameIdWhitespaceOnlyFails()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("   "));
        QVERIFY(!result.success);
    }

    void nameIdNonAlphanumericOnlyFails()
    {
        auto result = qapp::UrlHelpers::generateAppIdFromName(QStringLiteral("!!!"));
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("alphanumeric")));
    }
};

QTEST_MAIN(TestUrlHelpers)
#include "test-url-helpers.moc"
