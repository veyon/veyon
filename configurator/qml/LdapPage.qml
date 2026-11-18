import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    property var controller
    property var selectedOperation: controller && operation.currentIndex >= 0 ? (controller.operations[operation.currentIndex] || ({})) : ({})
    property var browseFields: [
        {key:"BaseDN", label:qsTr("Base distinguished name")}, {key:"UserTree", label:qsTr("User tree")},
        {key:"GroupTree", label:qsTr("Group tree")}, {key:"ComputerTree", label:qsTr("Computer tree")},
        {key:"ComputerGroupTree", label:qsTr("Computer group tree")}, {key:"BindDN", label:qsTr("Bind distinguished name")},
        {key:"UserLoginNameAttribute", label:qsTr("User login name attribute")}, {key:"GroupMemberAttribute", label:qsTr("Group member attribute")},
        {key:"ComputerDisplayNameAttribute", label:qsTr("Computer display name attribute")}, {key:"ComputerHostNameAttribute", label:qsTr("Computer hostname attribute")},
        {key:"ComputerMacAddressAttribute", label:qsTr("Computer MAC address attribute")}, {key:"LocationNameAttribute", label:qsTr("Location name attribute")},
        {key:"UserGroupsAttribute", label:qsTr("User groups attribute")}, {key:"ComputerLocationAttribute", label:qsTr("Computer location attribute")},
        {key:"NamingContextAttribute", label:qsTr("Naming context attribute")}
    ]
    readonly property var theme: Ui.Theme
    spacing: 12
    Label { text: qsTr("Computer location discovery"); font.bold: true; color: theme.text }
    Ui.ComboBox { Layout.fillWidth: true; enabled: !!root.controller; model: [qsTr("Computer groups"), qsTr("Container hierarchy"), qsTr("Computer attribute")]; currentIndex: root.controller ? root.controller.locationMode : 0; onActivated: root.controller.setLocationMode(currentIndex) }
    Label { text: qsTr("Directory diagnostics"); font.bold: true; color: theme.text }
    Label { visible: !root.controller; text: qsTr("LDAP support is not installed."); color: theme.warning }
    RowLayout {
        enabled: !!root.controller && !root.controller.busy
        Layout.fillWidth: true
        Ui.ComboBox { id: operation; Layout.fillWidth: true; model: root.controller ? root.controller.operations : []; textRole: "label" }
        Ui.ActionButton { text: qsTr("Run test"); iconName: "check"; enabled: !root.selectedOperation.argument || argument.text.trim().length > 0; onClicked: root.controller.run(root.selectedOperation.id, argument.text) }
    }
    Ui.TextField { id: argument; Layout.fillWidth: true; visible: !!root.selectedOperation.argument; placeholderText: root.selectedOperation.argument || ""; onAccepted: if (root.controller && !root.controller.busy) root.controller.run(root.selectedOperation.id, text) }
    Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider }
    Label { text: qsTr("Browse directory"); font.bold: true; color: theme.text }
    RowLayout {
        Layout.fillWidth: true; enabled: !!root.controller && !root.controller.busy
        Ui.ComboBox { id: field; Layout.fillWidth: true; model: root.browseFields; textRole: "label"; valueRole: "key" }
        Ui.ActionButton { text: qsTr("Browse"); iconName: "folder-open"; onClicked: root.controller.browse(field.currentValue, browseDn.text, false) }
        Ui.ActionButton { text: qsTr("Attributes"); enabled: browseDn.text.length > 0; onClicked: root.controller.browse(field.currentValue, browseDn.text, true) }
    }
    Ui.TextField { id: browseDn; Layout.fillWidth: true; placeholderText: qsTr("Distinguished name (leave empty to browse naming contexts)"); selectByMouse: true }
    RowLayout { BusyIndicator { running: root.controller ? root.controller.busy : false; visible: running } Label { visible: root.controller ? root.controller.busy : false; text: qsTr("Querying LDAP…"); color: theme.muted } }
    Label { text: root.controller ? root.controller.error : ""; visible: text !== ""; color: theme.warning; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Label { visible: root.controller && root.controller.results.length > 0; text: qsTr("%1 results").arg(root.controller ? root.controller.results.length : 0); color: theme.muted }
    Repeater {
        model: root.controller ? root.controller.results : []
        delegate: RowLayout {
            required property string modelData
            Layout.fillWidth: true
            TextEdit { text: modelData; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: theme.text; Layout.fillWidth: true }
            Ui.ActionButton { visible: !!root.controller.targetKey && modelData.indexOf("=") >= 0; text: qsTr("Open"); onClicked: { browseDn.text = modelData; root.controller.browse(field.currentValue, modelData, false) } }
            Ui.ActionButton { visible: !!root.controller.targetKey; text: qsTr("Use value"); onClicked: root.controller.select(modelData) }
        }
    }
}
