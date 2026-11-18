import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui

ApplicationWindow {
    id: window
    visible: true
    width: 920; height: 760
    minimumWidth: 640; minimumHeight: 560
    title: qsTr("Collect files")
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
    onClosing: collectDialog.close()
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        Label { text: qsTr("Collect files"); color: Ui.Theme.text; font.pixelSize: 26; font.bold: true }
        Label { text: qsTr("Collect work from the selected classroom computers."); color: Ui.Theme.muted }
        GridLayout {
            Layout.fillWidth: true; columns: 3; enabled: !collectDialog.running
            Label { text: qsTr("Remote source directory"); color: Ui.Theme.text }
            Ui.TextField { id: source; objectName: "collectSource"; Layout.fillWidth: true; Layout.columnSpan: 2; text: collectDialog.sourceDirectory; placeholderText: qsTr("Relative or absolute path on remote computers") }
            Label { text: qsTr("File pattern"); color: Ui.Theme.text }
            Ui.TextField { id: pattern; objectName: "collectPattern"; Layout.fillWidth: true; Layout.columnSpan: 2; text: collectDialog.filePattern; placeholderText: qsTr("*.docx;*.pdf — empty for all files") }
            Label { text: qsTr("Subfolders"); color: Ui.Theme.text }
            Ui.ComboBox { id: recursive; objectName: "collectSubfolders"; Layout.fillWidth: true; Layout.columnSpan: 2; model: [qsTr("Use configured setting"), qsTr("Source directory only"), qsTr("Include subdirectories")] }
            Label { text: qsTr("Local destination"); color: Ui.Theme.text }
            Ui.TextField { objectName: "collectDestination"; Layout.fillWidth: true; text: collectDialog.destinationDirectory; readOnly: true }
            Ui.ActionButton { text: qsTr("Browse…"); iconName: "folder"; onClicked: collectDialog.browseDestination() }
        }
        Label { text: qsTr("Collected files"); color: Ui.Theme.text; font.bold: true }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            Column {
                width: window.width - 48; spacing: 8
                Repeater {
                    model: collectDialog.collections
                    delegate: Column {
                        id: collectionRow
                        required property var modelData
                        property bool expanded: true
                        width: parent.width; spacing: 8
                        Rectangle {
                            width: parent.width; height: 52; color: Ui.Theme.surface
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 10
                                ToolButton { text: collectionRow.expanded ? "▾" : "▸"; onClicked: collectionRow.expanded = !collectionRow.expanded; Accessible.name: qsTr("Expand or collapse files") }
                                Label { text: collectionRow.modelData.name; color: Ui.Theme.text; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: collectionRow.modelData.fileCount + " " + qsTr("files"); color: Ui.Theme.muted }
                                Label { text: collectionRow.modelData.status; color: Ui.Theme.muted }
                                ProgressBar { Layout.preferredWidth: 120; from: 0; to: 100; value: collectionRow.modelData.progress }
                                Label { text: collectionRow.modelData.progress + "%"; color: Ui.Theme.text }
                            }
                        }
                        Repeater {
                            model: collectionRow.expanded ? collectionRow.modelData.files : []
                            delegate: RowLayout {
                                required property var modelData
                                width: collectionRow.width; height: 32
                                Item { Layout.preferredWidth: 44 }
                                Label { text: modelData.name; color: Ui.Theme.text; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                ProgressBar { Layout.preferredWidth: 120; from: 0; to: 100; value: modelData.progress }
                                Label { text: modelData.progress + "%"; color: Ui.Theme.muted; Layout.preferredWidth: 48 }
                            }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Overall progress"); color: Ui.Theme.muted }
            ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: collectDialog.progress }
            Label { text: collectDialog.progress + "%"; color: Ui.Theme.text }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Ui.ActionButton { visible: collectDialog.completed; text: qsTr("Open output directory"); iconName: "folder"; onClicked: collectDialog.openOutputDirectory() }
            Ui.ActionButton { visible: collectDialog.running; text: qsTr("Cancel collection"); danger: true; onClicked: collectDialog.cancel() }
            Ui.ActionButton { objectName: "collectStart"; visible: !collectDialog.running; text: collectDialog.completed ? qsTr("Collect again") : qsTr("Start"); primary: true; onClicked: collectDialog.start(source.text, pattern.text, recursive.currentIndex) }
            Ui.ActionButton { text: qsTr("Close"); onClicked: window.close() }
        }
    }
}
