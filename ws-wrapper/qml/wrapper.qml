import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import QtCore
import QApp.WS
import QApp.Common

ApplicationWindow {
    id: root
    width: 1024
    height: 768
    visible: true
    title: {
        if (tabView.count === 0) return root.appId || "QApp"
        var current = tabView.currentView
        return current ? (current.title || root.appId) : root.appId
    }

    readonly property string appId: Qt.application.arguments.length > 1
        ? Qt.application.arguments[1] : "default"
    readonly property string startUrl: Qt.application.arguments.length > 2
        ? Qt.application.arguments[2] : "https://example.com"

    Settings {
        id: windowSettings
        category: root.appId
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    Settings {
        id: tabSettings
        category: root.appId + "/tabs"
        property string savedTabs: ""
    }

    Settings {
        id: profileSettings
        category: root.appId + "/profile"
        property int cacheSizeMB: 500
        property int jsHeapSizeMB: 4096
        property string userAgent: ""
    }

    property string _defaultUA: ""

    WebEngineProfile {
        id: appProfile
        storageName: root.appId
        offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
        httpCacheMaximumSize: profileSettings.cacheSizeMB * 1048576
        onDownloadRequested: function(download) {
            downloadHandler.handleDownload(download)
        }
    }

    WrapperHelper {
        id: helper
    }

    readonly property string effectiveDisplayMode: helper.metadataLoaded ? helper.displayMode : ""
    readonly property bool isBrowserMode: effectiveDisplayMode === "" || effectiveDisplayMode === "browser"
    readonly property bool isMinimalUi: effectiveDisplayMode === "minimal-ui"
    readonly property bool isFullscreen: effectiveDisplayMode === "fullscreen"
    readonly property bool showTabs: isBrowserMode
    readonly property bool showSaveAsApp: isBrowserMode

    function parseHexColor(hex) {
        if (!hex || hex.length < 4) return null
        var c = hex.replace("#", "")
        if (c.length === 3)
            c = c[0]+c[0]+c[1]+c[1]+c[2]+c[2]
        if (c.length !== 6) return null
        var r = parseInt(c.substring(0, 2), 16) / 255.0
        var g = parseInt(c.substring(2, 4), 16) / 255.0
        var b = parseInt(c.substring(4, 6), 16) / 255.0
        return { r: r, g: g, b: b }
    }

    readonly property bool hasThemeColor: helper.themeColor.length > 0
    readonly property var themeRgb: hasThemeColor ? parseHexColor(helper.themeColor) : null
    readonly property bool darkTheme: {
        if (!themeRgb) return false
        var lum = 0.299 * themeRgb.r + 0.587 * themeRgb.g + 0.114 * themeRgb.b
        return lum < 0.5
    }

    Component.onCompleted: {
        root._defaultUA = appProfile.httpUserAgent
        appProfile.httpUserAgent = profileSettings.userAgent.length > 0
            ? profileSettings.userAgent
            : root._defaultUA.replace(/QtWebEngine\/[\d.]+ /, "")

        helper.loadMetadata(root.appId)
        tabView.restoreTabs()
        if (root.isFullscreen) {
            root.showFullScreen()
            fullscreenHint.show()
        }
    }
    onClosing: tabView.saveTabs()

    Shortcut {
        sequence: "F10"
        onActivated: headerToolBar.openMenu()
    }

    Shortcut {
        sequence: "F11"
        onActivated: {
            if (root.visibility === ApplicationWindow.FullScreen) {
                root.showNormal()
                headerToolBar.visible = !root.isFullscreen
            } else {
                root.showFullScreen()
                headerToolBar.visible = false
            }
        }
    }

    header: WrapperHeader {
        id: headerToolBar
        visible: !root.isFullscreen
        themeColor: helper.themeColor
        hasThemeColor: root.hasThemeColor
        darkTheme: root.darkTheme
        isMinimalUi: root.isMinimalUi
        showTabs: root.showTabs
        showNewTabButton: root.isBrowserMode
        showSaveAsApp: root.showSaveAsApp
        canSave: !helper.busy && tabView.currentIndex >= 0
        tabModel: tabView.tabModel
        currentTabIndex: tabView.currentIndex

        canGoBack: tabView.currentView ? tabView.currentView.canGoBack : false
        canGoForward: tabView.currentView ? tabView.currentView.canGoForward : false

        onTabIndexChanged: function(index) { tabView.setCurrentIndex(index) }
        onCloseTabRequested: tabView.closeTab(index)
        onNewTabRequested: tabView.addTab(root.startUrl)
        onGoBackRequested: { var v = tabView.currentView; if (v) v.goBack() }
        onGoForwardRequested: { var v = tabView.currentView; if (v) v.goForward() }
        onSettingsRequested: settingsDialog.open()
        onAboutRequested: aboutDialog.open()
        onLicenseRequested: Qt.openUrlExternally("https://eupl.eu/1.2/en/")
        onSaveAsAppRequested: saveDialog.open()
    }

    WrapperTabView {
        id: tabView
        anchors.fill: parent
        profile: appProfile
        startUrl: root.startUrl
        appScope: helper.scope || ""
        tabSettings: tabSettings
    }

    QAppDownloadHandler {
        id: downloadHandler
    }

    QAppAboutDialog { id: aboutDialog }

    WrapperSettingsDialog {
        id: settingsDialog
        appId: root.appId
        profileSettings: profileSettings
        onClearDataRequested: {
            tabSettings.savedTabs = ""
            helper.clearAppDataAndRestart(root.appId)
        }
    }

    WrapperSaveDialog {
        id: saveDialog
        currentPageTitle: {
            var v = tabView.currentView
            return v ? (v.title || "") : ""
        }
        currentPageUrl: {
            var v = tabView.currentView
            return v ? v.url.toString() : ""
        }
        onSaveRequested: function(url, name) {
            helper.saveAsApp(url, name)
        }
    }

    QAppResumeOverlay {
        anchors.fill: parent
        windowActive: root.active
    }

    WrapperFullscreenHint {
        id: fullscreenHint
        anchors.centerIn: parent
        appId: root.appId
    }

    onVisibilityChanged: {
        if (root.isFullscreen && root.visibility === ApplicationWindow.FullScreen)
            fullscreenHint.show()
    }

    footer: ColumnLayout {
        spacing: 0

        QAppDownloadBar {
            Layout.fillWidth: true
            downloading: downloadHandler.downloading
            fileName: downloadHandler.fileName
            receivedBytes: downloadHandler.receivedBytes
            totalBytes: downloadHandler.totalBytes
            onCancelRequested: downloadHandler.cancelDownload()
        }

        ToolBar {
            Layout.fillWidth: true
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
}
