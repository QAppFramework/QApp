#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtWebEngineQuick>

// CXX-Qt generated header for CLI extern "Rust" functions
#include "qapp_installer/src/cli.cxx.h"

static bool hasCliFlags(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-' && argv[i][1] == '-')
            return true;
    }
    return false;
}

static int runCli(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("QApp Framework — turn any website into a desktop app"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption classifyOpt(
        QStringLiteral("classify"),
        QStringLiteral("Classify a URL and output JSON"),
        QStringLiteral("url"));
    QCommandLineOption installOpt(
        QStringLiteral("install"),
        QStringLiteral("Install a URL as a desktop app"),
        QStringLiteral("url"));
    QCommandLineOption uninstallOpt(
        QStringLiteral("uninstall"),
        QStringLiteral("Uninstall an app by ID"),
        QStringLiteral("appId"));
    QCommandLineOption listOpt(
        QStringLiteral("list"),
        QStringLiteral("List installed apps"));
    QCommandLineOption checkUpdatesOpt(
        QStringLiteral("check-updates"),
        QStringLiteral("Check installed apps for updates"));

    parser.addOption(classifyOpt);
    parser.addOption(installOpt);
    parser.addOption(uninstallOpt);
    parser.addOption(listOpt);
    parser.addOption(checkUpdatesOpt);

    parser.process(app);

    QString output;

    if (parser.isSet(classifyOpt)) {
        QString url = parser.value(classifyOpt);
        output = cli_classify(url);
    } else if (parser.isSet(installOpt)) {
        QString url = parser.value(installOpt);
        output = cli_install(url);
    } else if (parser.isSet(uninstallOpt)) {
        QString appId = parser.value(uninstallOpt);
        output = cli_uninstall(appId);
    } else if (parser.isSet(listOpt)) {
        output = cli_list();
    } else if (parser.isSet(checkUpdatesOpt)) {
        output = cli_check_updates();
    } else {
        parser.showHelp(1);
    }

    fprintf(stdout, "%s\n", qPrintable(output));
    return 0;
}

int main(int argc, char *argv[])
{
#ifdef QAPP_DEV_BUILD
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#endif

    // CLI mode: no GUI, no WebEngine init
    if (hasCliFlags(argc, argv)) {
        QCoreApplication app(argc, argv);
        app.setApplicationName(QStringLiteral(QAPP_APP_ID));
        app.setApplicationVersion(QStringLiteral(QAPP_VERSION));
        app.setOrganizationName(QStringLiteral("TwistedBrain"));
        return runCli(app);
    }

    // GUI mode
    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(QAPP_APP_ID));
    app.setApplicationVersion(QStringLiteral(QAPP_VERSION));
    app.setOrganizationName(QStringLiteral("TwistedBrain"));
    app.setDesktopFileName(QStringLiteral("qapp-installer"));
    app.setWindowIcon(QIcon(QStringLiteral(":/QApp/Installer/icons/qapp-dark-500.png")));

    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("appBuildSha"), QStringLiteral(QAPP_BUILD_SHA));

    const QUrl url(QStringLiteral("qrc:/qt/qml/QApp/Installer/qml/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &) { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
