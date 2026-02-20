import QtQuick
import QtQuick.Controls
import QtWebEngine

WebEngineView {
    id: webView
    anchors.fill: parent

    // Signal to parent to open URL in new tab
    signal openInNewTab(string url)

    onLoadingChanged: function(loadRequest) {
        if (loadRequest.status === WebEngineView.LoadFailedStatus) {
            console.error("Load failed:", loadRequest.errorString)
        }
    }

    // Intercept new window requests → open in new tab instead
    onNewWindowRequested: function(request) {
        webView.openInNewTab(request.requestedUrl.toString())
    }

    onContextMenuRequested: function(request) {
        request.accepted = true
        contextMenu.linkUrl = request.linkUrl.toString()
        contextMenu.selectedText = request.selectedText
        contextMenu.mediaUrl = request.mediaUrl.toString()
        contextMenu.canGoBack = webView.canGoBack
        contextMenu.canGoForward = webView.canGoForward
        contextMenu.popup()
    }

    Menu {
        id: contextMenu

        property string linkUrl: ""
        property string selectedText: ""
        property string mediaUrl: ""
        property bool canGoBack: false
        property bool canGoForward: false

        // ── Navigation ──────────────────────────────────
        MenuItem {
            text: "Back"
            enabled: contextMenu.canGoBack
            onTriggered: webView.goBack()
        }

        MenuItem {
            text: "Forward"
            enabled: contextMenu.canGoForward
            onTriggered: webView.goForward()
        }

        MenuItem {
            text: "Reload"
            onTriggered: webView.reload()
        }

        MenuSeparator {
            visible: contextMenu.linkUrl.length > 0
        }

        // ── Link actions ────────────────────────────────
        MenuItem {
            text: "Open link in new tab"
            visible: contextMenu.linkUrl.length > 0
            onTriggered: webView.openInNewTab(contextMenu.linkUrl)
        }

        MenuItem {
            text: "Copy link address"
            visible: contextMenu.linkUrl.length > 0
            onTriggered: {
                webView.triggerWebAction(WebEngineView.CopyLinkToClipboard)
            }
        }

        MenuSeparator {
            visible: contextMenu.selectedText.length > 0
        }

        // ── Text actions ────────────────────────────────
        MenuItem {
            text: "Copy"
            visible: contextMenu.selectedText.length > 0
            onTriggered: webView.triggerWebAction(WebEngineView.Copy)
        }

        MenuSeparator {
            visible: contextMenu.mediaUrl.length > 0
        }

        // ── Image actions ───────────────────────────────
        MenuItem {
            text: "Copy image"
            visible: contextMenu.mediaUrl.length > 0
            onTriggered: webView.triggerWebAction(WebEngineView.CopyImageToClipboard)
        }

        MenuItem {
            text: "Copy image address"
            visible: contextMenu.mediaUrl.length > 0
            onTriggered: webView.triggerWebAction(WebEngineView.CopyImageUrlToClipboard)
        }
    }
}
