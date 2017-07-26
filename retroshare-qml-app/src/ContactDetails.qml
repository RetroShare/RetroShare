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
import "." //Needed for ClipboardWrapper singleton
import "./components"
import "URI.js" as UriJs

Item
{
	id: cntDt
	property var md
	property bool is_contact: cntDt.md.is_contact
	property bool isOwn:  cntDt.md.own
	property string objectName: "contactDetails"

	ButtonText
	{
		id: avatarPicker

		text: (isOwn)? qsTr("Change your Avatar") : qsTr("Start Chat!")

		anchors.top: parent.top
		anchors.horizontalCenter: parent.horizontalCenter

		buttonTextPixelSize: 14
		iconUrl: (isOwn)? "/icons/attach-image.svg": "/icons/chat-bubble.svg"
		borderRadius: 0


		onClicked:
		{
			if (isOwn) fileChooser.open()
			else startDistantChat ()
		}
		function startDistantChat ()
		{
			ChatCache.chatHelper.startDistantChat(ChatCache.contactsCache.own.gxs_id,
												  cntDt.md.gxs_id,
												  cntDt.md.name,
												  function (chatId)
												  {
													  stackView.push("qrc:/ChatView.qml", {'chatId': chatId})
												  })
		}
		CustomFileChooser
		{
			id: fileChooser
			onResultFileChanged:
			{
				console.log("Result file changed! " , resultFile)

				var base64Image = androidImagePicker.imageToBase64(resultFile)

				rsApi.request("/identity/set_avatar", JSON.stringify({"gxs_id": cntDt.md.gxs_id, "avatar": base64Image }),
							    function (par)
								{
									var jP  = JSON.parse(par.response)
									if (jP.returncode === "ok")
									{
										console.log("Avatar changed! ")
										topFace.refresh()
									}
								})
			}
		}
	}


	AvatarOrColorHash
	{
		id: topFace

		gxs_id: cntDt.md.gxs_id

		anchors.top: avatarPicker.bottom
		anchors.topMargin: 6
		anchors.horizontalCenter: parent.horizontalCenter
	}

	Column
	{
		anchors.top: topFace.bottom
		anchors.topMargin: 6
		anchors.horizontalCenter: parent.horizontalCenter

		spacing: 6

		Row
		{
			height: 50
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 6

			ColorHash
			{
				hash: cntDt.md.gxs_id
				height: parent.height - 10
				anchors.verticalCenter: parent.verticalCenter
			}

			Text
			{
				text: cntDt.md.name
				anchors.verticalCenter: parent.verticalCenter
			}

			Image
			{
				source: cntDt.is_contact ?
							"qrc:/icons/rating.svg" :
							"qrc:/icons/rating-unrated.svg"
				height: parent.height -4
				fillMode: Image.PreserveAspectFit
				sourceSize.height: height
				anchors.verticalCenter: parent.verticalCenter

				MouseArea
				{
					anchors.fill: parent

					onClicked:
					{
						var jDt = JSON.stringify({gxs_id: cntDt.md.gxs_id})
						if(cntDt.is_contact)
							rsApi.request("/identity/remove_contact", jDt, tgCt)
						else rsApi.request("/identity/add_contact", jDt, tgCt)
					}

					function tgCt() { cntDt.is_contact = !cntDt.is_contact }
				}
			}
		}

		Text
		{
			text: "<pre>"+cntDt.md.gxs_id+"</pre>"
			anchors.horizontalCenter: parent.horizontalCenter
		}

		Text
		{
			visible: cntDt.md.pgp_linked
			text: qsTr("Owned by: %1").arg(cntDt.md.pgp_id)
			anchors.horizontalCenter: parent.horizontalCenter
		}
	}

	Row
	{
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter

		spacing: 6

		ButtonText
		{
			text: qsTr("Contact full link")
			borderRadius: 0
			buttonTextPixelSize: 14
			onClicked:
			{
				rsApi.request(
							"/identity/export_key",
							JSON.stringify({ gxs_id: cntDt.md.gxs_id }),
							function(par)
							{
								var jD = JSON.parse(par.response).data
								var contactUrl = (
										"retroshare://" +
										"identity?gxsid=" +
										cntDt.md.gxs_id +
										"&name=" +
										UriJs.URI.encode(cntDt.md.name) +
										"&groupdata=" +
										UriJs.URI.encode(jD.radix) )
								ClipboardWrapper.postToClipBoard(contactUrl)
								linkCopiedPopup.itemName = cntDt.md.name
								linkCopiedPopup.visible = true
								platformGW.shareUrl(contactUrl);
							}
							)
			}
		}

		ButtonText
		{
			text: qsTr("Contact short link")
			enabled: false
			borderRadius: 0
			buttonTextPixelSize: 14
		}
	}
}
