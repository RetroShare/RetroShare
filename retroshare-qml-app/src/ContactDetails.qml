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

	Button
	{
		text: "Open f d "
		onClicked:
		{
			fileChooser.open()
		}
		CustomFileChooser
		{
			id: fileChooser
			onResultFileChanged:
			{
				console.log("Result file changed! " , resultFile)
			}

		}
	}

	ColorHash
	{
		id: colorHash

		anchors.top: parent.top
		anchors.topMargin: 6
		anchors.horizontalCenter: parent.horizontalCenter

		height: 150
		hash: cntDt.md.gxs_id
	}

	Column
	{
		anchors.top: colorHash.bottom
		anchors.topMargin: 6
		anchors.horizontalCenter: parent.horizontalCenter

		spacing: 6

		Row
		{
			height: 50
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 6

			Text
			{
				text: cntDt.md.name
				anchors.verticalCenter: parent.verticalCenter
			}

			Image
			{

				source: cntDt.is_contact ?
							"qrc:/icons/rating.png" :
							"qrc:/icons/rating-unrated.png"
				height: parent.height - 4
				fillMode: Image.PreserveAspectFit
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

		Button
		{
			text: qsTr("Contact full link")
			onClicked:
			{
				rsApi.request(
							"/identity/export_key",
							JSON.stringify({ gxs_id: cntDt.md.gxs_id }),
							function(par)
							{
								var jD = JSON.parse(par.response).data
								ClipboardWrapper.postToClipBoard(
										"retroshare://" +
										"identity?gxsid=" +
										cntDt.md.gxs_id +
										"&name=" +
										UriJs.URI.encode(cntDt.md.name) +
										"&groupdata=" +
										UriJs.URI.encode(jD.radix))
								linkCopiedPopup.itemName = cntDt.md.name
								linkCopiedPopup.visible = true
							}
							)
			}
		}

		Button
		{
			text: qsTr("Contact short link")
			enabled: false
		}
	}
}
