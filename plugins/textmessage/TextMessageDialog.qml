import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0

Dialog {
	title: qsTr("Send text message")
	standardButtons: StandardButton.Ok | StandardButton.Cancel
	visible: true

	onAccepted: {
		context.acceptTextMessage(textArea.text)
		qmlCore.deleteLater(this)
	}
	onRejected: qmlCore.deleteLater(this)

	ColumnLayout {
		anchors.fill: parent
		Label {
			text: qsTr("Use the field below to type your message which will be sent to all selected users.")
		}

		TextArea {
			id: textArea
			focus: true
			Layout.fillWidth: true
			Layout.fillHeight: true
		}
	}
}
