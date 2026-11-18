import QtQuick
import QtQuick.Window
Item {
	id: icon
	property string name
	property color color: Theme.text
	property int size: 16
	implicitWidth: size
	implicitHeight: size
	Image {
		anchors.fill: parent
		sourceSize.width: icon.size * Screen.devicePixelRatio
		sourceSize.height: icon.size * Screen.devicePixelRatio
		source: icon.name.length ? "image://icons/" + icon.name + "?color=" + encodeURIComponent(icon.color) : ""
		fillMode: Image.PreserveAspectFit
		smooth: true
	}
}
