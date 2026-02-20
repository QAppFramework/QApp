import QtQuick
import QtQuick.Controls
import QtWebEngine
import QtCore

ApplicationWindow {
    id: root
    width: 1024
    height: 768
    visible: true
    title: webView.title || "QApp Wrapper"

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

    WebEngineProfile {
        id: appProfile
        storageName: root.appId
        offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
    }

    WebEngineView {
        id: webView
        anchors.fill: parent
        profile: appProfile
        url: root.startUrl

        onTitleChanged: root.title = title

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.status === WebEngineView.LoadFailedStatus) {
                console.error("Load failed:", loadRequest.errorString)
            }
        }
    }
}
