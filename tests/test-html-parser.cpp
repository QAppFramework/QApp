#include <QTest>
#include "html-parser.hpp"

class TestHtmlParser : public QObject {
    Q_OBJECT
private slots:
    void manifestLink() {
        auto meta = qapp::HtmlParser::parse(
            R"(<html><head><link rel="manifest" href="/manifest.json"></head></html>)",
            QUrl("https://example.com/page"));
        QCOMPARE(meta.manifestUrl, "https://example.com/manifest.json");
    }

    void manifestLinkReversed() {
        auto meta = qapp::HtmlParser::parse(
            R"(<link href="/app.webmanifest" rel="manifest">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.manifestUrl, "https://example.com/app.webmanifest");
    }

    void title() {
        auto meta = qapp::HtmlParser::parse(
            "<html><head><title> My App </title></head></html>",
            QUrl("https://example.com"));
        QCOMPARE(meta.title, "My App");
    }

    void icons() {
        auto meta = qapp::HtmlParser::parse(
            R"(<link rel="icon" href="/favicon.png" sizes="32x32">
               <link rel="apple-touch-icon" href="/apple.png" sizes="180x180">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.icons.size(), 2);
        QCOMPARE(meta.icons[0].first, "https://example.com/favicon.png");
        QCOMPARE(meta.icons[0].second, "32x32");
        QCOMPARE(meta.icons[1].first, "https://example.com/apple.png");
    }

    void themeColor() {
        auto meta = qapp::HtmlParser::parse(
            R"(<meta name="theme-color" content="#24292e">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.themeColor, "#24292e");
    }

    void themeColorReversed() {
        auto meta = qapp::HtmlParser::parse(
            R"(<meta content="#ff0000" name="theme-color">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.themeColor, "#ff0000");
    }

    void ogImage() {
        auto meta = qapp::HtmlParser::parse(
            R"(<meta property="og:image" content="/images/og.png">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.ogImage, "https://example.com/images/og.png");
    }

    void swHint() {
        auto meta = qapp::HtmlParser::parse(
            "<script>navigator.serviceWorker.register('/sw.js')</script>",
            QUrl("https://example.com"));
        QVERIFY(meta.swHint);
    }

    void noSwHint() {
        auto meta = qapp::HtmlParser::parse(
            "<html><body>Hello</body></html>",
            QUrl("https://example.com"));
        QVERIFY(!meta.swHint);
    }

    void emptyHtml() {
        auto meta = qapp::HtmlParser::parse("", QUrl("https://example.com"));
        QVERIFY(meta.manifestUrl.isEmpty());
        QVERIFY(meta.title.isEmpty());
        QVERIFY(meta.icons.isEmpty());
        QVERIFY(!meta.swHint);
    }

    void absoluteUrls() {
        auto meta = qapp::HtmlParser::parse(
            R"(<link rel="manifest" href="https://cdn.example.com/manifest.json">)",
            QUrl("https://example.com"));
        QCOMPARE(meta.manifestUrl, "https://cdn.example.com/manifest.json");
    }
};

QTEST_MAIN(TestHtmlParser)
#include "test-html-parser.moc"
