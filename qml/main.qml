import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PWAApp

ApplicationWindow {
    id: root
    width: 640
    height: 480
    minimumWidth: 400
    minimumHeight: 300
    visible: true
    title: "PWAApp"

    // URL validation — matches http(s)://domain.tld with optional path
    // Canonical logic: src/url-validator.js (keep in sync)
    readonly property var _urlPattern: /^https?:\/\/[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z]{2,})+([\/\w\-._~:?#[\]@!$&'()*+,;=%]*)?$/

    function isValidUrl(text) {
        if (text.length === 0) return false;
        return _urlPattern.test(text);
    }

    // Auto-prepend https:// to bare domains
    function normalizeUrl(text) {
        if (/^https?:\/\//.test(text)) return text;
        return "https://" + text;
    }

    // Check if input looks like a domain (with or without protocol)
    function isValidInput(text) {
        if (text.length === 0) return false;
        return _urlPattern.test(normalizeUrl(text));
    }

    SiteClassifier {
        id: classifier
    }

    AppInstaller {
        id: appInstaller
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12

            Label {
                text: "PWAApp"
                font.pixelSize: 18
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Item { Layout.fillHeight: true }

        Label {
            text: "Enter a URL to classify"
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
                enabled: !classifier.busy

                property bool hasInput: text.length > 0
                property bool valid: root.isValidInput(text)

                background: Rectangle {
                    implicitHeight: 40
                    radius: 4
                    border.width: urlField.activeFocus || urlField.hasInput ? 2 : 1
                    border.color: {
                        if (!urlField.hasInput) return palette.mid;
                        if (urlField.valid) return "#4caf50";
                        return "#f44336";
                    }
                    color: palette.base
                }

                onAccepted: {
                    if (valid) classifyButton.clicked();
                }
            }

            Button {
                id: classifyButton
                text: classifier.busy ? "Classifying..." : "Classify"
                enabled: urlField.valid && !classifier.busy
                font.pixelSize: 14

                onClicked: {
                    var url = root.normalizeUrl(urlField.text);
                    classifier.classify(url);
                    statusText.text = "Classifying: " + url + " ...";
                }
            }
        }

        Label {
            id: validationHint
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 12
            opacity: 0.6
            text: {
                if (!urlField.hasInput) return " ";
                if (urlField.valid) return "Valid URL";
                return "Enter a valid URL starting with http:// or https://";
            }
            color: {
                if (!urlField.hasInput) return palette.text;
                if (urlField.valid) return "#4caf50";
                return "#f44336";
            }
        }

        // Classification results panel
        Rectangle {
            id: resultsPanel
            Layout.fillWidth: true
            Layout.maximumWidth: 500
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: resultsColumn.implicitHeight + 32
            radius: 8
            color: palette.alternateBase
            border.width: 1
            border.color: palette.mid
            visible: classifier.hasResult

            ColumnLayout {
                id: resultsColumn
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // Error display
                Label {
                    visible: classifier.errorMessage.length > 0
                    text: classifier.errorMessage
                    color: "#f44336"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Level badge + name row
                RowLayout {
                    visible: classifier.errorMessage.length === 0
                    spacing: 12

                    Rectangle {
                        id: levelBadge
                        width: 64
                        height: 28
                        radius: 4
                        color: {
                            if (classifier.level === "PWAPP") return "#4caf50";
                            if (classifier.level === "WAPP") return "#2196f3";
                            return "#9e9e9e";
                        }

                        Label {
                            anchors.centerIn: parent
                            text: classifier.level
                            font.pixelSize: 13
                            font.bold: true
                            color: "white"
                        }
                    }

                    Label {
                        text: classifier.name
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Metadata grid
                GridLayout {
                    visible: classifier.errorMessage.length === 0
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 6
                    Layout.fillWidth: true

                    Label { text: "Display mode:"; opacity: 0.6; font.pixelSize: 12 }
                    Label { text: classifier.displayMode; font.pixelSize: 12 }

                    Label { text: "Theme color:"; opacity: 0.6; font.pixelSize: 12; visible: classifier.themeColor.length > 0 }
                    RowLayout {
                        visible: classifier.themeColor.length > 0
                        spacing: 6
                        Rectangle {
                            width: 14; height: 14; radius: 2
                            color: classifier.themeColor.length > 0 ? classifier.themeColor : "transparent"
                            border.width: 1; border.color: palette.mid
                        }
                        Label { text: classifier.themeColor; font.pixelSize: 12 }
                    }

                    Label { text: "Start URL:"; opacity: 0.6; font.pixelSize: 12; visible: classifier.startUrl.length > 0 }
                    Label { text: classifier.startUrl; font.pixelSize: 12; visible: classifier.startUrl.length > 0; elide: Text.ElideRight; Layout.fillWidth: true }

                    Label { text: "Manifest:"; opacity: 0.6; font.pixelSize: 12 }
                    Label { text: classifier.hasManifest ? "Yes" : "No"; font.pixelSize: 12; color: classifier.hasManifest ? "#4caf50" : "#9e9e9e" }

                    Label { text: "Service worker:"; opacity: 0.6; font.pixelSize: 12 }
                    Label { text: classifier.hasServiceWorker ? "Yes" : "No"; font.pixelSize: 12; color: classifier.hasServiceWorker ? "#4caf50" : "#9e9e9e" }
                }
            }
        }

        // Install/Remove actions
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 500
            spacing: 12
            visible: classifier.hasResult && classifier.errorMessage.length === 0 && !appInstaller.busy

            Button {
                text: appInstaller.installed ? "Reinstall" : "Install"
                enabled: !appInstaller.busy
                font.pixelSize: 14

                onClicked: {
                    var url = root.normalizeUrl(urlField.text);
                    appInstaller.install(url);
                }
            }

            Button {
                text: "Remove"
                visible: appInstaller.installed
                enabled: !appInstaller.busy
                font.pixelSize: 14

                onClicked: {
                    appInstaller.uninstall(appInstaller.appId);
                }
            }
        }

        // Install status feedback
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 500
            spacing: 4
            visible: appInstaller.installed || appInstaller.errorMessage.length > 0

            Label {
                visible: appInstaller.installed
                text: "Installed! \"" + appInstaller.appName + "\" is available in your system launcher."
                font.pixelSize: 13
                color: "#4caf50"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                visible: appInstaller.errorMessage.length > 0
                text: appInstaller.errorMessage
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
            running: classifier.busy || appInstaller.busy
            visible: classifier.busy || appInstaller.busy
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Installing..."
            font.pixelSize: 13
            opacity: 0.6
            visible: appInstaller.busy
        }

        Item { Layout.fillHeight: true }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12

            Label {
                id: statusText
                text: {
                    if (appInstaller.busy) return "Installing...";
                    if (classifier.busy) return "Classifying...";
                    if (appInstaller.installed) return "Installed: " + appInstaller.appName;
                    if (classifier.hasResult && classifier.errorMessage.length === 0)
                        return "Classification complete";
                    if (classifier.hasResult && classifier.errorMessage.length > 0)
                        return "Classification failed";
                    return "Ready";
                }
                font.pixelSize: 12
                opacity: 0.6
            }

            Item { Layout.fillWidth: true }
        }
    }
}
