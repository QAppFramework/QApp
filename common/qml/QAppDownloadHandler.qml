import QtQuick
import QtQuick.Dialogs

Item {
    id: handler

    readonly property bool downloading: internal.active
    readonly property string fileName: internal.fileName
    readonly property real receivedBytes: internal.receivedBytes
    readonly property real totalBytes: internal.totalBytes

    function handleDownload(download) {
        if (internal.active) {
            download.cancel()
            return
        }
        internal.currentDownload = download
        internal.fileName = download.suggestedFileName
        fileDialog.currentFolder = "file://" + download.downloadDirectory
        fileDialog.selectedFile = "file://" + download.downloadDirectory + "/" + download.suggestedFileName
        fileDialog.open()
    }

    function cancelDownload() {
        if (internal.currentDownload) {
            internal.currentDownload.cancel()
            internal.reset()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Save file"
        fileMode: FileDialog.SaveFile

        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            var dir = path.substring(0, path.lastIndexOf("/"))
            var name = path.substring(path.lastIndexOf("/") + 1)
            internal.currentDownload.downloadDirectory = dir
            internal.currentDownload.downloadFileName = name
            internal.fileName = name
            internal.active = true
            internal.currentDownload.accept()
        }
        onRejected: {
            if (internal.currentDownload)
                internal.currentDownload.cancel()
            internal.reset()
        }
    }

    Connections {
        target: internal.currentDownload
        enabled: internal.active

        function onReceivedBytesChanged() {
            internal.receivedBytes = internal.currentDownload.receivedBytes
        }
        function onTotalBytesChanged() {
            internal.totalBytes = internal.currentDownload.totalBytes
        }
        function onIsFinishedChanged() {
            if (internal.currentDownload && internal.currentDownload.isFinished)
                internal.reset()
        }
    }

    QtObject {
        id: internal
        property bool active: false
        property string fileName: ""
        property real receivedBytes: 0
        property real totalBytes: 0
        property var currentDownload: null

        function reset() {
            active = false
            fileName = ""
            receivedBytes = 0
            totalBytes = 0
            currentDownload = null
        }
    }
}
