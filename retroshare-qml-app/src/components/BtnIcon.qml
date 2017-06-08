import QtQuick 2.0

Item {

	id: root
	signal clicked
//	signal pressed
//	signal released

	property var imgUrl: ""

	Image {

		anchors.fill: parent
		source: imgUrl

		height:  parent.height
		width: parent.width
	}

	MouseArea {
		anchors.fill: root
		onClicked: { root.clicked() }
//		onPressed: { root.pressed() }
//		onReleased: { root.released() }
	}
}
