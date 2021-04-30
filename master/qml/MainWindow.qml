import QtQuick 2.0
import QtQuick.Window 2.1
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Layouts 1.0
import Veyon.Master 5.0
import "qrc:/master"

ApplicationWindow {
	id: appWindow
	visible: true
	width: 1600
	height: 900
	font.pointSize: 10

	property color themeColor: "#00BCD4";
	property bool landscape: width > height

	Material.theme: Material.Light
	Material.accent: Material.Cyan
	Material.primary: Material.Cyan

	title: qsTr("Veyon Master")

	Component.onCompleted: {
		masterCore.appWindow = this
		masterCore.appContainer = container
	}

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		TitleBar {
			Layout.fillWidth: true
			container: container
			appDrawer: AppDrawer {
				width: Math.max(300, landscape ? 0.25 * appWindow.width : 0.5*appWindow.width)
				height: appWindow.height
			}
		}

		SwipeView {
			id: container
			Layout.fillHeight: true
			Layout.fillWidth: true

			ComputerMonitoring { }
		}
	}
}
