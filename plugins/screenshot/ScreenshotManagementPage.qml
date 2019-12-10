import QtQuick 2.0
import QtQuick.Controls 2.0

Page {
	title: qsTr("Screenshots")
	property bool canBeClosed: false

	Label {
		id: screenshots
		anchors.fill: parent
		text: "Hello"
	}
}
