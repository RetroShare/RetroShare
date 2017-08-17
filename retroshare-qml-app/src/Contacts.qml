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
import "." //Needed for TokensManager singleton
import Qt.labs.settings 1.0

Item
{
	id: contactsView
	property string own_gxs_id: ""
	property string own_nick: ""
	property bool searching: false
	onSearchingChanged: !searching && contactsSortWorker.sendMessage({})

	property string objectName:"contactsView"

	Component.onCompleted:
	{
		refreshAll()
	}
	onFocusChanged: focus && refreshAll()

	function changeState ()
	{
		toolBar.state = "CONTACTSVIEW"
		toolBar.searchBtnCb = toggleSearchBox
	}

	WorkerScript
	{
		id: contactsSortWorker
		source: "qrc:/ContactSort.js"
		onMessage: contactsListModel.json = JSON.stringify(messageObject)
	}

	function toggleSearchBox (){
		if (searching) searching = false
		else searching = true
	}

	function refreshAll()
	{
		refreshOwn()
		refreshContacts()
		refreshUnread()
	}

	function refreshContactsCallback(par)
	{
		console.log("contactsView.refreshContactsCB()", visible)
		var token = JSON.parse(par.response).statetoken
		ChatCache.contactsCache.contactsList = JSON.parse(par.response).data
		TokensManager.registerToken(token, refreshContacts)
		contactsSortWorker.sendMessage(
					{'action': 'refreshContacts', 'response': par.response})
	}
	function refreshContacts()
	{
		console.log("contactsView.refreshContacts()", visible)
		if(!visible) return
		rsApi.request("/identity/*/", "", refreshContactsCallback)
	}

	function refreshOwnCallback(par)
	{
		console.log("contactsView.refreshOwnCallback(par)", visible)
		var json = JSON.parse(par.response)
		var token = json.statetoken
		TokensManager.registerToken(token, refreshOwn)

		if(json.data.length > 0)
		{
			ChatCache.contactsCache.own = json.data[0]
			contactsView.own_gxs_id = json.data[0].gxs_id
			contactsView.own_nick = json.data[0].name
			if(mainWindow.user_name.length === 0)
				mainWindow.user_name = json.data[0].name
		}
		else if (!settings.defaultIdentityCreated)
		{
			console.log("refreshOwnCallback(par)", "creating new identity" )
			settings.defaultIdentityCreated = true

			var jsonData = { "name": mainWindow.user_name, "pgp_linked": false }
			rsApi.request(
						"/identity/create_identity",
						JSON.stringify(jsonData),
						refreshOwn)
		}
	}
	function refreshOwn()
	{
		console.log("contactsView.refreshOwn()", visible)
		rsApi.request("/identity/own", "", refreshOwnCallback)
	}

	function refreshUnreadCallback(par)
	{
		console.log("contactsView.refreshUnreadCB()", visible)
		var json = JSON.parse(par.response)
		TokensManager.registerToken(json.statetoken, refreshUnread)
		contactsSortWorker.sendMessage(
					{'action': 'refreshUnread', 'response': par.response})
		json.data.forEach (function (chat)
		{
			ChatCache.lastMessageCache.updateLastMessageCache(chat.chat_id)
			ChatCache.lastMessageCache.setRemoteGXS (chat.chat_id, { gxs: chat.remote_author_id, name: chat.remote_author_name})
		})
	}
	function refreshUnread()
	{
		console.log("contactsView.refreshUnread()", visible)
		if(!visible) return
		rsApi.request("/chat/unread_msgs", "", refreshUnreadCallback)
	}

	/** This must be equivalent to
		p3GxsTunnelService::makeGxsTunnelId(...) */
	function getChatId(from_gxs, to_gxs)
	{
		return from_gxs < to_gxs ? from_gxs + to_gxs : to_gxs + from_gxs
	}

	JSONListModel
	{
		id: contactsListModel
		query: "$.data[*]"
	}

	ListView
	{
		id: contactsListView
		width: parent.width
		height: contactsView.searching ?
					parent.height - searchBox.height : parent.height
		model: contactsListModel.model
		anchors.top: contactsView.searching ? searchBox.bottom : parent.top
		delegate: GxsIdentityDelegate {}
	}

	Rectangle
	{
		id: searchBox
//		visible: contactsView.searching

		height: searchText.height + 10
		width: parent.width * 0.9
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.leftMargin: 5
		anchors.rightMargin: 5
		anchors.horizontalCenter: parent.horizontalCenter
		color: "white"

		TextField
		{
			id: searchText
			anchors.verticalCenter: parent.verticalCenter
//			placeholderText : "Search contacts..."
			width: parent.width
			anchors.leftMargin: 5
			height: 0

			background: Rectangle
			{
				border.width: 2
				radius: 5
				border.color: searchText.focus ? "cornflowerblue" : "lightgrey"
				color: searchText.focus ? "white" : "ghostwhite"
			}

			onTextChanged:
				contactsSortWorker.sendMessage(
					        {'action': 'searchContact', 'sexp': text})
		}

		states:
		[
			State
			{
				when: contactsView.searching;
				PropertyChanges {   target: searchText; height: implicitHeight  }
				PropertyChanges {   target: searchText; placeholderText : "Search contacts..." }
				PropertyChanges {   target: searchBox; height: searchText.height + 10   }
			},
			State
			{
				when: !contactsView.searching;
				PropertyChanges {   target: searchText; height: 0   }
				PropertyChanges {   target: searchBox; height: 0   }
			}
		]
		transitions: Transition
		{
			NumberAnimation { property: "height"; duration: 500; easing.type: Easing.InOutQuad}
		}
	}

	Text
	{
		id: selectedOwnIdentityView
		color: "green"
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		width: parent.width
		text: "Open Chat as: " + contactsView.own_nick + " " + contactsView.own_gxs_id
	}

	Settings
	{
		id: settings
		category: "contactsView"

		property bool defaultIdentityCreated: false
	}

}
