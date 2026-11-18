import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Veyon 1.0
import "qrc:/quick" as Ui
ApplicationWindow {
    id: root
    objectName: "masterWindow"
    visible: false
    width: 1440; height: 900; minimumWidth: 960; minimumHeight: 640
    title: masterUi.currentRoom + " — Veyon Master"
    color: Ui.Theme.background
    font.family: "sans-serif"
    font.pixelSize: 13
    palette.window: Ui.Theme.background
    palette.windowText: Ui.Theme.text
    palette.base: Ui.Theme.surface
    palette.alternateBase: Ui.Theme.background
    palette.text: Ui.Theme.text
    palette.placeholderText: Ui.Theme.muted
    palette.button: Ui.Theme.surface
    palette.buttonText: Ui.Theme.text
    palette.mid: Ui.Theme.divider
    palette.dark: Ui.Theme.divider
    palette.light: Ui.Theme.surface
    palette.midlight: Ui.Theme.hover
    palette.highlight: Ui.Theme.accent
    palette.highlightedText: Ui.Theme.onAccent
    palette.toolTipBase: Ui.Theme.surface
    palette.toolTipText: Ui.Theme.text
    palette.link: Ui.Theme.accent
    palette.linkVisited: Ui.Theme.muted
    palette.disabled.text: Ui.Theme.muted
    palette.disabled.windowText: Ui.Theme.muted
    palette.disabled.buttonText: Ui.Theme.muted
    property bool listMode: false
    property bool showDetails: true
    property string panel: "computers"
    property var contextFeatures: []
    property bool contextSelectedOnly: false
    property int contextToken: 0
    property string selectedScreenshot: ""
    property string selectedScreenshotName: ""
    property var screenshotSelection: []
    property var screenshotMetadata: ({})
    Connections { target: masterUi; function onChanged() { if (masterUi.customPositions) root.listMode = false } }
    onClosing: function(close) { close.accepted = masterUi.prepareClose() }
    function showPreview(computer, name) {
        preview.computer = computer
        preview.title = name
        preview.visible = true
    }
    function hidePreview() { if (preview.visible) preview.close() }
    function openFeatures(button, selectedOnly) {
        contextFeatures = masterUi.features
        contextSelectedOnly = selectedOnly
        contextToken = masterUi.captureTargets(selectedOnly)
        featureMenu.popup(button, 0, button.height)
    }
    function shareSelectedScreen() {
        const selectedScope = masterUi.selectedScope
        masterUi.selectedScope = false
        const token = masterUi.captureTargets(false)
        masterUi.selectedScope = selectedScope
        masterUi.triggerSnapshot("ShareUserScreenFullScreen", token)
        masterUi.releaseTargets(token)
    }
    function customNeighbour(index, key) {
        const current = customRepeater.itemAt(index)
        if (!current) return index
        let nearest = index
        let distance = Infinity
        for (let row = 0; row < customRepeater.count; ++row) {
            const item = customRepeater.itemAt(row)
            if (!item || row === index) continue
            const dx = item.x - current.x, dy = item.y - current.y
            if ((key === Qt.Key_Left && dx >= 0) || (key === Qt.Key_Right && dx <= 0)
                || (key === Qt.Key_Up && dy >= 0) || (key === Qt.Key_Down && dy <= 0)) continue
            const candidateDistance = dx * dx + dy * dy
            if (candidateDistance < distance) { distance = candidateDistance; nearest = row }
        }
        return nearest
    }
    function navigateComputer(event, index, columns, anchor, custom) {
        const count = masterUi.visibleCount
        if (count === 0) return -1
        let next = Math.max(0, Math.min(count - 1, index))
        if ([Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down].indexOf(event.key) >= 0) {
            if (custom) next = customNeighbour(next, event.key)
            else if (event.key === Qt.Key_Left) next--
            else if (event.key === Qt.Key_Right) next++
            else if (event.key === Qt.Key_Up) next -= columns
            else next += columns
        } else if (event.key === Qt.Key_Home) next = 0
        else if (event.key === Qt.Key_End) next = count - 1
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            masterUi.doubleClick(masterUi.computerIdAt(next)); event.accepted = true; return -1
        } else if (event.key === Qt.Key_Space) {
            masterUi.select(masterUi.computerIdAt(next), true); event.accepted = true; return -1
        } else if (event.key === Qt.Key_Menu || (event.key === Qt.Key_F10 && event.modifiers & Qt.ShiftModifier)) {
            root.openFeatures(anchor, true); event.accepted = true; return -1
        } else return -1
        next = Math.max(0, Math.min(count - 1, next))
        masterUi.select(masterUi.computerIdAt(next), !!(event.modifiers & Qt.ControlModifier), !!(event.modifiers & Qt.ShiftModifier))
        event.accepted = true
        return next
    }
    function focusCustomComputer(index) {
        const item = customRepeater.itemAt(index)
        if (!item) return
        item.forceActiveFocus()
        const x = Math.min(item.x, Math.max(customLayout.contentX, item.x + item.width - customLayout.width))
        const y = Math.min(item.y, Math.max(customLayout.contentY, item.y + item.height - customLayout.height))
        customLayout.contentX = Math.max(0, Math.min(customLayout.contentWidth - customLayout.width, x))
        customLayout.contentY = Math.max(0, Math.min(customLayout.contentHeight - customLayout.height, y))
    }
    Menu {
        id: featureMenu
        width: Math.min(360, root.width - 32)
        onClosed: { let token = root.contextToken; Qt.callLater(function() { masterUi.releaseTargets(token) }) }
        Instantiator {
            model: root.contextFeatures
            delegate: Menu {
                id: featureGroup
                width: Math.min(360, root.width - 32)
                required property var modelData
                title: modelData.title
                MenuItem { text: featureGroup.modelData.title; onTriggered: masterUi.triggerSnapshot(featureGroup.modelData.uid, root.contextToken) }
                MenuSeparator { visible: featureGroup.modelData.children.length > 0 }
                Instantiator {
                    model: featureGroup.modelData.name === "StartApp" || featureGroup.modelData.name === "OpenWebsite" ? [featureGroup.modelData.children] : []
                    delegate: Menu {
                        id: removePresets
                        required property var modelData
                        title: qsTr("Remove saved action")
                        Instantiator {
                            model: removePresets.modelData
                            delegate: MenuItem {
                                required property var modelData
                                text: modelData.title
                                onTriggered: masterUi.removePreset(modelData.uid)
                            }
                            onObjectAdded: function(index, object) { removePresets.insertItem(index, object) }
                            onObjectRemoved: function(index, object) { removePresets.removeItem(object) }
                        }
                    }
                    onObjectAdded: function(index, object) { featureGroup.addMenu(object) }
                    onObjectRemoved: function(index, object) { featureGroup.removeMenu(object) }
                }
                Instantiator {
                    model: featureGroup.modelData.children
                    delegate: MenuItem {
                        required property var modelData
                        text: modelData.title; checkable: true; checked: modelData.checked
                        onTriggered: masterUi.triggerSnapshot(modelData.uid, root.contextToken)
                    }
                    onObjectAdded: function(index, object) { featureGroup.insertItem(index + 2, object) }
                    onObjectRemoved: function(index, object) { featureGroup.removeItem(object) }
                }
            }
            onObjectAdded: function(index, object) { featureMenu.insertMenu(index, object) }
            onObjectRemoved: function(index, object) { featureMenu.removeMenu(object) }
        }
    }
    Shortcut { enabled: masterUi.shortcutContextActive; sequence: "F5"; onActivated: masterUi.reloadComputers() }
    Shortcut { enabled: masterUi.shortcutContextActive; sequence: "F11"; onActivated: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { enabled: masterUi.shortcutContextActive; sequences: [StandardKey.Find]; onActivated: { searchField.forceActiveFocus(); searchField.selectAll() } }
    Instantiator {
        model: masterUi.features
        delegate: Shortcut {
            required property var modelData
            sequence: modelData.shortcut
            enabled: masterUi.shortcutContextActive && modelData.shortcut.length > 0
            onActivated: masterUi.trigger(modelData.uid)
        }
    }
    Shortcut { enabled: masterUi.shortcutContextActive; sequences: [StandardKey.SelectAll]; onActivated: masterUi.selectAll(true) }
    Shortcut { enabled: masterUi.shortcutContextActive; sequence: "Escape"; onActivated: masterUi.selectAll(false) }
    Menu {
        id: workspaceMenu
        MenuItem { text: qsTr("All actions"); onTriggered: root.openFeatures(overflowButton, false) }
        MenuSeparator { }
        MenuItem { text: qsTr("Refresh computers"); onTriggered: masterUi.reloadComputers() }
        MenuItem { text: qsTr("Add room…"); visible: masterUi.canAddRooms; onTriggered: masterUi.addRoom() }
        MenuItem { text: qsTr("Remove room"); visible: masterUi.canAddRooms; onTriggered: masterUi.removeRoom() }
        MenuItem { text: qsTr("Select all"); onTriggered: masterUi.selectAll(true) }
        MenuItem { text: qsTr("Clear selection"); onTriggered: masterUi.selectAll(false) }
        MenuSeparator { }
        MenuItem { text: qsTr("Custom computer positions"); checkable: true; checked: masterUi.customPositions; onTriggered: { masterUi.customPositions = checked; root.listMode = false } }
        MenuItem { text: qsTr("Align computers"); onTriggered: masterUi.alignPositions() }
        MenuItem { text: qsTr("Load positions…"); onTriggered: masterUi.positionsFile(false) }
        MenuItem { text: qsTr("Save positions…"); onTriggered: masterUi.positionsFile(true) }
        MenuItem { text: qsTr("Export computers and users…"); onTriggered: masterUi.exportComputers() }
        MenuSeparator { }
        MenuItem { text: qsTr("Documentation"); onTriggered: masterUi.documentation() }
        MenuItem { text: qsTr("About Veyon"); onTriggered: masterUi.about() }
    }
    Menu {
        id: powerMenu
        width: Math.min(360, root.width - 32)
        property int targetToken: 0
        onAboutToShow: targetToken = masterUi.captureTargets(false)
        onClosed: { let token = targetToken; Qt.callLater(function() { masterUi.releaseTargets(token) }) }
        Instantiator {
            model: {
                let actions = []
                for (const feature of masterUi.features) {
                    if (["PowerOn", "Reboot", "PowerDown"].indexOf(feature.name) < 0) continue
                    actions.push(feature)
                    for (const child of feature.children) actions.push(child)
                }
                return actions
            }
            delegate: MenuItem {
                required property var modelData
                text: modelData.title
                onTriggered: masterUi.triggerSnapshot(modelData.uid, powerMenu.targetToken)
            }
            onObjectAdded: function(index, object) { powerMenu.insertItem(index, object) }
            onObjectRemoved: function(index, object) { powerMenu.removeItem(object) }
        }
    }
    Menu {
        id: filesMenu
        property int targetToken: 0
        onAboutToShow: targetToken = masterUi.captureTargets(false)
        onClosed: { let token = targetToken; Qt.callLater(function() { masterUi.releaseTargets(token) }) }
        MenuItem { text: masterUi.feature("DistributeFiles").title || qsTr("Distribute files"); visible: masterUi.feature("DistributeFiles").uid !== undefined; onTriggered: masterUi.triggerSnapshot("DistributeFiles", filesMenu.targetToken) }
        MenuItem { text: masterUi.feature("FileCollect").title || qsTr("Collect files"); visible: masterUi.feature("FileCollect").uid !== undefined; onTriggered: masterUi.triggerSnapshot("FileCollect", filesMenu.targetToken) }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            objectName: "classroomSidebar"
            Layout.preferredWidth: 216; Layout.minimumWidth: 216; Layout.maximumWidth: 216
            Layout.fillHeight: true; color: Ui.Theme.surface
            Rectangle { anchors.right: parent.right; height: parent.height; width: 1; color: Ui.Theme.divider }
            ColumnLayout {
                anchors.fill: parent; spacing: 0
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 48
                    Ui.TextField {
                        id: searchField; objectName: "computerSearch"
                        anchors.fill: parent; anchors.margins: 10; anchors.bottomMargin: 8
                        placeholderText: qsTr("Computer or student"); selectByMouse: true
                        leftPadding: 29; font.pixelSize: 12
                        onTextChanged: masterUi.setSearch(text)
                        Ui.Icon { x: 8; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 14; color: Ui.Theme.muted }
                    }
                }
                Label { text: qsTr("ROOMS"); color: Ui.Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8; Layout.leftMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 6 }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; model: masterUi.rooms; clip: true; spacing: 0
                    delegate: Rectangle {
                        id: roomRow
                        required property var modelData
                        property bool expanded: masterUi.expandRooms || modelData.searchMatch
                        width: ListView.view.width; height: 54 + (expanded ? hostRows.implicitHeight : 0)
                        color: modelData.current ? Ui.Theme.background : "transparent"
                        Rectangle { width: 3; height: 54; visible: modelData.current; color: Ui.Theme.accent }
                        Column {
                            x: 12 + roomRow.modelData.depth * 8; y: 8
                            width: parent.width - x - 37; spacing: 4
                            Label { text: roomRow.modelData.name; color: Ui.Theme.text; font.bold: true; font.pixelSize: 12; elide: Text.ElideRight; width: parent.width }
                            Label { text: roomRow.modelData.count + " " + qsTr("computers"); color: Ui.Theme.muted; font.pixelSize: 11 }
                        }
                        MouseArea {
                            id: roomMouse
                            objectName: "room-" + modelData.uid
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            height: 54; anchors.rightMargin: 30; hoverEnabled: true
                            onClicked: { Window.window.panel = "computers"; masterUi.activateRoom(modelData.uid) }
                        }
                        Ui.CheckBox {
                            id: includeRoom
                            x: parent.width - width - 4; y: 3; width: 24; height: 24
                            tristate: true; checkState: modelData.checkState
                            onClicked: masterUi.setRoomChecked(modelData.uid, checked)
                            Accessible.name: qsTr("Include this room") + ": " + modelData.name
                            ToolTip.visible: hovered; ToolTip.text: qsTr("Include this room")
                        }
                        ToolButton {
                            x: parent.width - width - 4; y: 28; width: 24; height: 22
                            text: roomRow.expanded ? "−" : "+"; visible: modelData.hosts.length > 0
                            onClicked: roomRow.expanded = !roomRow.expanded
                            Accessible.name: qsTr("Show computers")
                        }
                        Column {
                            id: hostRows; y: 54; x: 20 + modelData.depth * 8; width: parent.width - x; visible: parent.expanded
                            Repeater {
                                model: modelData.hosts
                                Ui.CheckBox { required property var modelData; width: hostRows.width; height: 30; font.pixelSize: 12; text: modelData.name; checked: modelData.checked; onClicked: masterUi.setComputerChecked(modelData.uid, checked) }
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Ui.Theme.divider }
                RowLayout {
                    Layout.fillWidth: true; Layout.margins: 8; spacing: 4
                    Ui.ActionButton { iconName: "monitor"; checked: root.panel === "computers"; Layout.fillWidth: true; Accessible.name: qsTr("Computers"); ToolTip.visible: hovered; ToolTip.text: qsTr("Computers"); onClicked: root.panel = "computers" }
                    Ui.ActionButton { iconName: "camera"; checked: root.panel === "screenshots"; Layout.fillWidth: true; Accessible.name: qsTr("Screenshots"); ToolTip.visible: hovered; ToolTip.text: qsTr("Screenshots"); onClicked: root.panel = "screenshots" }
                    Ui.ActionButton { iconName: "play"; Layout.fillWidth: true; Accessible.name: qsTr("Slideshow"); ToolTip.visible: hovered; ToolTip.text: qsTr("Slideshow"); onClicked: { slideshow.visible = true; masterUi.slideshowRunning = true } }
                    Ui.ActionButton { iconName: "star"; checked: root.panel === "spotlight"; Layout.fillWidth: true; Accessible.name: qsTr("Spotlight"); ToolTip.visible: hovered; ToolTip.text: qsTr("Spotlight"); onClicked: root.panel = "spotlight" }
                }
            }
        }
        ColumnLayout {
            id: workspace
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            Rectangle {
                id: toolbar
                objectName: "classroomToolbar"
                Layout.fillWidth: true; Layout.preferredHeight: 45
                color: Ui.Theme.background
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Ui.Theme.divider }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 3
                    Label { text: qsTr("TO"); color: Ui.Theme.muted; font.pixelSize: 10; font.letterSpacing: 0.8; visible: toolbar.width >= 850; Layout.rightMargin: 5 }
                    RowLayout {
                        spacing: 0
                        Ui.ActionButton { text: qsTr("Whole class") + " · " + masterUi.totalCount; checked: !masterUi.selectedScope; font.pixelSize: 12; leftPadding: 9; rightPadding: 9; onClicked: masterUi.selectedScope = false }
                        Ui.ActionButton { text: qsTr("Selected") + " · " + masterUi.selectedCount; enabled: masterUi.selectedCount > 0; checked: masterUi.selectedScope; font.pixelSize: 12; leftPadding: 9; rightPadding: 9; onClicked: masterUi.selectedScope = true }
                    }
                    Rectangle { width: 1; height: 24; color: Ui.Theme.divider; Layout.leftMargin: 7; Layout.rightMargin: 6 }
                    Ui.ActionButton {
                        id: demoButton
                        text: toolbar.width >= 1000 ? qsTr("Demonstration") : ""
                        iconName: "presentation"; bordered: false; chevron: true
                        visible: masterUi.feature("Demo").uid !== undefined
                        Accessible.name: qsTr("Demonstration"); ToolTip.visible: hovered; ToolTip.text: qsTr("Demonstration")
                        leftPadding: 6; rightPadding: 6
                        onClicked: demoMenu.open()
                        Menu {
                            id: demoMenu; y: parent.height; width: Math.min(340, root.width - 32)
                            property int targetToken: 0
                            onAboutToShow: targetToken = masterUi.captureTargets(false)
                            onClosed: { let token = targetToken; Qt.callLater(function() { masterUi.releaseTargets(token) }) }
                            MenuItem { text: qsTr("My screen · fullscreen"); enabled: masterUi.feature("ShareOwnScreenFullScreen").uid !== undefined; onTriggered: masterUi.triggerSnapshot("ShareOwnScreenFullScreen", demoMenu.targetToken) }
                            MenuItem { text: qsTr("My screen · window"); enabled: masterUi.feature("ShareOwnScreenWindow").uid !== undefined; onTriggered: masterUi.triggerSnapshot("ShareOwnScreenWindow", demoMenu.targetToken) }
                            MenuItem { text: qsTr("Selected student · fullscreen"); enabled: masterUi.selectedCount === 1 && masterUi.feature("ShareUserScreenFullScreen").uid !== undefined; onTriggered: masterUi.triggerSnapshot("ShareUserScreenFullScreen", demoMenu.targetToken) }
                            MenuItem { text: qsTr("Selected student · window"); enabled: masterUi.selectedCount === 1 && masterUi.feature("ShareUserScreenWindow").uid !== undefined; onTriggered: masterUi.triggerSnapshot("ShareUserScreenWindow", demoMenu.targetToken) }
                        }
                    }
                    Ui.ActionButton { text: toolbar.width >= 690 ? qsTr("Lock screens") : ""; iconName: "lock"; bordered: false; visible: masterUi.feature("ScreenLock").uid !== undefined; leftPadding: 6; rightPadding: 6; Accessible.name: qsTr("Lock screens"); ToolTip.visible: hovered; ToolTip.text: qsTr("Lock screens"); onClicked: masterUi.trigger("ScreenLock") }
                    Ui.ActionButton { iconName: "message-square"; bordered: false; visible: masterUi.feature("TextMessage").uid !== undefined; leftPadding: 6; rightPadding: 6; Accessible.name: qsTr("Message"); ToolTip.visible: hovered; ToolTip.text: qsTr("Message"); onClicked: masterUi.trigger("TextMessage") }
                    Ui.ActionButton { iconName: "globe"; bordered: false; visible: toolbar.width >= 650 && masterUi.feature("OpenWebsite").uid !== undefined; leftPadding: 6; rightPadding: 6; Accessible.name: masterUi.feature("OpenWebsite").title || qsTr("Open website"); ToolTip.visible: hovered; ToolTip.text: Accessible.name; onClicked: masterUi.trigger("OpenWebsite") }
                    Ui.ActionButton { iconName: "app-window"; bordered: false; visible: toolbar.width >= 650 && masterUi.feature("StartApp").uid !== undefined; leftPadding: 6; rightPadding: 6; Accessible.name: masterUi.feature("StartApp").title || qsTr("Start application"); ToolTip.visible: hovered; ToolTip.text: Accessible.name; onClicked: masterUi.trigger("StartApp") }
                    Ui.ActionButton { id: filesButton; iconName: "files"; bordered: false; visible: toolbar.width >= 590 && (masterUi.feature("DistributeFiles").uid !== undefined || masterUi.feature("FileCollect").uid !== undefined); leftPadding: 6; rightPadding: 6; Accessible.name: qsTr("File transfer"); ToolTip.visible: hovered; ToolTip.text: qsTr("File transfer"); onClicked: filesMenu.popup(filesButton, 0, height) }
                    Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }
                    Ui.ActionButton { id: powerButton; text: toolbar.width >= 760 ? qsTr("Power") : ""; iconName: "power"; chevron: true; leftPadding: 6; rightPadding: 6; visible: masterUi.feature("PowerDown").uid !== undefined || masterUi.feature("PowerOn").uid !== undefined || masterUi.feature("Reboot").uid !== undefined; Accessible.name: qsTr("Power"); ToolTip.visible: hovered; ToolTip.text: qsTr("Power"); onClicked: powerMenu.popup(powerButton, 0, height) }
                    Ui.ActionButton { id: overflowButton; iconName: "ellipsis"; bordered: false; leftPadding: 5; rightPadding: 5; Accessible.name: qsTr("All actions"); ToolTip.visible: hovered; ToolTip.text: qsTr("All actions"); onClicked: workspaceMenu.popup(overflowButton, 0, height) }
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; visible: root.panel === "computers"
                Rectangle {
                    objectName: "computerListHeader"
                    x: 12; y: 6; width: parent.width-24; height: 30; visible: root.listMode; color: Ui.Theme.surface
                    Label { x: 104; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Computer"); color: Ui.Theme.muted }
                    Label { x: parent.width*0.33; anchors.verticalCenter: parent.verticalCenter; text: qsTr("User"); color: Ui.Theme.muted }
                    Label { x: parent.width*0.51; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Status"); color: Ui.Theme.muted }
                    Label { x: parent.width*0.69; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Address"); color: Ui.Theme.muted }
                }
                GridView {
                    id: monitoring; objectName: "computerMonitoringView"; anchors.fill: parent; anchors.margins: 12; anchors.topMargin: root.listMode ? 42 : 12; clip: true
                    onWidthChanged: if (masterUi.autoFit && !root.listMode) masterUi.fitCards(width,height)
                    onHeightChanged: if (masterUi.autoFit && !root.listMode) masterUi.fitCards(width,height)
                    onCountChanged: if (masterUi.autoFit && !root.listMode) masterUi.fitCards(width,height)
                    model: masterUi.customPositions ? null : masterUi.computers
                    focus: true
                    Keys.onPressed: function(event) {
                        const columns = root.listMode ? 1 : Math.max(1, Math.floor(width / cellWidth))
                        const next = root.navigateComputer(event, currentIndex, columns, monitoring, false)
                        if (next >= 0) {
                            currentIndex = next
                            positionViewAtIndex(next, GridView.Contain)
                        }
                    }
                    cellWidth: root.listMode ? width : Math.max(180, width/Math.max(1,Math.floor(width/(masterUi.cardWidth+16))))
                    cellHeight: root.listMode ? 70 : (cellWidth-12)*9/16+62
                    delegate: computerCard
                    ScrollBar.vertical: ScrollBar { }
                }
                Flickable {
                    id: customLayout
                    anchors.fill: parent; anchors.margins: 12; clip: true; visible: masterUi.customPositions
                    contentWidth: Math.max(width,customItems.childrenRect.width + 30); contentHeight: Math.max(height, customItems.childrenRect.height + 30)
                    Item { id: customItems
                        Repeater { id: customRepeater; model: masterUi.customPositions ? masterUi.computers : null; delegate: computerCard }
                    }
                    ScrollBar.vertical: ScrollBar { }
                    ScrollBar.horizontal: ScrollBar { }
                }
                Label { anchors.centerIn: parent; width: parent.width-40; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; text: masterUi.totalCount === 0 ? qsTr("Choose a room to begin monitoring.") : qsTr("No computers match the current filters."); color: Ui.Theme.muted; visible: monitoring.count === 0 && !masterUi.customPositions }
            }
            Loader { Layout.fillWidth: true; Layout.fillHeight: true; active: root.panel === "screenshots"; visible: active; sourceComponent: screenshotsPanel }
            Loader { Layout.fillWidth: true; Layout.fillHeight: true; active: root.panel === "spotlight"; visible: active; sourceComponent: spotlightPanel }
            Rectangle {
                id: statusBar
                objectName: "classroomStatusBar"
                Layout.fillWidth: true; Layout.preferredHeight: 30
                color: Ui.Theme.surface
                Rectangle { width: parent.width; height: 1; color: Ui.Theme.divider }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 6
                    Ui.Icon { name: "monitor"; size: 12; color: Ui.Theme.muted }
                    Label { text: masterUi.totalCount + " " + qsTr("computers"); color: Ui.Theme.text; font.pixelSize: 11 }
                    Ui.Icon { name: "circle-check"; size: 12; color: Ui.Theme.ok; visible: statusBar.width >= 590 }
                    Label { text: masterUi.onlineCount + " " + qsTr("online"); color: Ui.Theme.text; font.pixelSize: 11; visible: statusBar.width >= 590 }
                    Ui.Icon { name: "power-off"; size: 12; color: Ui.Theme.muted; visible: statusBar.width >= 760 }
                    Label { text: (masterUi.totalCount - masterUi.onlineCount) + " " + qsTr("Offline"); color: Ui.Theme.text; font.pixelSize: 11; visible: statusBar.width >= 760 }
                    Label { text: masterUi.activeSummary; color: Ui.Theme.accent; font.pixelSize: 11; elide: Text.ElideRight; Layout.maximumWidth: Math.max(60, statusBar.width * 0.2); visible: masterUi.activeSummary.length > 0 }
                    Ui.ActionButton { text: qsTr("Stop"); iconName: "square"; implicitHeight: 24; bordered: false; leftPadding: 4; rightPadding: 4; visible: masterUi.activeSummary.length > 0; onClicked: masterUi.stopFeatures() }
                    Item { Layout.fillWidth: true }
                    RowLayout {
                        visible: root.panel === "computers"; spacing: 2
                        Ui.ActionButton { iconName: "grid-2x2"; checked: !root.listMode; implicitHeight: 24; leftPadding: 6; rightPadding: 6; foreground: checked ? Ui.Theme.background : Ui.Theme.text; background: Rectangle { color: parent.checked ? Ui.Theme.text : "transparent"; border.width: 1; border.color: Ui.Theme.divider } Accessible.name: qsTr("Grid"); ToolTip.visible: hovered; ToolTip.text: qsTr("Grid"); onClicked: root.listMode = false }
                        Ui.ActionButton { iconName: "list"; checked: root.listMode; enabled: !masterUi.customPositions; implicitHeight: 24; leftPadding: 6; rightPadding: 6; foreground: checked ? Ui.Theme.background : Ui.Theme.text; background: Rectangle { color: parent.checked ? Ui.Theme.text : "transparent"; border.width: 1; border.color: Ui.Theme.divider } Accessible.name: qsTr("List"); ToolTip.visible: hovered; ToolTip.text: qsTr("List"); onClicked: root.listMode = true }
                        Ui.ActionButton { iconName: "scan"; checked: masterUi.autoFit; visible: !root.listMode; implicitHeight: 24; leftPadding: 5; rightPadding: 5; bordered: false; Accessible.name: qsTr("Fit all"); ToolTip.visible: hovered; ToolTip.text: qsTr("Fit all"); onClicked: { masterUi.autoFit = !masterUi.autoFit; masterUi.fitCards(monitoring.width, monitoring.height) } }
                        Ui.ActionButton { iconName: "circle-minus"; enabled: !masterUi.autoFit; visible: !root.listMode && statusBar.width >= 650; implicitHeight: 24; leftPadding: 3; rightPadding: 3; bordered: false; Accessible.name: qsTr("Zoom out"); onClicked: masterUi.cardWidth = Math.max(150, masterUi.cardWidth - 25) }
                        Slider { implicitWidth: statusBar.width >= 760 ? 90 : 60; implicitHeight: 24; enabled: !masterUi.autoFit; from: 150; to: 400; value: masterUi.cardWidth; visible: !root.listMode; onMoved: masterUi.cardWidth = value; Accessible.name: qsTr("Thumbnail size") }
                        Ui.ActionButton { iconName: "circle-plus"; enabled: !masterUi.autoFit; visible: !root.listMode && statusBar.width >= 650; implicitHeight: 24; leftPadding: 3; rightPadding: 3; bordered: false; Accessible.name: qsTr("Zoom in"); onClicked: masterUi.cardWidth = Math.min(400, masterUi.cardWidth + 25) }
                        Ui.ActionButton {
                            id: filterButton; iconName: "list-filter"; checked: masterUi.filterIndex !== 0
                            implicitHeight: 24; leftPadding: 5; rightPadding: 5; bordered: false
                            Accessible.name: qsTr("Filter computers"); ToolTip.visible: hovered; ToolTip.text: qsTr("Filter computers")
                            onClicked: filterMenu.popup(filterButton, 0, -filterMenu.height)
                            Menu {
                                id: filterMenu
                                Instantiator {
                                    model: [qsTr("All computers"), qsTr("Online"), qsTr("Logged-in users"), qsTr("Online logged-in users")]
                                    delegate: MenuItem { required property string modelData; required property int index; text: modelData; checkable: true; checked: masterUi.filterIndex === index; onTriggered: masterUi.setFilter(index) }
                                    onObjectAdded: function(index, object) { filterMenu.insertItem(index, object) }
                                    onObjectRemoved: function(index, object) { filterMenu.removeItem(object) }
                                }
                            }
                        }
                        Ui.ActionButton { iconName: "panel-right"; checked: root.showDetails; implicitHeight: 24; leftPadding: 5; rightPadding: 5; bordered: false; Accessible.name: qsTr("Computer details"); ToolTip.visible: hovered; ToolTip.text: qsTr("Computer details"); onClicked: root.showDetails = !root.showDetails }
                    }
                }
            }
        }
        Rectangle {
            objectName: "computerDetails"
            Layout.preferredWidth: 264; Layout.minimumWidth: 264; Layout.maximumWidth: 264
            Layout.fillHeight: true; visible: root.panel === "computers" && root.showDetails && masterUi.selectedCount > 0
            color: Ui.Theme.surface
            Rectangle { width: 1; height: parent.height; color: Ui.Theme.divider }
            ColumnLayout {
                anchors.fill: parent; spacing: 0
                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 44; Layout.leftMargin: 13; Layout.rightMargin: 6
                    Label { text: masterUi.details.name || ""; color: Ui.Theme.text; font.bold: true; font.pixelSize: 17; Layout.fillWidth: true; elide: Text.ElideRight }
                    Ui.ActionButton { iconName: "x"; implicitHeight: 26; leftPadding: 5; rightPadding: 5; bordered: false; Accessible.name: qsTr("Close"); onClicked: root.showDetails = false }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width; spacing: 0
                        Rectangle {
                            Layout.fillWidth: true; Layout.leftMargin: 13; Layout.rightMargin: 13
                            Layout.preferredHeight: width * 9 / 16
                            color: Ui.Theme.background; border.color: Ui.Theme.divider
                            Framebuffer { id: detailsFrame; anchors.fill: parent; anchors.margins: 1; frame: masterUi.details.thumbnail; visible: !!masterUi.details.online && sourceSize.width > 0 }
                            ColumnLayout { anchors.centerIn: parent; visible: !detailsFrame.visible; spacing: 8; Ui.Icon { name: masterUi.details.online ? "monitor" : "power-off"; size: 28; color: Ui.Theme.muted; Layout.alignment: Qt.AlignHCenter } Label { text: masterUi.details.online ? qsTr("Waiting for screen") : qsTr("Offline"); color: Ui.Theme.muted; font.pixelSize: 11 } }
                        }
                        RowLayout {
                            Layout.leftMargin: 13; Layout.rightMargin: 13; Layout.topMargin: 11; spacing: 6
                            Rectangle { width: 7; height: 7; radius: 4; color: masterUi.details.online ? Ui.Theme.ok : Ui.Theme.muted }
                            Label { text: masterUi.details.online ? qsTr("Online") : qsTr("Offline"); color: masterUi.details.online ? Ui.Theme.ok : Ui.Theme.muted; font.bold: true; font.pixelSize: 12 }
                        }
                        GridLayout {
                            columns: 2; columnSpacing: 10; rowSpacing: 7
                            Layout.fillWidth: true; Layout.margins: 13; Layout.topMargin: 14
                            Label { text: qsTr("User"); color: Ui.Theme.muted; font.pixelSize: 12; Layout.preferredWidth: 78 }
                            Label { text: masterUi.details.user || qsTr("No user logged in"); color: Ui.Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            Label { text: qsTr("Address"); color: Ui.Theme.muted; font.pixelSize: 12 }
                            Label { text: masterUi.details.address || "—"; color: Ui.Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
                            Label { text: qsTr("Session duration"); color: Ui.Theme.muted; font.pixelSize: 12; visible: !!masterUi.details.sessionDuration }
                            Label { text: masterUi.details.sessionDuration || ""; color: Ui.Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap; visible: !!masterUi.details.sessionDuration }
                            Label { text: qsTr("Location"); color: Ui.Theme.muted; font.pixelSize: 12; visible: !!masterUi.details.location }
                            Label { text: masterUi.details.location || ""; color: Ui.Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap; visible: !!masterUi.details.location }
                        }
                        Label { text: masterUi.details.error || ""; color: Ui.Theme.warning; visible: text.length > 0; Layout.fillWidth: true; Layout.leftMargin: 13; Layout.rightMargin: 13; Layout.bottomMargin: 10; wrapMode: Text.WordWrap; font.pixelSize: 12 }
                        Rectangle { Layout.fillWidth: true; height: 1; color: Ui.Theme.divider }
                        RowLayout {
                            Layout.fillWidth: true; Layout.margins: 13; spacing: 8
                            Ui.ActionButton { text: qsTr("View"); iconName: "eye"; primary: true; Layout.fillWidth: true; leftPadding: 7; rightPadding: 7; visible: masterUi.feature("RemoteView").uid !== undefined; onClicked: masterUi.trigger("RemoteView", true) }
                            Ui.ActionButton { text: qsTr("Control"); iconName: "mouse-pointer-2"; Layout.fillWidth: true; leftPadding: 7; rightPadding: 7; visible: masterUi.feature("RemoteControl").uid !== undefined; onClicked: masterUi.trigger("RemoteControl", true) }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; spacing: 1
                            Ui.ActionButton { text: qsTr("Message…"); iconName: "message-square"; bordered: false; Layout.fillWidth: true; visible: masterUi.feature("TextMessage").uid !== undefined; onClicked: masterUi.trigger("TextMessage", true) }
                            Ui.ActionButton { text: qsTr("Lock screens"); iconName: "lock"; bordered: false; Layout.fillWidth: true; visible: masterUi.feature("ScreenLock").uid !== undefined; onClicked: masterUi.trigger("ScreenLock", true) }
                            Ui.ActionButton { text: qsTr("Show this screen to everyone"); iconName: "screen-share"; bordered: false; Layout.fillWidth: true; visible: masterUi.feature("ShareUserScreenFullScreen").uid !== undefined; enabled: masterUi.selectedCount === 1; font.pixelSize: 12; onClicked: root.shareSelectedScreen() }
                            Ui.ActionButton { text: qsTr("Reboot…"); iconName: "rotate-ccw"; bordered: false; Layout.fillWidth: true; visible: masterUi.feature("Reboot").uid !== undefined; onClicked: masterUi.trigger("Reboot", true) }
                            Ui.ActionButton { text: qsTr("Power down…"); iconName: "power-off"; bordered: false; danger: true; Layout.fillWidth: true; visible: masterUi.feature("PowerDown").uid !== undefined; onClicked: masterUi.trigger("PowerDown", true) }
                            Ui.ActionButton { text: qsTr("Add to spotlight"); iconName: "star"; bordered: false; Layout.fillWidth: true; onClicked: masterUi.addSpotlight() }
                        }
                    }
                }
                Label { text: qsTr("%1 selected").arg(masterUi.selectedCount); color: Ui.Theme.muted; font.pixelSize: 11; Layout.leftMargin: 13; Layout.bottomMargin: 9; visible: masterUi.selectedCount > 1 }
            }
        }
    }
    Component {
        id: computerCard
        Rectangle {
            id: card
            objectName: "computerCard"
            readonly property var appWindow: card.Window.window
            readonly property var gridView: card.GridView.view
            readonly property bool listMode: appWindow ? appWindow.listMode : false
            required property int index
            required property string computerUid
            required property string computerName
            required property string userName
            required property string address
            required property bool online
            required property string stateText
            required property bool selected
            required property var thumbnail
            required property var computerObject
            required property int positionX
            required property int positionY
            Keys.onPressed: function(event) {
                if (!masterUi.customPositions || !card.appWindow) return
                const next = card.appWindow.navigateComputer(event, card.index, 1, card, true)
                if (next >= 0) card.appWindow.focusCustomComputer(next)
            }
            Binding { target: card; property: "x"; value: card.positionX; when: masterUi.customPositions }
            Binding { target: card; property: "y"; value: card.positionY; when: masterUi.customPositions }
            width: gridView ? gridView.cellWidth-12 : masterUi.cardWidth
            height: card.listMode ? 62 : width*9/16+50
            color: selected ? Ui.Theme.background : Ui.Theme.surface
            border.color: selected ? Ui.Theme.accent : Ui.Theme.divider
            border.width: selected ? 2 : 1
            radius: 0
            Rectangle {
                id: thumbnailArea
                x: card.listMode ? 5 : 1; y: card.listMode ? 5 : 1
                width: card.listMode ? 85 : card.width-2
                height: card.listMode ? 50 : width*9/16
                color: Ui.Theme.background
                Framebuffer {
                    id: cardFrame
                    anchors.fill: parent
                    frame: card.thumbnail
                    visible: card.online && sourceSize.width > 0
                }
                Column {
                    anchors.centerIn: parent; spacing: 6
                    visible: !cardFrame.visible
                    Ui.Icon { anchors.horizontalCenter: parent.horizontalCenter; name: card.online ? "monitor" : "power-off"; size: card.listMode ? 20 : 28; color: Ui.Theme.muted }
                    Label { visible: !card.listMode; text: card.online ? qsTr("Waiting for screen") : card.stateText; color: Ui.Theme.muted; font.pixelSize: 11; width: Math.min(implicitWidth, thumbnailArea.width-20); elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter }
                }
            }
            Column {
                x: card.listMode ? 104 : 10
                y: card.listMode ? 12 : thumbnailArea.height+10
                width: card.listMode ? Math.max(48,card.width*0.33-112) : card.width-20
                spacing: 5
                RowLayout {
                    width: parent.width; spacing: 6
                    Label { text: card.computerName; color: Ui.Theme.text; font.bold: true; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                    Rectangle { visible: !card.listMode; width: 6; height: 6; radius: 3; color: card.online ? Ui.Theme.ok : Ui.Theme.muted }
                    Label { visible: !card.listMode; text: card.stateText; color: card.online ? Ui.Theme.ok : Ui.Theme.muted; font.pixelSize: 10; Layout.maximumWidth: card.width*0.4; elide: Text.ElideRight }
                }
                Label { visible: !card.listMode; text: card.userName || "—"; color: Ui.Theme.muted; font.pixelSize: 11; width: parent.width; elide: Text.ElideRight }
            }
            Label { visible: card.listMode; anchors.verticalCenter: parent.verticalCenter; x: parent.width*0.33; width: parent.width*0.17; text: card.userName || qsTr("No user"); color: Ui.Theme.text; elide: Text.ElideRight }
            Label { visible: card.listMode; anchors.verticalCenter: parent.verticalCenter; x: parent.width*0.51; width: parent.width*0.17; text: card.stateText; color: card.online ? Ui.Theme.ok : Ui.Theme.muted; elide: Text.ElideRight }
            Label { visible: card.listMode; anchors.verticalCenter: parent.verticalCenter; x: parent.width*0.69; width: parent.width*0.29; text: card.address; color: Ui.Theme.muted; elide: Text.ElideRight }
            MouseArea {
                id: cardMouse; anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton; hoverEnabled: true
                drag.target: masterUi.customPositions ? card : null
                drag.minimumX: 0; drag.minimumY: 0
                onClicked: function(mouse) {
                    if (card.gridView) {
                        card.gridView.currentIndex = card.index
                        card.gridView.forceActiveFocus()
                    } else card.forceActiveFocus()
                    if (mouse.button === Qt.RightButton) { if (!card.selected) masterUi.select(card.computerUid); card.appWindow.openFeatures(card, true) }
                    else masterUi.select(card.computerUid, !!(mouse.modifiers & Qt.ControlModifier), !!(mouse.modifiers & Qt.ShiftModifier))
                }
                onDoubleClicked: masterUi.doubleClick(card.computerUid)
                onPressAndHold: if (!masterUi.customPositions && card.online) card.appWindow.showPreview(card.computerObject, card.computerName)
                onCanceled: card.appWindow.hidePreview()
                onReleased: { card.appWindow.hidePreview(); if (drag.active) masterUi.setPosition(card.computerUid,card.x,card.y) }
            }
            Ui.CheckBox {
                id: cardSelection
                x: 6; y: 6; width: 18; height: 18; padding: 0
                checked: card.selected
                Accessible.name: qsTr("Select %1").arg(card.computerName)
                onClicked: {
                    if (card.gridView) {
                        card.gridView.currentIndex = card.index
                        card.gridView.forceActiveFocus()
                    } else card.forceActiveFocus()
                    masterUi.select(card.computerUid, true)
                }
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.color: Ui.Theme.accent; border.width: 2; visible: card.selected }
            ToolTip.visible: cardMouse.containsMouse
            ToolTip.delay: 900
            ToolTip.text: card.computerName + "\n" + card.userName + "\n" + card.stateText + "\n" + card.address
        }
    }
    Window {
        id: preview; flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus; width: 900; height: 600; color: "black"; transientParent: root
        property var computer: null
        Framebuffer { anchors.fill: parent; computer: preview.visible ? preview.computer : null; live: true; viewOnly: true }
    }
    ApplicationWindow {
        palette: root.palette
        font: root.font
        id: slideshow; width: 1024; height: 740; title: qsTr("Veyon — Slideshow"); color: Ui.Theme.background
        onClosing: masterUi.slideshowRunning = false
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12
            Framebuffer { Layout.fillWidth: true; Layout.fillHeight: true; computer: slideshow.visible ? masterUi.slideshowComputer : null; live: true }
            RowLayout {
                Layout.fillWidth: true
                Ui.ActionButton { text: qsTr("Previous"); iconName: "chevron-left"; onClicked: masterUi.nextSlide(-1) }
                Ui.ActionButton { text: masterUi.slideshowRunning ? qsTr("Pause") : qsTr("Play"); iconName: masterUi.slideshowRunning ? "pause" : "play"; onClicked: masterUi.slideshowRunning = !masterUi.slideshowRunning }
                Ui.ActionButton { text: qsTr("Next"); iconName: "chevron-right"; onClicked: masterUi.nextSlide(1) }
                Slider { from: 1000; to: 30000; stepSize: 1000; value: masterUi.slideshowDuration; Layout.fillWidth: true; onMoved: masterUi.slideshowDuration = value }
                Label { text: Math.round(masterUi.slideshowDuration/1000) + " s"; color: Ui.Theme.text }
                Ui.ActionButton { text: qsTr("Fullscreen"); iconName: "maximize"; onClicked: slideshow.visibility = slideshow.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
            }
        }
    }
    Component {
        id: screenshotsPanel
        RowLayout {
            spacing: 12
            ListView {
                Layout.preferredWidth: 260; Layout.fillHeight: true; clip: true; model: masterUi.screenshots
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width; height: 64
                    contentItem: RowLayout {
                        Ui.CheckBox {
                            checked: root.screenshotSelection.indexOf(modelData.name) >= 0
                            onClicked: {
                                let selection = root.screenshotSelection.slice()
                                let position = selection.indexOf(modelData.name)
                                if (checked && position < 0) selection.push(modelData.name)
                                else if (!checked && position >= 0) selection.splice(position,1)
                                root.screenshotSelection = selection
                            }
                        }
                        Label { text: modelData.host + "\n" + modelData.user + "\n" + modelData.date; color: Ui.Theme.text; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    onClicked: { root.selectedScreenshot = modelData.url; root.selectedScreenshotName = modelData.name; root.screenshotMetadata = modelData }
                }
                ScrollBar.vertical: ScrollBar { }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true
                Image { Layout.fillWidth: true; Layout.fillHeight: true; source: root.selectedScreenshot; fillMode: Image.PreserveAspectFit }
                Label { text: (root.screenshotMetadata.user || "") + " · " + (root.screenshotMetadata.host || "") + " · " + (root.screenshotMetadata.date || ""); color: Ui.Theme.muted; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                RowLayout {
                    Ui.ActionButton { text: qsTr("Open"); iconName: "maximize"; enabled: root.selectedScreenshot.length > 0; onClicked: screenshotViewer.createObject(root, {imageSource: root.selectedScreenshot, title: root.selectedScreenshotName}) }
                    Ui.ActionButton { text: qsTr("Delete") + (root.screenshotSelection.length ? " · " + root.screenshotSelection.length : ""); iconName: "trash-2"; danger: true; enabled: root.screenshotSelection.length > 0 || root.selectedScreenshotName.length > 0; onClicked: { masterUi.deleteScreenshots(root.screenshotSelection.length ? root.screenshotSelection : [root.selectedScreenshotName]); root.selectedScreenshot = ""; root.selectedScreenshotName = ""; root.screenshotSelection = []; root.screenshotMetadata = ({}) } }
                }
            }
        }
    }
    Component {
        id: screenshotViewer
        Window {
            id: viewer
            required property url imageSource
            width: 1000; height: 700; visible: true; color: Ui.Theme.background
            onClosing: Qt.callLater(function() { viewer.destroy() })
            Image { anchors.fill: parent; source: viewer.imageSource; fillMode: Image.PreserveAspectFit }
        }
    }
    Component {
        id: spotlightPanel
        ColumnLayout {
            RowLayout {
                Ui.ActionButton { text: qsTr("Add selected computers"); iconName: "plus"; onClicked: masterUi.addSpotlight() }
                Ui.CheckBox { text: qsTr("Realtime"); checked: masterUi.spotlightRealtime; onClicked: masterUi.spotlightRealtime = checked }
            }
            GridView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: masterUi.spotlight
                cellWidth: width/Math.max(1,Math.floor(width/360)); cellHeight: cellWidth*9/16+55
                delegate: ColumnLayout {
                    required property var modelData
                    required property int index
                    width: GridView.view.cellWidth-12; height: GridView.view.cellHeight-12
                    Framebuffer { Layout.fillWidth: true; Layout.fillHeight: true; computer: modelData.computer; live: masterUi.spotlightRealtime }
                    RowLayout { Label { text: modelData.name; color: Ui.Theme.text; Layout.fillWidth: true } Ui.ActionButton { iconName: "x"; text: qsTr("Remove"); onClicked: masterUi.removeSpotlight(index) } }
                }
                ScrollBar.vertical: ScrollBar { }
            }
        }
    }
}
