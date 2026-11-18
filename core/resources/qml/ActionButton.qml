import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Button {
	id: control
	property string iconName
	property bool chevron: false
	property bool primary: false
	property bool danger: false
	property bool bordered: true
	property color foreground: !enabled ? Theme.muted : checked ? Theme.onAccent : primary ? Theme.background : danger ? Theme.accent : Theme.text
	font.family: "sans-serif"
	font.pixelSize: 13
	font.weight: Font.DemiBold
	implicitHeight: 30
	implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
	leftPadding: 10
	rightPadding: 10
	topPadding: 5
	bottomPadding: 5
	hoverEnabled: true
	background: Rectangle {
		color: control.checked ? Theme.accent : control.primary ? Theme.text : control.hovered ? Theme.hover : "transparent"
		border.width: control.bordered || control.activeFocus ? 1 : 0
		border.color: control.activeFocus || control.checked ? Theme.accent : Theme.divider
	}
	contentItem: RowLayout {
		spacing: 6
		Icon { name: control.iconName; color: control.foreground; visible: name.length > 0; size: 16; Layout.alignment: Qt.AlignVCenter }
		Text { text: control.text; font: control.font; color: control.foreground; visible: text.length > 0; Layout.alignment: Qt.AlignVCenter }
		Icon { name: "chevron-down"; color: control.foreground; visible: control.chevron; size: 12; Layout.alignment: Qt.AlignVCenter }
	}
}
