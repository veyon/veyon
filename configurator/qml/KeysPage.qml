import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    property var controller
    property string selectedKey: ""
    property string operation: "create"
    property string inputPath: ""
    property string authResult: ""
    readonly property var theme: Ui.Theme
    Label { text: qsTr("Authentication keys"); color: theme.text; font.bold: true }
    Label { text: qsTr("Key operations take effect immediately. Configuration Apply and Revert do not undo changes to key files."); color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Flow {
        Layout.fillWidth: true; spacing: 8
        Ui.ActionButton { text: qsTr("Create key pair…"); iconName: "plus"; enabled: !!root.controller; onClicked: { root.operation = "create"; input.text = ""; inputDialog.title = qsTr("Name of the group or role"); inputDialog.open() } }
        Ui.ActionButton { text: qsTr("Import key…"); iconName: "download"; enabled: !!root.controller; onClicked: { root.inputPath = configurator.chooseFile(false, "Key files (*.pem)"); if (root.inputPath) { root.operation = "import"; input.text = ""; inputDialog.title = qsTr("Key name (leave empty to infer from filename)"); inputDialog.open() } } }
        Ui.ActionButton { text: qsTr("Refresh"); iconName: "refresh-cw"; enabled: !!root.controller; onClicked: root.controller.refresh() }
        Ui.ActionButton {
            text: qsTr("Test authentication"); iconName: "check"
            onClicked: {
                root.authResult = ""
                if (configurator.properties.value("Authentication/Method") === 1)
                    root.authResult = configurator.testAuthentication("", "") ? qsTr("Authentication succeeded.") : ""
                else authDialog.open()
            }
        }
    }
    Label { text: root.authResult; visible: text !== ""; color: theme.ok; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Label { text: root.controller ? root.controller.message : ""; visible: text !== ""; color: theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.keys : []
        delegate: ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                Ui.Icon { name: "key-round"; size: 17; color: theme.muted }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.name + " / " + modelData.type; font.bold: true; color: theme.text }
                    Label { text: qsTr("Access group: %1 · Pair ID: %2").arg(modelData.group).arg(modelData.pairId); color: theme.muted; font.pixelSize: 11 }
                }
                Ui.ActionButton { text: qsTr("Test key"); onClicked: root.controller.test(modelData.key) }
                Ui.ActionButton { text: qsTr("Access group…"); onClicked: { root.selectedKey = modelData.key; root.operation = "group"; input.text = modelData.group; inputDialog.title = qsTr("Group allowed to access this key"); inputDialog.open() } }
                Ui.ActionButton { text: qsTr("Export…"); onClicked: { const path = configurator.chooseFile(true, "Key files (*.pem)"); if (path) root.controller.exportKey(modelData.key, path) } }
                Ui.ActionButton { text: qsTr("Delete"); danger: true; iconName: "trash-2"; onClicked: { root.selectedKey = modelData.key; removeDialog.open() } }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider }
        }
    }
    Dialog {
        id: inputDialog; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(520, parent.width - 40); modal: true; standardButtons: Dialog.Ok | Dialog.Cancel
        Ui.TextField { id: input; width: parent.width; selectByMouse: true; onAccepted: inputDialog.accept() }
        onAccepted: { if (root.operation === "create") root.controller.create(input.text); else if (root.operation === "import") root.controller.importKey(input.text, root.inputPath); else root.controller.setAccessGroup(root.selectedKey, input.text) }
    }
    Dialog { id: removeDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; title: qsTr("Permanently delete key “%1”?").arg(root.selectedKey); standardButtons: Dialog.Yes | Dialog.No; onAccepted: root.controller.remove(root.selectedKey) }
    Dialog {
        id: authDialog; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(440, parent.width - 40); modal: true; title: qsTr("Test logon authentication"); standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            width: parent.width
            Ui.TextField { id: username; Layout.fillWidth: true; placeholderText: qsTr("Username") }
            Ui.TextField { id: password; Layout.fillWidth: true; placeholderText: qsTr("Password"); echoMode: TextInput.Password }
        }
        onAccepted: { root.authResult = configurator.testAuthentication(username.text, password.text) ? qsTr("Authentication succeeded.") : ""; password.clear() }
        onRejected: password.clear()
    }
}
