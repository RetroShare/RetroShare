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
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import "." //Needed for TokensManager singleton
import "./components"
Item
{
	id: chatView
	property string chatId
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
		} )
	}

	Component.onCompleted: refreshData()
	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: chatModel
		query: "$.data[*]"
	}

	ListView
	{
		id: chatListView
		width: parent.width - 7
		anchors.horizontalCenter: parent.horizontalCenter
		height: 300
		model: chatModel.model
		delegate: ChatBubbleDelegate {}
		spacing: 3
		preferredHighlightBegin: 1

	}

	Item {

		property var styles: StyleChat.inferiorPanel

		id: inferiorPanel
		height: styles.height
		width: parent.width
		anchors.bottom: parent.bottom

		Rectangle {
			anchors.fill: parent.fill
			width: parent.width
			height: parent.height
			color:inferiorPanel.styles.backgroundColor
			border.color: inferiorPanel.styles.borderColor
		}

		BtnIcon {

			id: attachButton

			property var styles: StyleChat.inferiorPanel.btnIcon

			height: styles.height
			width: styles.width

			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left

			imgUrl: styles.attachIconUrl
		}


		TextField
		{
			property var styles: StyleChat.inferiorPanel.msgComposer

			id: msgComposer
			anchors.bottom: parent.bottom
			anchors.left: attachButton.right

			width: chatView.width - sendButton.width - attachButton.width - emojiButton.width
			height: parent.height -5

			placeholderText: styles.placeHolder
			background: styles.background

			onTextChanged: {
				if (msgComposer.length == 0)
				{
					sendButton.state = ""
				}
				else if (msgComposer.length > 0)
				{
					sendButton.state = "SENDBTN"
				}
			}

		}

		BtnIcon {

			id: emojiButton

			property var styles: StyleChat.inferiorPanel.btnIcon

			height: styles.height
			width: styles.width

			anchors.verticalCenter: parent.verticalCenter
			anchors.left: msgComposer.right

			imgUrl: styles.emojiIconUrl
		}

		BtnIcon {

			id: sendButton

			property var styles: StyleChat.inferiorPanel.btnIcon
			property alias icon: sendButton.imgUrl

			height: styles.height
			width: styles.width

			anchors.verticalCenter: parent.verticalCenter
			anchors.left: emojiButton.right

			imgUrl: styles.microIconUrl

			onClicked:
			{
				if (sendButton.state == "SENDBTN" ) {
					var jsonData = {"chat_id":chatView.chatId, "msg":msgComposer.text}
					rsApi.request( "/chat/send_message", JSON.stringify(jsonData),
								   function(par) { msgComposer.text = ""; } )
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


			states: [
				State {
					name: ""
					PropertyChanges { target: sendButton; icon: styles.microIconUrl}
				},
				State {
					name: "RECORDING"
					PropertyChanges { target: sendButton; icon: styles.microMuteIconUrl}
				},
				State {
					name: "SENDBTN"
					PropertyChanges { target: sendButton; icon: styles.sendIconUrl}
				}
			]
		}

	}
}
