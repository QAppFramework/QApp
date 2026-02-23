import QtQuick
import QtQuick.Controls
import QtWebEngine

Item {
    id: tabView

    required property var profile
    required property string startUrl
    required property string appScope
    required property var tabSettings

    readonly property alias tabModel: _tabModel
    property int currentIndex: 0
    readonly property int count: _tabModel.count
    property var currentView: null

    ListModel { id: _tabModel }

    onCurrentIndexChanged: _updateVisibility()

    function saveTabs() {
        var urls = []
        for (var i = 0; i < tabView.children.length; i++) {
            var view = tabView.children[i]
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
            addTab(startUrl)
        } else {
            for (var i = 0; i < urls.length; i++)
                addTab(urls[i])
        }
    }

    function addTab(url) {
        var component = Qt.createComponent("WrapperTab.qml")
        if (component.status === Component.Ready) {
            var view = component.createObject(tabView, {
                "profile": profile,
                "url": url,
                "visible": false,
                "appScope": appScope
            })
            _connectTab(view, url)
        }
    }

    function addTabFromRequest(request) {
        var component = Qt.createComponent("WrapperTab.qml")
        if (component.status === Component.Ready) {
            var view = component.createObject(tabView, {
                "profile": profile,
                "visible": false,
                "appScope": appScope
            })
            request.openIn(view)
            _connectTab(view, request.requestedUrl.toString())
        }
    }

    function _connectTab(view, url) {
        _tabModel.append({ "tabTitle": "Loading...", "tabUrl": url })
        var idx = _tabModel.count - 1
        view.titleChanged.connect(function() {
            if (idx < _tabModel.count)
                _tabModel.set(idx, { "tabTitle": view.title || "Untitled" })
        })
        view.openInNewTab.connect(function(newUrl) {
            addTab(newUrl)
        })
        view.openRequestInNewTab.connect(function(req) {
            addTabFromRequest(req)
        })
        view.windowCloseRequested.connect(function() {
            var tabIdx = -1
            for (var i = 0; i < tabView.children.length; i++) {
                if (tabView.children[i] === view) { tabIdx = i; break }
            }
            if (tabIdx >= 0 && _tabModel.count > 1) closeTab(tabIdx)
        })
        currentIndex = idx
        _updateVisibility()
    }

    function closeTab(index) {
        if (_tabModel.count <= 1) return
        var view = tabView.children[index]
        _tabModel.remove(index)
        if (view) view.destroy()

        for (var i = 0; i < tabView.children.length; i++) {
            var v = tabView.children[i]
            if (v) {
                var capturedIdx = i
                v.titleChanged.connect(function() {
                    if (capturedIdx < _tabModel.count)
                        _tabModel.set(capturedIdx, { "tabTitle": v.title || "Untitled" })
                })
            }
        }

        if (currentIndex >= _tabModel.count)
            currentIndex = _tabModel.count - 1
        _updateVisibility()
        saveTabs()
    }

    function setCurrentIndex(idx) {
        currentIndex = idx
    }

    function _updateVisibility() {
        for (var i = 0; i < tabView.children.length; i++) {
            var view = tabView.children[i]
            if (view) view.visible = (i === currentIndex)
        }
        currentView = (currentIndex >= 0 && currentIndex < tabView.children.length)
            ? tabView.children[currentIndex] : null
    }
}
