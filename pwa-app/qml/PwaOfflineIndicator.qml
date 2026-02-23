import QtQuick
import QtQuick.Controls

Rectangle {
    id: indicator

    required property bool isOnline

    visible: !indicator.isOnline
    width: label.implicitWidth + 24
    height: label.implicitHeight + 12
    radius: 4
    color: "#e74c3c"
    z: 998

    Label {
        id: label
        anchors.centerIn: parent
        text: "Offline"
        font.pixelSize: 12
        font.weight: Font.Medium
        color: "white"
    }

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }
}
