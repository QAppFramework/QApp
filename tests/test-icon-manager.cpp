#include <QTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include "icon-manager.hpp"

class TestIconManager : public QObject {
    Q_OBJECT
private slots:
    void hashToHueDeterministic() {
        int h1 = qapp::IconManager::hashToHue("github");
        int h2 = qapp::IconManager::hashToHue("github");
        QCOMPARE(h1, h2);
    }

    void hashToHueRange() {
        QStringList names = {"a", "github", "test", "zzz", "Hello World"};
        for (const auto &name : names) {
            int h = qapp::IconManager::hashToHue(name);
            QVERIFY2(h >= 0 && h < 360,
                     qPrintable(name + " hue=" + QString::number(h)));
        }
    }

    void hashToHueDifferent() {
        QVERIFY(qapp::IconManager::hashToHue("foo") !=
                qapp::IconManager::hashToHue("bar"));
    }

    void generateLetterIcon() {
        auto icon = qapp::IconManager::generateLetterIcon("GitHub");
        QVERIFY(icon.startsWith("data:image/svg+xml,"));
        QVERIFY(icon.contains("G")); // First letter uppercase
    }

    void generateLetterIconEmpty() {
        auto icon = qapp::IconManager::generateLetterIcon("");
        QVERIFY(icon.startsWith("data:image/svg+xml,"));
        QVERIFY(icon.contains("%3F")); // '?' URL-encoded
    }

    void defaultIcon() {
        auto icon = qapp::IconManager::defaultIcon();
        QVERIFY(icon.startsWith("data:image/svg+xml,"));
    }

    void detectFormatDataUri() {
        QCOMPARE(qapp::IconManager::detectFormat("data:image/svg+xml,..."), "svg");
        QCOMPARE(qapp::IconManager::detectFormat("data:image/png;base64,..."), "png");
        QCOMPARE(qapp::IconManager::detectFormat("data:image/ico,..."), "ico");
        QCOMPARE(qapp::IconManager::detectFormat("data:image/webp;base64,..."), "webp");
    }

    void detectFormatExtension() {
        QCOMPARE(qapp::IconManager::detectFormat("/path/icon.svg"), "svg");
        QCOMPARE(qapp::IconManager::detectFormat("/path/icon.ico"), "ico");
        QCOMPARE(qapp::IconManager::detectFormat("/path/icon.webp"), "webp");
        QCOMPARE(qapp::IconManager::detectFormat("/path/icon.png"), "png");
        QCOMPARE(qapp::IconManager::detectFormat("/path/icon.jpg"), "png"); // default
    }

    void saveDataUriBase64() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.path() + "/icon.png";
        // Minimal valid base64 PNG header
        auto r = qapp::IconManager::saveDataUri(
            "data:image/png;base64,iVBORw0KGgo=", path);
        QVERIFY(r.success);
        QVERIFY(QFile::exists(path));
    }

    void saveDataUriPercentEncoded() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.path() + "/icon.svg";
        auto r = qapp::IconManager::saveDataUri(
            "data:image/svg+xml,%3Csvg%3E%3C/svg%3E", path);
        QVERIFY(r.success);
        QVERIFY(QFile::exists(path));
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), "<svg></svg>");
    }

    void saveDataUriInvalid() {
        auto r = qapp::IconManager::saveDataUri("not-a-data-uri", "/tmp/test.png");
        QVERIFY(!r.success);
        QCOMPARE(r.error, "Invalid data URI");
    }

    void saveDataUriCreatesDir() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.path() + "/sub/deep/icon.png";
        auto r = qapp::IconManager::saveDataUri(
            "data:image/png;base64,iVBORw0KGgo=", path);
        QVERIFY(r.success);
        QVERIFY(QFile::exists(path));
    }
};

QTEST_MAIN(TestIconManager)
#include "test-icon-manager.moc"
