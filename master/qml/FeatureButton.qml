import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12

Button {

	property alias iconSource: icon.source

	id: control

	topInset: 0
	bottomInset: 0
	padding: 4
	width: height*1.3

	background: Rectangle {
		color: control.down ? "#ddd" : control.hovered ? "#eee" : "transparent"
		width: parent.width
	}

	contentItem: ColumnLayout {
		id: col
		spacing: 0
		Image {
			id: icon
			Layout.maximumHeight: appWindow.landscape ? label.implicitHeight*3 : label.implicitHeight * 2
			Layout.maximumWidth: Layout.maximumHeight
			Layout.alignment: Qt.AlignCenter
		}

		Label {
			id: label
			text: control.text
			horizontalAlignment: Label.AlignHCenter
			verticalAlignment: Label.AlignBottom
			fontSizeMode: Label.Fit
			Layout.fillWidth: true
			Layout.leftMargin: 5
			Layout.rightMargin: 5
			Layout.bottomMargin: 5
			minimumPointSize: font.pointSize * 0.75
			elide: Label.ElideRight
		}
	}
}
