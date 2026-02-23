import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: saveDialog
    title: "Save as app"
    anchors.centerIn: parent
    width: 360
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    required property string currentPageTitle
    required property string currentPageUrl

    signal saveRequested(string url, string name)

    onOpened: {
        appNameField.text = saveDialog.currentPageTitle
        appNameField.selectAll()
        appNameField.forceActiveFocus()
    }

    onAccepted: {
        if (appNameField.text.trim().length > 0)
            saveRequested(saveDialog.currentPageUrl, appNameField.text.trim())
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "App name:"
            font.pixelSize: 13
        }

        TextField {
            id: appNameField
            Layout.fillWidth: true
            placeholderText: "e.g. QAppFramework"
            font.pixelSize: 14
            selectByMouse: true
            onAccepted: saveDialog.accept()
        }

        Label {
            text: saveDialog.currentPageUrl
            font.pixelSize: 11
            opacity: 0.6
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
