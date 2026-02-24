import QtQuick
import QtQuick.Controls

Menu {
    id: appMenu

    required property bool canGoBack
    required property bool canGoForward
    required property real zoomFactor

    signal goBackRequested()
    signal goForwardRequested()
    signal reloadRequested()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()
    signal openInBrowserRequested()

    signal aboutRequested()
    signal licenseRequested()

    MenuItem {
        text: "\u25C0  Back"
        enabled: appMenu.canGoBack
        onTriggered: appMenu.goBackRequested()
    }

    MenuItem {
        text: "\u25B6  Forward"
        enabled: appMenu.canGoForward
        onTriggered: appMenu.goForwardRequested()
    }

    MenuItem {
        text: "\u21BB  Reload"
        onTriggered: appMenu.reloadRequested()
    }

    MenuSeparator {}

    MenuItem {
        text: "\uFF0B  Zoom In"
        onTriggered: appMenu.zoomInRequested()
    }

    MenuItem {
        text: "\uFF0D  Zoom Out"
        onTriggered: appMenu.zoomOutRequested()
    }

    MenuItem {
        text: "100%  Reset Zoom"
        enabled: Math.abs(appMenu.zoomFactor - 1.0) > 0.01
        onTriggered: appMenu.zoomResetRequested()
    }

    MenuSeparator {}

    MenuItem {
        text: "\u2197  Open in Browser"
        onTriggered: appMenu.openInBrowserRequested()
    }

    MenuSeparator {}

    MenuItem {
        text: "About QApp"
        onTriggered: appMenu.aboutRequested()
    }

    MenuItem {
        text: "License (EUPL v1.2)"
        onTriggered: appMenu.licenseRequested()
    }
}
