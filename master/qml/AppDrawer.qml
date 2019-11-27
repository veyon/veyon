import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQml.Models 2.1

Drawer {
	id: drawer

	ColumnLayout {
		anchors.fill: parent
		Rectangle {
			color: themeColor
			Layout.fillWidth: true
			height: drawerTitleLayout.implicitHeight
			RowLayout {
				id: drawerTitleLayout
				anchors.fill: parent
				Image {
					id: logo
					source: "qrc:/core/icon64.png"
					Layout.maximumHeight: titleLabel.implicitHeight*1.4
					Layout.maximumWidth: Layout.maximumHeight
					Layout.margins: 10
				}
				Label {
					id: titleLabel
					color: "white"
					text: drawerMenu.subPageTitle ? drawerMenu.subPageTitle : appWindow.title
					font.capitalization: Font.AllUppercase
					font.bold: true
					fontSizeMode: Label.Fit
					font.pointSize: dummy.font.pointSize*1.4
				}
				Label {
					id: dummy
					Layout.fillWidth: true
				}
			}
		}

		MenuItem {
			id: drawerGoBackItem
			visible: drawerMenu.model.rootIndex.valid
			onClicked: drawerMenu.model.rootIndex = drawerMenu.model.parentModelIndex()
			Image {
				anchors.left: parent.left
				source: "qrc:/master/go-previous.png"
				width: height
				height: parent.implicitHeight*0.8
				anchors.verticalCenter: parent.verticalCenter
			}
			Layout.fillWidth: true
		}

		ListView {
			id: drawerMenu
			property var subPageTitle: masterCore.computerSelectModel.value(model.rootIndex, "name")
			model: DelegateModel {
				model: masterCore.computerSelectModel
				delegate: RowLayout {
					spacing: 0
					width: drawerMenu.width
					MenuItem {
						text: name
						checkable: true
						checked: checkState
						visible: String(type) === "Host"
						Layout.fillWidth: true
						onClicked: checkState = !checkState
					}
					CheckBox {
						id: checkBox
						checkState: model.checkState
						onClicked: model.checkState = checkState
						visible: String(type) === "Location"
						tristate: checkState == Qt.PartiallyChecked
						Layout.maximumWidth: height
					}
					MenuItem {
						text: name
						visible: String(type) === "Location"
						Image {
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
							source: "qrc:/master/go-next.png"
							width: height
							height: parent.implicitHeight*0.8
						}
						Layout.fillWidth: true
						onClicked: drawerMenu.model.rootIndex = drawerMenu.model.modelIndex(index)
					}
				}
			}
			Layout.fillWidth: true
			Layout.fillHeight: true
		}
	}
}
