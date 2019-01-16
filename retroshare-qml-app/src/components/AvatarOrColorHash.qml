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
	property bool onlyCached: false
	signal clicked ()

	height: 130
	width: height


////////////// The following should be considered privates /////////////////////

	property bool has_avatar: false
	property bool default_image: false
	property int avatarAttemptCnt: 0
    property string noGxsImage: "/icons/icon.png"

	function getDetails()
	{
		console.log("getDetails() ", compRoot.gxs_id )
		++compRoot.avatarAttemptCnt
		if (gxs_id)
		{
			default_image = false
			var hasAvatarCached =  hasAvatar (ChatCache.contactsCache.getIdentityAvatar(gxs_id))
			if ( !hasAvatarCached )
			{
				rsApi.request(
							"/identity/get_identity_details",
							JSON.stringify({ gxs_id: compRoot.gxs_id }),
							function(par)
							{
								var jData = JSON.parse(par.response).data
								saveDetails(jData)
								if(!compRoot.has_avatar &&
										compRoot.avatarAttemptCnt < 3) getDetails()
							})
			}
			else
			{
				setImage(ChatCache.contactsCache.getIdentityDetails(gxs_id))
			}
		}
		else
		{
			has_avatar = true
			default_image = true
			contactAvatar.source = noGxsImage
		}
	}
	function saveDetails(data)
	{
		ChatCache.contactsCache.setIdentityDetails(data)
		setImage(data)

	}
	function setImage (data)
	{
		compRoot.has_avatar = hasAvatar (data.avatar)
		if(compRoot.has_avatar)
		{
			contactAvatar.source =
					"data:image/png;base64," + data.avatar
		}
	}

	function hasAvatar (avatar)
	{
		return  avatar.length > 0
	}

	function showDetails()
	{
		console.log("showDetails() ", gxs_id)

		if (stackView.currentItem.objectName != "contactDetails")
		{
			stackView.push(
						"qrc:/ContactDetails.qml",
						{md: ChatCache.contactsCache.getContactFromGxsId(gxs_id)})

		}
	}
	function refresh()
	{
		ChatCache.contactsCache.delIdentityAvatar(gxs_id)
		compRoot.avatarAttemptCnt = 0
		getDetails()

	}

	Component.onCompleted: startComponent ()

	onVisibleChanged: startComponent ()

	function startComponent ()
	{
		if (onlyCached && hasAvatar (ChatCache.contactsCache.getIdentityAvatar(gxs_id) ) )
		{
			console.log("load cached avatar")
			setImage(ChatCache.contactsCache.getIdentityDetails(gxs_id))

		}
		else if (!onlyCached)
		{
			if(visible && (!has_avatar || default_image ) ) getDetails()
		}
	}

	Image
	{
		id: contactAvatar
		anchors.fill: parent
		visible: compRoot.has_avatar
		fillMode: Image.PreserveAspectFit
	}

	Faces
	{
		visible: !compRoot.has_avatar
		hash: compRoot.gxs_id
		anchors.fill: parent
		iconSize: parent.height
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
