import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    property int editedIndex: -1
    property var edited: ({})
    property string testResult: ""
    property var allGroups: []
    property var groupSuggestions: []
    property var locationSuggestions: []
    readonly property var theme: Ui.Theme

    Label { text: qsTr("Access policy"); color: theme.text; font.bold: true }
    Ui.ComboBox {
        Layout.fillWidth: true
        model: [qsTr("Allow all authenticated users"), qsTr("Restrict access to user groups"), qsTr("Process access control rules")]
        currentIndex: configurator.accessMode
        onActivated: configurator.setAccessMode(currentIndex)
    }
    ColumnLayout {
        visible: configurator.accessMode === 1
        Layout.fillWidth: true
        RowLayout {
            Label { text: qsTr("Authorized user groups"); color: theme.text; Layout.fillWidth: true }
            Ui.ActionButton { text: qsTr("Load groups"); onClicked: root.allGroups = configurator.availableGroups() }
        }
        Repeater {
            model: root.allGroups
            delegate: Ui.CheckBox {
                required property string modelData
                text: modelData
                checked: configurator.authorizedGroups.indexOf(modelData) >= 0
                onToggled: {
                    let groups = configurator.properties.value("AccessControl/AuthorizedUserGroups") || []
                    groups = groups.filter(function(value) { return value !== modelData })
                    if (checked) groups.push(modelData)
                    configurator.properties.setValue("AccessControl/AuthorizedUserGroups", groups)
                }
            }
        }
        TextArea {
            objectName: "authorizedGroupsEditor"
            readonly property bool configurationEditorPending: text !== configurator.authorizedGroups.join("\n")
            Layout.fillWidth: true
            text: configurator.authorizedGroups.join("\n")
            placeholderText: qsTr("One authorized group per line")
            onActiveFocusChanged: if (!activeFocus) configurator.properties.setValue("AccessControl/AuthorizedUserGroups", text.split("\n").filter(function(value) { return value.trim() !== "" }))
        }
        RowLayout {
            Ui.TextField { id: groupTestUser; Layout.fillWidth: true; placeholderText: qsTr("User login name to test") }
            Ui.ActionButton { text: qsTr("Test access"); onClicked: root.testResult = configurator.testUserGroupAccess(groupTestUser.text) }
        }
        Label { text: root.testResult; color: theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Access rules"); font.bold: true; color: theme.text; Layout.fillWidth: true }
        Ui.ActionButton { text: qsTr("Test rules…"); iconName: "check"; onClicked: testDialog.open() }
        Ui.ActionButton { text: qsTr("Add rule"); iconName: "plus"; onClicked: root.edit(-1, configurator.newRule()) }
    }
    Label { text: qsTr("Rules are evaluated in order. The first matching rule determines access."); color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Repeater {
        model: configurator.rules
        delegate: ColumnLayout {
            required property var modelData
            required property int index
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                Label { text: String(index + 1); color: theme.muted; Layout.preferredWidth: 22 }
                ColumnLayout { Layout.fillWidth: true; Label { text: modelData.name; color: theme.text; font.bold: true } Label { text: modelData.description; color: theme.muted; visible: text.length > 0 } }
                Label { text: modelData.action === 1 ? qsTr("Allow") : modelData.action === 2 ? qsTr("Deny") : modelData.action === 3 ? qsTr("Ask permission") : qsTr("No action"); color: modelData.action === 1 ? theme.ok : theme.warning }
                Ui.ActionButton { text: qsTr("Edit"); iconName: "pencil"; onClicked: root.edit(index, modelData) }
                Ui.ActionButton { text: "↑"; enabled: index > 0; Accessible.name: qsTr("Move rule up"); onClicked: configurator.moveRule(index, -1) }
                Ui.ActionButton { text: "↓"; enabled: index + 1 < configurator.rules.length; Accessible.name: qsTr("Move rule down"); onClicked: configurator.moveRule(index, 1) }
                Ui.ActionButton { text: qsTr("Remove"); iconName: "trash-2"; onClicked: configurator.removeRule(index) }
            }
            Rectangle { height: 1; Layout.fillWidth: true; color: theme.divider }
        }
    }
    function edit(index, value) {
        editedIndex = index; edited = JSON.parse(JSON.stringify(value))
        ruleName.text = edited.name; description.text = edited.description; action.currentIndex = edited.action
        ignore.checked = edited.ignore; invert.checked = edited.invert
        groupSuggestions = configurator.ruleArguments(1); locationSuggestions = configurator.ruleArguments(3)
        editor.open()
    }
    Dialog {
        id: editor; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(780, parent.width - 40); height: Math.min(720, parent.height - 40); modal: true
        title: qsTr("Access rule"); standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            Ui.TextField { id: ruleName; Layout.fillWidth: true; placeholderText: qsTr("Rule name") }
            Ui.TextField { id: description; Layout.fillWidth: true; placeholderText: qsTr("Description") }
            Ui.ComboBox { id: action; Layout.fillWidth: true; model: [qsTr("No action"), qsTr("Allow access"), qsTr("Deny access"), qsTr("Ask logged-on user for permission")] }
            RowLayout { Ui.CheckBox { id: ignore; text: qsTr("Ignore conditions") } Ui.CheckBox { id: invert; text: qsTr("Invert conditions") } }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; contentWidth: availableWidth; clip: true
                ColumnLayout {
                    width: parent.width
                    Repeater {
                        model: root.edited.conditions || []
                        delegate: ColumnLayout {
                            id: condition
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true; enabled: !ignore.checked
                            Ui.CheckBox { text: configurator.conditionTypes[condition.index].label; checked: condition.modelData.enabled; onToggled: root.edited.conditions[condition.index].enabled = checked }
                            RowLayout {
                                visible: condition.modelData.type === 1 || condition.modelData.type === 3
                                Layout.fillWidth: true; Layout.leftMargin: 26
                                Ui.ComboBox {
                                    Layout.preferredWidth: 190
                                    model: condition.modelData.type === 1 ? [{label:qsTr("Accessing user"), value:1}, {label:qsTr("Local user"), value:3}] : [{label:qsTr("Accessing computer"), value:2}, {label:qsTr("Local computer"), value:4}]
                                    textRole: "label"; valueRole: "value"
                                    currentIndex: condition.modelData.subject === 3 || condition.modelData.subject === 4 ? 1 : 0
                                    onActivated: root.edited.conditions[condition.index].subject = currentValue
                                }
                                Ui.ComboBox {
                                    Layout.fillWidth: true
                                    editable: true
                                    model: condition.modelData.type === 1 ? root.groupSuggestions : root.locationSuggestions
                                    editText: condition.modelData.argument
                                    onEditTextChanged: root.edited.conditions[condition.index].argument = editText
                                }
                            }
                        }
                    }
                }
            }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancel"); onClicked: editor.close() }
                Button { text: qsTr("Save rule"); enabled: ruleName.text.trim().length > 0; onClicked: { root.edited.name = ruleName.text; root.edited.description = description.text; root.edited.action = action.currentIndex; root.edited.ignore = ignore.checked; root.edited.invert = invert.checked; if (configurator.saveRule(root.editedIndex, root.edited)) editor.close() } }
            }
        }
    }
    Dialog {
        id: testDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: Math.min(600, parent.width - 40); title: qsTr("Test access rules"); standardButtons: Dialog.Close
        ColumnLayout {
            width: parent.width
            Ui.TextField { id: accessingUser; Layout.fillWidth: true; placeholderText: qsTr("Accessing user") }
            Ui.TextField { id: accessingComputer; Layout.fillWidth: true; placeholderText: qsTr("Accessing computer") }
            Ui.TextField { id: localUser; Layout.fillWidth: true; placeholderText: qsTr("Local user") }
            Ui.TextField { id: localComputer; Layout.fillWidth: true; placeholderText: qsTr("Local computer") }
            Ui.TextField { id: connectedUsers; Layout.fillWidth: true; placeholderText: qsTr("Already connected users (comma separated)") }
            Button { text: qsTr("Run test"); onClicked: root.testResult = configurator.testRules({accessingUser:accessingUser.text, accessingComputer:accessingComputer.text, localUser:localUser.text, localComputer:localComputer.text, connectedUsers:connectedUsers.text}) }
            Label { text: root.testResult; color: theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }
}
