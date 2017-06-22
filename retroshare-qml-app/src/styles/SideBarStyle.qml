import QtQuick 2.0
pragma Singleton

QtObject {

	property var width: 1.5

	property QtObject item: QtObject
	{
		property var height: 40

		property string hoverColor: "lightgrey"
		property string defaultColor: "white"
		property string pressColor: "slategrey"
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
	}

}
