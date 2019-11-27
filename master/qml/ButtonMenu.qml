import QtQuick 2.0
import QtQuick.Controls 2.12

Menu {
	id: control

	property alias model: repeater.model

	signal itemSelected(var itemData)

	function exec(button)
	{
		parent = button;
		popup(0, button.height);
	}

	Repeater {
		id: repeater
		model: groupSelector.availableGroups
		MenuItem {
			contentItem: Text {
				text: modelData.text
				color: modelData.color
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
			}
			onClicked: control.itemSelected(modelData)
		}
	}
}
