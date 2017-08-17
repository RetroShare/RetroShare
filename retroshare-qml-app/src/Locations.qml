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

import QtQuick 2.7
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0

import "components/."

Item
{
	id: locationView
	state: "selectLocation"
	property bool attemptLogin: false
	property string password
	property string sslid

	states:
	[
		State
		{
			name: "selectLocation"
			PropertyChanges { target: locationsListView; visible: true }
			PropertyChanges { target: bottomButton; visible: true }
			PropertyChanges { target: loginView; visible: false }
		},
		State
		{
			name: "createLocation"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges { target: bottomButton; visible: false }
			PropertyChanges
			{
				target: loginView
				visible: true
				buttonText: qsTr("Save")
				iconUrl: "qrc:/icons/edit-image-face-detect.svg"
				suggestionText: qsTr("Create your profile")
				onSubmit:
				{
					busyIndicator.running = true
					var jsonData = { pgp_name: login, ssl_name: login,
						pgp_password: password }
					rsApi.request(
								"/control/create_location/",
								JSON.stringify(jsonData))
					mainWindow.user_name = login
					locationView.state = "selectLocation"
					bottomButton.enabled = false
					bottomButton.text = "Creating profile..."
				}
				onCancel:
				{
					locationView.state = "selectLocation"
				}

			}
		},
		State
		{
			name: "login"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges { target: bottomButton; visible: false }
			PropertyChanges
			{
				target: loginView
				visible: true
				advancedMode: true
				onSubmit:
				{
					locationView.password = password
					console.log("locationView.sslid: ", locationView.sslid)
					rsApi.request( "/control/login/",
								   JSON.stringify({id: locationView.sslid}) )
					locationView.attemptLogin = true
					attemptTimer.start()
				}
				onCancel:
				{
					locationView.state = "selectLocation"
				}
			}
		}
	]

	function requestLocationsListCB(par)
	{
		var jsonData = JSON.parse(par.response).data
		if(jsonData.length === 1)
		{
			// There is only one location so we can jump selecting location
			var location = jsonData[0]
			loginView.login = location.name
			mainWindow.user_name = location.name
			locationView.sslid = location.peer_id
			locationView.state = "login"
		}
		else if (jsonData.length === 0)
		{
			// The user haven't created a location yet
			locationView.state = "createLocation"
		}
		else
		{
			// There is more then one location to choose from
			locationsModel.json = par.response
		}
	}
	function requestLocationsList()
	{ rsApi.request("/control/locations/", "", requestLocationsListCB) }

	onFocusChanged: focus && requestLocationsList()
	Component.onCompleted: requestLocationsList()

	JSONListModel
	{
		id: locationsModel
		query: "$.data[*]"
	}
	ListView
	{
		id: locationsListView
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: bottomButton.top
		anchors.horizontalCenter: parent.horizontalCenter
		model: locationsModel.model
		spacing: 3
		delegate: Item
		{
			id: delegate
			width: parent.width
			height: 60
			Rectangle
			{
				id: backgroundRectangle
				anchors.fill: parent.fill
				anchors.horizontalCenter: parent.horizontalCenter
				width: parent.width /2
				height: parent.height

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

			ButtonText
			{
				id: locationButton
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
				text: model.name
				borderRadius:0
				iconUrl: "/icons/edit-image-face-detect.svg"
				color: "white"
				pressColor: "lightsteelblue"
				buttonTextPixelSize: 20

				onClicked:
				{
					loginView.login = text
					locationView.sslid = model.id
					locationView.state = "login"
					mainWindow.user_name = model.name
				}
			}
		}
		visible: false
	}

	ButtonText
	{
		id: bottomButton
		text: "Create new location"
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter
		onClicked: locationView.state = "createLocation"
		buttonTextPixelSize: 15
		iconUrl: "/icons/add.svg"
		borderRadius: 0
	}

	RsLoginPassView
	{
		id: loginView
		visible: false
		anchors.fill: parent
	}

	BusyIndicator
	{
		id: busyIndicator
		anchors.centerIn: parent
		running: false

		Connections
		{
			target: locationView
			onAttemptLoginChanged:
				if(locationView.attemptLogin) busyIndicator.running = true
		}
	}

	LibresapiLocalClient
	{
		id: loginApi
		Component.onCompleted: openConnection(apiSocketPath)
		onGoodResponseReceived:
		{
			var jsonData = JSON.parse(msg)
			if(jsonData && jsonData.data  && jsonData.data.key_name)
			{
				if(jsonData.data.want_password)
				{
					// if Server requested password
					var jsonPass = { password: locationView.password }
					request( "/control/password/", JSON.stringify(jsonPass) )
					locationView.attemptLogin = false
					console.debug("RS core asked for password")
				}
				else
				{
					// if Already logged in
					bottomButton.enabled = false
					bottomButton.text = "Unlocking location..."
					locationView.attemptLogin = false
					locationView.state = "selectLocation"
					locationsListView.enabled = false
					console.debug("Already logged in")
				}
			}
		}
	}
	Timer
	{
		id: attemptTimer
		interval: 1000
		repeat: true
		triggeredOnStart: true
		onTriggered:
		{
			if (locationView.attemptLogin)
				loginApi.request("/control/password/", "")
		}
	}
}
