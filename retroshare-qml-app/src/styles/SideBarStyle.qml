import QtQuick 2.0
pragma Singleton

QtObject {

	property var width: 2 // Number of third parts of screen (for example 2/3)

	property QtObject item: QtObject
	{
		property var height: 50

		property string hoverColor: "lightgrey"
		property string defaultColor: "white"
		property string pressColor: "slategrey"
		property int pixelSize: 14
	}
	property QtObject header: QtObject
	{
		property var color: "#0398d5"
		property var avatarHeight: 50
		property var avatarMargins: 15
		property var textColor: "white"
		property var textNickSize: 14
		property var textGxsidSize: 11
	}
	property QtObject footer: QtObject
	{
		property var color: "white"
		property var textColor: "grey"
		property var margins: 8
        property string text: "UnseenP2P Android alpha"
	}

}
