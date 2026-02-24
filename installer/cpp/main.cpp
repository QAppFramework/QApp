#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtWebEngineQuick>
#include <QtQml/qqmlextensionplugin.h>
#include <cstring>

#include "classify-pipeline.hpp"
#include "app-manager.hpp"

Q_IMPORT_QML_PLUGIN(QApp_CommonPlugin)

using namespace Qt::StringLiterals;

static int runGui(int argc, char *argv[]);
static int runCli(int argc, char *argv[]);

/// Detect CLI mode by checking if any --flag is present.
static bool hasCliFlags(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--", 2) == 0)
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (hasCliFlags(argc, argv))
        return runCli(argc, argv);
    return runGui(argc, argv);
}

// --- GUI mode (existing behavior) ---

static int runGui(int argc, char *argv[])
{
#ifdef QAPP_DEV_BUILD
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#endif

    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setApplicationName(QAPP_APP_ID);
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("TwistedBrain");
    app.setDesktopFileName(u"qapp-installer"_s);
    app.setWindowIcon(QIcon(u":/QApp/Installer/icons/qapp-dark-500.png"_s));

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;
    engine.addImportPath(u"qrc:/"_s);

    const QUrl url(u"qrc:/QApp/Installer/qml/main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl &) { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

// --- CLI mode (headless, JSON output) ---

static void writeJson(const QJsonObject &obj)
{
    QTextStream out(stdout);
    out << QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

static void writeJson(const QJsonArray &arr)
{
    QTextStream out(stdout);
    out << QJsonDocument(arr).toJson(QJsonDocument::Indented);
}

static void writeError(const QString &msg)
{
    QTextStream err(stderr);
    err << msg << "\n";
}

static int runCli(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QAPP_APP_ID);
    app.setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("QApp installer — install web apps as native Linux apps");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption classifyOpt(u"classify"_s, u"Classify a URL"_s, u"url"_s);
    QCommandLineOption installOpt(u"install"_s, u"Install a URL as app"_s, u"url"_s);
    QCommandLineOption uninstallOpt(u"uninstall"_s, u"Uninstall an app"_s, u"appId"_s);
    QCommandLineOption listOpt(u"list"_s, u"List installed apps"_s);
    QCommandLineOption checkUpdatesOpt(u"check-updates"_s, u"Check for app updates"_s);

    parser.addOption(classifyOpt);
    parser.addOption(installOpt);
    parser.addOption(uninstallOpt);
    parser.addOption(listOpt);
    parser.addOption(checkUpdatesOpt);

    parser.process(app);

    // --- classify ---
    if (parser.isSet(classifyOpt)) {
        QString url = parser.value(classifyOpt);
        auto *pipeline = new qapp::installer::ClassifyPipeline(&app);

        QObject::connect(pipeline, &qapp::installer::ClassifyPipeline::finished,
                         &app, [&](const qapp::installer::ClassifyResult &result) {
            writeJson(result.toJson());
            app.exit(0);
        });
        QObject::connect(pipeline, &qapp::installer::ClassifyPipeline::failed,
                         &app, [&](const QString &error) {
            writeError(u"classify failed: "_s + error);
            app.exit(1);
        });

        pipeline->start(QUrl(url));
        return app.exec();
    }

    // --- install ---
    if (parser.isSet(installOpt)) {
        QString url = parser.value(installOpt);
        auto *installer = new qapp::installer::AppInstaller(&app);

        QObject::connect(installer, &qapp::installer::AppInstaller::resultChanged,
                         &app, [&]() {
            if (installer->installed()) {
                QJsonObject out;
                out[u"appId"_s] = installer->appId();
                out[u"name"_s] = installer->appName();
                out[u"desktopPath"_s] = installer->desktopPath();
                writeJson(out);
                app.exit(0);
            } else if (!installer->errorMessage().isEmpty()) {
                writeError(u"install failed: "_s + installer->errorMessage());
                app.exit(1);
            }
        });

        installer->install(url);
        return app.exec();
    }

    // --- uninstall ---
    if (parser.isSet(uninstallOpt)) {
        QString appId = parser.value(uninstallOpt);
        auto *installer = new qapp::installer::AppInstaller(&app);

        QObject::connect(installer, &qapp::installer::AppInstaller::resultChanged,
                         &app, [&]() {
            if (!installer->errorMessage().isEmpty()) {
                writeError(u"uninstall failed: "_s + installer->errorMessage());
                app.exit(1);
            } else {
                QJsonObject out;
                out[u"uninstalled"_s] = appId;
                writeJson(out);
                app.exit(0);
            }
        });

        installer->uninstall(appId);
        return app.exec();
    }

    // --- list ---
    if (parser.isSet(listOpt)) {
        auto *installer = new qapp::installer::AppInstaller(&app);

        QObject::connect(installer, &qapp::installer::AppInstaller::installedAppsChanged,
                         &app, [&]() {
            writeJson(installer->installedApps());
            app.exit(0);
        });

        installer->listApps();
        return app.exec();
    }

    // --- check-updates ---
    if (parser.isSet(checkUpdatesOpt)) {
        auto *installer = new qapp::installer::AppInstaller(&app);

        QObject::connect(installer, &qapp::installer::AppInstaller::appUpdatesChanged,
                         &app, [&]() {
            writeJson(installer->appUpdates());
            app.exit(0);
        });

        installer->checkUpdates();
        return app.exec();
    }

    // No recognized option — show help
    parser.showHelp(1);
    return 1;
}
