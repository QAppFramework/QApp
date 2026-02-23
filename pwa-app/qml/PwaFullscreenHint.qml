import QtQuick
import QtQuick.Controls
import QtCore

Rectangle {
    id: hint

    required property string appId

    visible: false
    width: label.implicitWidth + 48
    height: label.implicitHeight + 24
    radius: 8
    color: "#cc000000"
    z: 999

    Settings {
        id: hintSettings
        category: hint.appId + "/hints"
        property bool fullscreenHintShown: false
    }

    Label {
        id: label
        anchors.centerIn: parent
        text: "Press F11 to exit fullscreen"
        font.pixelSize: 16
        color: "white"
    }

    NumberAnimation on opacity {
        id: fadeOut
        running: false
        from: 1.0; to: 0.0
        duration: 1000
        onFinished: hint.visible = false
    }

    Timer {
        id: timer
        interval: 3000
        onTriggered: fadeOut.running = true
    }

    function show() {
        if (hintSettings.fullscreenHintShown) return
        hintSettings.fullscreenHintShown = true
        hint.visible = true
        hint.opacity = 1.0
        timer.start()
    }
}
