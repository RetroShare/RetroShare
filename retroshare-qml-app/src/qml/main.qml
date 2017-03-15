/*
 * RetroShare Android QML App
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
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

import QtQuick 2.2
import QtQuick.Controls 1.4
import org.retroshare.qml_components.LibresapiLocalClient 1.0

ApplicationWindow
{
	id: mainWindow
	visible: true
	title: qsTr("RSChat")
	width: 400
	height: 400

	property string activeChatId;

	Rectangle
	{
		id: mainView
		anchors.fill: parent
		states:
			[
			    State
			    {
					name: "waiting_account_select"
					PropertyChanges { target: swipeView; currentIndex: 0 }
					PropertyChanges { target: locationsTab; enabled: true }
				},
				State
				{
					name: "running_ok"
					PropertyChanges { target: swipeView; currentIndex: 1 }
					PropertyChanges { target: locationsTab; enabled: false }
				},
				State
				{
					name: "running_ok_no_full_control"
					PropertyChanges { target: swipeView; currentIndex: 1 }
					PropertyChanges { target: locationsTab; enabled: false }
				}
		]

		LibresapiLocalClient
		{
			onGoodResponseReceived:
			{
				var jsonReponse = JSON.parse(msg)
				mainView.state = jsonReponse.data.runstate
			}
			Component.onCompleted:
			{
				openConnection(apiSocketPath)
				request("/control/runstate/", "")
			}
		}

		TabView
		{
			id: swipeView
			anchors.fill: parent
			visible: true
			currentIndex: 0

			Tab
			{
				title:"Locations"
				id: locationsTab
				Locations { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Trusted Nodes"
				TrustedNodesView { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Contacts"
				Contacts { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Add Node"
				AddTrustedNode { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Chat"
				ChatView
				{
					id: chatView
					chatId: mainWindow.activeChatId
					onVisibleChanged: focus = visible
				}
			}
		}
	}
}
