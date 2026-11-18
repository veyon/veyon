import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    property var controller
    property bool website: false
    property var edited: ({})
    readonly property var theme: Ui.Theme
    TabBar { Layout.fillWidth: true; onCurrentIndexChanged: root.website = currentIndex === 1; TabButton { text: qsTr("Applications") } TabButton { text: qsTr("Websites") } }
    RowLayout {
        Layout.fillWidth: true
        Label { text: root.website ? qsTr("Websites available to teachers") : qsTr("Applications available to teachers"); color: theme.muted; Layout.fillWidth: true }
        Ui.ActionButton { text: qsTr("Add preset"); iconName: "plus"; enabled: !!root.controller; onClicked: root.edit({uid:"", name:"", path:""}) }
    }
    Repeater {
        model: root.controller ? (root.website ? root.controller.websites : root.controller.applications) : []
        delegate: RowLayout {
            required property var modelData
            Layout.fillWidth: true
            ColumnLayout { Layout.fillWidth: true; Label { text: modelData.name; color: theme.text } Label { text: modelData.path; color: theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true } }
            Ui.ActionButton { text: qsTr("Edit"); iconName: "pencil"; onClicked: root.edit(modelData) }
            Ui.ActionButton { text: "↑"; Accessible.name: qsTr("Move up"); onClicked: root.controller.move(root.website, modelData.uid, -1) }
            Ui.ActionButton { text: "↓"; Accessible.name: qsTr("Move down"); onClicked: root.controller.move(root.website, modelData.uid, 1) }
            Ui.ActionButton { text: qsTr("Remove"); iconName: "trash-2"; onClicked: root.controller.remove(root.website, modelData.uid) }
        }
    }
    function edit(value) { edited = value; presetName.text = value.name; path.text = value.path; editor.open() }
    Dialog {
        id: editor; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(570, parent.width - 40); modal: true
        title: root.website ? qsTr("Website preset") : qsTr("Application preset"); standardButtons: Dialog.NoButton
        ColumnLayout {
            width: parent.width
            Label { text: qsTr("Name"); color: theme.text }
            Ui.TextField { id: presetName; Layout.fillWidth: true }
            Label { text: root.website ? qsTr("Website URL") : qsTr("Application command or path"); color: theme.text }
            Ui.TextField { id: path; Layout.fillWidth: true }
            RowLayout { Item { Layout.fillWidth: true } Button { text: qsTr("Cancel"); onClicked: editor.close() } Button { text: qsTr("Save"); enabled: presetName.text.trim().length > 0 && path.text.trim().length > 0; onClicked: { if (root.controller.save(root.website, root.edited.uid, presetName.text, path.text)) editor.close() } } }
        }
    }
}
