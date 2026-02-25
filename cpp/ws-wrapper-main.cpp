#include <QApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QtWebEngineQuick>

int main(int argc, char *argv[])
{
#ifdef QAPP_DEV_BUILD
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");
#endif

    // Derive per-app identity from argv[1] (appId)
    QString appId = (argc > 1) ? QString::fromUtf8(argv[1]) : QStringLiteral("default");
    QString appName = QStringLiteral("qapp-") + appId;

    // Read per-app profile settings before WebEngine init
    QSettings settings(QStringLiteral("QAppFramework"), appName);
    int heapMB = settings.value("profile/jsHeapSizeMB", 4096).toInt();
    if (heapMB > 0) {
        QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        if (!flags.isEmpty()) flags += ' ';
        flags += "--js-flags=--max-old-space-size=" + QByteArray::number(heapMB);
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);
    }

    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setApplicationName(appName);
    app.setApplicationVersion(QStringLiteral(QAPP_VERSION " (" QAPP_BUILD_SHA ")"));
    app.setOrganizationName(QStringLiteral("QAppFramework"));

    // Set window icon from installed app metadata
    QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString metaPath = dataHome + QStringLiteral("/qapp-framework/apps/") + appId
                       + QStringLiteral("/config.json");
    QFile metaFile(metaPath);
    if (metaFile.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(metaFile.readAll()).object();
        QString iconPath = obj.value(QStringLiteral("iconPath")).toString();
        if (!iconPath.isEmpty())
            app.setWindowIcon(QIcon(iconPath));
    }

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("appBuildSha"), QStringLiteral(QAPP_BUILD_SHA));

    const QUrl qmlUrl(QStringLiteral("qrc:/qt/qml/QApp/WS/qml/wrapper.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &) { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(qmlUrl);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
