import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: manageTab

    required property var appInstaller

    function hasAppUpdate(appId) {
        for (var i = 0; i < manageTab.appInstaller.appUpdates.length; i++) {
            var entry = manageTab.appInstaller.appUpdates[i]
            if (entry.appId === appId && entry.hasUpdate)
                return true
        }
        return false
    }

    Layout.margins: 20
    spacing: 16

    Label {
        text: manageTab.appInstaller.installedApps.length === 0 && !manageTab.appInstaller.busy
            ? "No apps installed"
            : manageTab.appInstaller.installedApps.length + " installed app" + (manageTab.appInstaller.installedApps.length !== 1 ? "s" : "")
        font.pixelSize: 14
        Layout.alignment: Qt.AlignHCenter
        opacity: 0.7
    }

    ListView {
        id: appListView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumWidth: 500
        Layout.alignment: Qt.AlignHCenter
        clip: true
        spacing: 8
        model: manageTab.appInstaller.installedApps

        delegate: Rectangle {
            required property var modelData
            required property int index
            width: appListView.width
            height: delegateRow.implicitHeight + 24
            radius: 8
            color: palette.alternateBase
            border.width: 1
            border.color: palette.mid

            RowLayout {
                id: delegateRow
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Rectangle {
                    width: 48; height: 24; radius: 4
                    color: {
                        var level = modelData.level || ""
                        if (level === "PWAPP") return "#4caf50"
                        if (level === "WAPP") return "#2196f3"
                        return "#9e9e9e"
                    }

                    Label {
                        anchors.centerIn: parent
                        text: modelData.level || "WS"
                        font.pixelSize: 11
                        font.bold: true
                        color: "white"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    RowLayout {
                        spacing: 8

                        Label {
                            text: modelData.name || modelData.appId
                            font.pixelSize: 14
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            width: updateLabel.implicitWidth + 12
                            height: 18
                            radius: 9
                            color: "#ff9800"
                            visible: manageTab.hasAppUpdate(modelData.appId)

                            Label {
                                id: updateLabel
                                anchors.centerIn: parent
                                text: "Update"
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }
                    }

                    Label {
                        text: modelData.url || ""
                        font.pixelSize: 11
                        opacity: 0.6
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Button {
                    text: "Update"
                    font.pixelSize: 12
                    visible: manageTab.hasAppUpdate(modelData.appId)
                    enabled: !manageTab.appInstaller.busy
                    onClicked: manageTab.appInstaller.install(modelData.url, "", modelData.appId)
                }

                Button {
                    text: "Open"
                    font.pixelSize: 12
                    onClicked: manageTab.appInstaller.launch(modelData.appId, modelData.url)
                }

                Button {
                    text: "Remove"
                    font.pixelSize: 12
                    enabled: !manageTab.appInstaller.busy
                    onClicked: manageTab.appInstaller.uninstall(modelData.appId)
                }
            }
        }
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        running: manageTab.appInstaller.busy
        visible: manageTab.appInstaller.busy
    }

    Label {
        text: "Checking for updates..."
        font.pixelSize: 11
        opacity: 0.5
        Layout.alignment: Qt.AlignHCenter
        visible: manageTab.appInstaller.checkingUpdates
    }
}
