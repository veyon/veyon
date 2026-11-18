import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs as Dialogs
import "qrc:/quick" as Ui

ApplicationWindow {
	id: window
	readonly property string kind: dialogRequest.kind
	readonly property var request: dialogRequest.data
	readonly property bool credentials: ["password", "authenticate", "login"].indexOf(kind) >= 0
	readonly property bool filePicker: ["file", "files", "folder"].indexOf(kind) >= 0
	readonly property bool notification: kind === "notification"
	readonly property bool editor: ["input", "website", "application", "message"].indexOf(kind) >= 0
	property var values: ({})
	property int remaining: Number(request.seconds || 0)
	width: notification ? 380 : Math.min(Screen.width - 80, Math.max(500, Number(request.width || 0), dialogButtons.implicitWidth + 40))
	height: notification ? 220 : Math.min(Screen.height - 80, Math.max(260, content.implicitHeight + 40))
	visible: !filePicker
	color: Theme.background
	title: request.title || qsTr("Veyon")
	font.family: "sans-serif"
	font.pixelSize: 13
	flags: notification ? Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint :
		(kind === "countdown" || kind === "busy") ? Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowStaysOnTopHint :
		Qt.Dialog | ((request.stayOnTop || kind === "access") ? Qt.WindowStaysOnTopHint : 0)
	onClosing: close => {
		if (kind === "busy" || (kind === "countdown" && remaining > 0)) close.accepted = false
		else dialogRequest.reject()
	}
	palette.window: Theme.background
	palette.windowText: Theme.text
	palette.base: Theme.surface
	palette.text: Theme.text
	palette.placeholderText: Theme.muted
	palette.disabled.text: Theme.muted
	palette.disabled.windowText: Theme.muted
	palette.disabled.buttonText: Theme.muted
	palette.button: Theme.surface
	palette.buttonText: Theme.text
	palette.highlight: Theme.accent
	palette.highlightedText: Theme.onAccent
	Component.onCompleted: {
		if (notification) {
			x = Screen.virtualX + Screen.width - width - 24
			y = Screen.virtualY + Screen.height - height - 56
		}
		if (request.fields) for (let field of request.fields) values[field.key] = field.value === undefined ? "" : field.value
		if (credentials) Qt.callLater(function() { (username.text.length ? password : username).forceActiveFocus() })
		else if (editor) Qt.callLater(entry.forceActiveFocus)
		if (kind === "folder") Qt.callLater(folderDialog.open)
		else if (filePicker) Qt.callLater(fileDialog.open)
		if (kind === "countdown" && remaining <= 0) Qt.callLater(() => dialogRequest.accept())
	}
	Connections { target: dialogRequest; function onCompleted() { window.close() } }
	function submit() {
		let result = {value: entry.text, username: username.text, password: password.text,
			title: messageTitle.text, remember: remember.checked, presetName: presetName.text,
			seconds: minutes.value * 60 + seconds.value, richText: kind === "message"}
		for (let key in values) result[key] = values[key]
		if (request.choices && request.choices.length) result.value = choice.currentText
		dialogRequest.accept(result)
	}
	Shortcut { sequence: "Escape"; enabled: kind !== "busy" && kind !== "countdown"; onActivated: dialogRequest.reject() }
	Timer {
		interval: 1000; repeat: true; running: kind === "countdown" && remaining > 0
		onTriggered: { remaining--; if (remaining <= 0) dialogRequest.accept() }
	}
	Rectangle { anchors.fill: parent; color: "transparent"; border.width: notification ? 2 : 0; border.color: Theme.text }
	ColumnLayout {
		id: content
		anchors.fill: parent
		anchors.margins: notification ? 14 : 20
		spacing: 12
		RowLayout {
			Layout.fillWidth: true
			Icon { name: request.iconName || (notification ? "message-square-text" : kind === "error" ? "circle-alert" : ""); visible: name.length > 0; size: notification ? 18 : 22; color: kind === "error" ? Theme.accent : Theme.text; Layout.alignment: Qt.AlignVCenter }
			Label { text: window.title; font.pixelSize: notification ? 15 : 18; font.bold: true; color: Theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
		}
		BusyIndicator { visible: kind === "busy"; running: visible; Layout.alignment: Qt.AlignHCenter }
		Label { visible: kind === "countdown"; text: qsTr("The computer will power down in %1 minutes, %2 seconds. Save your work.").arg(Math.floor(remaining / 60)).arg(remaining % 60); wrapMode: Text.WordWrap; color: Theme.text; Layout.fillWidth: true }
		Label { visible: !!request.summary; text: request.summary || ""; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
		ScrollView {
			id: bodyScroll
			visible: !!request.text
			Layout.fillWidth: true
			Layout.fillHeight: notification || kind === "about"
			Layout.preferredHeight: Math.min(body.implicitHeight, kind === "about" ? 400 : 240)
			Layout.minimumHeight: 32
			clip: true
			TextArea {
				id: body
				objectName: "dialogBody"
				width: bodyScroll.availableWidth
				text: request.text || ""
				textFormat: request.richText ? TextEdit.RichText : TextEdit.PlainText
				color: Theme.text
				wrapMode: TextEdit.Wrap
				readOnly: true
				selectByMouse: true
				background: null
				onLinkActivated: link => veyonUi.openUrl(link)
			}
		}
		Label { visible: credentials; text: qsTr("Username"); color: Theme.muted }
		Ui.TextField { id: username; objectName: "dialogUsername"; visible: credentials; text: request.username || ""; Layout.fillWidth: true; selectByMouse: true; onAccepted: password.forceActiveFocus() }
		Label { visible: credentials; text: qsTr("Password"); color: Theme.muted }
		Ui.TextField { id: password; objectName: "dialogPassword"; visible: credentials; text: request.password || ""; echoMode: TextInput.Password; Layout.fillWidth: true; selectByMouse: true; onAccepted: { if (accept.enabled) window.submit() } }
		Label { visible: editor; text: request.label || (kind === "website" ? qsTr("Website address") : kind === "application" ? qsTr("Application or command") : kind === "message" ? qsTr("Message text") : ""); color: Theme.text }
		TextArea {
			id: entry
				objectName: "dialogEntry"
			visible: window.editor && (!window.request.choices || !window.request.choices.length)
			Layout.fillWidth: true
			Layout.preferredHeight: kind === "application" || kind === "message" ? 100 : 30
			text: request.value || ""
			textFormat: kind === "message" ? TextEdit.RichText : TextEdit.PlainText
			background: Rectangle { color: Theme.surface; border.color: entry.activeFocus ? Theme.accent : Theme.divider; border.width: 1 }
			placeholderText: request.placeholder || (kind === "website" ? "https://" : "")
			wrapMode: TextEdit.Wrap
			selectByMouse: true
		}
		Ui.ComboBox { id: choice; objectName: "dialogChoice"; visible: editor && !!request.choices; model: request.choices || []; Layout.fillWidth: true }
		Ui.CheckBox { id: remember; objectName: "dialogRemember"; visible: kind === "website" || kind === "application"; text: qsTr("Remember and add to the menu"); checked: !!request.remember }
		Ui.TextField { id: presetName; objectName: "dialogPresetName"; visible: remember.visible && remember.checked; text: request.presetName || ""; placeholderText: qsTr("Name"); Layout.fillWidth: true }
		Ui.TextField { id: messageTitle; objectName: "dialogMessageTitle"; visible: kind === "message"; text: request.messageTitle || ""; placeholderText: qsTr("Message title"); Layout.fillWidth: true }
		RowLayout {
			visible: kind === "schedule"
			Label { text: qsTr("Minutes"); color: Theme.text }
			Ui.SpinBox { id: minutes; objectName: "dialogMinutes"; from: 0; to: 100000; editable: true }
			Label { text: qsTr("Seconds"); color: Theme.text }
			Ui.SpinBox { id: seconds; objectName: "dialogSeconds"; from: 0; to: 59; editable: true }
		}
		Repeater {
			model: request.fields || []
			delegate: ColumnLayout {
				required property var modelData
				Layout.fillWidth: true
				Label { text: modelData.label || modelData.key; color: Theme.muted }
				Ui.TextField {
					objectName: "dialogField_" + modelData.key
					Layout.fillWidth: true
					text: modelData.value === undefined ? "" : String(modelData.value)
					echoMode: modelData.secret ? TextInput.Password : TextInput.Normal
					selectByMouse: true
					onTextEdited: window.values[modelData.key] = text
				}
			}
		}
		Label { visible: dialogRequest.errorString.length > 0; text: dialogRequest.errorString; color: Theme.accent; wrapMode: Text.WordWrap; Layout.fillWidth: true }
		Item { Layout.fillHeight: true; Layout.minimumHeight: 0 }
		Rectangle { color: Theme.divider; Layout.fillWidth: true; implicitHeight: 1 }
		RowLayout {
			id: dialogButtons
			Layout.fillWidth: true
			Item { Layout.fillWidth: true }
			Repeater {
				model: request.buttons || []
				delegate: ActionButton {
					required property var modelData
					objectName: "dialogChoice_" + modelData.value
					text: modelData.text; primary: !!modelData.primary; danger: !!modelData.danger
					onClicked: dialogRequest.accept({choice: modelData.value})
				}
			}
			ActionButton { objectName: "dialogCancel"; visible: !request.buttons && !notification && ["error", "about", "busy", "countdown"].indexOf(kind) < 0; text: qsTr("Cancel"); onClicked: dialogRequest.reject() }
			ActionButton {
				id: accept
				objectName: "dialogAccept"
				visible: !request.buttons && kind !== "busy" && kind !== "countdown"
				text: request.acceptText || (notification ? qsTr("Got it") : kind === "website" ? qsTr("Open") : kind === "application" ? qsTr("Run") : kind === "message" ? qsTr("Send") : qsTr("OK"))
				primary: true
				enabled: (!credentials || (username.text.length > 0 && (request.allowEmptyPassword || password.text.length > 0))) &&
					(!editor || (request.choices ? choice.currentIndex >= 0 : entry.text.length > 0 && entry.getText(0, entry.length).trim().length > 0)) &&
					(!remember.visible || !remember.checked || presetName.text.trim().length > 0) &&
					(kind !== "schedule" || minutes.value * 60 + seconds.value > 0)
				onClicked: window.submit()
			}
		}
	}
	Dialogs.FileDialog {
		id: fileDialog
		title: window.title
		fileMode: window.kind === "files" ? Dialogs.FileDialog.OpenFiles : window.request.save ? Dialogs.FileDialog.SaveFile : Dialogs.FileDialog.OpenFile
		nameFilters: window.request.filters || [qsTr("All files (*)")]
		currentFolder: window.request.folder ? veyonUi.fileUrl(String(window.request.folder)) : ""
		onAccepted: {
			let files = selectedFiles.map(url => veyonUi.localPath(url))
			dialogRequest.accept({files: files, path: files.length ? files[0] : "", value: files.length ? files[0] : ""})
		}
		onRejected: dialogRequest.reject()
	}
	Dialogs.FolderDialog {
		id: folderDialog
		title: window.title
		currentFolder: window.request.folder ? veyonUi.fileUrl(String(window.request.folder)) : ""
		onAccepted: { let path = veyonUi.localPath(selectedFolder); dialogRequest.accept({path: path, value: path}) }
		onRejected: dialogRequest.reject()
	}
}
