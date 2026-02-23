import QtQuick
import QtQuick.Controls
import QtWebEngine
import QtCore
import QApp.PWA
import QApp.Common

ApplicationWindow {
    id: root
    width: 1024
    height: 768
    visible: true
    title: helper.appName || webView.title || root.appId || "QApp PWA"

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

    WebEngineProfile {
        id: appProfile
        storageName: root.appId
        offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
    }

    PwaHelper {
        id: helper
    }

    readonly property string effectiveUrl: helper.startUrl || root.startUrl

    WebEngineView {
        id: webView
        anchors.fill: parent
        profile: appProfile
        url: root.effectiveUrl

        onNavigationRequested: function(request) {
            // Scope-aware navigation: out-of-scope opens system browser
            if (helper.scope && request.navigationType === WebEngineNavigationRequest.LinkClickedNavigation) {
                var targetUrl = request.url.toString()
                if (targetUrl.indexOf(helper.scope) !== 0) {
                    request.action = WebEngineNavigationRequest.IgnoreRequest
                    Qt.openUrlExternally(request.url)
                }
            }
        }
    }

    Component.onCompleted: {
        appProfile.httpUserAgent = appProfile.httpUserAgent.replace(/QtWebEngine\/[\d.]+ /, "")
        helper.loadMetadata(root.appId)
    }

    Connections {
        target: helper
        function onMetadataChanged() {
            if (helper.displayMode === "fullscreen")
                root.showFullScreen()
            if (helper.startUrl)
                webView.url = helper.startUrl
        }
    }
}
