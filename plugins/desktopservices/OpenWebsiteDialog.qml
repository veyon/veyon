import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.0

Dialog {
	title: qsTr("Open website")
	x: (parent.width - width) / 2
	y: (parent.height - height) / 2
	standardButtons: Dialog.Ok | Dialog.Cancel
	visible: true
	modal: true

	onAccepted: {
		context.acceptOpenWebsiteDialog(urlInput.text, remember.checked ? saveName.text : "")
		qmlCore.deleteLater(this)
	}
	onRejected: qmlCore.deleteLater(this)

	ColumnLayout {
		anchors.fill: parent
		Label {
			text: qsTr("Please enter the URL of the website to open:")
			wrapMode: Label.WordWrap
			Layout.fillWidth: true
		}

		TextField {
			id: urlInput
			focus: true
			placeholderText: qsTr("e.g. www.veyon.io")
			Layout.fillWidth: true
		}

		CheckBox {
			id: remember
			text: qsTr("Remember and add to website menu")
		}

		TextField {
			id: saveName
			enabled: remember.checked
			placeholderText: qsTr("Website name")
			Layout.fillWidth: true
		}
	}
}
