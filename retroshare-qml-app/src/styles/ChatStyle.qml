import QtQuick 2.0
pragma Singleton

QtObject {

	property QtObject bubble: QtObject{
		//	Bubble measures
		readonly property int lMarginBubble: 10
		readonly property int rMarginBubble: 10
		readonly property int tMarginBubble: 5
		readonly property int bMarginBubble: 10
		readonly property int aditionalBubbleHeight: tMarginBubble * 2
		readonly property int aditionalBubbleWidth: 30
		readonly property real bubbleMaxWidth: 0.5 // % from parent

		// BubbleProps
		readonly property int radius: 5


		//	Colors
		readonly property string colorIncoming: "lightGreen"
		readonly property string colorOutgoing: "aliceblue"
		readonly property string colorSenderName: "cornflowerblue"
		readonly property string colorMessageTime: "grey"

		// Text
		readonly property int messageTextSize: 15

	}


	property QtObject inferiorPanel: QtObject{
		// Panel globals
		readonly property int height: 50
		readonly property string backgroundColor: "white"
		readonly property string borderColor: "lightGrey"

		property QtObject msgComposer: QtObject{
			readonly property string placeHolder: "Send message..."
			readonly property int maxHeight: 180
			readonly property int messageBoxTextSize: 15

			property QtObject background: Rectangle {
				color: "transparent"
			}
		}


		// Button Icon
		property QtObject btnIcon: QtObject{
			readonly property int width: 35
			readonly property int height: 35

			readonly property int margin: 5

			readonly property string sendIconUrl: "/icons/send-message.svg"
			readonly property string attachIconUrl: "/icons/attach.svg"
			readonly property string microIconUrl: "/icons/microphone.svg"
			readonly property string microMuteIconUrl: "/icons/microphone_mute.svg"
			readonly property string emojiIconUrl: "/icons/smiley.svg"


		}
	}

	property QtObject chat: QtObject {
		// Measures
		readonly property int bubbleMargin: 20
		readonly property int bubbleSpacing: 10

	}


}
