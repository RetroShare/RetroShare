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
import "components/."

Item
{
	property ApplicationWindow mW

	property int infoIconHeight: 20

	Column
	{
		anchors.fill: parent
		spacing: 7

		Text
		{
			text: qsTr("Import node from clipboard")
			font.bold: true
			wrapMode: Text.Wrap
			anchors.horizontalCenter: parent.horizontalCenter

			font.pixelSize: 13

			Rectangle
			{
				id: backgroundRectangle
				anchors.fill: parent.fill
				anchors.horizontalCenter: parent.horizontalCenter
				width: parent.width + 20
				height: parent.height + 5

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
		}
		Row
		{
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 5

			ButtonIcon
			{
				height: infoIconHeight
				anchors.verticalCenter: importButton.verticalCenter
				width:  height
				imgUrl: "/icons/info.svg"
				onClicked:
				{
					tooltipSpace.show(qsTr("Import a friend certificate from your clipboard. <br>"+
										   "This will add him as trusted node."))

				}
			}

			ButtonText
			{
				id: importButton
				text: qsTr("Import trusted node")
				fontSize: 14

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
		}

		Text
		{
			text: qsTr("Export node")
			font.bold: true
			wrapMode: Text.Wrap
			anchors.horizontalCenter: parent.horizontalCenter

			font.pixelSize: 13

			Rectangle
			{
				anchors.fill: parent.fill
				anchors.horizontalCenter: parent.horizontalCenter
				width: parent.width + 20
				height: parent.height + 5

				color:"transparent"

				Rectangle
				{
					    width:  parent.width
						height: 1
						anchors.bottom: parent.bottom
						anchors.right: parent.right
						color: "lightgrey"
				}
			}
		}

		Row
		{
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 5

			ButtonIcon
			{
				height: infoIconHeight
				anchors.verticalCenter: btRsCert.verticalCenter
				width:  height
				imgUrl: "/icons/info.svg"
				onClicked:
				{
                    tooltipSpace.show(qsTr("Share your UnseenP2P link! <br>"+
										   "Send it to a friend and start talk!"))

				}
			}

			ButtonText
			{
				id: btRsCert
				text: qsTr("Export own certificate link")
				fontSize: 14
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
		}

		ButtonText
		{
			onClicked: plainCertificateRow.visible = !plainCertificateRow.visible
			iconUrl: "/icons/options.svg"
			anchors.horizontalCenter: parent.horizontalCenter
			color: "white"
			borderWidth: 1
			text: qsTr("Advanced")

		}

		Row
		{
			id: plainCertificateRow
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 5
			visible: false

			ButtonIcon
			{
				height: infoIconHeight
				anchors.verticalCenter: btPlainCert.verticalCenter
				width:  height
				imgUrl: "/icons/info.svg"
				onClicked:
				{
                    tooltipSpace.show(qsTr("This will copy your UnseenP2P plain certificate.<br>"+
										   "Add it manually to your friend client."))

				}
			}

			ButtonText
			{
				id: btPlainCert
				text: qsTr("Export own plain certificate")
				fontSize: 14
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


	}

	Rectangle
	{
		id: tooltipSpace
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter

		ToolTip
		{
			id: infoTooltip
			timeout: 5000
			Component.onCompleted: show("afafsaf",0)
		}
		function show (infoText)
		{
			infoTooltip.text = infoText
			infoTooltip.open()
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
