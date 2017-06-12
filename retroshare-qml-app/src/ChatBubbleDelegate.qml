import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0
import "." // To load styles
import "./components"

Item {

	id: chatBubbleDelegate
	height: bubble.height
	width: parent.width

	property var styles: StyleChat.bubble


	Rectangle {
		id: rootBubble
		anchors.fill: parent
		width: parent.width
		height: parent.height

		Rectangle
		{
			id: bubble
			width: Math.min (
					   rootBubble.width * styles.bubbleMaxWidth,
					    Math.max(mesageText.implicitWidth, sendersName.implicitWidth )
					   )  + timeText.implicitWidth + styles.aditionalBubbleWidth
			height: mesageText.height + sendersName.height  + styles.aditionalBubbleHeight


			anchors.left: (model.incoming)?  parent.left : undefined
			anchors.right: (!model.incoming)?  parent.right : undefined


			color: (!model.incoming)? styles.colorOutgoing : styles.colorIncoming
			radius:  styles.radius


			Text {
				id: sendersName
				visible: model.incoming
				text: (model.incoming)? model.author_name + ":" : ""
				color: styles.colorSenderName
				font.bold: true

				anchors.leftMargin: styles.lMarginBubble
				anchors.rightMargin: styles.rMarginBubble
				anchors.topMargin: styles.tMarginBubble
				anchors.top: bubble.top

				anchors.left: (model.incoming)?  parent.left : undefined
				anchors.right:(!model.incoming)?  parent.right : undefined

				// Used for give minimum heigh to time when the message is bigger than the bubble in sended messages
				height: (model.incoming || !model.incoming &&
						 mesageText.implicitWidth >= (rootBubble.width * styles.bubbleMaxWidth) )? implicitHeight : 0
			}

			Text {
				id: timeText
				text: getMessageTime()
				color: styles.colorMessageTime

				anchors.left: (!model.incoming)?  parent.left : undefined
				anchors.right:(model.incoming)?  parent.right : undefined

				anchors.top:  bubble.top
				anchors.leftMargin: styles.lMarginBubble
				anchors.rightMargin: styles.rMarginBubble
				anchors.topMargin: styles.tMarginBubble

			}


			Text {
				id: mesageText
				text: model.msg
				width: rootBubble.width * styles.bubbleMaxWidth + timeText.width
				anchors.left: (model.incoming)?  parent.left : undefined
				anchors.right:(!model.incoming)?  parent.right : undefined

				anchors.top: sendersName.bottom
				anchors.leftMargin: styles.lMarginBubble
				anchors.rightMargin: styles.rMarginBubble

				// Used for the correct alineation when the message must be on right
				horizontalAlignment:(!model.incoming &&
									     mesageText.implicitWidth <= (rootBubble.width * styles.bubbleMaxWidth)
									                       )? Text.AlignRight : Text.AlignLeft

				wrapMode: Text.Wrap
			}

		}

	}

	function getMessageTime(){

		var timeFormat = "hh:mm";
		var recvDate = new Date(model.recv_time*1000)

		var timeString = Qt.formatDateTime(recvDate, timeFormat)
		return timeString
	}

 }



