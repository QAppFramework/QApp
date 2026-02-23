import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: splash

    required property string backgroundColor
    required property string iconPath
    required property string appName

    visible: true
    color: splash.backgroundColor || "#ffffff"
    z: 1000

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 24

        Image {
            source: splash.iconPath ? "file://" + splash.iconPath : ""
            visible: splash.iconPath !== ""
            Layout.alignment: Qt.AlignHCenter
            sourceSize.width: 128
            sourceSize.height: 128
        }

        Label {
            text: splash.appName
            visible: splash.appName !== ""
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 20
            font.weight: Font.Medium
            color: internal.isDark ? "#ffffff" : "#333333"
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: splash.visible
            palette.dark: internal.isDark ? "#ffffff" : "#333333"
        }
    }

    QtObject {
        id: internal
        readonly property bool isDark: {
            if (!splash.backgroundColor) return false
            var c = splash.backgroundColor
            if (c.charAt(0) === '#') c = c.substring(1)
            if (c.length === 3) c = c[0]+c[0]+c[1]+c[1]+c[2]+c[2]
            var r = parseInt(c.substring(0,2), 16) / 255
            var g = parseInt(c.substring(2,4), 16) / 255
            var b = parseInt(c.substring(4,6), 16) / 255
            return (0.299*r + 0.587*g + 0.114*b) < 0.5
        }
    }

    NumberAnimation on opacity {
        id: fadeOut
        running: false
        from: 1.0; to: 0.0
        duration: 400
        onFinished: splash.visible = false
    }

    function dismiss() {
        fadeOut.running = true
    }
}
