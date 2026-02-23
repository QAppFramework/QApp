import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: updateDialog
    title: "Update QApp"
    anchors.centerIn: parent
    width: 400
    modal: true

    required property var appInstaller

    standardButtons: appInstaller.busy ? Dialog.NoButton : Dialog.Close

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            text: updateDialog.appInstaller.busy
                ? "Updating QApp..."
                : (updateDialog.appInstaller.updateStatus.length > 0
                    ? updateDialog.appInstaller.updateStatus
                    : "Check for updates from GitHub and install the latest version.\nAll wrapper apps will be updated.")
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: updateDialog.appInstaller.busy
            visible: updateDialog.appInstaller.busy
        }

        Label {
            visible: updateDialog.appInstaller.busy && updateDialog.appInstaller.updateStatus.length > 0
            text: updateDialog.appInstaller.updateStatus
            font.pixelSize: 11
            opacity: 0.6
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            text: "Update now"
            Layout.alignment: Qt.AlignHCenter
            visible: !updateDialog.appInstaller.busy && !updateDialog.appInstaller.updateStatus.startsWith("Updated!")
            onClicked: updateDialog.appInstaller.updateQApp()
        }
    }
}
