import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: settingsDialog
    title: "Settings"
    anchors.centerIn: parent
    width: 380
    modal: true
    standardButtons: Dialog.Close

    required property string appId
    required property var profileSettings

    signal clearDataRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            text: "App: " + settingsDialog.appId
            font.pixelSize: 13
            opacity: 0.7
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid }

        Label {
            text: "Clear all browsing data (cookies, cache, local storage) and return to start page."
            font.pixelSize: 12
            opacity: 0.6
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            text: "Clear data & restart"
            Layout.alignment: Qt.AlignHCenter
            onClicked: settingsDialog.clearDataRequested()
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid }

        Label { text: "Profile"; font.pixelSize: 14; font.bold: true }

        GridLayout {
            columns: 2
            columnSpacing: 12
            rowSpacing: 8
            Layout.fillWidth: true

            Label { text: "HTTP cache (MB):"; font.pixelSize: 12 }
            SpinBox {
                from: 100; to: 10000; stepSize: 100
                value: settingsDialog.profileSettings.cacheSizeMB
                editable: true
                onValueModified: settingsDialog.profileSettings.cacheSizeMB = value
            }

            Label { text: "JS heap (MB):"; font.pixelSize: 12 }
            SpinBox {
                from: 512; to: 32768; stepSize: 512
                value: settingsDialog.profileSettings.jsHeapSizeMB
                editable: true
                onValueModified: settingsDialog.profileSettings.jsHeapSizeMB = value
            }

            Label { text: "Browser identity:"; font.pixelSize: 12 }
            ComboBox {
                id: uaCombo
                Layout.fillWidth: true
                font.pixelSize: 12
                model: [
                    "Auto (Chrome-like)",
                    "Chrome on Linux",
                    "Firefox on Linux",
                    "Edge on Linux",
                    "Custom..."
                ]
                readonly property var uaStrings: [
                    "",
                    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
                    "Mozilla/5.0 (X11; Linux x86_64; rv:133.0) Gecko/20100101 Firefox/133.0",
                    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0",
                    "_custom_"
                ]
                Component.onCompleted: {
                    var saved = settingsDialog.profileSettings.userAgent
                    if (saved.length === 0) { currentIndex = 0; return }
                    for (var i = 1; i < uaStrings.length - 1; i++) {
                        if (uaStrings[i] === saved) { currentIndex = i; return }
                    }
                    currentIndex = 4
                }
                onActivated: function(index) {
                    if (index < 4) {
                        settingsDialog.profileSettings.userAgent = uaStrings[index]
                        customUaField.visible = false
                    } else {
                        customUaField.visible = true
                        customUaField.forceActiveFocus()
                    }
                }
            }

            Item { width: 1; height: 1; visible: customUaField.visible }
            TextField {
                id: customUaField
                Layout.fillWidth: true
                font.pixelSize: 11
                placeholderText: "Paste full UA string"
                visible: uaCombo.currentIndex === 4
                text: uaCombo.currentIndex === 4 ? settingsDialog.profileSettings.userAgent : ""
                onEditingFinished: settingsDialog.profileSettings.userAgent = text
            }
        }

        Label {
            text: "UA/heap changes require restart."
            font.pixelSize: 11
            opacity: 0.4
        }
    }
}
