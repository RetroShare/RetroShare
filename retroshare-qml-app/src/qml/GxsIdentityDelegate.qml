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

import QtQuick 2.0

Item
{
	height: 40
	width: parent.width

	MouseArea
	{
		anchors.fill: parent
		onClicked:
		{
			console.log("Contacts view onclicked:", model.name,
						model.gxs_id)
			contactsView.searching = false
			if(model.own) contactsView.own_gxs_id = model.gxs_id
			else
			{
				var jsonData = { "own_gxs_hex": contactsView.own_gxs_id,
					"remote_gxs_hex": model.gxs_id }
				rsApi.request("/chat/initiate_distant_chat",
							  JSON.stringify(jsonData),
							  contactsView.startChatCallback)
			}
		}
		Rectangle
		{
			id: colorHash
			height: parent.height - 4
			width: height
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
			anchors.leftMargin: 2
			color: "white"
			property int childHeight : height/2

			Image
			{
				source: "qrc:/qml/icons/edit-image-face-detect.png"
				anchors.fill: parent
			}

			Rectangle
			{
				color: '#' + model.gxs_id.substring(1, 9)
				height: parent.childHeight
				width: height
				anchors.top: parent.top
				anchors.left: parent.left
			}
			Rectangle
			{
				color: '#' + model.gxs_id.substring(9, 17)
				height: parent.childHeight
				width: height
				anchors.top: parent.top
				anchors.right: parent.right
			}
			Rectangle
			{
				color: '#' + model.gxs_id.substring(17, 25)
				height: parent.childHeight
				width: height
				anchors.bottom: parent.bottom
				anchors.left: parent.left
			}
			Rectangle
			{
				color: '#' + model.gxs_id.slice(-8)
				height: parent.childHeight
				width: height
				anchors.bottom: parent.bottom
				anchors.right: parent.right
			}

			MouseArea
			{
				anchors.fill: parent
				onPressAndHold:
				{
					fingerPrintDialog.nick = model.name
					fingerPrintDialog.gxs_id = model.gxs_id
					fingerPrintDialog.visible = true
				}
			}
		}
		Text
		{
			id: nickText
			color: model.own ? "blue" : "black"
			text: model.name
			anchors.left: colorHash.right
			anchors.leftMargin: 5
			anchors.verticalCenter: parent.verticalCenter
		}
		Rectangle
		{
			visible: contactsView.unreadMessages.hasOwnProperty(model.gxs_id)

			anchors.right: parent.right
			anchors.rightMargin: 10
			anchors.verticalCenter: parent.verticalCenter
			color: "cornflowerblue"
			antialiasing: true
			border.color: "blue"
			border.width: 1
			radius: height/2
			height: parent.height - 10
			width: height

			Text
			{
				color: "white"
				font.bold: true
				text: contactsView.unreadMessages.hasOwnProperty(model.gxs_id) ?
						  contactsView.unreadMessages[model.gxs_id] : ''
				anchors.centerIn: parent
			}
		}
	}
}
