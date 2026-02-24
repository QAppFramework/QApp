import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: headerToolBar

    required property string themeColor
    required property bool hasThemeColor
    required property bool darkTheme
    required property bool isMinimalUi
    required property bool showTabs
    required property bool showNewTabButton
    required property bool showSaveAsApp
    required property bool canSave
    required property var tabModel
    required property int currentTabIndex
    required property bool canGoBack
    required property bool canGoForward
    required property real zoomFactor

    signal tabIndexChanged(int index)
    signal closeTabRequested(int index)
    signal newTabRequested()
    signal goBackRequested()
    signal goForwardRequested()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()
    signal settingsRequested()
    signal aboutRequested()
    signal licenseRequested()
    signal saveAsAppRequested()

    function openMenu() { hamburgerMenu.popup() }

    background: Rectangle {
        color: headerToolBar.hasThemeColor ? headerToolBar.themeColor : palette.window
    }

    palette.buttonText: headerToolBar.darkTheme ? "#ffffff" : undefined
    palette.windowText: headerToolBar.darkTheme ? "#ffffff" : undefined

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ToolButton {
            text: "\u25C0"
            font.pixelSize: 14
            implicitWidth: 36
            visible: headerToolBar.isMinimalUi
            enabled: headerToolBar.canGoBack
            onClicked: headerToolBar.goBackRequested()
        }

        ToolButton {
            text: "\u25B6"
            font.pixelSize: 14
            implicitWidth: 36
            visible: headerToolBar.isMinimalUi
            enabled: headerToolBar.canGoForward
            onClicked: headerToolBar.goForwardRequested()
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            visible: headerToolBar.showTabs
            currentIndex: headerToolBar.currentTabIndex

            onCurrentIndexChanged: headerToolBar.tabIndexChanged(currentIndex)

            Repeater {
                model: headerToolBar.tabModel

                TabButton {
                    width: Math.min(200, tabBar.width / (headerToolBar.tabModel.count + 1))
                    contentItem: RowLayout {
                        spacing: 4

                        Label {
                            text: model.tabTitle || "Loading..."
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            font.pixelSize: 12
                        }

                        ToolButton {
                            text: "\u2715"
                            font.pixelSize: 10
                            implicitWidth: 20
                            implicitHeight: 20
                            visible: headerToolBar.tabModel.count > 1
                            onClicked: headerToolBar.closeTabRequested(model.index)
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            visible: !headerToolBar.showTabs
        }

        ToolButton {
            text: "+"
            font.pixelSize: 16
            font.bold: true
            implicitWidth: 36
            visible: headerToolBar.showNewTabButton
            onClicked: headerToolBar.newTabRequested()
        }

        ToolButton {
            text: "\u2630"
            font.pixelSize: 16
            implicitWidth: 36
            onClicked: hamburgerMenu.popup()
        }
    }

    Menu {
        id: hamburgerMenu

        MenuItem {
            text: "Save as app..."
            visible: headerToolBar.showSaveAsApp
            height: visible ? implicitHeight : 0
            enabled: headerToolBar.canSave
            onTriggered: headerToolBar.saveAsAppRequested()
        }

        MenuSeparator {
            visible: headerToolBar.showSaveAsApp
            height: visible ? implicitHeight : 0
        }

        Action {
            text: "Settings..."
            onTriggered: headerToolBar.settingsRequested()
        }

        MenuSeparator {}

        MenuItem {
            text: "\uFF0B  Zoom In"
            onTriggered: headerToolBar.zoomInRequested()
        }

        MenuItem {
            text: "\uFF0D  Zoom Out"
            onTriggered: headerToolBar.zoomOutRequested()
        }

        MenuItem {
            text: "100%  Reset Zoom"
            enabled: Math.abs(headerToolBar.zoomFactor - 1.0) > 0.01
            onTriggered: headerToolBar.zoomResetRequested()
        }

        MenuSeparator {}

        Action {
            text: "About QApp"
            onTriggered: headerToolBar.aboutRequested()
        }

        Action {
            text: "License (EUPL v1.2)"
            onTriggered: headerToolBar.licenseRequested()
        }
    }
}
