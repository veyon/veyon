import QtQuick 2.0
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.0

Rectangle {
	id: item
	width: view.cellWidth
	height: view.cellHeight

	property var textColor
	property var view
	property bool isComputerItem: true
	property bool selected: false
	property var objectUid: uid
	property var groups: model.groups

	function addGroup(group)
	{
		var groups = Array.from(model.groups);
		groups.push(String(group))
		model.groups = Array.from(new Set(groups));
	}

	function removeGroup(group)
	{
		model.groups = Array.from(model.groups).filter(item => item !== String(group))
	}

	color: selected ? themeColor : "transparent";

	ColumnLayout {
		anchors.fill: parent
		//clip: true
		id: computerItemLayout
		spacing: 0
		Image {
			source: imageId;
			Layout.alignment: Qt.AlignCenter
			Layout.margins: 5
			MouseArea {
				anchors.fill: parent
				onClicked: item.selected = !item.selected
			}
		}
		Label {
			id: label
			text: display;
			Layout.maximumWidth: view.cellWidth
			Layout.margins: 5
			Layout.alignment: Qt.AlignCenter
			clip: true
			elide: Label.ElideRight
			color: parent.GridView.isCurrentItem ? "white" : item.textColor

		}

		RowLayout  {
			Repeater {
				model: item.groups
				delegate: Rectangle {
					width: view.cellWidth / 5
					height: 3
					color: modelData
				}
			}
		}
	}

}
