import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import "qrc:/quick" as Ui

ApplicationWindow {
    id: window
    width: 1120; height: 780; minimumWidth: 850; minimumHeight: 600
    visible: true
    title: qsTr("Veyon Configurator") + (configurator.dirty ? " *" : "")
    color: theme.background
    font.family: "sans-serif"
    font.pixelSize: 13
    palette.window: theme.background
    palette.windowText: theme.text
    palette.base: theme.surface
    palette.text: theme.text
    palette.placeholderText: theme.muted
    palette.disabled.text: theme.muted
    palette.disabled.windowText: theme.muted
    palette.disabled.buttonText: theme.muted
    palette.button: theme.surface
    palette.buttonText: theme.text
    palette.highlight: theme.accent
    palette.highlightedText: theme.onAccent
    property string page: "overview"
    property var currentPage: configurator.pages[0]
    property bool allowClose: false
    readonly property bool editorPending: !!activeFocusItem && activeFocusItem.configurationEditorPending === true
    property string confirmAction: ""
    readonly property var theme: Ui.Theme
    function selectPage(item) {
        page = item.id; currentPage = item
        configurator.properties.pageId = item.id === "overview" ? "__overview__" : item.id
        contentScroll.contentItem.contentY = 0
    }
    function openSettings(pageId) {
        search.text = ""
        for (const item of configurator.pages) if (item.id === pageId) {
            if (item.advanced) configurator.advanced = true
            window.selectPage(item)
        }
    }
    Component.onCompleted: configurator.properties.pageId = "__overview__"
    onClosing: function(close) { window.contentItem.forceActiveFocus(); if (configurator.dirty && !allowClose) { close.accepted = false; closeDialog.open() } }
    Shortcut { sequences: [StandardKey.Save]; onActivated: { window.contentItem.forceActiveFocus(); configurator.apply(true) } }
    Shortcut { sequences: [StandardKey.Find]; onActivated: { search.forceActiveFocus(); search.selectAll() } }
    Connections { target: configurator; function onRestartRecommended() { restartDialog.open() } }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 236; Layout.fillHeight: true; color: theme.surface
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 12
                Label { text: qsTr("VEYON CONFIGURATOR"); color: theme.muted; font.pixelSize: 11; font.bold: true; Layout.topMargin: 7 }
                Ui.TextField {
                    id: search; Layout.fillWidth: true; placeholderText: qsTr("Search settings  Ctrl+F")
                    onTextChanged: {
                        configurator.properties.search = text
                        configurator.properties.pageId = text.length ? "" : window.page === "overview" ? "__overview__" : window.page
                    }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    Column {
                        width: parent.width; spacing: 2
                        Repeater {
                            model: configurator.pages
                            delegate: Column {
                                required property var modelData
                                required property int index
                                width: parent.width
                                visible: !modelData.advanced || configurator.advanced
                                Label {
                                    visible: index === 0 || configurator.pages[index - 1].group !== modelData.group
                                    text: modelData.group.toUpperCase(); color: theme.muted; font.pixelSize: 10; font.bold: true
                                    topPadding: 18; bottomPadding: 8; leftPadding: 8
                                }
                                ItemDelegate {
                                    width: parent.width; height: 36
                                    highlighted: window.page === modelData.id && !search.text.length
                                    background: Rectangle { color: parent.highlighted ? Qt.alpha(theme.accent, 0.13) : parent.hovered ? Qt.alpha(theme.text, 0.05) : "transparent"; radius: 5 }
                                    contentItem: RowLayout {
                                        spacing: 10
                                        Ui.Icon { name: modelData.icon; color: window.page === modelData.id ? theme.accent : theme.muted; size: 16 }
                                        Label { text: modelData.title; color: window.page === modelData.id ? theme.accent : theme.text; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                                    }
                                    onClicked: { search.text = ""; window.selectPage(modelData) }
                                }
                            }
                        }
                    }
                }
                Ui.CheckBox { text: qsTr("Advanced settings"); checked: configurator.advanced; onToggled: configurator.advanced = checked; font.pixelSize: 12 }
                Label { text: qsTr("Local computer configuration"); color: theme.muted; font.pixelSize: 10 }
            }
        }
        Rectangle { Layout.fillHeight: true; width: 1; color: theme.divider }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            ColumnLayout {
                Layout.fillWidth: true; Layout.margins: 28; Layout.bottomMargin: 12; spacing: 5
                Label { text: search.text.length ? qsTr("Search results") : window.currentPage.title; font.pixelSize: 25; font.bold: true; color: theme.text }
                Label { text: search.text.length ? qsTr("Settings matching “%1”").arg(search.text) : window.currentPage.description || qsTr("Configure %1.").arg(window.currentPage.title); color: theme.muted; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Rectangle { Layout.fillWidth: true; Layout.topMargin: 12; height: 1; color: theme.divider }
            }
            ScrollView {
                id: contentScroll; Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: contentScroll.availableWidth - 56; x: 28; spacing: 18
                    Overview { visible: window.page === "overview" && !search.text.length; Layout.fillWidth: true; onNavigate: function(id) { for (const item of configurator.pages) if (item.id === id) window.selectPage(item) } }
                    Label { visible: configurator.error !== ""; text: configurator.error; color: theme.warning; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        visible: (window.page === "service" || window.page === "logs") && !search.text.length
                        Label { visible: window.page === "service"; text: configurator.serviceRunning ? qsTr("Service is running") : qsTr("Service is stopped"); color: configurator.serviceRunning ? theme.ok : theme.warning; Layout.fillWidth: true }
                        Ui.ActionButton { visible: window.page === "service"; text: configurator.serviceRunning ? qsTr("Restart") : qsTr("Start service"); iconName: "play"; onClicked: configurator.service(configurator.serviceRunning ? "restart" : "start") }
                        Ui.ActionButton { visible: window.page === "service" && configurator.serviceRunning; text: qsTr("Stop"); iconName: "square"; onClicked: configurator.service("stop") }
                        Ui.ActionButton { visible: window.page === "logs"; text: qsTr("Open log folder"); iconName: "folder-open"; onClicked: configurator.openLogDirectory() }
                        Ui.ActionButton { visible: window.page === "logs"; text: qsTr("Clear log files…"); iconName: "trash-2"; onClicked: { window.confirmAction = "logs"; confirmDialog.title = qsTr("Clear log files? The service will be stopped temporarily."); confirmDialog.open() } }
                    }
                    Repeater {
                        id: propertyRows
                        model: configurator.properties
                        delegate: DelegateChooser {
                            role: "editor"
                            DelegateChoice { roleValue: "boolean"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "boolean" } }
                            DelegateChoice { roleValue: "integer"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "integer" } }
                            DelegateChoice { roleValue: "choice"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "choice" } }
                            DelegateChoice { roleValue: "password"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "password" } }
                            DelegateChoice { roleValue: "file"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "file" } }
                            DelegateChoice { roleValue: "directory"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "directory" } }
                            DelegateChoice { roleValue: "color"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "color" } }
                            DelegateChoice { roleValue: "custom"; PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId); editorKind: "custom" } }
                            DelegateChoice { PropertyRow { propertyRepeater: propertyRows; onOpenPage: window.openSettings(pageId);} }
                        }
                    }
                    DirectoryPage { visible: window.page === "computers" && !search.text.length; Layout.fillWidth: true; controller: configurator.plugins.directory || null }
                    KeysPage { visible: window.page === "authentication" && !search.text.length; Layout.fillWidth: true; controller: configurator.plugins.keys || null }
                    RulesPage { visible: window.page === "access" && !search.text.length; Layout.fillWidth: true }
                    PresetsPage { visible: window.page === "applications" && !search.text.length; Layout.fillWidth: true; controller: configurator.plugins.presets || null }
                    LdapPage { visible: window.page === "ldap" && !search.text.length; Layout.fillWidth: true; controller: configurator.plugins.ldap || null }
                    ColumnLayout {
                        visible: window.page === "monitoring" && !search.text.length; Layout.fillWidth: true
                        Label { text: qsTr("Available classroom features"); font.bold: true; color: theme.text }
                        Repeater { model: configurator.features; delegate: Ui.CheckBox { required property var modelData; text: modelData.name; checked: modelData.enabled; onToggled: configurator.setFeatureEnabled(modelData.uid, checked) } }
                    }
                    Item { height: 20 }
                }
            }
            Rectangle { height: 1; color: theme.divider; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true; Layout.margins: 12; spacing: 10
                Ui.Icon { name: configurator.dirty ? "triangle-alert" : "circle-check"; color: configurator.dirty ? theme.warning : theme.ok; size: 15 }
                Label { text: configurator.applyPending ? qsTr("Saved — application incomplete") : configurator.dirty ? qsTr("Unsaved changes") : qsTr("All changes saved"); color: theme.muted; Layout.fillWidth: true; font.pixelSize: 12 }
                Ui.ActionButton { text: qsTr("More"); iconName: "ellipsis"; onClicked: moreMenu.popup(); Menu { id: moreMenu; MenuItem { text: qsTr("Save"); onTriggered: { window.contentItem.forceActiveFocus(); configurator.apply(true) } } MenuSeparator {} MenuItem { text: qsTr("Import configuration…"); onTriggered: configurator.importConfiguration(configurator.chooseFile(false, "JSON (*.json)")) } MenuItem { text: qsTr("Export configuration…"); onTriggered: configurator.exportConfiguration(configurator.chooseFile(true, "JSON (*.json)")) } MenuSeparator {} MenuItem { text: qsTr("Factory reset…"); onTriggered: { window.confirmAction = "reset"; confirmDialog.title = qsTr("Restore factory defaults in this draft?"); confirmDialog.open() } } } }
                Ui.ActionButton { text: qsTr("Revert"); enabled: configurator.dirty || window.editorPending; onClicked: { window.contentItem.forceActiveFocus(); configurator.revert() } }
                Ui.ActionButton { text: qsTr("Apply"); iconName: "check"; primary: true; enabled: configurator.dirty || window.editorPending; onClicked: { window.contentItem.forceActiveFocus(); configurator.apply() } }
            }
        }
    }
    Dialog {
        id: closeDialog; anchors.centerIn: parent; modal: true; title: qsTr("Unsaved settings")
        standardButtons: Dialog.NoButton
        ColumnLayout {
            Label { text: configurator.applyPending ? qsTr("Settings were saved, but system changes are incomplete. Retry Apply before closing?") : qsTr("Apply your changes before closing?"); color: theme.text }
            RowLayout {
                Button { text: qsTr("Cancel"); onClicked: closeDialog.close() }
                Button { text: configurator.applyPending ? qsTr("Close without retry") : qsTr("Discard"); onClicked: { window.allowClose = true; window.close() } }
                Button { text: qsTr("Apply"); onClicked: { window.contentItem.forceActiveFocus(); if (configurator.apply()) { window.allowClose = true; window.close() } else closeDialog.close() } }
            }
        }
    }
    Dialog { id: restartDialog; anchors.centerIn: parent; modal: true; title: qsTr("Restart Veyon service?"); standardButtons: Dialog.Yes | Dialog.No; Label { text: qsTr("The changed service settings require a restart."); color: theme.text } onAccepted: configurator.service("restart") }
    Dialog { id: confirmDialog; anchors.centerIn: parent; modal: true; standardButtons: Dialog.Yes | Dialog.No; onAccepted: { window.contentItem.forceActiveFocus(); if (window.confirmAction === "reset") configurator.factoryReset(); else if (window.confirmAction === "logs") configurator.clearLogs() } }
}
