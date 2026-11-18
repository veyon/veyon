import QtQuick
import QtQuick.Window
import Veyon 1.0
Window {
	id: window
	width: 1000; height: 680; visible: true
	title: qsTr("Veyon Demo")
	color: "#141312"
	flags: Qt.Window | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint
	onClosing: close => { close.accepted = false }
	Framebuffer {
		id: frame
		anchors.fill: parent
		computer: demoConnection
		viewport: demoViewport
		live: true
		viewOnly: true
		onSourceSizeChanged: {
			if (sourceSize.width > 0 && sourceSize.height > 0) {
				let scale = Math.min(1, (Screen.desktopAvailableWidth - 80) / sourceSize.width, (Screen.desktopAvailableHeight - 100) / sourceSize.height)
				window.width = Math.round(sourceSize.width * scale)
				window.height = Math.round(sourceSize.height * scale)
			}
		}
	}
}
