import QtQuick 2.0
import QtQuick.Controls 2.0

Page {
	title: qsTr("Remote access: %1").arg(context.computerName)
	Rectangle {
		anchors.fill: parent
		color: "darkGray"
	}
	Component.onCompleted: {
		context.view.parent = this;
		context.view.anchors.fill = this;
		SwipeView.view.currentIndex = SwipeView.view.count-1
	}
}
