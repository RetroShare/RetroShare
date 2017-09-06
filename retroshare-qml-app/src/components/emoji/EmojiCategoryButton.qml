import QtQuick 2.7
import QtQuick.Controls.Styles 1.2

Rectangle
{
    id: emojiCategoryButton
    property string categoryName

	property var fontName


	function completedHandler()
	{
        categoryName = eCatName

        //initialize
		if (parent.currSelEmojiButton === undefined)
		{
            clickedHandler()
        }
    }

	function pressedHandler()
	{
		if (state != "SELECTED")
		{
            state = state == "PRESSED" ? "RELEASED" :  "PRESSED"
        }
    }

	function clickedHandler()
	{
		if (parent.currSelEmojiButton !== undefined)
		{
            parent.currSelEmojiButton.state = "RELEASED"
        }

        parent.currSelEmojiButton = emojiCategoryButton
        state = "SELECTED"
        Qt.emojiCategoryChangedHandler(emojiCategoryButton.categoryName)
    }


	Text
	{
        id: emojiText
        color: "gray"
        text: qsTr(eCatText)
        font.pixelSize: emojiCategoryButton.width - 8
        anchors.centerIn: parent
		font.family: fontName
    }


    state: "RELEASED"
	states:
		[
		State
		{
            name: "PRESSED"
			PropertyChanges
			{
                target: emojiText
                font.pixelSize: emojiCategoryButton.width - 10
            }
        },
		State
		{
            name: "RELEASED"
			PropertyChanges
			{
                target: emojiText
                font.pixelSize: emojiCategoryButton.width - 8
            }
        },
		State
		{
            name: "SELECTED"
			PropertyChanges
			{
                target: emojiCategoryButton
                color: "#ADD6FF"
            }
        }
    ]

	MouseArea
	{
        anchors.fill: parent
        hoverEnabled: true
        onEntered: emojiText.color = "black"
        onExited: emojiText.color = "gray"
        onPressedChanged: pressedHandler()
        onClicked: clickedHandler()
    }

    Component.onCompleted: completedHandler()
}
