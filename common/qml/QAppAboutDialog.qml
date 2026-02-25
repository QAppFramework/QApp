import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    title: "About QApp"
    anchors.centerIn: parent
    width: 340
    modal: true
    standardButtons: Dialog.Close

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "QApp"
            font.pixelSize: 20
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Turn any website into a standalone desktop app"
            font.pixelSize: 12
            opacity: 0.7
            Layout.alignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: "Version " + Qt.application.version
            font.pixelSize: 11
            opacity: 0.5
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: palette.mid
        }

        Label {
            text: "<a href=\"https://mathiason.engineer/\">mathiason.engineer</a>"
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Label {
            text: "Licensed under <a href=\"https://eupl.eu/1.2/en/\">EUPL v1.2</a>"
            font.pixelSize: 11
            opacity: 0.6
            Layout.alignment: Qt.AlignHCenter
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}
