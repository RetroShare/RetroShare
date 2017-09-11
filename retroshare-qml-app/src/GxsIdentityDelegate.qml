/*
 * RetroShare Android QML App
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
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
import "." //Needed for ChatCache singleton
import "./components"

Item
{
	id: delegateRoot
	height: 57
	width: parent.width


	property var chatId: undefined
	property var lastMessageData: ({})
	property var locale: Qt.locale()

	Rectangle
	{
		id: contactItem

		anchors.fill: parent
		color: contactItemArea.containsPress ? "lightgrey" : "transparent"
		width: parent.width
		height: parent.height

		MouseArea
		{
			id: contactItemArea
			anchors.fill: parent
			onClicked:
			{
				console.log("GxsIntentityDelegate onclicked:", model.name,
							model.gxs_id)
				contactsView.searching = false
				if(model.own)
				{
					contactsView.own_gxs_id = model.gxs_id
					stackView.push(
								"qrc:/ContactDetails.qml",
								{md: ChatCache.contactsCache.getContactFromGxsId(model.gxs_id)})

				}
				else
				{
					startDistantChat()
				}
			}

			onPressAndHold: showDetails()
			hoverEnabled: true
		}

		Rectangle
		{
			id: backgroundRectangle
			anchors.fill: parent.fill
			anchors.right: parent.right
			width: parent.width - colorHash.width - 15
			height: parent.height
			color:"transparent"

			Rectangle
			{
				    id: borderBottom
					width:  parent.width
					height: 1
					anchors.bottom: parent.bottom
					anchors.right: parent.right
					color: "lightgrey"
			}
		}

		Rectangle
		{
			anchors.fill: parent
			color: "transparent"
			anchors.margins: 5

			AvatarOrColorHash
			{
				id: colorHash

				gxs_id: model.gxs_id

				height: parent.height - 4
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: parent.left
				anchors.leftMargin: 2

				onlyCached: true
			}

			Column
			{

				id: chatInfoRow
				height: parent.height
				width: parent.width - isContactRow.width - colorHash.width
				anchors.left: colorHash.right
				anchors.leftMargin: 10
				anchors.right: isContactRow.left
				anchors.rightMargin: 5


				Item
				{
					width: parent.width
					height: parent.height /2

					Text
					{
						id: nickText
						color: model.own ? "blue" : "black"
						text: model.name
						font.bold: true
						anchors.bottom: parent.bottom
					}

					Text
					{
						text:  setTime()
						anchors.right: parent.right
						color: "darkslategrey"
						anchors.bottom: parent.bottom
					}
				}

				Item
				{
					id: lastMessageText
					width: parent.width
					height: (lastMessageData && lastMessageData.msg !== undefined)? parent.height /2 : 0

					Text
					{
						id: lastMessageSender
						font.italic: true
						color: "royalblue"
						text:  ((lastMessageData && lastMessageData.incoming !== undefined) && !lastMessageData.incoming)?  "You: " : ""
						height: parent.height
					}

					Text
					{
						id: lastMessageMsg
						anchors.left: lastMessageSender.right
						text: (lastMessageData && lastMessageData.msg !== undefined)?  lastMessageData.msg : ""
						rightPadding: 5
						elide: Text.ElideRight
						color: "darkslategrey"
						width: chatInfoRow.width - 30
						height: parent.height
					}

					Rectangle
					{
						visible: model.unread_count > 0

						color: "cornflowerblue"
						antialiasing: true
						height: parent.height - 6
						radius: height/2
						width: height
//						anchors.verticalCenter: parent.verticalCenter
						anchors.right: parent.right

						Text
						{
							color: "white"
							font.bold: true
							text: model.unread_count
							anchors.centerIn: parent
						}
					}

				}

			}

			Row
			{
				id: isContactRow
				anchors.right: parent.right
				anchors.rightMargin: 10
				anchors.verticalCenter: parent.verticalCenter
				height: parent.height - 10
				spacing: 4

				Image
				{
					source: model.is_contact ?
								"qrc:/icons/rating.svg" :
								"qrc:/icons/rating-unrated.svg"
					height: parent.height - 4
					sourceSize.height: height
					fillMode: Image.PreserveAspectFit
					anchors.verticalCenter: parent.verticalCenter

					id: isContactIcon
				}
			}
		}
	}


	Component.onCompleted:
	{
		if (!chatId){
			chatId = getChatIdFromGXS()
		}
		if (chatId) {
			var last = getChatLastMessage(chatId)
			if (last) lastMessageData = last
		}
	}

	Connections
	{
		target: ChatCache.lastMessageCache
		onLastMessageChanged: {
			if (!chatId) {
				chatId = getChatIdFromGXS()
			}
			if (chatId && chatId === chatI){
				console.log("New last message received!")
				lastMessageData = newLastMessage
			}
		}
	}


	function getChatLastMessage (chatId)
	{
		return ChatCache.lastMessageCache.getChatLastMessage(chatId)
	}

	function getChatIdFromGXS ()
	{
		var id= ChatCache.lastMessageCache.getChatIdFromGxs(model.gxs_id)
		return ChatCache.lastMessageCache.getChatIdFromGxs(model.gxs_id)
	}
	function setTime()
	{
		if (!lastMessageData || lastMessageData.recv_time === undefined) return ""

		var timeFormat = "dd.MM.yyyy";
		var recvDate = new Date(lastMessageData.recv_time*1000)

		// Check if is today
		if ( new Date (lastMessageData.recv_time*1000).setHours(0,0,0,0) ==  new Date ().setHours(0,0,0,0))
		{
			timeFormat = "hh:mm"
		}
		var timeString = Qt.formatDateTime(recvDate, timeFormat)
		return timeString
	}

	function showDetails()
	{
		console.log("showDetails()", index)
		contactsView.searching = false
		stackView.push(
					"qrc:/ContactDetails.qml",
					{md: contactsListView.model.get(index)})
	}

	function startDistantChat ()
	{
		ChatCache.chatHelper.startDistantChat(contactsView.own_gxs_id,  model.gxs_id, model.name,
											  function (chatId)
											  {
												  stackView.push("qrc:/ChatView.qml", {'chatId': chatId})
											  })
	}
}

