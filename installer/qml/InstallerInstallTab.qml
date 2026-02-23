import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: installTab

    required property var classifier
    required property var appInstaller

    property string customIconPath: ""

    // URL validation — accepts domains, bare hostnames (mimer, localhost), IPs, optional port
    readonly property var _urlPattern: /^https?:\/\/[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*(:\d{1,5})?([\/\w\-._~:?#[\]@!$&'()*+,;=%]*)?$/

    function isValidUrl(text) {
        if (text.length === 0) return false
        return _urlPattern.test(text)
    }

    function normalizeUrl(text) {
        if (/^https?:\/\//.test(text)) return text
        return "https://" + text
    }

    function isValidInput(text) {
        if (text.length === 0) return false
        return _urlPattern.test(normalizeUrl(text))
    }

    FileDialog {
        id: iconFileDialog
        title: "Choose app icon"
        nameFilters: ["Image files (*.png *.svg *.ico)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (path.startsWith("file://")) path = path.substring(7)
            installTab.customIconPath = path
        }
    }

    Layout.margins: 20
    spacing: 16

    Item { Layout.fillHeight: true }

    Label {
        text: "Enter a URL to install as app"
        font.pixelSize: 14
        Layout.alignment: Qt.AlignHCenter
        opacity: 0.7
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: 500
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        TextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "example.com"
            selectByMouse: true
            font.pixelSize: 14
            enabled: !installTab.classifier.busy

            property bool hasInput: text.length > 0
            property bool valid: installTab.isValidInput(text)

            background: Rectangle {
                implicitHeight: 40
                radius: 4
                border.width: urlField.activeFocus || urlField.hasInput ? 2 : 1
                border.color: {
                    if (!urlField.hasInput) return palette.mid
                    if (urlField.valid) return "#4caf50"
                    return "#f44336"
                }
                color: palette.base
            }

            onAccepted: {
                if (valid) classifyButton.clicked()
            }
        }

        Button {
            id: classifyButton
            text: installTab.classifier.busy ? "Classifying..." : "Classify"
            enabled: urlField.valid && !installTab.classifier.busy
            font.pixelSize: 14

            onClicked: {
                installTab.customIconPath = ""
                installTab.appInstaller.reset()
                var url = installTab.normalizeUrl(urlField.text)
                installTab.classifier.classify(url)
            }
        }
    }

    Label {
        Layout.alignment: Qt.AlignHCenter
        font.pixelSize: 12
        opacity: 0.6
        text: {
            if (!urlField.hasInput) return " "
            if (urlField.valid) return "Valid URL"
            return "Enter a valid URL starting with http:// or https://"
        }
        color: {
            if (!urlField.hasInput) return palette.text
            if (urlField.valid) return "#4caf50"
            return "#f44336"
        }
    }

    InstallerResultsPanel {
        classifier: installTab.classifier
        customIconPath: installTab.customIconPath
        onChooseIconRequested: iconFileDialog.open()
    }

    // Install/Remove actions
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 500
        spacing: 12
        visible: installTab.classifier.hasResult && installTab.classifier.errorMessage.length === 0 && !installTab.appInstaller.busy

        Button {
            text: installTab.appInstaller.installed ? "Reinstall" : "Install"
            enabled: !installTab.appInstaller.busy
            font.pixelSize: 14

            onClicked: {
                if (installTab.classifier.classifyResultJson.length > 0) {
                    installTab.appInstaller.installFromData(installTab.classifier.classifyResultJson, installTab.customIconPath)
                } else {
                    var url = installTab.normalizeUrl(urlField.text)
                    installTab.appInstaller.install(url, installTab.customIconPath)
                }
            }
        }

        Button {
            text: "Remove"
            visible: installTab.appInstaller.installed
            enabled: !installTab.appInstaller.busy
            font.pixelSize: 14
            onClicked: installTab.appInstaller.uninstall(installTab.appInstaller.appId)
        }
    }

    // Install status feedback
    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 500
        spacing: 4
        visible: installTab.appInstaller.installed || installTab.appInstaller.errorMessage.length > 0

        Label {
            visible: installTab.appInstaller.installed
            text: "Installed! \"" + installTab.appInstaller.appName + "\" is available in your system launcher."
            font.pixelSize: 13
            color: "#4caf50"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            visible: installTab.appInstaller.errorMessage.length > 0
            text: installTab.appInstaller.errorMessage
            font.pixelSize: 13
            color: "#f44336"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // Loading indicator
    BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        running: installTab.classifier.busy || installTab.appInstaller.busy
        visible: installTab.classifier.busy || installTab.appInstaller.busy
    }

    Label {
        Layout.alignment: Qt.AlignHCenter
        text: "Installing..."
        font.pixelSize: 13
        opacity: 0.6
        visible: installTab.appInstaller.busy
    }

    Item { Layout.fillHeight: true }
}
