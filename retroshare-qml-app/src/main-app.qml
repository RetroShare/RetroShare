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
import "URI.js" as UriJs
import "." //Needed for TokensManager and ClipboardWrapper singleton

ApplicationWindow
{
	id: mainWindow
	visible: true
	title: "RetroShare"
	width: 400
	height: 400

	property string user_name

	property bool coreReady: stackView.state === "running_ok" ||
							 stackView.state === "running_ok_no_full_control"

	Component.onCompleted:
	{
		addUriHandler("/certificate", certificateLinkHandler)
		addUriHandler("/identity", contactLinkHandler)

		var argc = mainArgs.length
		for(var i=0; i<argc; ++i)
		{
			var dump = UriJs.URI.parse(mainArgs[i])
			if(dump.protocol && (dump.query || dump.path))
				handleIntentUri(mainArgs[i])
		}
	}

	property var uriHandlersRegister: ({})
	property var pendingUriRegister: []
	function addUriHandler(path, fun) { uriHandlersRegister[path] = fun }
	function delUriHandler(path, fun) { delete uriHandlersRegister[path] }

	header: ToolBar
	{
		id: toolBar

		Image
		{
			id: rsIcon
			fillMode: Image.PreserveAspectFit
			height: Math.max(30, parent.height - 4)
			anchors.verticalCenter: parent.verticalCenter
			source: "icons/retroshare06.png"
		}
		Label
		{
			text: "RetroShare"
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: rsIcon.right
			anchors.leftMargin: 20
		}
		MouseArea
		{
			height: parent.height
			width: parent.height
			anchors.right: parent.right
			anchors.rightMargin: 2
			anchors.verticalCenter: parent.verticalCenter

			onClicked: menu.open()

			Image
			{
				source: "qrc:/icons/application-menu.png"
				height: parent.height - 10
				width: parent.height - 10
				anchors.centerIn: parent
			}

			Menu
			{
				id: menu
				y: parent.y + parent.height

				MenuItem
				{
					text: qsTr("Trusted Nodes")
					//iconSource: "qrc:/icons/document-share.png"
					onTriggered: stackView.push("qrc:/TrustedNodesView.qml")
					enabled: mainWindow.coreReady
				}
				MenuItem
				{
					text: qsTr("Search Contacts")
					onTriggered:
						stackView.push("qrc:/Contacts.qml",
									   {'searching': true} )
					enabled: mainWindow.coreReady
				}
				MenuItem
				{
					text: "Paste Link"
					onTriggered:
						handleIntentUri(ClipboardWrapper.getFromClipBoard())

					enabled: mainWindow.coreReady
				}
				MenuItem
				{
					text: "Terminate Core"
					onTriggered: rsApi.request("/control/shutdown")
					visible: !Q_OS_ANDROID
				}
			}
		}
	}

	StackView
	{
		id: stackView
		anchors.fill: parent
		focus: true
		onCurrentItemChanged: if (currentItem) currentItem.focus = true
		Keys.onReleased:
			if (event.key === Qt.Key_Back && stackView.depth > 1)
			{
				stackView.pop();
				event.accepted = true;
			}

		state: "core_down"
		initialItem: BusyOverlay { message: qsTr("Connecting to core...") }

		states: [
			State
			{
				name: "core_down"
				PropertyChanges { target: stackView; enabled: false }
			},
			State
			{
				name: "waiting_account_select"
				PropertyChanges { target: stackView; enabled: true }
				StateChangeScript
				{
					script:
					{
						console.log("StateChangeScript waiting_account_select")
						stackView.clear()
						stackView.push("qrc:/Locations.qml")
					}
				}
			},
			State
			{
				name: "waiting_startup"
				PropertyChanges { target: stackView; enabled: false }
				StateChangeScript
				{
					script:
					{
						console.log("StateChangeScript waiting_startup")
						stackView.clear()
						stackView.push("qrc:/BusyOverlay.qml",
									   { message: "Core initializing..."})
					}
				}
			},
			State
			{
				name: "running_ok"
				PropertyChanges { target: stackView; enabled: true }
				StateChangeScript
				{
					script:
					{
						console.log("StateChangeScript running_ok")
						coreStateCheckTimer.stop()
						stackView.clear()
						stackView.push("qrc:/Contacts.qml")
						while(mainWindow.pendingUriRegister.length > 0)
							mainWindow.handleIntentUri(
										mainWindow.pendingUriRegister.shift())
					}
				}
			},
			State
			{
				name: "running_ok_no_full_control"
				PropertyChanges { target: stackView; state: "running_ok" }
			}
		]
	}

	Timer
	{
		id: coreStateCheckTimer
		interval: 1000
		repeat: true
		triggeredOnStart: true
		onTriggered:
		{
			var ret = rsApi.request("/control/runstate/", "", runStateCallback)
			if ( ret < 1 )
			{
				console.log("checkCoreStatus() core is down")
				stackView.state = "core_down"
			}
		}
		Component.onCompleted: start()

		function runStateCallback(par)
		{
			var jsonReponse = JSON.parse(par.response)
			var runState = jsonReponse.data.runstate
			if(typeof(runState) === 'string') stackView.state = runState
			else
			{
				stackView.state = "core_down"
				console.log("runStateCallback(...) core is down")
			}
		}
	}

	function handleIntentUri(uriStr)
	{
		console.log("handleIntentUri(uriStr)")

		if(!Array.isArray(uriStr.match(/:\/\/[a-zA-Z.-]*\//g)))
		{
			/* RetroShare GUI produces links without hostname and only two
			 * slashes after scheme causing the first piece of the path part
			 * being interpreted as host, this is awckard and should be fixed in
			 * the GUI, in the meantime we add a slash for easier parsing, in
			 * case there is no hostname and just two slashes, we might consider
			 * to use +hostname+ part for some trick in the future, for example
			 * it could help other application to recognize retroshare link by
			 * putting a domain name there that has no meaning for retroshare
			 */
			uriStr = uriStr.replace("://", ":///")
		}

		var uri = new UriJs.URI(uriStr)
		var hPath = uri.path() // no nesting ATM segmentCoded()
		console.log(hPath)

		if(typeof uriHandlersRegister[hPath] == "function")
		{
			console.log("handleIntentUri(uriStr)", "found handler for path",
						hPath, uriHandlersRegister[hPath])
			uriHandlersRegister[hPath](uriStr)
		}
	}

	function certificateLinkHandler(uriStr)
	{
		console.log("certificateLinkHandler(uriStr)", coreReady)

		if(!coreReady)
		{
			// Save cert uri for later processing as we need core to examine it
			pendingUriRegister.push(uriStr)
			return
		}

		var uri = new UriJs.URI(uriStr)
		var uQuery = uri.search(true)
		if(uQuery.radix)
		{
			var certStr = UriJs.URI.decode(uQuery.radix)

			// Workaround https://github.com/RetroShare/RetroShare/issues/772
			certStr = certStr.replace(/ /g, "+")

			rsApi.request(
						"/peers/examine_cert/",
						JSON.stringify({cert_string: certStr}),
						function(par)
						{
							console.log("/peers/examine_cert/ CB", par)
							var jData = JSON.parse(par.response).data
							stackView.push(
										"qrc:/TrustedNodeDetails.qml",
										{
											nodeCert: certStr,
											pgpName: jData.name,
											pgpId: jData.pgp_id,
											locations:
												[{
													location: jData.location,
													peer_id: jData.peer_id
												}]
										}
										)
						}
						)
		}
	}

	function contactLinkHandler(uriStr)
	{
		console.log("contactLinkHandler(uriStr)", coreReady)

		if(!coreReady)
		{
			// Save cert uri for later processing as we need core to examine it
			pendingUriRegister.push(uriStr)
			return
		}

		var uri = new UriJs.URI(uriStr)
		var uQuery = uri.search(true)
		if(uQuery.groupdata)
		{
			contactImportPopup.expectedName = uQuery.name
			contactImportPopup.expectedGxsId = uQuery.gxsid

			rsApi.request(
						"/identity/import_key",
						JSON.stringify({radix: uQuery.groupdata}),
						function(par)
						{
							var jD = JSON.parse(par.response).data
							contactImportPopup.realGxsId = jD.gxs_id
							contactImportPopup.open()
						}
						)
		}
	}
	Popup
	{
		id: contactImportPopup
		property string expectedName
		property string expectedGxsId
		property string realGxsId

		function idMatch() { return expectedGxsId === realGxsId }

		visible: false
		onVisibleChanged: if(visible && idMatch()) contactImportTimer.start()

		x: parent.x + parent.width/2 - width/2
		y: parent.y + parent.height/2 - height/2

		Column
		{
			spacing: 3
			anchors.centerIn: parent

			Text
			{
				text: qsTr("%1 key imported").arg(
						  contactImportPopup.expectedName)
				anchors.horizontalCenter: parent.horizontalCenter
			}

			Text
			{
				text: qsTr("Link malformed!")
				color: "red"
				visible: contactImportPopup.visible &&
						 !contactImportPopup.idMatch()
				anchors.horizontalCenter: parent.horizontalCenter
			}

			Text
			{
				text:
					qsTr("Expected id and real one differs:") +
					"<br/><pre>" + contactImportPopup.expectedGxsId +
					"<br/>" + contactImportPopup.realGxsId + "</pre>"
				visible: contactImportPopup.visible &&
						 !contactImportPopup.idMatch()
				anchors.horizontalCenter: parent.horizontalCenter
			}
		}

		Timer
		{
			id: contactImportTimer
			interval: 1500
			onTriggered: contactImportPopup.close()
		}
	}

	Popup
	{
		id: linkCopiedPopup
		property string itemName

		visible: false
		onVisibleChanged: if(visible) contactLinkTimer.start()

		x: parent.x + parent.width/2 - width/2
		y: parent.y + parent.height/2 - height/2

		Text
		{
			text:
				qsTr("%1 link copied to clipboard").arg(
					linkCopiedPopup.itemName)
		}

		Timer
		{
			id: contactLinkTimer
			interval: 1500
			onTriggered: linkCopiedPopup.close()
		}
	}
}
