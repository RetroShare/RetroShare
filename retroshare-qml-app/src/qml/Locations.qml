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

import QtQuick 2.0
import QtQuick.Controls 1.4
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	id: locationView
	state: "selectLocation"
	property var qParent
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
			PropertyChanges	{ target: bottomButton; visible: false }
			PropertyChanges
			{
				target: loginView
				visible: true
				buttonText: "Save"
				onSubmit:
				{
					var jsonData = { pgp_name: login, ssl_name: login, pgp_password: password }
					rsApi.request("/control/create_location/", JSON.stringify(jsonData))
					locationView.state = "selectLocation"
				}
			}
		},
		State
		{
			name: "login"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges	{ target: bottomButton; visible: false }
			PropertyChanges
			{
				target: loginView
				visible: true
				onSubmit:
				{
					locationView.password = password
					console.log("locationView.sslid: ", locationView.sslid)
					rsApi.request("/control/login/", JSON.stringify({id: locationView.sslid}))
					locationView.attemptLogin = true
					busyIndicator.running = true
					attemptTimer.start()
				}
			}
		}
	]

	function requestLocationsList() { rsApi.request("/control/locations/", "") }

	onFocusChanged: focus && requestLocationsList()

	LibresapiLocalClient
	{
		id: rsApi
		Component.onCompleted:
		{
			openConnection(apiSocketPath)
			locationView.requestLocationsList()
		}
		onGoodResponseReceived:
		{
			var jsonData = JSON.parse(msg)


			if(jsonData)
			{
				if(jsonData.data)
				{
					if(jsonData.data[0] && jsonData.data[0].pgp_id)
					{
						// if location list update
						locationsModel.json = msg
						busyIndicator.running = false
					}
					if (jsonData.data.key_name)
					{
						if(jsonData.data.want_password)
						{
							// if Server requested password
							var jsonPass = { password: locationView.password }
							rsApi.request("/control/password/", JSON.stringify(jsonPass))
							locationView.attemptLogin = false
							console.debug("RS core asked for password")
						}
						else
						{
							// if Already logged in
							bottomButton.enabled = false
							bottomButton.text = "Already logged in"
							locationView.attemptLogin = false
							busyIndicator.running = false
							locationView.state = "selectLocation"
							locationsListView.enabled = false
							console.debug("Already logged in")
						}
					}
				}
			}
		}
	}

	BusyIndicator { id: busyIndicator; anchors.centerIn: parent }

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
		model: locationsModel.model
		delegate: Button
		{
		    text: model.name
			onClicked:
			{
				loginView.login = text
				locationView.sslid = model.id
				locationView.state = "login"
			}
	    }
	    visible: false
	}

    Button
    {
		id: bottomButton
		text: "Create new location"
		anchors.bottom: parent.bottom
		onClicked: locationView.state = "createLocation"
	}

	RsLoginPassView
	{
		id: loginView
		visible: false
		anchors.fill: parent
	}

	Timer
	{
		id: attemptTimer
		interval: 500
		repeat: true
		onTriggered:
		{
			if(locationView.focus)
				locationView.requestLocationsList()

			if (locationView.attemptLogin)
				rsApi.request("/control/password/", "")
		}
	}
}
