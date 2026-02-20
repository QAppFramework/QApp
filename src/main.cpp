#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QDebug>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("PWAApp");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("TwistedBrain");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;

    const QUrl url(u"qrc:/PWAApp/qml/main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &url) {
            qCritical() << "QML object creation failed for:" << url;
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [](QObject *obj, const QUrl &url) {
            if (!obj) {
                qCritical() << "QML object is null for:" << url;
            } else {
                qDebug() << "QML loaded successfully:" << url;
            }
        },
        Qt::QueuedConnection);

    qDebug() << "Loading QML from:" << url;
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No root objects after load — QML failed";
        return -1;
    }

    return app.exec();
}
