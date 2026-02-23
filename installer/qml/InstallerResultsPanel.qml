import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: resultsPanel

    required property var classifier
    required property string customIconPath

    signal chooseIconRequested()

    radius: 8
    color: palette.alternateBase
    border.width: 1
    border.color: palette.mid
    visible: classifier.hasResult

    Layout.fillWidth: true
    Layout.maximumWidth: 500
    Layout.alignment: Qt.AlignHCenter
    Layout.preferredHeight: resultsColumn.implicitHeight + 32

    ColumnLayout {
        id: resultsColumn
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Error display
        Label {
            visible: resultsPanel.classifier.errorMessage.length > 0
            text: resultsPanel.classifier.errorMessage
            color: "#f44336"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Level badge + name row
        RowLayout {
            visible: resultsPanel.classifier.errorMessage.length === 0
            spacing: 12

            Rectangle {
                width: 64; height: 28; radius: 4
                color: {
                    if (resultsPanel.classifier.level === "PWAPP") return "#4caf50"
                    if (resultsPanel.classifier.level === "WAPP") return "#2196f3"
                    return "#9e9e9e"
                }

                Label {
                    anchors.centerIn: parent
                    text: resultsPanel.classifier.level
                    font.pixelSize: 13
                    font.bold: true
                    color: "white"
                }
            }

            Label {
                text: resultsPanel.classifier.name
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // Icon preview + picker
        RowLayout {
            visible: resultsPanel.classifier.errorMessage.length === 0
            spacing: 12

            Rectangle {
                width: 64; height: 64; radius: 8
                color: palette.base
                border.width: 1; border.color: palette.mid

                Image {
                    anchors.centerIn: parent
                    width: 56; height: 56
                    fillMode: Image.PreserveAspectFit
                    source: resultsPanel.customIconPath.length > 0
                        ? "file://" + resultsPanel.customIconPath
                        : resultsPanel.classifier.iconUrl
                    sourceSize: Qt.size(56, 56)
                }
            }

            Button {
                text: "Choose icon..."
                font.pixelSize: 12
                onClicked: resultsPanel.chooseIconRequested()
            }

            Label {
                visible: resultsPanel.customIconPath.length > 0
                text: resultsPanel.customIconPath.split("/").pop()
                font.pixelSize: 11
                opacity: 0.6
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
        }

        // Metadata grid
        GridLayout {
            visible: resultsPanel.classifier.errorMessage.length === 0
            columns: 2
            columnSpacing: 12
            rowSpacing: 6
            Layout.fillWidth: true

            Label { text: "Display mode:"; opacity: 0.6; font.pixelSize: 12 }
            Label { text: resultsPanel.classifier.displayMode; font.pixelSize: 12 }

            Label { text: "Theme color:"; opacity: 0.6; font.pixelSize: 12; visible: resultsPanel.classifier.themeColor.length > 0 }
            RowLayout {
                visible: resultsPanel.classifier.themeColor.length > 0
                spacing: 6
                Rectangle {
                    width: 14; height: 14; radius: 2
                    color: resultsPanel.classifier.themeColor.length > 0 ? resultsPanel.classifier.themeColor : "transparent"
                    border.width: 1; border.color: palette.mid
                }
                Label { text: resultsPanel.classifier.themeColor; font.pixelSize: 12 }
            }

            Label { text: "Start URL:"; opacity: 0.6; font.pixelSize: 12; visible: resultsPanel.classifier.startUrl.length > 0 }
            Label { text: resultsPanel.classifier.startUrl; font.pixelSize: 12; visible: resultsPanel.classifier.startUrl.length > 0; elide: Text.ElideRight; Layout.fillWidth: true }

            Label { text: "Manifest:"; opacity: 0.6; font.pixelSize: 12 }
            Label { text: resultsPanel.classifier.hasManifest ? "Yes" : "No"; font.pixelSize: 12; color: resultsPanel.classifier.hasManifest ? "#4caf50" : "#9e9e9e" }

            Label { text: "Service worker:"; opacity: 0.6; font.pixelSize: 12 }
            Label { text: resultsPanel.classifier.hasServiceWorker ? "Yes" : "No"; font.pixelSize: 12; color: resultsPanel.classifier.hasServiceWorker ? "#4caf50" : "#9e9e9e" }
        }
    }
}
