import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0

FileDialog {
	title: qsTr("Select one or more files to transfer")
	selectMultiple: true
	visible: true

	Component.onCompleted: folder = context.lastFileTransferSourceDirectory

	onAccepted: {
		context.acceptSelectedFiles(fileUrls)
		qmlCore.deleteLater(this)
	}
	onRejected: qmlCore.deleteLater(this)
}
