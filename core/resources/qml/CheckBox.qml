import QtQuick
import QtQuick.Controls as Controls

Controls.CheckBox {
	id: control
	font.family: "sans-serif"
	font.pixelSize: 13
	spacing: 8
	hoverEnabled: true
	opacity: enabled ? 1 : 0.45
	indicator: Rectangle {
		implicitWidth: 18
		implicitHeight: 18
		x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
		y: control.topPadding + (control.availableHeight - height) / 2
		color: control.checkState !== Qt.Unchecked ? Theme.accent : control.down ? Theme.hover : "transparent"
		border.width: control.visualFocus ? 2 : 1.5
		border.color: control.checkState !== Qt.Unchecked || control.visualFocus ? Theme.accent : Theme.divider
		Icon {
			objectName: "checkGlyph"
			anchors.centerIn: parent
			name: control.checkState === Qt.PartiallyChecked ? "minus" : "check"
			size: 14
			color: "white"
			visible: control.checkState !== Qt.Unchecked
		}
	}
	contentItem: Text {
		leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
		rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
		text: control.text
		font: control.font
		color: Theme.text
		verticalAlignment: Text.AlignVCenter
		wrapMode: Text.WordWrap
	}
}
