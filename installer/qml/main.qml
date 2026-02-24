import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QApp.Installer
import QApp.Common

ApplicationWindow {
    id: root
    width: 640
    height: 480
    minimumWidth: 400
    minimumHeight: 300
    visible: true
    title: Qt.application.name === "qapp-installer-dev" ? "QApp Dev" : "QApp"

    Settings {
        id: windowSettings
        category: "window"
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    SiteClassifier {
        id: classifier
    }

    AppInstaller {
        id: appInstaller
    }

    // ── Keyboard shortcuts ──────────────────────────────────
    Shortcut {
        sequence: "F10"
        onActivated: mainMenu.popup()
    }

    Menu {
        id: mainMenu

        Action {
            text: "Update QApp"
            enabled: !appInstaller.busy
            onTriggered: updateDialog.open()
        }

        MenuSeparator {}

        Action {
            text: "About QApp"
            onTriggered: aboutDialog.open()
        }

        Action {
            text: "License (EUPL v1.2)"
            onTriggered: Qt.openUrlExternally("https://eupl.eu/1.2/en/")
        }
    }

    // ── Dialogs ─────────────────────────────────────────────
    InstallerUpdateDialog {
        id: updateDialog
        appInstaller: appInstaller
    }

    QAppAboutDialog { id: aboutDialog }

    // ── Header ──────────────────────────────────────────────
    header: ToolBar {
        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12

                Label {
                    text: root.title
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    text: "\u2630"
                    font.pixelSize: 16
                    onClicked: mainMenu.popup()
                }
            }

            TabBar {
                id: tabBar
                Layout.fillWidth: true

                TabButton {
                    text: "Install"
                }
                TabButton {
                    text: "Manage"
                    onClicked: {
                        appInstaller.listApps()
                        appInstaller.checkUpdates()
                    }
                }
            }
        }
    }

    // ── Content ─────────────────────────────────────────────
    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        InstallerInstallTab {
            classifier: classifier
            appInstaller: appInstaller
        }

        InstallerManageTab {
            appInstaller: appInstaller
        }
    }

    // ── Footer ──────────────────────────────────────────────
    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12

            Label {
                text: {
                    if (appInstaller.busy) return "Installing..."
                    if (classifier.busy && classifier.statusMessage.length > 0)
                        return classifier.statusMessage
                    if (classifier.busy) return "Classifying..."
                    if (appInstaller.installed) return "Installed: " + appInstaller.appName
                    if (classifier.hasResult && classifier.errorMessage.length === 0)
                        return "Classification complete"
                    if (classifier.hasResult && classifier.errorMessage.length > 0)
                        return "Classification failed"
                    return "Ready"
                }
                font.pixelSize: 12
                opacity: 0.6
            }

            Item { Layout.fillWidth: true }
        }
    }
}
