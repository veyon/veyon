import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Veyon 1.0
import "qrc:/quick" as Ui
ApplicationWindow {
    id: root
    objectName: "remoteAccessWindow"
    readonly property var remoteSession: remoteWorkspace.currentSession
    onClosing: remoteWorkspace.closeAll(false)
    visible: true
    width: 1180; height: 800
    minimumWidth: 760; minimumHeight: 480
    title: remoteSession ? remoteSession.title + " — Veyon" : "Veyon"
    color: Ui.Theme.background
    font.family: "sans-serif"
    font.pixelSize: 13
    palette.window: Ui.Theme.background
    palette.windowText: Ui.Theme.text
    palette.base: Ui.Theme.surface
    palette.alternateBase: Ui.Theme.background
    palette.text: Ui.Theme.text
    palette.placeholderText: Ui.Theme.muted
    palette.button: Ui.Theme.surface
    palette.buttonText: Ui.Theme.text
    palette.mid: Ui.Theme.divider
    palette.dark: Ui.Theme.divider
    palette.light: Ui.Theme.surface
    palette.midlight: Ui.Theme.hover
    palette.highlight: Ui.Theme.accent
    palette.highlightedText: Ui.Theme.onAccent
    palette.toolTipBase: Ui.Theme.surface
    palette.toolTipText: Ui.Theme.text
    palette.link: Ui.Theme.accent
    palette.linkVisited: Ui.Theme.muted
    palette.disabled.text: Ui.Theme.muted
    palette.disabled.windowText: Ui.Theme.muted
    palette.disabled.buttonText: Ui.Theme.muted
    property bool fit: true
    header: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 54; color: Ui.Theme.surface
            RowLayout {
                anchors.fill: parent; anchors.margins: 10
                Ui.ActionButton { text: qsTr("Classroom"); iconName: "arrow-left"; enabled: !!remoteSession; onClicked: if (remoteSession) remoteSession.showMaster() }
                Label { text: remoteSession ? remoteSession.title : ""; elide: Text.ElideRight; color: Ui.Theme.text; font.bold: true; Layout.fillWidth: true }
                Label { text: remoteSession ? remoteSession.status : ""; color: remoteSession && remoteSession.connected ? Ui.Theme.ok : Ui.Theme.warning }
                Ui.ActionButton { text: qsTr("Open in window"); iconName: "external-link"; enabled: !!remoteSession; onClicked: remoteWorkspace.detachCurrent() }
            }
        }
        TabBar {
            Layout.fillWidth: true
            currentIndex: remoteWorkspace.currentIndex
            Repeater {
                model: remoteWorkspace.sessions
                TabButton {
                    id: tab
                    required property var modelData
                    required property int index
                    width: Math.min(260,Math.max(160,implicitWidth))
                    onClicked: remoteWorkspace.activate(index)
                    contentItem: RowLayout {
                        Label { text: tab.modelData ? tab.modelData.title : ""; color: Ui.Theme.text; elide: Text.ElideRight; Layout.fillWidth: true }
                        ToolButton { text: "×"; Accessible.name: qsTr("Close remote session"); enabled: !!tab.modelData; onClicked: if (tab.modelData) tab.modelData.close() }
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: remoteActions.implicitHeight + 16; color: Ui.Theme.surface
            Flow {
                id: remoteActions
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; spacing: 6
                Ui.ActionButton { text: qsTr("View"); iconName: "eye"; checked: remoteSession ? remoteSession.viewOnly : false; enabled: !!remoteSession && (remoteSession.canToggle || remoteSession.viewOnly); onClicked: if (remoteSession) remoteSession.viewOnly = true }
                Ui.ActionButton { text: qsTr("Control"); iconName: "mouse-pointer-2"; checked: remoteSession ? !remoteSession.viewOnly : false; enabled: !!remoteSession && (remoteSession.canToggle || !remoteSession.viewOnly); onClicked: if (remoteSession) { remoteSession.viewOnly = false; framebuffer.forceActiveFocus() } }
                Ui.ActionButton { text: qsTr("Shortcuts"); iconName: "keyboard"; chevron: true; enabled: !!remoteSession && !remoteSession.viewOnly && remoteSession.connected; onClicked: shortcuts.open()
                    Menu { id: shortcuts; y: parent.height
                        Repeater { model: ["Ctrl+Alt+Del", "Ctrl+Esc", "Alt+Tab", "Alt+F4", "Win+Tab", "Win", "Menu", "Ctrl+Alt+F1"]
                            MenuItem { text: modelData; enabled: !!remoteSession && !remoteSession.viewOnly && remoteSession.connected; onTriggered: framebuffer.sendShortcut(index) }
                        }
                    }
                }
                Ui.ActionButton { iconName: "camera"; text: qsTr("Screenshot"); enabled: !!remoteSession && remoteSession.connected; onClicked: if (remoteSession) remoteSession.screenshot() }
                Ui.ActionButton { iconName: "scan"; text: qsTr("Fit"); checked: root.fit; onClicked: root.fit = !root.fit }
                ComboBox { width: 160; enabled: !!remoteSession; model: remoteSession ? remoteSession.screens : []; textRole: "name"; valueRole: "index"
                    currentIndex: { if (!remoteSession) return -1; let screens = remoteSession.screens; for (let i=0; i<screens.length; ++i) if (screens[i].index === remoteSession.currentScreen) return i; return 0 }
                    onActivated: if (remoteSession) remoteSession.selectScreen(currentValue)
                }
                Item { Layout.fillWidth: true }
                Ui.ActionButton { iconName: "maximize"; text: qsTr("Fullscreen"); onClicked: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
                Ui.ActionButton { iconName: "log-out"; text: qsTr("End session"); enabled: !!remoteSession; onClicked: if (remoteSession) remoteSession.close() }
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 36; color: !remoteSession || remoteSession.viewOnly ? Ui.Theme.surface : Ui.Theme.accent
            Label { anchors.centerIn: parent; text: !remoteSession ? "" : remoteSession.viewOnly ? qsTr("View only · remote input is disabled") : qsTr("You are controlling this computer"); color: !remoteSession || remoteSession.viewOnly ? Ui.Theme.muted : Ui.Theme.onAccent }
        }
        Flickable {
            id: stage; Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: root.fit ? width : Math.max(width, framebuffer.sourceSize.width)
            contentHeight: root.fit ? height : Math.max(height, framebuffer.sourceSize.height)
            interactive: !root.fit && !!remoteSession && remoteSession.viewOnly
            ScrollBar.horizontal: ScrollBar { }
            ScrollBar.vertical: ScrollBar { }
            Framebuffer {
                id: framebuffer; width: stage.contentWidth; height: stage.contentHeight
                computer: remoteSession ? remoteSession.computer : null; live: !!remoteSession; viewOnly: !remoteSession || remoteSession.viewOnly; viewport: remoteSession ? remoteSession.viewport : Qt.rect(0, 0, 0, 0)
                focus: true
            }
            BusyIndicator { anchors.centerIn: parent; running: remoteSession ? !remoteSession.connected : false; visible: running }
        }
    }
    Shortcut { sequence: "Ctrl+Shift+F"; onActivated: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
}
