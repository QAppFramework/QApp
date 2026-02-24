import QtQuick
import QtQuick.Controls

Rectangle {
    id: overlay

    required property bool windowActive

    visible: false
    color: Qt.rgba(palette.window.r, palette.window.g, palette.window.b, 0.75)
    z: 900

    BusyIndicator {
        anchors.centerIn: parent
        running: overlay.visible
        width: 48; height: 48
    }

    // Track deactivation time to avoid flicker on quick focus changes
    property real _deactivatedAt: 0

    onWindowActiveChanged: {
        if (!windowActive) {
            _deactivatedAt = Date.now()
        } else if (_deactivatedAt > 0 && (Date.now() - _deactivatedAt) > 500) {
            overlay.opacity = 1.0
            overlay.visible = true
            dismissTimer.restart()
        }
    }

    Timer {
        id: dismissTimer
        interval: 2000
        onTriggered: fadeOut.running = true
    }

    NumberAnimation on opacity {
        id: fadeOut
        running: false
        from: 1.0; to: 0.0
        duration: 350
        onFinished: overlay.visible = false
    }
}
