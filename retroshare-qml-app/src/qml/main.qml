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

import QtQuick 2.2
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0

ApplicationWindow
{
	id: mainWindow
	visible: true
	title: "RetroShare"
	width: 400
	height: 400

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
				source: "qrc:/qml/icons/application-menu.png"
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
					//iconSource: "qrc:/qml/icons/document-share.png"
					onTriggered:
						stackView.push("qrc:/qml/TrustedNodesView.qml")
				}
				MenuItem
				{
					text: qsTr("Search Contacts")
					onTriggered:
						stackView.push(
							"qrc:/qml/Contacts.qml", {'searching': true} )
				}
				MenuItem
				{
					text: "Terminate Core"
					onTriggered: rsApi.request("/control/shutdown")
				}
			}
		}
	}

	StackView
	{
		id: stackView
		anchors.fill: parent
		Keys.onReleased:
			if (event.key === Qt.Key_Back && stackView.depth > 1)
			{
				stackView.pop();
				event.accepted = true;
			}

		function checkCoreStatus()
		{
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
			var ret = rsApi.request("/control/runstate/", "", runStateCallback)
			if ( ret < 1 )
			{
				console.log("checkCoreStatus() core is down")
				stackView.state = "core_down"
			}
		}

		Timer
		{
			id: refreshTimer
			interval: 800
			repeat: true
			triggeredOnStart: true
			onTriggered: if(stackView.visible) stackView.checkCoreStatus()
			Component.onCompleted: start()
		}

		state: "core_down"
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
						stackView.push("qrc:/qml/Locations.qml")
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
						console.log("StateChangeScript waiting_account_select")
						stackView.clear()
						stackView.push("qrc:/qml/Contacts.qml")
					}
				}
			},
			State
			{
				name: "running_ok_no_full_control"
				PropertyChanges { target: stackView; state: "running_ok" }
			}
		]

		initialItem: Rectangle
		{
			anchors.fill: parent
			color: "green"
			border.color: "black"

			Text { text: "Connecting to core..." }
		}
	}
}
