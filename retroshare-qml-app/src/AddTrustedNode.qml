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
import "." //Needed for ClipboardWrapper singleton
import "URI.js" as UriJs

Item
{
	property ApplicationWindow mW

	Column
	{
		anchors.fill: parent

		Text
		{
			text: qsTr("Import/export node from/to clipboard")
			font.bold: true
			wrapMode: Text.Wrap
		}

		Button
		{
			text: qsTr("Export own certificate link")
			onClicked:
			{
				console.log("onClicked", text)
				rsApi.request(
					"/peers/self/certificate/", "",
					function(par)
					{
						var radix = JSON.parse(par.response).data.cert_string
						var name = mainWindow.user_name
						var encodedName = UriJs.URI.encode(name)
						var nodeUrl = (
							"retroshare://certificate?" +
							"name=" + encodedName +
							"&radix=" + UriJs.URI.encode(radix) +
							"&location=" + encodedName )
						ClipboardWrapper.postToClipBoard(nodeUrl)
						linkCopiedPopup.itemName = name
						linkCopiedPopup.open()
						platformGW.shareUrl(nodeUrl);
					})
			}
		}

		Button
		{
			text: qsTr("Import trusted node")

			onClicked:
			{
				var cptext = ClipboardWrapper.getFromClipBoard()

				console.log("typeof(cptext)", typeof(cptext))
				if(cptext.search("://") > 0)
					mainWindow.handleIntentUri(cptext)
				else
					rsApi.request(
							"/peers/examine_cert/",
							JSON.stringify({cert_string: cptext}),
							function(par)
							{
								console.log("/peers/examine_cert/ CB",
											par.response)
								var resp = JSON.parse(par.response)
								if(resp.returncode === "fail")
								{
									importErrorPop.text = resp.debug_msg
									importErrorPop.open()
									return
								}

								var jData = resp.data
								stackView.push(
											"qrc:/TrustedNodeDetails.qml",
											{
												nodeCert: cptext,
												pgpName: jData.name,
												pgpId: jData.pgp_id,
												locationName: jData.location,
												sslIdTxt: jData.peer_id
											}
											)
							}
							)
			}
		}

		Button
		{
			text: qsTr("Export own plain certificate")
			onClicked:
			{
				rsApi.request(
					"/peers/self/certificate/", "",
					function(par)
					{
						var radix = JSON.parse(par.response).data.cert_string
						var name = mainWindow.user_name
						ClipboardWrapper.postToClipBoard(radix)

						linkCopiedPopup.itemName = name
						linkCopiedPopup.open()
					})
			}
		}
	}

	TimedPopup
	{
		id: importErrorPop
		property alias text: popText.text

		Text
		{
			id: popText
			anchors.fill: parent
		}
	}
}
