import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
	property bool active: false
	property alias text: label.text
	property bool fixed: true

	signal clicked()
	signal closed()

	id: item

	implicitWidth: labelLayout.implicitWidth + 2 * label.Layout.margins
	implicitHeight: labelLayout.implicitHeight - label.Layout.margins

	MouseArea {
		anchors.fill: parent
		onClicked: item.clicked();
	}

	Rectangle {
		color: "white"
		visible: active
		anchors.fill: parent
	}

	RowLayout {
		id: labelLayout
		anchors.fill: parent
		Label {
			id: label
			Layout.margins: 10
			font.bold: true
			font.capitalization: Font.AllUppercase
			color: active ? "#444" : "white"
		}
		Item {
			width: height
			height: closeLabel.implicitHeight*2
			visible: !fixed
			Rectangle {
				anchors.fill: parent
				opacity: closeButtonMouseArea.pressed ? 0.5 : closeButtonMouseArea.containsMouse ? 0.3 : 0
				color: closeButtonMouseArea.pressed ? "#aaaaaa" : "white"
			}
			//width: parent.width
			Label {
				id: closeLabel
				anchors.centerIn: parent
				color: label.color
				text: "×"
				font.bold: true
			}

			MouseArea {
				id: closeButtonMouseArea
				anchors.fill: parent
				hoverEnabled: true
				onClicked: closed()
			}
		}
	}
}
