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
import QtQuick.Layouts 1.3
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	id: locationView
	state: "selectLocation"

	states:
	[
		State
		{
			name: "selectLocation"
			PropertyChanges { target: locationsListView; visible: true }
			PropertyChanges { target: createLocationView; visible: false }
			PropertyChanges
			{
				target: bottomButton
				text: "Create new location"
				onClicked: locationView.state = "createLocation"
			}
		},
		State
		{
			name: "createLocation"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges { target: createLocationView; visible: true }
			PropertyChanges
			{
				target: bottomButton
				text: "Save"
				onClicked:
				{
					var jsonData = { pgp_name: nameField.text, ssl_name: nameField.text, pgp_password: passwordField.text }
					rsApi.request("/control/create_location/", JSON.stringify(jsonData))
					onClicked: locationView.state = "savingLocation"
				}
			}
		},
		State
		{
			name: "savingLocation"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges { target: createLocationView; color: "grey" }
			PropertyChanges
			{
				target: bottomButton
				text: "Saving..."
				enabled: false
			}
		},
		State
		{
			name: "loggingIn"
			PropertyChanges { target: locationsListView; visible: false }
			PropertyChanges { target: createLocationView; visible: true }
			PropertyChanges { target: nameField; enabled: false}
			PropertyChanges
			{
				target: bottomButton
				text: "Login"
				enabled: true
				onClicked:
				{
					var jsonData = { id: nameField.sslid, autologin: false }
					rsApi.request("/control/login/", JSON.stringify(jsonData))
					jsonData = { password: passwordField.text }
					rsApi.request("/control/password/", JSON.stringify(jsonData))
				}
			}
		}
	]

	Component.onCompleted:
	{
		rsApi.openConnection(apiSocketPath)
		rsApi.request("/control/locations/", "")
	}

	LibresapiLocalClient
	{
		id: rsApi
		onGoodResponseReceived: locationsModel.json = msg
	}

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
			property string sslid: model.id
			onClicked:
			{
				locationView.state = "loggingIn"
				nameField.text = text
			}
	    }
	    visible: false
	}

    ColumnLayout
    {
		id: createLocationView
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: bottomButton.top
		visible: false

		Row { Text {text: "Name:" } TextField { id: nameField; property string sslid } }
		Row { Text {text: "Password:" } TextField { id: passwordField; echoMode: PasswordEchoOnEdit } }
	}

	Text { text: "Locations View"; anchors.bottom: bottomButton.top }

	Button
	{
		id: bottomButton
		text: "Create new location"
		anchors.bottom: parent.bottom
	}
}
