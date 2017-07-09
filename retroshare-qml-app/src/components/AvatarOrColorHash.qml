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
import "../" //Needed for Chat Cache


Item
{
	id: compRoot

	property string gxs_id
	signal clicked ()

	height: 130
	width: height


////////////// The following should be considered privates /////////////////////

	property bool has_avatar: false
	property bool default_image: false
	property int avatarAttemptCnt: 0
	property string noGxsImage: "/icons/retroshare06.png"

	function getDetails()
	{
		console.log("getDetails() ", compRoot.gxs_id )
		++compRoot.avatarAttemptCnt
		if (gxs_id)
		{
			default_image = false
			rsApi.request(
						"/identity/get_identity_details",
						JSON.stringify({ gxs_id: compRoot.gxs_id }),
						function(par)
						{
							var jData = JSON.parse(par.response).data
							setDetails(jData)
							if(!compRoot.has_avatar &&
									compRoot.avatarAttemptCnt < 3) getDetails()
						})
		}
		else
		{
			has_avatar = true
			default_image = true
			contactAvatar.source = noGxsImage
		}
	}
	function setDetails(data)
	{
		compRoot.has_avatar = data.avatar.length > 0
		if(compRoot.has_avatar)
		{
			contactAvatar.source =
					"data:image/png;base64," + data.avatar
		}
	}

	function showDetails()
	{
		console.log("showDetails() ", gxs_id)

		stackView.push(
					"qrc:/ContactDetails.qml",
					{md: ChatCache.contactsCache.getContactFromGxsId(gxs_id)})

	}

	Component.onCompleted: if(visible && (!has_avatar || default_image ) ) getDetails()

	onVisibleChanged: if(visible && (!has_avatar || default_image ) ) getDetails()

	Image
	{
		id: contactAvatar
		anchors.fill: parent
		visible: compRoot.has_avatar
	}

	ColorHash
	{
		anchors.fill: parent
		visible: !compRoot.has_avatar
		hash: compRoot.gxs_id
	}

	MouseArea
	{
		anchors.fill: parent
		onClicked:
		{
			compRoot.clicked()
			showDetails()
		}
	}
}
