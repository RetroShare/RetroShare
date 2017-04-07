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

import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
import QtQml.Models 2.2
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	id: contactsView
	property string own_gxs_id: ""
	property string own_nick: ""
	property bool searching: false
	onSearchingChanged: !searching && contactsSortWorker.sendMessage({})

	Component.onCompleted: refreshAll()
	onFocusChanged: focus && refreshAll()

	WorkerScript
	{
		id: contactsSortWorker
		source: "qrc:/qml/ContactSort.js"
		onMessage: contactsListModel.json = JSON.stringify(messageObject)
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
		if (contactsListModel.model.count < 1)
			contactsListModel.json = par.response
		var token = JSON.parse(par.response).statetoken
		mainWindow.registerToken(token, refreshContacts)
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
		mainWindow.registerToken(token, refreshOwn)

		if(json.data.length > 0)
		{
			contactsView.own_gxs_id = json.data[0].gxs_id
			contactsView.own_nick = json.data[0].name
		}
		else
		{
			var jsonData = { "name": mainWindow.pgp_name, "pgp_linked": false }
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
		mainWindow.registerToken(json.statetoken, refreshUnread)
		contactsSortWorker.sendMessage(
					{'action': 'refreshUnread', 'response': par.response})
	}
	function refreshUnread()
	{
		console.log("contactsView.refreshUnread()", visible)
		if(!visible) return
		rsApi.request("/chat/unread_msgs", "", refreshUnreadCallback)
	}

	function startChatCallback(par)
	{
		var chId = JSON.parse(par.response).data.chat_id
		stackView.push("qrc:/qml/ChatView.qml", {'chatId': chId})
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
		id: locationsListView
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
		visible: contactsView.searching

		height: searchText.height
		width: searchText.width
		anchors.right: parent.right
		anchors.top: parent.top

		Image
		{
			id: searchIcon
			height: searchText.height - 4
			width: searchText.height - 4
			anchors.verticalCenter: parent.verticalCenter
			source: "qrc:/qml/icons/edit-find.png"
		}

		TextField
		{
			id: searchText
			anchors.left: searchIcon.right
			anchors.verticalCenter: parent.verticalCenter
			onTextChanged:
				contactsSortWorker.sendMessage(
					        {'action': 'searchContact', 'sexp': text})
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

	Popup
	{
		id: fingerPrintDialog
		visible: false
		property string nick
		property string gxs_id
		width: fingerPrintText.contentWidth + 20
		height: fingerPrintText.contentHeight + 20
		x: parent.x + parent.width/2 - width/2
		y: parent.y + parent.height/2 - height/2

		Text
		{
			id: fingerPrintText
			anchors.centerIn: parent
			text: "<pre>" +
				  fingerPrintDialog.gxs_id.substring(1, 9) + "<br/>" +
				  fingerPrintDialog.gxs_id.substring(9, 17) + "<br/>" +
				  fingerPrintDialog.gxs_id.substring(17, 25) + "<br/>" +
				  fingerPrintDialog.gxs_id.slice(-8) + "<br/>" +
				  "</pre>"
		}
	}
}
