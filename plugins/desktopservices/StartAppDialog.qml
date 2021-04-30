import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0

Dialog {
	title: qsTr("Start application")
	x: (parent.width - width) / 2
	y: (parent.height - height) / 2
	standardButtons: Dialog.Ok | Dialog.Cancel
	visible: true
	modal: true

	onAccepted: {
		context.acceptStartAppDialog(appNameInput.text, remember.checked ? saveName.text : "")
		qmlCore.deleteLater(this)
	}
	onRejected: qmlCore.deleteLater(this)

	ColumnLayout {
		anchors.fill: parent
		Label {
			text: qsTr("Please enter the applications to start on the selected computers. You can separate multiple applications by line.")
			wrapMode: Label.WordWrap
			Layout.fillWidth: true
		}

		TextArea {
			id: appNameInput
			focus: true
			// work around bug in lupdate which requires double-escaped backslashes
			placeholderText: qsTr("e.g. \"C:\\\\Program Files\\\\VideoLAN\\\\VLC\\\\vlc.exe\"").replace(/\\\\/g, '\\')
			Layout.fillWidth: true
			Layout.fillHeight: true
		}

		CheckBox {
			id: remember
			text: qsTr("Remember and add to application menu")
		}

		TextField {
			id: saveName
			enabled: remember.checked
			placeholderText: qsTr("Application name")
			Layout.fillWidth: true
		}
	}
}
