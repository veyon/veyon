import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/quick" as Ui

ColumnLayout {
    id: row
    required property var model
    required property int index
    property var propertyRepeater
    signal openPage(string pageId)
    property string editorKind: "text"
    property bool customHandled: model.key === "AccessControl/AccessControlRules" || model.key === "AccessControl/AccessRestrictedToUserGroups" || model.key === "AccessControl/AccessControlRulesProcessingEnabled" || model.key === "AccessControl/AuthorizedUserGroups" || model.key === "Features/DisabledFeatures" || model.key === "BuiltinDirectory/NetworkObjects" || model.key.indexOf("DesktopServices/Predefined") === 0 || model.key === "LDAP/ComputerLocationsByContainer" || model.key === "LDAP/ComputerLocationsByAttribute"
    Layout.fillWidth: true
    width: parent ? parent.width : 600
    visible: model.visible && (!customHandled || configurator.properties.search.length > 0)
    spacing: 7
    readonly property var theme: Ui.Theme
    Label {
        visible: row.model.groupId !== "" && (row.index === 0 || !row.propertyRepeater || !row.propertyRepeater.itemAt(row.index - 1) || row.propertyRepeater.itemAt(row.index - 1).model.groupId !== row.model.groupId)
        text: row.model.groupId
        font.bold: true
        color: theme.text
    }
    RowLayout {
        Layout.fillWidth: true
        enabled: row.model.enabled
        spacing: 20
        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 300
            Label { text: row.model.label; color: theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { visible: row.model.toolTip !== ""; text: row.model.toolTip; color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true; font.pixelSize: 11 }
        }
        Loader {
            Layout.preferredWidth: Math.min(row.editorKind === "integer" ? 160 : 300, row.width * 0.47)
            Layout.minimumWidth: 140
            sourceComponent: row.customHandled ? customLink : row.editorKind === "boolean" ? booleanEditor : row.editorKind === "integer" ? integerEditor : row.editorKind === "choice" ? choiceEditor : row.editorKind === "custom" ? listEditor : textEditor
        }
    }
    Label { visible: row.model.error !== ""; text: row.model.error; color: theme.warning; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider; opacity: 0.6 }
    Component {
        id: customLink
        Ui.ActionButton { text: qsTr("Open settings"); chevron: true; onClicked: row.openPage(row.model.pageId) }
    }
    Component {
        id: booleanEditor
        Ui.CheckBox { checked: !!row.model.value; onToggled: configurator.properties.setValue(row.model.key, checked); Accessible.name: row.model.label }
    }
    Component {
        id: integerEditor
        Ui.SpinBox {
            id: integerField
            readonly property string unitSuffix: row.model.suffix.trim()
            font.pixelSize: 15
            from: row.model.minimum === undefined || row.model.minimum === null ? -2147483647 : Number(row.model.minimum)
            to: row.model.maximum === undefined || row.model.maximum === null ? 2147483647 : Number(row.model.maximum)
            stepSize: Number(row.model.step) || 1
            value: Number(row.model.value)
            editable: true
            onValueModified: configurator.properties.setValue(row.model.key, value)
            Accessible.name: row.model.label
            textFromValue: function(value, locale) {
                return value.toLocaleString(locale, "f", 0) + (unitSuffix.length ? " " + unitSuffix : "")
            }
            valueFromText: function(text, locale) {
                let numericText = text.trim()
                if (unitSuffix.length && numericText.endsWith(unitSuffix))
                    numericText = numericText.slice(0, -unitSuffix.length).trim()
                try {
                    const parsed = Number.fromLocaleString(locale, numericText)
                    return Number.isInteger(parsed) ? parsed : integerField.value
                } catch (error) {
                    return integerField.value
                }
            }
            function escapedPattern(text) {
                return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
            }
            validator: RegularExpressionValidator {
                regularExpression: new RegExp("^[+-]?[0-9\\s" + integerField.escapedPattern(integerField.locale.groupSeparator) + "]+(?:\\s*" + integerField.escapedPattern(integerField.unitSuffix) + ")?$")
            }
        }
    }
    Component {
        id: choiceEditor
        Ui.ComboBox {
            model: row.model.choices
            textRole: "label"
            valueRole: "value"
            currentIndex: {
                for (let i = 0; i < row.model.choices.length; ++i)
                    if (String(row.model.choices[i].value) === String(row.model.value)) return i
                return -1
            }
            onActivated: configurator.properties.setValue(row.model.key, currentValue)
            Accessible.name: row.model.label
        }
    }
    Component {
        id: textEditor
        RowLayout {
            Ui.TextField {
                id: field
                readonly property bool configurationEditorPending: text !== String(row.model.value === undefined ? "" : row.model.value)
                objectName: "propertyTextEditor"
                Layout.fillWidth: true
                text: String(row.model.value === undefined ? "" : row.model.value)
                echoMode: row.editorKind === "password" ? TextInput.Password : TextInput.Normal
                selectByMouse: true
                onTextEdited: if (row.editorKind !== "color") configurator.properties.setValue(row.model.key, text)
                onEditingFinished: if (text !== String(row.model.value === undefined ? "" : row.model.value)) configurator.properties.setValue(row.model.key, text)
                Accessible.name: row.model.label
            }
            Ui.ActionButton {
                visible: row.editorKind === "file" || row.editorKind === "directory"
                text: "…"; iconName: "folder-open"
                onClicked: {
                    const path = row.editorKind === "directory" ? configurator.chooseDirectory() : configurator.chooseFile(false)
                    if (path) configurator.properties.setValue(row.model.key, path)
                }
            }
            Rectangle { visible: row.editorKind === "color"; width: 24; height: 24; radius: 3; color: field.text; border.color: theme.divider }
        }
    }
    Component {
        id: listEditor
        TextArea {
            readonly property bool configurationEditorPending: text !== (row.model.value && row.model.value.join ? row.model.value.join("\n") : String(row.model.value || ""))
            text: row.model.value && row.model.value.join ? row.model.value.join("\n") : String(row.model.value || "")
            placeholderText: qsTr("One entry per line")
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            onActiveFocusChanged: if (!activeFocus && text !== (row.model.value && row.model.value.join ? row.model.value.join("\n") : String(row.model.value || ""))) configurator.properties.setValue(row.model.key, text.split("\n").filter(function(line) { return line.trim() !== "" }))
            Accessible.name: row.model.label
        }
    }
}
