import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import QtCore
import QAppWrapper

ApplicationWindow {
    id: root
    width: 1024
    height: 768
    visible: true
    title: {
        if (tabModel.count === 0) return root.appId || "QApp"
        var current = tabStack.children[tabBar.currentIndex]
        return current ? (current.title || root.appId) : root.appId
    }

    // argv[0]=binary, argv[1]=app-id, argv[2]=url
    readonly property string appId: Qt.application.arguments.length > 1
        ? Qt.application.arguments[1]
        : "default"
    readonly property string startUrl: Qt.application.arguments.length > 2
        ? Qt.application.arguments[2]
        : "https://example.com"

    Settings {
        id: windowSettings
        category: root.appId
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    // Tab state persistence
    Settings {
        id: tabSettings
        category: root.appId + "/tabs"
        property string savedTabs: ""
    }

    WebEngineProfile {
        id: appProfile
        storageName: root.appId
        offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
    }

    WrapperHelper {
        id: helper
    }

    ListModel {
        id: tabModel
    }

    // ── Save/restore tab state ──────────────────────────────
    function saveTabs() {
        var urls = []
        for (var i = 0; i < tabStack.children.length; i++) {
            var view = tabStack.children[i]
            if (view && view.url)
                urls.push(view.url.toString())
        }
        tabSettings.savedTabs = JSON.stringify(urls)
    }

    function restoreTabs() {
        var urls = []
        try {
            if (tabSettings.savedTabs.length > 0)
                urls = JSON.parse(tabSettings.savedTabs)
        } catch(e) {}

        if (urls.length === 0) {
            addTab(root.startUrl)
        } else {
            for (var i = 0; i < urls.length; i++)
                addTab(urls[i])
        }
    }

    // ── Tab management ──────────────────────────────────────
    function addTab(url) {
        var component = Qt.createComponent("WrapperTab.qml")
        if (component.status === Component.Ready) {
            var view = component.createObject(tabStack, {
                "profile": appProfile,
                "url": url,
                "visible": false
            })
            tabModel.append({ "tabTitle": "Loading...", "tabUrl": url })
            var idx = tabModel.count - 1
            view.titleChanged.connect(function() {
                if (idx < tabModel.count)
                    tabModel.set(idx, { "tabTitle": view.title || "Untitled" })
            })
            view.openInNewTab.connect(function(newUrl) {
                addTab(newUrl)
            })
            tabBar.currentIndex = idx
            updateVisibility()
        }
    }

    function closeTab(index) {
        if (tabModel.count <= 1) return  // keep at least one tab
        var view = tabStack.children[index]
        tabModel.remove(index)
        if (view) view.destroy()

        // Fix tab title bindings after removal
        for (var i = 0; i < tabStack.children.length; i++) {
            var v = tabStack.children[i]
            if (v) {
                var capturedIdx = i
                // Rebind title updates
                v.titleChanged.connect(function() {
                    if (capturedIdx < tabModel.count)
                        tabModel.set(capturedIdx, { "tabTitle": v.title || "Untitled" })
                })
            }
        }

        if (tabBar.currentIndex >= tabModel.count)
            tabBar.currentIndex = tabModel.count - 1
        updateVisibility()
        saveTabs()
    }

    function updateVisibility() {
        for (var i = 0; i < tabStack.children.length; i++) {
            var view = tabStack.children[i]
            if (view) view.visible = (i === tabBar.currentIndex)
        }
    }

    Component.onCompleted: restoreTabs()
    onClosing: saveTabs()

    // ── Header: TabBar ──────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true

                onCurrentIndexChanged: {
                    updateVisibility()
                    // Update window title
                    if (tabBar.currentIndex >= 0 && tabBar.currentIndex < tabStack.children.length) {
                        var current = tabStack.children[tabBar.currentIndex]
                        if (current) root.title = current.title || root.appId
                    }
                }

                Repeater {
                    model: tabModel

                    TabButton {
                        width: Math.min(200, tabBar.width / (tabModel.count + 1))
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
                                visible: tabModel.count > 1
                                onClicked: closeTab(index)
                            }
                        }
                    }
                }
            }

            ToolButton {
                text: "+"
                font.pixelSize: 16
                font.bold: true
                implicitWidth: 36
                onClicked: addTab(root.startUrl)
            }

            ToolButton {
                id: hamburgerButton
                text: "\u2630"
                font.pixelSize: 16
                implicitWidth: 36
                onClicked: hamburgerMenu.popup()
            }
        }
    }

    // ── Hamburger menu (F10) ────────────────────────────────
    Shortcut {
        sequence: "F10"
        onActivated: hamburgerMenu.popup()
    }

    Menu {
        id: hamburgerMenu

        Action {
            text: "Save as app..."
            enabled: !helper.busy && tabBar.currentIndex >= 0
            onTriggered: saveDialog.open()
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

    // ── About dialog ────────────────────────────────────────
    Dialog {
        id: aboutDialog
        title: "About QApp"
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Close

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "QApp"
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Turn any website into a standalone desktop app"
                font.pixelSize: 12
                opacity: 0.7
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                text: "Version 0.1.0-alpha"
                font.pixelSize: 11
                opacity: 0.5
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: palette.mid
            }

            Label {
                text: "<a href=\"https://northheim.com/\">Northheim</a>"
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
                onLinkActivated: Qt.openUrlExternally(link)
            }

            Label {
                text: "Licensed under <a href=\"https://eupl.eu/1.2/en/\">EUPL v1.2</a>"
                font.pixelSize: 11
                opacity: 0.6
                Layout.alignment: Qt.AlignHCenter
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }

    // ── Save as app dialog ──────────────────────────────────
    Dialog {
        id: saveDialog
        title: "Save as app"
        anchors.centerIn: parent
        width: 360
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            // Pre-fill with current page title
            var view = tabStack.children[tabBar.currentIndex]
            appNameField.text = view ? (view.title || "") : ""
            appNameField.selectAll()
            appNameField.forceActiveFocus()
        }

        onAccepted: {
            var view = tabStack.children[tabBar.currentIndex]
            if (view && appNameField.text.trim().length > 0) {
                helper.saveAsApp(view.url.toString(), appNameField.text.trim())
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "App name:"
                font.pixelSize: 13
            }

            TextField {
                id: appNameField
                Layout.fillWidth: true
                placeholderText: "e.g. QAppFramework"
                font.pixelSize: 14
                selectByMouse: true

                onAccepted: saveDialog.accept()
            }

            Label {
                text: {
                    var view = tabStack.children[tabBar.currentIndex]
                    return view ? view.url.toString() : ""
                }
                font.pixelSize: 11
                opacity: 0.6
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    // ── Content: WebEngine views ────────────────────────────
    Item {
        id: tabStack
        anchors.fill: parent
    }

    // ── Footer: status ──────────────────────────────────────
    footer: ToolBar {
        height: helper.statusMessage.length > 0 ? implicitHeight : 0
        visible: helper.statusMessage.length > 0

        Label {
            anchors.fill: parent
            anchors.leftMargin: 12
            text: helper.statusMessage
            font.pixelSize: 11
            opacity: 0.7
            verticalAlignment: Text.AlignVCenter
        }
    }
}
