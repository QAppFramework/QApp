#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick>
#include <cstdio>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setApplicationName("QApp Wrapper");
    app.setOrganizationName("TwistedBrain");

    QQmlApplicationEngine engine;

    const QUrl qmlUrl(QStringLiteral("qrc:/QAppWrapper/qml/wrapper.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &url) {
            fprintf(stderr, "QML object creation failed: %s\n", url.toString().toUtf8().constData());
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(qmlUrl);

    if (engine.rootObjects().isEmpty()) {
        fprintf(stderr, "QML failed to load\n");
        return -1;
    }

    return app.exec();
}
