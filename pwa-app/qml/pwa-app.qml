import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import QtCore
import QApp.PWA
import QApp.Common

ApplicationWindow {
    id: root
    width: 1024; height: 768; visible: true
    title: helper.appName || webView.title || root.appId || "QApp PWA"

    readonly property string appId: Qt.application.arguments.length > 1
        ? Qt.application.arguments[1] : "default"
    readonly property string startUrl: Qt.application.arguments.length > 2
        ? Qt.application.arguments[2] : "https://example.com"
    readonly property string dm: helper.displayMode
    readonly property bool isFullscreen: dm === "fullscreen"
    readonly property bool isMinimalUi: dm === "minimal-ui"

    Settings {
        id: windowSettings; category: root.appId
        property alias x: root.x; property alias y: root.y
        property alias width: root.width; property alias height: root.height
    }

    WebEngineProfile {
        id: appProfile
        storageName: root.appId; offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
        onDownloadRequested: function(download) {
            downloadHandler.handleDownload(download)
        }
    }

    // Version-safe: pushServiceEnabled + presentNotification may not exist in all Qt versions
    Connections {
        target: appProfile
        ignoreUnknownSignals: true
        function onPresentNotification(notification) {
            notifHandler.show(notification.title, notification.message)
            notification.show()
        }
    }

    PwaHelper { id: helper }
    PwaPermissionHandler { id: permissionHandler }
    PwaNotificationHandler {
        id: notifHandler
        appName: helper.appName || root.appId; iconPath: helper.iconPath
    }
    PwaOsIntegration {
        id: osIntegration
        desktopFileId: "qapp-" + root.appId + ".desktop"
    }

    readonly property string effectiveUrl: helper.startUrl || root.startUrl

    // Header: minimal-ui → full toolbar, standalone → hamburger only, fullscreen → none
    header: root.isMinimalUi ? minimalUiToolbar : (root.isFullscreen ? null : standaloneHeader)

    PwaToolbar {
        id: minimalUiToolbar
        visible: root.isMinimalUi && root.visibility !== Window.FullScreen
        themeColor: helper.themeColor; currentUrl: webView.url.toString()
        canGoBack: webView.canGoBack; canGoForward: webView.canGoForward
        onGoBackRequested: webView.goBack()
        onGoForwardRequested: webView.goForward()
        onReloadRequested: webView.reload()
        onMenuRequested: pwaMenu.popup()
    }

    ToolBar {
        id: standaloneHeader
        visible: !root.isMinimalUi && !root.isFullscreen
                 && root.visibility !== Window.FullScreen
        background: Rectangle { color: helper.themeColor || palette.window }
        RowLayout {
            anchors.fill: parent; spacing: 0
            Item { Layout.fillWidth: true }
            ToolButton {
                text: "\u2630"; font.pixelSize: 16; implicitWidth: 36
                onClicked: pwaMenu.popup()
            }
        }
    }

    PwaAppMenu {
        id: pwaMenu
        canGoBack: webView.canGoBack; canGoForward: webView.canGoForward
        zoomFactor: webView.zoomFactor
        onGoBackRequested: webView.goBack()
        onGoForwardRequested: webView.goForward()
        onReloadRequested: webView.reload()
        onZoomInRequested: webView.zoomFactor = Math.min(webView.zoomFactor + 0.1, 3.0)
        onZoomOutRequested: webView.zoomFactor = Math.max(webView.zoomFactor - 0.1, 0.3)
        onZoomResetRequested: webView.zoomFactor = 1.0
        onOpenInBrowserRequested: Qt.openUrlExternally(webView.url)
        onPrintRequested: webView.printToPdf("", webView.contentsSize)
        onAboutRequested: aboutDialog.open()
        onLicenseRequested: Qt.openUrlExternally("https://eupl.eu/1.2/en/")
    }

    QAppDownloadHandler { id: downloadHandler }
    QAppAboutDialog { id: aboutDialog }

    QAppResumeOverlay {
        anchors.fill: parent
        windowActive: root.active
    }

    PwaSplashScreen {
        id: splashScreen; anchors.fill: parent
        backgroundColor: helper.backgroundColor; iconPath: helper.iconPath
        appName: helper.appName; visible: helper.backgroundColor !== ""
    }
    PwaFullscreenHint {
        id: fullscreenHint; appId: root.appId
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 40 }
    }
    PwaOfflineIndicator {
        isOnline: helper.isOnline
        anchors { right: parent.right; top: parent.top; margins: 8 }
    }

    WebEngineView {
        id: webView; anchors.fill: parent; profile: appProfile; url: root.effectiveUrl

        onPermissionRequested: function(permission) {
            if (permissionHandler.shouldGrant(permission.permissionType)) {
                permission.grant()
                permissionHandler.storeDecision(permission.permissionType, true)
            } else { permission.deny() }
        }
        onNavigationRequested: function(request) {
            if (helper.scope && request.navigationType === WebEngineNavigationRequest.LinkClickedNavigation) {
                var targetUrl = request.url.toString()
                if (targetUrl.indexOf(helper.scope) !== 0) {
                    request.reject()
                    Qt.openUrlExternally(request.url)
                }
            }
        }
        onLoadingChanged: function(info) {
            if (info.status === WebEngineView.LoadSucceededStatus) {
                webView.runJavaScript(osIntegration.polyfillScript())
                splashScreen.dismiss()
            } else if (info.status === WebEngineView.LoadFailedStatus) {
                splashScreen.dismiss()
            }
        }
        onJavaScriptConsoleMessage: function(level, message, lineNumber, sourceID) {
            if (!message.startsWith("__QAPP_CMD:")) return
            var payload = message.substring(11)
            if (payload.startsWith("badge:")) {
                osIntegration.setBadge(parseInt(payload.substring(6)) || 0)
            } else if (payload.startsWith("share:")) {
                var d = JSON.parse(payload.substring(6))
                osIntegration.share(d.title || "", d.text || "", d.url || "")
            }
        }
        onFullScreenRequested: function(request) {
            request.accept()
            if (request.toggleOn) { root.showFullScreen(); fullscreenHint.show() }
            else { root.showNormal() }
        }
    }

    footer: QAppDownloadBar {
        downloading: downloadHandler.downloading
        fileName: downloadHandler.fileName
        receivedBytes: downloadHandler.receivedBytes
        totalBytes: downloadHandler.totalBytes
        onCancelRequested: downloadHandler.cancelDownload()
    }

    Shortcut { sequence: "F10"; onActivated: pwaMenu.popup() }
    Shortcut {
        sequence: "F11"
        onActivated: root.visibility === Window.FullScreen
            ? root.showNormal() : (root.showFullScreen(), fullscreenHint.show())
    }
    Shortcut { sequence: "Escape"; enabled: root.visibility === Window.FullScreen; onActivated: root.showNormal() }

    Component.onCompleted: {
        appProfile.httpUserAgent = appProfile.httpUserAgent.replace(/QtWebEngine\/[\d.]+ /, "")
        if ("pushServiceEnabled" in appProfile)
            appProfile.pushServiceEnabled = true
        helper.loadMetadata(root.appId)
    }
    Connections {
        target: helper
        function onMetadataChanged() {
            if (helper.displayMode === "fullscreen") { root.showFullScreen(); fullscreenHint.show() }
            if (helper.startUrl) webView.url = helper.startUrl
        }
    }
}
