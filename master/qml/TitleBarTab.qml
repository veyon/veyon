import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
	property bool active: false
	property alias text: label.text
	property bool canBeClosed: true

	signal clicked()
	signal closed()

	id: item

	implicitWidth: labelLayout.implicitWidth
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
		spacing: 0
		Label {
			id: label
			Layout.margins: 8
			font.bold: true
			font.capitalization: Font.AllUppercase
			color: active ? "#444" : "white"
		}
		Item {
			width: height
			height: closeLabel.implicitHeight
			visible: canBeClosed
			Rectangle {
				anchors.centerIn: parent
				width: height
				height: parent.height
				opacity: closeButtonMouseArea.pressed ? 0.5 : closeButtonMouseArea.containsMouse ? 0.3 : 0
				color: closeButtonMouseArea.pressed ? "#aaaaaa" : active ? label.color : "white"
			}
			Label {
				id: closeLabel
				anchors.centerIn: parent
				color: label.color
				text: "×"
				font.bold: true
				font.pointSize: label.font.pointSize*1.5
			}

			MouseArea {
				id: closeButtonMouseArea
				anchors.fill: parent
				hoverEnabled: true
				onClicked: closed()
			}
		}
		Item {
			id: spacer
			visible: canBeClosed
			width: label.Layout.margins
			height: 1
		}
	}
}
