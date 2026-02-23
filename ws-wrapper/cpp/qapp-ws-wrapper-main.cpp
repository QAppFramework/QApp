#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QtWebEngineQuick>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(QApp_CommonPlugin)

int main(int argc, char *argv[])
{
#ifdef QAPP_DEV_BUILD
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");
#endif

    // Derive per-app identity from argv[1] (appId)
    // Each wrapper app is self-contained: own QSettings, own WebEngine profile
    QString appId = (argc > 1) ? QString::fromUtf8(argv[1]) : QStringLiteral("default");
    QString appName = QStringLiteral("qapp-") + appId;

    // Read per-app profile settings before WebEngine init (V8 flags must be set early)
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
    app.setOrganizationName(QStringLiteral("QAppFramework"));

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));

    const QUrl qmlUrl(QStringLiteral("qrc:/QApp/WS/qml/wrapper.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &) { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(qmlUrl);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
