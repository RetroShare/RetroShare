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

import QtQuick 2.0
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	id: chatView
	property string chatId

	function refreshData()
	{
		rsApi.request( "/chat/messages/"+chatId, "", function(par)
		{
			chatModel.json = par.response
			if(visible) rsApi.request("/chat/mark_chat_as_read/"+chatId, "",
									  null)
		} )
	}

	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: chatModel
		query: "$.data[*]"
	}

	Component
	{
		id: chatMessageDelegate
		Item
		{
			height: 20
			Row
			{
				Text { text: author_name }
				Text { text: ": " + msg }
			}
		}
	}

	ListView
	{
		width: parent.width
		height: 300
		model: chatModel.model
		delegate: chatMessageDelegate
	}

	TextField
	{
		id: msgComposer
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		width: chatView.width - sendButton.width
	}

	Button
	{
		id: sendButton
		text: "Send"
		anchors.bottom: parent.bottom
		anchors.right: parent.right

		onClicked:
		{
			var jsonData = {"chat_id":chatView.chatId, "msg":msgComposer.text}
			rsApi.request( "/chat/send_message", JSON.stringify(jsonData),
						   function(par) { msgComposer.text = ""; } )
		}
	}

	Timer
	{
		id: refreshTimer
		interval: 800
		repeat: true
		triggeredOnStart: true
		onTriggered: if(chatView.visible) chatView.refreshData()
		Component.onCompleted: start()
	}
}
