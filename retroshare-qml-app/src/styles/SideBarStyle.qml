import QtQuick 2.0
pragma Singleton

QtObject {

	property var width: 1.5

	property QtObject item: QtObject
	{
		property var height: 50

		property string hoverColor: "lightgrey"
		property string defaultColor: "white"
		property string pressColor: "slategrey"

	}

}
