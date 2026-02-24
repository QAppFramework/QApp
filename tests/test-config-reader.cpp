#include <QTest>
#include <QTemporaryDir>
#include <QJsonObject>
#include "config-reader.hpp"

class TestConfigReader : public QObject
{
    Q_OBJECT

private slots:
    void readValidJson()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/test.json";

        // Write a test file
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"name": "GitHub", "level": "WS"})");
        file.close();

        auto result = qapp::ConfigReader::readJson(path);
        QVERIFY(result.success);
        QCOMPARE(result.data.value("name").toString(), QStringLiteral("GitHub"));
        QCOMPARE(result.data.value("level").toString(), QStringLiteral("WS"));
    }

    void readMissingFileFails()
    {
        auto result = qapp::ConfigReader::readJson(QStringLiteral("/nonexistent/path.json"));
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("File not found")));
    }

    void readEmptyPathFails()
    {
        auto result = qapp::ConfigReader::readJson(QString());
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Path must not be empty")));
    }

    void readInvalidJsonFails()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/bad.json";

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not valid json {{{");
        file.close();

        auto result = qapp::ConfigReader::readJson(path);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("JSON parse error")));
    }

    void readJsonArrayFails()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/array.json";

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[1, 2, 3]");
        file.close();

        auto result = qapp::ConfigReader::readJson(path);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("not an object")));
    }

    void writeAndReadRoundtrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/out.json";

        QJsonObject obj;
        obj["appId"] = QStringLiteral("github-com");
        obj["name"] = QStringLiteral("GitHub");

        auto writeResult = qapp::ConfigReader::writeJson(path, obj);
        QVERIFY(writeResult.success);

        auto readResult = qapp::ConfigReader::readJson(path);
        QVERIFY(readResult.success);
        QCOMPARE(readResult.data.value("appId").toString(), QStringLiteral("github-com"));
        QCOMPARE(readResult.data.value("name").toString(), QStringLiteral("GitHub"));
    }

    void writeCreatesDirectories()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/deep/nested/dir/config.json";

        QJsonObject obj;
        obj["test"] = true;

        auto result = qapp::ConfigReader::writeJson(path, obj);
        QVERIFY(result.success);

        // Verify file exists and is readable
        auto readResult = qapp::ConfigReader::readJson(path);
        QVERIFY(readResult.success);
        QCOMPARE(readResult.data.value("test").toBool(), true);
    }

    void writeEmptyPathFails()
    {
        QJsonObject obj;
        auto result = qapp::ConfigReader::writeJson(QString(), obj);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Path must not be empty")));
    }
};

QTEST_MAIN(TestConfigReader)
#include "test-config-reader.moc"
