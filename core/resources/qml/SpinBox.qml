import QtQuick
import QtQuick.Controls as Controls

Controls.SpinBox {
	id: control
	implicitWidth: 160
	implicitHeight: 30
	font.family: "sans-serif"
	font.pixelSize: 13
	up.indicator: Rectangle {
		x: control.mirrored ? 0 : control.width - width
		height: control.height
		implicitWidth: 28
		implicitHeight: 30
		color: control.up.pressed ? Theme.hover : Theme.surface
		Icon { anchors.centerIn: parent; name: "plus"; size: 14; color: control.enabled ? Theme.text : Theme.muted }
	}
	down.indicator: Rectangle {
		x: control.mirrored ? control.width - width : 0
		height: control.height
		implicitWidth: 28
		implicitHeight: 30
		color: control.down.pressed ? Theme.hover : Theme.surface
		Icon { anchors.centerIn: parent; name: "minus"; size: 14; color: control.enabled ? Theme.text : Theme.muted }
	}
}
