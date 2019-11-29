import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Repeater {
	model: [ "#ff0000", "#00aa00", "#0000ff", "#ffaa00", "#00aaff"]

	property var availableGroups: {
		var groups = []
		for( var i = 0; i < count; ++i )
		{
			var item = itemAt(i)
			groups.push( { "text": item.name, "color": item.groupColor } )
		}
		return groups
	}

	property var selectedGroups: {
		var groups = []
		for( var i = 0; i < count; ++i )
		{
			var item = itemAt(i)
			if( item.checked )
			{
				groups.push( item.groupColor )
			}
		}
		return groups
	}

	Button {
		id: control
		checkable: true
		padding: 5
		leftPadding: padding*3
		rightPadding: padding*3
		Layout.fillWidth: true

		property var groupColor: modelData
		property alias name: nameInput.text

		background: Rectangle {
			anchors.fill: parent
			color: checked ? modelData : "white"
			border.color: modelData
			border.width: 1
		}

		contentItem: TextInput {
			id: nameInput
			color: control.checked ? "white" : modelData
			text: qsTr("Group %1").arg(index+1)
			onEditingFinished: focus = false
			MouseArea {
				anchors.fill: parent
				onClicked: {
					if( nameInput.focus )
					{
						nameInput.focus = false
					}
					else
					{
						control.checked = !control.checked
					}
				}
				onPressAndHold: nameInput.focus = true
			}
		}
	}
}
