/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.2
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import "." //Needed for TokensManager singleton
import "./components"
import "./components/emoji"

Item
{
	id: chatView
	property string chatId
	property var gxsInfo: ({})
	property int token: 0

	property string objectName:"chatView"


	function refreshData()
	{
		console.log("chatView.refreshData()", visible)
		if(!visible) return

		rsApi.request( "/chat/messages/"+chatId, "", function(par)
		{
			chatModel.json = par.response
			token = JSON.parse(par.response).statetoken
			TokensManager.registerToken(token, refreshData)

			ChatCache.lastMessageCache.updateLastMessageCache(chatId, chatModel.json)

			if(chatListView.visible)
			{
				chatListView.positionViewAtEnd()
				rsApi.request("/chat/mark_chat_as_read/"+chatId)
			}
		})
	}

	Component.onCompleted:
	{
		refreshData()
	}
	onFocusChanged: focus && refreshData()

	function changeState ()
	{
		toolBar.state = "CHATVIEW"
		gxsInfo = ChatCache.lastMessageCache.getGxsFromChatId(chatView.chatId)
		toolBar.gxsSource = gxsInfo.gxs
		toolBar.titleText =  gxsInfo.name
	}


	JSONListModel
	{
		id: chatModel
		query: "$.data[*]"
	}

	ListView
	{
		property var styles: StyleChat.chat
		id: chatListView
		width: parent.width - styles.bubbleMargin
		height: parent.height - inferiorPanel.height
		anchors.horizontalCenter: parent.horizontalCenter
		model: chatModel.model
		delegate: ChatBubbleDelegate {}
		spacing: styles.bubbleSpacing
		preferredHighlightBegin: 1

		onHeightChanged:
		{
			chatListView.currentIndex = count - 1
		}

	}

	EmojiPicker {
		id: emojiPicker

		anchors.fill: parent
		anchors.topMargin: parent.height / 2
		anchors.bottomMargin: if(!androidMode) categorySelectorHeight

		property int categorySelectorHeight: 50

		color: "white"
		buttonWidth: 40
		textArea: inferiorPanel.textMessageArea //the TextArea in which EmojiPicker is pasting the Emoji into

		state: "EMOJI_HIDDEN"
		states: [
			State {
				name: "EMOJI_HIDDEN"
				PropertyChanges { target: emojiPicker; anchors.topMargin: parent.height }
				PropertyChanges { target: emojiPicker; anchors.bottomMargin: -1 }
				PropertyChanges { target: emojiPicker; height: 0 }
			},
			State {
				name: "EMOJI_SHOWN"
				PropertyChanges { target: emojiPicker; anchors.topMargin: parent.height / 2 }
				PropertyChanges { target: emojiPicker; anchors.bottomMargin: categorySelectorHeight }
			}
		]
	}

	Item
	{

		property var styles: StyleChat.inferiorPanel
		property alias textMessageArea: msgComposer.textMessageArea

		id: inferiorPanel
		height:  ( msgComposer.height > styles.height)? msgComposer.height: styles.height
		width: parent.width
		anchors.bottom: emojiPicker.androidMode ? emojiPicker.top : parent.bottom

		Rectangle
		{
			id: backgroundRectangle
			anchors.fill: parent.fill
			width: parent.width
			height: parent.height
			color:inferiorPanel.styles.backgroundColor
			border.color: inferiorPanel.styles.borderColor
		}

		ButtonIcon
		{

			id: attachButton

			property var styles: StyleChat.inferiorPanel.btnIcon

			height: styles.height
			width: styles.width

			anchors.left: parent.left
			anchors.bottom: parent.bottom

			anchors.margins: styles.margin

			imgUrl: styles.attachIconUrl
		}


		RowLayout
		{
			id: msgComposer
			property var styles: StyleChat.inferiorPanel.msgComposer
			property alias textMessageArea: flickable.msgField

			anchors.verticalCenter: parent.verticalCenter
			anchors.left: attachButton.right

			width: chatView.width -
				   (sendButton.width + sendButton.anchors.margins) -
				   (attachButton.width + attachButton.anchors.margins) -
				   (emojiButton.width + emojiButton.anchors.margins)

			height: (flickable.contentHeight <  styles.maxHeight)? flickable.contentHeight : styles.maxHeight

			Flickable
			{
				id: flickable
				property alias msgField: msgField

				anchors.fill: parent
				flickableDirection: Flickable.VerticalFlick

				width: parent.width

				contentWidth: msgField.width
				contentHeight: msgField.height
				contentY: contentHeight - height

				ScrollBar.vertical: ScrollBar {}

				clip: true

				TextArea
				{
					property var styles: StyleChat.inferiorPanel.msgComposer
					id: msgField

					height: contentHeight + font.pixelSize

					width: parent.width

					placeholderText: styles.placeHolder
					background: styles.background

					wrapMode: TextEdit.Wrap

					focus: true

					inputMethodHints: Qt.ImhMultiLine

					font.pixelSize: styles.messageBoxTextSize

					onTextChanged:
					{
						var msgLenght = (msgField.preeditText)? msgField.preeditText.length : msgField.length

						if (msgLenght == 0)
						{
							sendButton.state = ""
						}
						else if (msgLenght > 0 )
						{
							sendButton.state = "SENDBTN"
						}
					}

					property bool shiftPressed: false
					Keys.onPressed:
					{
						if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
								&& !shiftPressed)
						{
							if (sendButton.state == "SENDBTN" )
							{
								 chatView.sendMessage ()
							}
						}
						else if (event.key === Qt.Key_Shift)
						{
							shiftPressed = true
						}
					}

					Keys.onReleased:
					{
						if (event.key === Qt.Key_Shift)
						{
							shiftPressed = false
						}
					}
					function reset ()
					{
						msgField.text = ""
						Qt.inputMethod.reset()
					}
				}
			}
		}

		ButtonIcon
		{

			id: emojiButton

			property var styles: StyleChat.inferiorPanel.btnIcon

			height: styles.height
			width: styles.width

			anchors.right: sendButton.left
			anchors.bottom: parent.bottom

			anchors.margins: styles.margin

			imgUrl: styles.emojiIconUrl

			onClicked: {
				if (emojiPicker.state == "EMOJI_HIDDEN") {
					emojiPicker.state = "EMOJI_SHOWN"
				} else {
					emojiPicker.state = "EMOJI_HIDDEN"
				}
			}

		}

		ButtonIcon
		{

			id: sendButton

			property var styles: StyleChat.inferiorPanel.btnIcon
			property alias icon: sendButton.imgUrl

			height: styles.height
			width: styles.width

			anchors.right: parent.right
			anchors.bottom: parent.bottom

			anchors.margins: styles.margin

			imgUrl: styles.microIconUrl

			onClicked:
			{
				if (sendButton.state == "SENDBTN" )
				{
					 chatView.sendMessage ()
				}
			}

			onPressed:
			{
				if (sendButton.state == "RECORDING" )
				{
					sendButton.state = ""
				}
				else if (sendButton.state == "" )
				{
					sendButton.state = "RECORDING"
				}
			}

			onReleased:
			{
				if (sendButton.state == "RECORDING" )
				{
					sendButton.state = ""
				}

			}

			states:
			[
				State
				{
					name: ""
					PropertyChanges { target: sendButton; icon: styles.microIconUrl}
				},
				State
				{
					name: "RECORDING"
					PropertyChanges { target: sendButton; icon: styles.microMuteIconUrl}
				},
				State
				{
					name: "SENDBTN"
					PropertyChanges { target: sendButton; icon: styles.sendIconUrl}
				}
			]
		}
	}

	function sendMessage ()
	{
		if (emojiPicker.state == "EMOJI_SHOWN") emojiPicker.state = "EMOJI_HIDDEN"

		msgField.text = getCompleteMessageText () + " " // Needed to prevent pre edit text problems
		var msgText = msgField.text

		var jsonData = {"chat_id":chatView.chatId, "msg":msgText}
		rsApi.request( "/chat/send_message", JSON.stringify(jsonData),
					   function(par)
					   {
						   msgField.reset();
					   })
	}

	// This function is needed for the compatibility with auto predictive keyboards
	function getCompleteMessageText (){
		var completeMsg
		if (msgField.preeditText) completeMsg = msgField.text + msgField.preeditText
		else completeMsg =  msgField.text
		return completeMsg


	}

}
