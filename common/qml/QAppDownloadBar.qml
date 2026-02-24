import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: bar

    required property bool downloading
    required property string fileName
    required property real receivedBytes
    required property real totalBytes

    signal cancelRequested()

    visible: bar.downloading
    height: visible ? implicitHeight : 0

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 8
        spacing: 8

        Label {
            text: bar.fileName
            elide: Text.ElideMiddle
            Layout.maximumWidth: 200
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: bar.totalBytes > 0 ? bar.totalBytes : 1
            value: bar.totalBytes > 0 ? bar.receivedBytes : 0
            indeterminate: bar.totalBytes <= 0
        }

        Label {
            text: {
                function fmt(bytes) {
                    if (bytes < 1024) return bytes + " B"
                    if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
                    return (bytes / 1048576).toFixed(1) + " MB"
                }
                if (bar.totalBytes > 0)
                    return fmt(bar.receivedBytes) + " / " + fmt(bar.totalBytes)
                return fmt(bar.receivedBytes)
            }
            font.pixelSize: 11
            opacity: 0.7
        }

        ToolButton {
            text: "\u2715"
            implicitWidth: 32
            onClicked: bar.cancelRequested()
        }
    }
}
