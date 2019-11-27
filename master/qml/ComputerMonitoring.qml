import QtQuick 2.6
import QtQuick.Controls 2.6
import QtQuick.Layouts 1.1
import Veyon.Master 5.0

Page {

	title: qsTr("Computers")

	header: Flow {
		Repeater {
			model: masterCore.featureListModel
			delegate: FeatureButton {
				text: displayName
				iconSource: iconUrl
				onClicked: computerMonitoring.runFeature(uid)
			}
		}
	}

	ButtonMenu {
		id: addToGroupPopupMenu
		model: computerGroupSelector.availableGroups
		onItemSelected: computerMonitoringView.addSelectedToGroup(itemData.color)
	}

	ButtonMenu {
		id: removeFromGroupPopupMenu
		model: computerGroupSelector.availableGroups
		onItemSelected: computerMonitoringView.removeSelectedFromGroup(itemData.color)
	}

	ComputerMonitoringItem {

		id: computerMonitoring
		searchFilter: searchFilterInput.text
		groupFilter: computerGroupSelector.selectedGroups
		anchors.fill: parent

		selectedObjects : {
			var objectUids = [];
			for( var item in computerMonitoringView.selectedItems )
			{
				objectUids.push(computerMonitoringView.selectedItems[item].objectUid)
			}
			return objectUids;
		}

		Rectangle {
			anchors.fill: parent
			color: computerMonitoring.backgroundColor
		}

		GridView {
			id: computerMonitoringView
			anchors.fill: parent
			clip: true
			anchors.margins: 2
			model: computerMonitoring.model
			cellWidth: computerMonitoring.iconSize.width + 10
			cellHeight: computerMonitoring.iconSize.height + 25 + dummyLabel.implicitHeight + 3
			delegate: ComputerDelegate {
				view: computerMonitoringView
				textColor: computerMonitoring.textColor
			}

			Label {
				id: dummyLabel
				visible: false
			}

			function setAllSelected( selected )
			{
				for( var child in contentItem.children )
				{
					var item = contentItem.children[child];
					if( item.isComputerItem )
					{
						item.selected = selected;
					}
				}
			}

			function addSelectedToGroup( group )
			{
				for( var item in selectedItems )
				{
					selectedItems[item].addGroup( [group] );
				}
			}

			function removeSelectedFromGroup( group )
			{
				for( var item in selectedItems )
				{
					selectedItems[item].removeGroup( [group] );
				}
			}

			property var selectedItems : {
				var items = [];
				for( var child in contentItem.children )
				{
					var item = contentItem.children[child];
					if( item.isComputerItem && item.selected )
					{
						items.push(item)
					}
				}
				return items;
			}

			property var allItems : {
				var items = [];
				for( var child in contentItem.children )
				{
					var item = contentItem.children[child];
					if( item.isComputerItem )
					{
						items.push(item)
					}
				}
				return items;
			}

			property bool selecting: selectedItems.length > 0
		}
	}

	footer: GridLayout {
		columnSpacing: 10
		columns: appWindow.landscape ? 10 : 1

		RowLayout {
			Layout.leftMargin: 5
			Layout.rightMargin: 5
			visible: !computerMonitoringView.selecting
			ComputerGroupSelector {
				id: computerGroupSelector
			}
			Layout.maximumWidth: appWindow.landscape ? implicitWidth : parent.width - Layout.leftMargin - Layout.rightMargin
		}

		TextField {
			id: searchFilterInput
			placeholderText: qsTr("Search users and computers")
			implicitWidth: parent.width / 5
			visible: !computerMonitoringView.selecting && appWindow.landscape
			padding: 3
		}

		RowLayout {
			visible: computerMonitoringView.selecting
			Layout.alignment: Qt.AlignCenter
			Repeater {
				model: ListModel {
					ListElement {
						displayName: qsTr("Select all")
						iconUrl: "qrc:/master/edit-select-all.png"
						handler: () => { computerMonitoringView.setAllSelected(true) }
					}
					ListElement {
						displayName: qsTr("Unselect all")
						iconUrl: "qrc:/master/homerun.png"
						handler: () => { computerMonitoringView.setAllSelected(false) }
					}
					ListElement {
						displayName: qsTr("Add to group")
						iconUrl: "qrc:/master/add-group.png"
						handler: (button) => { addToGroupPopupMenu.exec(button); }
					}
					ListElement {
						displayName: qsTr("Remove from group")
						iconUrl: "qrc:/master/remove-group.png"
						handler: (button) => { removeFromGroupPopupMenu.exec(button) }
					}
				}

				Button {
					padding: 7
					leftPadding: 10
					rightPadding: 10
					topInset: 0
					bottomInset: 0
					leftInset: 0
					rightInset: 0
					background: Rectangle {
						color: parent.down ? "#ccc" : parent.hovered ? "#ddd" : "white"
						border.color: "#888"
						border.width: 1
					}
					text: displayName
					display: Button.TextUnderIcon
					icon.source: iconUrl
					icon.color: "transparent"
					onClicked: handler(this)
				}
			}
		}

		Slider {
			id: computerScreenSizeSlider
			from: ComputerMonitoringItem.MinimumComputerScreenSize
			to: ComputerMonitoringItem.MaximumComputerScreenSize
			value: computerMonitoring.iconSize.width
			implicitWidth: appWindow.landscape ? parent.width / 4 : parent.width
			visible: !computerMonitoringView.selecting
			onMoved: computerMonitoring.computerScreenSize = value
			Layout.alignment: Qt.AlignRight
			Layout.rightMargin: 10
		}
	}
}
