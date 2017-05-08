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

Item
{
	height: 40
	width: parent.width

	MouseArea
	{
		anchors.fill: parent
		onClicked:
		{
			console.log("GxsIntentityDelegate onclicked:", model.name,
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

		onPressAndHold:
		{
			console.log("GxsIntentityDelegate onPressAndHold:", model.name,
						model.gxs_id)
			contactsView.searching = false
			stackView.push(
						"qrc:/ContactDetails.qml",
						{md: contactsListView.model.get(index)})
		}

		ColorHash
		{
			id: colorHash

			hash: model.gxs_id
			height: parent.height - 4
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
			anchors.leftMargin: 2
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

		Row
		{
			anchors.right: parent.right
			anchors.rightMargin: 10
			anchors.verticalCenter: parent.verticalCenter
			height: parent.height - 10

			Rectangle
			{
				visible: model.unread_count > 0

				color: "cornflowerblue"
				antialiasing: true
				border.color: "blue"
				border.width: 1
				height: parent.height
				radius: height/2
				width: height

				Text
				{
					color: "white"
					font.bold: true
					text: model.unread_count > 0 ? model.unread_count : ''
					anchors.centerIn: parent
				}
			}

			Image
			{
				source: model.is_contact ?
							"qrc:/icons/rating.png" :
							"qrc:/icons/rating-unrated.png"
				height: parent.height - 4
				fillMode: Image.PreserveAspectFit
				anchors.verticalCenter: parent.verticalCenter
			}
		}
	}
}
