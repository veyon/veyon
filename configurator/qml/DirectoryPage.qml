import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    property var controller
    property var edited: ({})
    property string parentFilter: ""
    readonly property var theme: Ui.Theme
    spacing: 10
    Label { text: qsTr("Local computer directory"); font.bold: true; color: theme.text }
    Label { visible: !root.controller; text: qsTr("The local directory plugin is not installed."); color: theme.warning }
    RowLayout {
        enabled: !!root.controller
        Ui.ActionButton { text: qsTr("Add room"); iconName: "plus"; onClicked: root.edit({uid:"", type:2, name:"", parentUid:root.parentFilter, host:"", mac:""}) }
        Ui.ActionButton { text: qsTr("Add computer"); iconName: "plus"; onClicked: root.edit({uid:"", type:3, name:"", parentUid:root.parentFilter, host:"", mac:""}) }
        Item { Layout.fillWidth: true }
        Ui.ActionButton { text: qsTr("Import…"); iconName: "download"; onClicked: { const path = configurator.chooseFile(false, "JSON (*.json)"); if (path) { root.edited = {path:path}; importDialog.open() } } }
        Ui.ActionButton { text: qsTr("Export…"); iconName: "upload"; onClicked: { const path = configurator.chooseFile(true, "JSON (*.json)"); if (path) root.controller.exportFile(path) } }
    }
    RowLayout {
        Label { text: qsTr("Location"); color: theme.muted }
        Ui.ComboBox { Layout.fillWidth: true; model: root.locations(); textRole: "name"; valueRole: "uid"; onActivated: root.parentFilter = currentValue }
    }
    Label { visible: !!root.controller && root.controller.error !== ""; text: root.controller ? root.controller.error : ""; color: theme.warning; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.objects : []
        delegate: ColumnLayout {
            required property var modelData
            visible: modelData.parentUid === root.parentFilter
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                Ui.Icon { name: modelData.type === 2 ? "folder" : "monitor"; size: 17; color: theme.muted }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.name; color: theme.text }
                    Label { visible: modelData.type === 3; text: modelData.host + (modelData.mac ? " · " + modelData.mac : ""); color: theme.muted; font.pixelSize: 11 }
                }
                Ui.ActionButton { visible: modelData.type === 2; text: qsTr("Open"); chevron: true; onClicked: root.parentFilter = modelData.uid }
                Ui.ActionButton { text: qsTr("Edit"); iconName: "pencil"; onClicked: root.edit(modelData) }
                Ui.ActionButton { iconName: "arrow-up"; text: "↑"; Accessible.name: qsTr("Move up"); onClicked: root.controller.move(modelData.uid, -1) }
                Ui.ActionButton { iconName: "arrow-down"; text: "↓"; Accessible.name: qsTr("Move down"); onClicked: root.controller.move(modelData.uid, 1) }
                Ui.ActionButton { iconName: "trash-2"; text: qsTr("Remove"); danger: true; onClicked: { root.edited = modelData; removeDialog.open() } }
            }
            Rectangle { height: 1; Layout.fillWidth: true; color: theme.divider }
        }
    }
    function locations() {
        const result = [{uid:"", name:qsTr("All top-level objects")}]
        if (controller) for (const object of controller.objects) if (object.type === 2) result.push(object)
        return result
    }
    function edit(object) {
        edited = JSON.parse(JSON.stringify(object))
        objectName.text = object.name || ""; host.text = object.host || ""; mac.text = object.mac || ""
        parentLocation.currentIndex = parentLocation.indexOfValue(object.parentUid || "")
        editor.open()
    }
    Dialog {
        id: editor; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: Math.min(550, parent.width - 40)
        title: root.edited.type === 2 ? qsTr("Edit room") : qsTr("Edit computer")
        standardButtons: Dialog.NoButton
        ColumnLayout {
            width: parent.width
            Label { text: qsTr("Name"); color: theme.text }
            Ui.TextField { id: objectName; Layout.fillWidth: true; placeholderText: qsTr("Name") }
            Label { text: qsTr("Parent room"); color: theme.text }
            Ui.ComboBox { id: parentLocation; Layout.fillWidth: true; model: root.locations(); textRole: "name"; valueRole: "uid" }
            Label { visible: root.edited.type === 3; text: qsTr("Hostname or IP address"); color: theme.text }
            Ui.TextField { id: host; visible: root.edited.type === 3; Layout.fillWidth: true }
            Label { visible: root.edited.type === 3; text: qsTr("MAC address"); color: theme.text }
            Ui.TextField { id: mac; visible: root.edited.type === 3; Layout.fillWidth: true }
            Label { text: root.controller ? root.controller.error : ""; color: theme.warning; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancel"); onClicked: editor.close() }
                Button { text: qsTr("Save"); enabled: objectName.text.trim().length > 0; onClicked: { const value = root.edited; value.name = objectName.text; value.host = host.text; value.mac = mac.text; value.parentUid = parentLocation.currentValue || ""; if (root.controller.save(value)) editor.close() } }
            }
        }
    }
    Dialog { id: removeDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; title: qsTr("Remove “%1” and its contents?").arg(root.edited.name || ""); standardButtons: Dialog.Yes | Dialog.No; onAccepted: root.controller.remove(root.edited.uid) }
    Dialog { id: importDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; title: qsTr("Replace the directory in this draft?"); standardButtons: Dialog.Yes | Dialog.No; onAccepted: root.controller.importFile(root.edited.path) }
}
