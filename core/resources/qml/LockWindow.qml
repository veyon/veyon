import QtQuick
import QtQuick.Window
import Veyon 1.0
Window {
	id: window
	property var demoComputer: null
	property rect demoViewport: Qt.rect(0, 0, 0, 0)
	property date clock: new Date()
	visible: false
	color: lockState.mode === 2 ? "#000000" : "#141312"
	onClosing: close => { close.accepted = false }
	Image {
		anchors.fill: parent
		visible: lockState.mode === 0 || lockState.mode === 1
		source: "image://lockBackground/screen"
		fillMode: lockState.mode === 0 ? Image.Stretch : Image.Pad
	}
	Framebuffer { anchors.fill: parent; computer: window.demoComputer; viewport: window.demoViewport; live: computer !== null; viewOnly: true; visible: window.demoComputer !== null }
	Column {
		visible: lockState.mode === 3
		x: Math.max(24, Math.min(120, window.width / 10))
		y: Math.max(24, window.height / 2 - 100)
		width: window.width - 2 * x
		spacing: 20
		Icon { name: "lock"; color: "#ff563c"; size: 44 }
		Text { text: qsTranslate("LockWidget", "Screen locked"); color: "#edebea"; font.family: "sans-serif"; font.pixelSize: Math.max(26, Math.min(44, window.width / 28)); font.bold: true; width: parent.width; wrapMode: Text.WordWrap }
		Text { text: qsTranslate("LockWidget", "Your programs and files stay open. The lock ends when your teacher unlocks this computer."); color: "#bab6b6"; font.family: "sans-serif"; font.pixelSize: 16; width: Math.min(560, parent.width); wrapMode: Text.WordWrap }
	}
	Text { visible: lockState.mode === 3; x: Math.max(24, Math.min(120, window.width / 10)); y: window.height - 62; text: Qt.formatTime(window.clock, "HH:mm"); color: "#9b9797"; font.pixelSize: 15 }
	Timer { interval: 1000; running: lockState.mode === 3; repeat: true; onTriggered: window.clock = new Date() }
}
