import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui

ApplicationWindow {
    id: window
    visible: true
    width: 880; height: 700
    minimumWidth: 600; minimumHeight: 500
    title: qsTr("Distribute files")
    color: Ui.Theme.background
    font.family: "sans-serif"
    font.pixelSize: 13
    palette.window: Ui.Theme.background
    palette.highlightedText: Ui.Theme.onAccent
    palette.windowText: Ui.Theme.text
    palette.text: Ui.Theme.text
    palette.placeholderText: Ui.Theme.muted
    palette.disabled.text: Ui.Theme.muted
    palette.disabled.windowText: Ui.Theme.muted
    palette.disabled.buttonText: Ui.Theme.muted
    palette.base: Ui.Theme.surface
    palette.button: Ui.Theme.surface
    palette.buttonText: Ui.Theme.text
    palette.highlight: Ui.Theme.accent
    onClosing: transferDialog.close()
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        Label { text: qsTr("Distribute files"); color: Ui.Theme.text; font.pixelSize: 26; font.bold: true }
        Label { text: qsTr("Send files to the selected classroom computers."); color: Ui.Theme.muted }
        GridLayout {
            columns: 2; Layout.fillWidth: true; enabled: !transferDialog.running
            Label { text: qsTr("Destination on remote computers"); color: Ui.Theme.text }
            Ui.TextField { id: destination; objectName: "transferDestination"; Layout.fillWidth: true; placeholderText: qsTr("Configured destination (or enter a path)") }
            Label { text: qsTr("After transfer"); color: Ui.Theme.text }
            Ui.ComboBox { id: openMode; objectName: "transferOpenMode"; Layout.fillWidth: true; model: [qsTr("Do nothing"), qsTr("Open destination folder"), qsTr("Open files in application")] }
            Item { width: 1; height: 1 }
            Ui.CheckBox { id: overwrite; objectName: "transferOverwrite"; text: qsTr("Overwrite existing files") }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true
                Label { text: qsTr("Files"); color: Ui.Theme.text; font.bold: true }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    objectName: "transferFiles"
                    model: transferDialog.files
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: ListView.view.width; height: fileRow.implicitHeight + 20
                        color: index % 2 ? Ui.Theme.surface : "transparent"
                        RowLayout {
                            id: fileRow; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 10; anchors.verticalCenter: parent.verticalCenter
                            Label {
                                objectName: "transferFileName"
                                text: modelData.name; Layout.fillWidth: true; elide: Text.ElideMiddle; color: Ui.Theme.text
                                HoverHandler { id: fileHover; objectName: "transferFileHover" }
                                ToolTip.visible: fileHover.hovered
                                ToolTip.text: modelData.path
                            }
                            Label { text: modelData.status; color: Ui.Theme.muted }
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: false; Layout.minimumWidth: 220; Layout.preferredWidth: 240; Layout.maximumWidth: 260; Layout.fillHeight: true
                Label { text: qsTr("Recipients"); color: Ui.Theme.text; font.bold: true }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    objectName: "transferTargets"
                    model: transferDialog.targets
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Column {
                        required property var modelData
                        width: ListView.view.width; spacing: 4
                        Label { text: modelData.name; color: Ui.Theme.text; width: parent.width; elide: Text.ElideRight }
                        Label { text: modelData.status; color: Ui.Theme.muted }
                        Item { width: 1; height: 12 }
                    }
                }
            }
        }
        Label { objectName: "transferErrors"; visible: transferDialog.errors.length > 0; text: transferDialog.errors; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Ui.Theme.warning; maximumLineCount: 5 }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Sending progress"); color: Ui.Theme.muted }
            ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: transferDialog.progress }
            Label { text: transferDialog.progress + "%"; color: Ui.Theme.text }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Ui.ActionButton { objectName: "transferCancel"; visible: transferDialog.running; text: qsTr("Cancel transfer"); danger: true; onClicked: transferDialog.cancel() }
            Ui.ActionButton { objectName: "transferStart"; visible: !transferDialog.running; text: transferDialog.completed ? qsTr("Send again") : qsTr("Start"); primary: true; onClicked: transferDialog.start(destination.text, openMode.currentIndex, overwrite.checked) }
            Ui.ActionButton { text: qsTr("Close"); onClicked: window.close() }
        }
    }
}
