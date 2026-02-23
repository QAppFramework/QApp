import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: toolbar

    required property string themeColor
    required property string currentUrl
    required property bool canGoBack
    required property bool canGoForward

    signal goBackRequested()
    signal goForwardRequested()
    signal reloadRequested()
    signal menuRequested()

    readonly property bool hasBg: toolbar.themeColor !== ""
    readonly property bool isDark: {
        if (!toolbar.hasBg) return false
        var c = toolbar.themeColor
        if (c.charAt(0) === '#') c = c.substring(1)
        if (c.length === 3) c = c[0]+c[0]+c[1]+c[1]+c[2]+c[2]
        var r = parseInt(c.substring(0,2), 16) / 255
        var g = parseInt(c.substring(2,4), 16) / 255
        var b = parseInt(c.substring(4,6), 16) / 255
        return (0.299*r + 0.587*g + 0.114*b) < 0.5
    }

    background: Rectangle {
        color: toolbar.hasBg ? toolbar.themeColor : palette.window
    }

    palette.buttonText: toolbar.isDark ? "#ffffff" : palette.buttonText
    palette.windowText: toolbar.isDark ? "#ffffff" : palette.windowText

    RowLayout {
        anchors.fill: parent
        spacing: 2

        ToolButton {
            text: "\u25C0"
            font.pixelSize: 14
            implicitWidth: 36
            enabled: toolbar.canGoBack
            onClicked: toolbar.goBackRequested()
        }

        ToolButton {
            text: "\u25B6"
            font.pixelSize: 14
            implicitWidth: 36
            enabled: toolbar.canGoForward
            onClicked: toolbar.goForwardRequested()
        }

        ToolButton {
            text: "\u21BB"
            font.pixelSize: 14
            implicitWidth: 36
            onClicked: toolbar.reloadRequested()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            radius: 4
            color: toolbar.isDark ? Qt.rgba(1,1,1,0.15) : Qt.rgba(0,0,0,0.06)

            Label {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                verticalAlignment: Text.AlignVCenter
                text: toolbar.currentUrl
                elide: Text.ElideMiddle
                font.pixelSize: 12
                color: toolbar.isDark ? "#dddddd" : "#666666"
            }
        }

        ToolButton {
            text: "\u2630"
            font.pixelSize: 16
            implicitWidth: 36
            onClicked: toolbar.menuRequested()
        }
    }
}
