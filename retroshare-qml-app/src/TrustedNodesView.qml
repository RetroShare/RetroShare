/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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
import "jsonpath.js" as JSONPath
import "." //Needed for TokensManager singleton
import "components/."

Item
{
	id: trustedNodesView
	property int token: 0

	Component.onCompleted: refreshData()
	Component.onDestruction: TokensManager.unRegisterToken(token, refreshData)
	onVisibleChanged: visible && refreshData()

	function refreshDataCallback(par)
	{
		jsonModel.json = par.response
		token = JSON.parse(par.response).statetoken
		TokensManager.registerToken(token, refreshData)
	}
	function refreshData()
	{ if(visible) rsApi.request("/peers/*", "", refreshDataCallback) }

	JSONListModel
	{
		id: jsonModel
		query: "$.data[*]"

		function isOnline(pgpId)
		{
			var qr = "$.data[?(@.pgp_id=='"+pgpId+"')].locations[*].is_online"
			var locOn = JSONPath.jsonPath(JSON.parse(jsonModel.json), qr)
			if (Array.isArray(locOn))
				return locOn.reduce(function(cur,acc){return cur || acc}, false)
			return Boolean(locOn)
		}

		function getLocations(pgpId)
		{
			var qr = "$.data[?(@.pgp_id=='"+pgpId+"')].locations"
			return JSONPath.jsonPath(JSON.parse(jsonModel.json), qr)
		}
	}

	ListView
	{
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: bottomButton.top
		model: jsonModel.model
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 3
		delegate: Item
		{
			property bool isOnline: jsonModel.isOnline(model.pgp_id)
			height: 54
			width: parent.width
			anchors.horizontalCenter: parent.horizontalCenter

			ButtonText
			{
				id: locationButton
//				anchors.horizontalCenter: parent.horizontalCenter
				text: model.name
				borderRadius:0
				iconUrl: isOnline?
							 "/icons/state-ok.svg" :
							 "/icons/state-offline.svg"
				color: "transparent"
				pressColor: "lightsteelblue"
				buttonTextPixelSize: 18
				iconHeight:parent.height - 4

				onClicked:
				{
					stackView.push(
								"qrc:/TrustedNodeDetails.qml",
								{
									pgpName: model.name,
									pgpId: model.pgp_id,
									isOnline: isOnline,
									locations: jsonModel.getLocations(
												   model.pgp_id)
								}
								)
				}
			}
		}
	}

	ButtonText
	{
		id: bottomButton
		text: qsTr("Add/Share Trusted Node")
		anchors.bottom: parent.bottom
		onClicked: stackView.push("qrc:/AddTrustedNode.qml")
		anchors.horizontalCenter: parent.horizontalCenter
//		width: parent.width

		borderRadius: 0
		buttonTextPixelSize: 14
		iconUrl: "/icons/add.svg"
	}
}
