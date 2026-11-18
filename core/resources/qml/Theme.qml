pragma Singleton
import QtQuick
QtObject {
	readonly property bool dark: veyonUi.dark
	readonly property color background: dark ? "#181716" : "#f3f2f2"
	readonly property color surface: dark ? "#22201f" : "#eae9e9"
	readonly property color text: dark ? "#edebea" : "#201e1d"
	readonly property color muted: dark ? "#bab6b6" : "#605d5d"
	readonly property color divider: dark ? "#595554" : "#aaa6a5"
	readonly property color accent: dark ? "#ff563c" : "#ec3013"
	readonly property color onAccent: dark ? "#181716" : "white"
	readonly property color ok: dark ? "#75c58c" : "#288747"
	readonly property color warning: dark ? "#e7b75f" : "#a96700"
	readonly property color hover: dark ? "#343130" : "#dedada"
}
