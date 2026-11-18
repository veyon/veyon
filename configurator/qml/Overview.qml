import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui
ColumnLayout {
    id: root
    signal navigate(string page)
    readonly property var theme: Ui.Theme
    spacing: 0
    Repeater {
        model: [
            {title: qsTr("Veyon service"), detail: configurator.serviceRunning ? qsTr("Running") : qsTr("Stopped"), icon: "server-cog", page: "service"},
            {title: qsTr("Authentication"), detail: qsTr("Configure teacher sign-in and authentication keys"), icon: "key-round", page: "authentication"},
            {title: qsTr("Computers and rooms"), detail: qsTr("Manage your local directory or connect an external directory"), icon: "monitor", page: "computers"},
            {title: qsTr("Access control"), detail: qsTr("Review who can access this computer"), icon: "shield", page: "access"},
            {title: qsTr("LDAP / Active Directory"), detail: qsTr("Connect your organization’s directory"), icon: "network", page: "ldap"}
        ]
        delegate: ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true; Layout.topMargin: 13; Layout.bottomMargin: 13; spacing: 14
                Ui.Icon { name: modelData.icon; size: 20; color: theme.muted }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 3
                    Label { text: modelData.title; font.bold: true; color: theme.text }
                    Label { text: modelData.detail; color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                Ui.ActionButton { text: qsTr("Open"); chevron: true; onClicked: root.navigate(modelData.page) }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider }
        }
    }
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 20
        Label { text: qsTr("Configuration"); font.bold: true; color: theme.text; Layout.fillWidth: true }
        Ui.ActionButton { text: qsTr("Export…"); iconName: "upload"; onClicked: configurator.exportConfiguration(configurator.chooseFile(true, "JSON (*.json)")) }
        Ui.ActionButton { text: qsTr("Import…"); iconName: "download"; onClicked: configurator.importConfiguration(configurator.chooseFile(false, "JSON (*.json)")) }
    }
}
