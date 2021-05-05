import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Veyon.Plugins.Screenshot 2.0

Page {
	title: qsTr("Screenshots")
	property bool canBeClosed: false

	Flipable {
		id: flipable
		property bool flipped: false
		anchors.fill: parent

		transform: Rotation {
			id: rotation
			origin.x: flipable.width/2
			origin.y: flipable.height/2
			axis.x: 0; axis.y: 1; axis.z: 0
			angle: 0
		}

		states: State {
			name: "back"
			PropertyChanges { target: rotation; angle: 180 }
			when: flipable.flipped
		}

		transitions: Transition {
			NumberAnimation { target: rotation; property: "angle"; duration: 250 }
		}

		front: GridView {
			id: gridView
			anchors.fill: parent
			model: ScreenshotListModel { }
			cellWidth: 300
			cellHeight: cellWidth * 9 / 16 + 20

			delegate: ColumnLayout {
				spacing: 5
				width: gridView.cellWidth
				height: gridView.cellHeight
				Image {
					Layout.margins: 2
					source: url
					Layout.alignment: Qt.AlignHCenter
					sourceSize.width: width//parent.width
					sourceSize.height: height//parent.height - label.height
					clip: true
					asynchronous: true
					Layout.fillHeight: true
					MouseArea {
						anchors.fill: parent
						onClicked: {
							screenshotView.source = url
							flipable.flipped = !flipable.flipped
						}
					}
				}
				Label {
					id: label
					text: name
					Layout.margins: 5
					elide: Label.ElideRight
					Layout.fillWidth: true
				}
			}
		}

		back: Image {
			anchors.fill: parent
			id: screenshotView
			MouseArea {
				anchors.fill: parent
				onClicked: flipable.flipped = !flipable.flipped
			}
		}
	}
}
