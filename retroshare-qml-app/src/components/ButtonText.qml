import QtQuick 2.0
import QtQuick.Controls 2.0

Item
{
	id: button
	property alias text: innerText.text;
	property alias buttonTextPixelSize: innerText.font.pixelSize
	property alias innerAnchors: innerElements.anchors;
	property alias rectangleButton: rectangleButton;

	property var iconUrl
	property int iconHeight: 20

	property color color: "lightsteelblue"
	property color hoverColor: color
	property color pressColor: color
	property int fontSize
	property int borderWidth
	property int borderRadius: 3
	property int innerMargin: 10

	height: innerText.height + innerMargin + innerMargin
	width: innerText.width + innerMargin + innerMargin + icon.width + icon.width

	scale: state === "Pressed" ? 0.96 : 1.0
	onEnabledChanged: state = ""
	signal clicked

	Rectangle
	{
		id: rectangleButton
		anchors.fill: parent
		radius: borderRadius
		color: button.enabled ? button.color : "lavender"
		border.width: borderWidth
		border.color: "black"

		ToolButton {
			id: innerElements

			Image
			{
				id: icon
				source: (iconUrl)? iconUrl: ""
				height: (iconUrl)? iconHeight: 0
				width: (iconUrl)? iconHeight: 0

				fillMode: Image.PreserveAspectFit

				visible: (iconUrl)? true: false
				anchors.left: innerElements.left

				anchors.margins:(iconUrl)? innerMargin : 0
				anchors.verticalCenter: parent.verticalCenter
				sourceSize.height: height
			}

			Text
			{
				anchors.margins: innerMargin
				id: innerText
				font.pointSize: fontSize
				anchors.left: icon.right
				anchors.verticalCenter: parent.verticalCenter
				color: button.enabled ? "black" : "grey"
			}
		}
	}

	states: [
		State
		{
			name: "Hovering"
			PropertyChanges
			{
				target: rectangleButton
				color: hoverColor
			}
		},
		State
		{
			name: "Pressed"
			PropertyChanges
			{
				target: rectangleButton
				color: pressColor
			}
		}
	]

	transitions: [
		Transition
		{
			from: ""; to: "Hovering"
			ColorAnimation { duration: 200 }
		},
		Transition
		{
			from: "*"; to: "Pressed"
			ColorAnimation { duration: 10 }
		}
	]

	MouseArea
	{
		hoverEnabled: true
		anchors.fill: button
		onEntered: { button.state='Hovering'}
		onExited: { button.state=''}
		onClicked: { button.clicked();}
		onPressed: { button.state="Pressed" }
		onReleased:
		{
			if (containsMouse)
				button.state="Hovering";
			else
				button.state="";
		}
	}

	Behavior on scale
	{
		NumberAnimation
		{
			duration: 100
			easing.type: Easing.InOutQuad
		}
	}
}
