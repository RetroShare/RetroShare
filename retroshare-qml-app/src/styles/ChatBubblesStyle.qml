import QtQuick 2.0
pragma Singleton

QtObject {

//	Margins
	readonly property int lMarginBubble: 10
	readonly property int rMarginBubble: 10
	readonly property int tMarginBubble: 5
	readonly property int bMarginBubble: 10
	readonly property int aditionalBubbleHeight: tMarginBubble * 2
	readonly property int aditionalBubbleWidth: 30

// BubbleProps
	readonly property int radius: 5


//	Colors
	readonly property string colorIncoming: "lightGreen"
	readonly property string colorOutgoing: "aliceblue"
	readonly property string colorSenderName: "cornflowerblue"
	readonly property string colorMessageTime: "grey"


}
