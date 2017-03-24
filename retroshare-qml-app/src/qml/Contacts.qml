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
	property var unreadMessages: ({})

	Component.onCompleted: refreshOwn()

	function refreshData()
	{
		function refreshCallback(par)
		{
			gxsIdsModel.json = par.response
			if(contactsView.own_gxs_id == "") refreshOwn()
		}
		rsApi.request("/identity/*/", "", refreshCallback)
	}
	function refreshOwn()
	{
		rsApi.request("/identity/own", "", function(par)
		{
			var json = JSON.parse(par.response)
			if(json.data.length > 0)
			{
				contactsView.own_gxs_id = json.data[0].gxs_id
				contactsView.own_nick = json.data[0].name
			}
			else createIdentityDialog.visible = true
		})
	}
	function startChatCallback(par)
	{
		var chId = JSON.parse(par.response).data.chat_id
		stackView.push("qrc:/qml/ChatView.qml", {'chatId': chId})
	}

	function refreshUnread()
	{
		rsApi.request("/chat/unread_msgs", "", function(par)
		{
			var jsonData = JSON.parse(par.response).data
			var dataLen = jsonData.length
			if(JSON.stringify(unreadMessages) != JSON.stringify(jsonData))
			{
				unreadMessages = {}
				for ( var i=0; i<dataLen; ++i)
				{
					var el = jsonData[i]
					if(el.is_distant_chat_id)
						unreadMessages[el.remote_author_id] = el.unread_count
				}

				visualModel.resetSorting()
			}
		})
	}

	/** This must be equivalent to
		p3GxsTunnelService::makeGxsTunnelId(...) */
	function getChatId(from_gxs, to_gxs)
	{
		return from_gxs < to_gxs ? from_gxs + to_gxs : to_gxs + from_gxs
	}



	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: gxsIdsModel
		query: "$.data[*]"
	}

	DelegateModel
	{
/* More documentation about this is available at:
 * http://doc.qt.io/qt-5/qml-qtqml-models-delegatemodel.html
 * http://doc.qt.io/qt-5/qtquick-tutorials-dynamicview-dynamicview4-example.html
 * http://imaginativethinking.ca/use-qt-quicks-delegatemodelgroup/
 */
		id: visualModel
		model: gxsIdsModel.model

		property var lessThan:
		[
			function(left, right)
			{
				var lfun = unreadMessages.hasOwnProperty(left.gxs_id) ?
							unreadMessages[left.gxs_id] : 0
				var rgun = unreadMessages.hasOwnProperty(right.gxs_id) ?
							unreadMessages[right.gxs_id] : 0
				if( lfun !== rgun ) return lfun > rgun
				if(left.name !== right.name) return left.name < right.name
				return left.gxs_id < right.gxs_id
			},
			function(left, right)
			{
				if(searchText.length > 0)
				{
					var mtc = searchText.text.toLowerCase()
					var lfn = left.name.toLowerCase()
					var rgn = right.name.toLowerCase()
					var lfml = lfn.indexOf(mtc)
					var rgml = rgn.indexOf(mtc)
					if ( lfml !== rgml )
					{
						lfml = lfml >= 0 ? lfml : Number.MAX_VALUE
						rgml = rgml >= 0 ? rgml : Number.MAX_VALUE
						return lfml < rgml
					}
				}
				return lessThan[0](left, right)
			}
		]

		property int sortOrder: contactsView.searching ? 1 : 0
		onSortOrderChanged: resetSorting()

		property bool isSorting: false

		function insertPosition(lessThan, item)
		{
			var lower = 0
			var upper = items.count
			while (lower < upper)
			{
				var middle = Math.floor(lower + (upper - lower) / 2)
				var result = lessThan(item.model, items.get(middle).model);
				if (result) upper = middle
				else lower = middle + 1
			}
			return lower
		}

		function resetSorting() { items.setGroups(0, items.count, "unsorted") }

		function sort()
		{
			while (unsortedItems.count > 0)
			{
				var item = unsortedItems.get(0)
				var index = insertPosition(lessThan[visualModel.sortOrder],
										   item)
				item.groups = ["items"]
				items.move(item.itemsIndex, index)
			}
		}

		items.includeByDefault: false

		groups:
		[
			DelegateModelGroup
			{
				id: unsortedItems
				name: "unsorted"

				includeByDefault: true
				onChanged: visualModel.sort()
			}
		]

		delegate: GxsIdentityDelegate {}
	}

	ListView
	{
		id: locationsListView
		width: parent.width
		height: contactsView.searching ?
					parent.height - searchBox.height : parent.height
		model: visualModel
		anchors.top: contactsView.searching ? searchBox.bottom : parent.top
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
			onTextChanged: visualModel.resetSorting()
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

	Timer
	{
		id: refreshTimer
		interval: 5000
		repeat: true
		triggeredOnStart: true
		onTriggered:
			if(contactsView.visible)
			{
				contactsView.refreshUnread()
				contactsView.refreshData()
			}
		Component.onCompleted: start()
	}

	Dialog
	{
		id: createIdentityDialog
		visible: false
		title: "You need to create a GXS identity to chat!"
		standardButtons: StandardButton.Save

		onAccepted: rsApi.request("/identity/create_identity", JSON.stringify({"name":identityNameTE.text, "pgp_linked": !psdnmCheckBox.checked }))

		TextField
		{
			id: identityNameTE
			width: 300
		}

		Row
		{
			anchors.top: identityNameTE.bottom
			Text { text: "Pseudonymous: " }
			CheckBox { id: psdnmCheckBox; checked: true; enabled: false }
		}
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
