import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import "qrc:/master"

Rectangle {
	id: control
	height: layout.implicitHeight
	//spacing: 0
	color: themeColor

	property var appDrawer
	property var container

	RowLayout {
		id: layout
		spacing: 0
		anchors.fill: parent

		Image {
			id: appMenu
			source: "qrc:/master/application-menu.png"
			Layout.maximumHeight: tabsLayout.implicitHeight // TODO
			Layout.maximumWidth: Layout.maximumHeight
			Layout.alignment: Qt.AlignVCenter
			Layout.margins: 5
			MouseArea {
				anchors.fill: parent
				onClicked: appDrawer.open()
			}
		}

		RowLayout {
			id: tabsLayout
			spacing: 0
			Repeater {
				model: container.contentChildren

				TitleBarTab {
					Layout.topMargin: 2
					Layout.fillHeight: true
					text: modelData.title
					active: container.currentIndex === index
					onClicked: {
						if( active )
						{
							if( container.currentItem.header )
							{
								container.currentItem.header.visible = !container.currentItem.header.visible
							}
						}
						else
						{
							container.currentIndex = index
						}
					}
				}
			}
		}

		Item {
			Layout.fillWidth: true
		}
	}
}
