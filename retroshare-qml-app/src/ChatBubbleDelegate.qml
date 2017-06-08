import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0
import "." // To load styles
//import './styles'
//import "styles/ChatBubblesStyle.qml" as Style

Item {

	id: chatBubbleDelegate
	height: bubble.height
	width: parent.width

//	property int lMarginBubble: 10
//	property int rMarginBubble: 10
//	property int tMarginBubble: 5
//	property int bMarginBubble: 10
//	property int aditionalBubbleHeight: tMarginBubble * 2
//	property int aditionalBubbleWidth: 30

	property var styles: StyleChatBubble


	Rectangle {
		anchors.fill: parent
		width: parent.width
		height: parent.height

		Rectangle
		{
			id: bubble
			width: Math.max(mesageText.implicitWidth, sendersName.implicitWidth ) + timeText.implicitWidth + styles.aditionalBubbleWidth
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

				height: (model.incoming)? implicitHeight : 0
			}

			Text {
				id: timeText
				text: getMessageTime()
				color: styles.colorMessageTime

				anchors.left: (!model.incoming)?  parent.left : undefined
				anchors.right:(model.incoming)?  parent.right : undefined

				anchors.bottom:  bubble.bottom
				anchors.leftMargin: styles.lMarginBubble
				anchors.rightMargin: styles.rMarginBubble
				anchors.topMargin: styles.tMarginBubble
			}


			Text {
				id: mesageText
				text: model.msg

				anchors.left: (model.incoming)?  parent.left : undefined
				anchors.right:(!model.incoming)?  parent.right : undefined

				anchors.top: sendersName.bottom
				anchors.leftMargin: styles.lMarginBubble
				anchors.rightMargin: styles.rMarginBubble

//				wrapMode: Text.Wrap
			}



		}

	}

	Component.onCompleted: {
		toolBar.state = "CHATVIEW"
		toolBar.titleText =  model.author_name
	}

	function getMessageTime(){

		var timeFormat = "hh:mm";
		var recvDate = new Date(model.recv_time*1000)

		var timeString = Qt.formatDateTime(recvDate, timeFormat)
		return timeString
	}

 }



