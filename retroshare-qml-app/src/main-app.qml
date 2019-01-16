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

import QtQuick 2.1
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import "URI.js" as UriJs
import "." //Needed for TokensManager and ClipboardWrapper singleton
import "components/."


ApplicationWindow
{
	id: mainWindow
	visible: true
    title: "UnseenP2P"
	width: 400
	height: 400

	property string user_name

	property bool coreReady: stackView.state === "running_ok" ||
							 stackView.state === "running_ok_no_full_control"

	Component.onCompleted:
	{
		addUriHandler("/certificate", certificateLinkHandler)
		addUriHandler("/identity", contactLinkHandler)
		addUriHandler("/contacts", openContactsViewLinkHandler)

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
		height: 50
		property alias titleText: toolBarText.text
		property alias loaderSource: imageLoader.sourceComponent
		property alias gxsSource: imageLoader.gxsSource
        property string defaultLabel: "UnseenP2P"

		property var iconsSize: (coreReady)? height - 10 : 0

		property var searchBtnCb

		function openMainPage ()
		{
			if (stackView.currentItem.objectName != "contactsView" )
			{
				stackView.push("qrc:/Contacts.qml")
			}
		}

		states:
		[
			State
			{
				name: "DEFAULT"
				PropertyChanges { target: toolBar; titleText: defaultLabel}
				PropertyChanges { target: toolBar; loaderSource: rsIcon}
			},
			State
			{
				name: "CHATVIEW"
				PropertyChanges { target: toolBarText; mouseA.visible: false }
				PropertyChanges { target: toolBar; loaderSource: userHash }
//				PropertyChanges { target: toolBar; backBtnVisible: true }
			},
			State
			{
				name: "CONTACTSVIEW"
				PropertyChanges { target: toolBar; titleText: defaultLabel}
				PropertyChanges { target: toolBar; loaderSource: rsIcon}
				PropertyChanges { target: searchIcon; searchIconVisibility: true}
			}
		]

		Item
		{
			id: tolbarLeftPadding
			width: 4
		}

		MouseArea
		{
			id: menu
			height: parent.height
			width: parent.height

			anchors.left: tolbarLeftPadding.right
			anchors.verticalCenter: parent.verticalCenter

			onClicked: sideBar.open()

			Image
			{
				source: "qrc:/icons/application-menu.svg"
				height: parent.height - 10
				width: height
				sourceSize.height: height
				sourceSize.width: height
				anchors.centerIn: parent
			}

			SideBar {
				id: sideBar
			}
		}



		Label
		{
			property alias mouseA: mouseA
			id: toolBarText
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: menu.right
			anchors.leftMargin: 20

			MouseArea {
				id: mouseA
				visible: true
				anchors.fill: parent
				onClicked: {  toolBar.openMainPage() }
			}

		}

		ButtonIcon
		{
			property bool searchIconVisibility: false
			property var onClickCB: function (){}

			id: searchIcon
			height: toolBar.iconsSize
			width: toolBar.iconsSize
			anchors.verticalCenter: parent.verticalCenter
			imgUrl: "qrc:/icons/search.svg"
			anchors.right: imageLoader.left
			anchors.rightMargin: 5
			visible: searchIconVisibility && coreReady
			onClicked:
			{
				toolBar.searchBtnCb()
			}
		}

		Loader
		{
			id: imageLoader
			height:  toolBar.height - 4

			asynchronous: true

			anchors.right: parent.right
			anchors.rightMargin: 2
			anchors.verticalCenter: parent.verticalCenter

			property string gxsSource;

		}

		Component
		{
			id: rsIcon
			ButtonIcon
			{
				height: imageLoader.height
				width: imageLoader.height
				fillMode: Image.PreserveAspectFit
                imgUrl: "/icons/icon.png"
				onClicked:
				{
					if (coreReady) toolBar.openMainPage()
				}
			}
		}

		Component
		{
			id: userHash

			AvatarOrColorHash
			{
				id: colorHash

				gxs_id: imageLoader.gxsSource
				height: toolBar.height - 4
				anchors.leftMargin: 2
			}

		}

	}

	DropShadow
	{
		anchors.fill: toolBar
		horizontalOffset: 0
		verticalOffset: 1
		radius: 12
		samples: 25
		color: "#80000000"
		source: toolBar
	}

	StackView
	{
		id: stackView
		anchors.fill: parent
		anchors.top: toolBar.bottom
		anchors.topMargin: 5
		focus: true
		onCurrentItemChanged:
		{
			if (currentItem)
			{
				setStatus  (currentItem)
			}
		}

		Keys.onReleased:
		{
			if ((event.key === Qt.Key_Back ||  Qt.Key_Backspace)   && stackView.depth > 1)
			{
				stackView.pop();
				event.accepted = true;
				setStatus (stackView.currentItem)
			}
		}

		function setStatus (currentItem)
		{
			if (currentItem)
			{
				if (currentItem.objectName != "chatView" &&
						currentItem.objectName != "contactsView" &&
						toolBar.state != "DEFAULT")
				{
					 toolBar.state = "DEFAULT"
				}
				else if (typeof currentItem.changeState === 'function')
				{
					currentItem.changeState ()
				}

				currentItem.focus = true
			}
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
		console.log("handleIntentUri(uriStr)", uriStr)

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
		console.log("hPath", hPath)

		var authority = uri.authority()
		console.log("authority", authority)

		if(typeof uriHandlersRegister[hPath] == "function")
		{
			console.log("handleIntentUri(uriStr)", "found handler for path",
						hPath, uriHandlersRegister[hPath])
			uriHandlersRegister[hPath](uriStr)
		}

		else if (typeof uriHandlersRegister[authority] == "function" )
		{
			console.log("handleIntentUri(uriStr)", "found handler for path",
						authority, uriHandlersRegister[authority])
			uriHandlersRegister[authority](uriStr)
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

	function openContactsViewLinkHandler (uriStr)
	{
		console.log("openContactsViewLinkHandler(uriStr)" , uriStr)
		if(coreReady)
		{
			var uri = new UriJs.URI(uriStr)
			var query = UriJs.URI.parseQuery(uri.search());
			if (query.gxsId && query.name)
			{

				ChatCache.chatHelper.startDistantChat(ChatCache.contactsCache.own.gxs_id,
													  query.gxsId,
													  query.name,
													  function (chatId)
													  {
														  stackView.push("qrc:/ChatView.qml", {'chatId': chatId})
													  })
			}
			else
			{
				stackView.push("qrc:/Contacts.qml" )
			}
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

	FontLoader { id: emojiFont; source: "/fonts/OpenSansEmoji.ttf" }

	QtObject
	{
		id: theme

		property var emojiFontName: emojiFont.name

		property var supportedEmojiFonts: ["Android Emoji"]
		property var rootFontName: emojiFont.name

		// If native emoji font exists use it, else use RS emoji font
		function selectFont ()
		{
			var fontFamilies =  Qt.fontFamilies()
			fontFamilies.some(function (f)
			{
				if (supportedEmojiFonts.indexOf(f) !== -1)
				{
					emojiFontName = f
					return true
				}
				return false
			})
		}

		Component.onCompleted:
		{
			selectFont()
		}
	}

	property var netStatus: ({})
	function refreshNetStatus(optCallback)
	{
		console.log("refreshNetStatus(optCallback)")
		rsApi.request("/peers/get_network_options", "", function(par)
		{
			var json = JSON.parse(par.response)
			mainWindow.netStatus = json.data
			if(typeof(optCallback) === "function") optCallback();
		})
	}

	function updateDhtMode(stopping)
	{
		console.log("updateDhtMode(stopping)", stopping)

		switch(AppSettings.dhtMode)
		{
		case "On": mainWindow.netStatus.discovery_mode = 0; break;
		case "Off": mainWindow.netStatus.discovery_mode = 1; break;
		case "Interactive": mainWindow.netStatus.discovery_mode = stopping ? 1:0; break
		}

		console.log("updateDhtMode(stopping)", stopping, netStatus.discovery_mode)

		rsApi.request("/peers/set_network_options", JSON.stringify(netStatus))
	}

	Component.onDestruction: if(coreReady) updateDhtMode(true)


	Connections
	{
		target: AppSettings
		onDhtModeChanged:
		{
			Qt.application.state
			console.log("onDhtModeChanged", AppSettings.dhtMode)
			if(coreReady) updateDhtMode(false)
		}
	}

	Connections
	{
		target: stackView
		onStateChanged: if(coreReady) refreshNetStatus(function() {updateDhtMode(false)})
	}

	Connections
	{
		target: Qt.application
		onStateChanged:
		{
			console.log("Qt.application.state changed to", Qt.ApplicationSuspended)
			if(coreReady && (Qt.application.state === Qt.ApplicationSuspended))
				updateDhtMode(true)
		}
	}
}
