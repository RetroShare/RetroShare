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
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import "jsonpath.js" as JSONPath

Item
{
	id: trustedNodesView

	function refreshData()
	{
		rsApi.request("/peers/*", "",
					  function(par) { jsonModel.json = par.response })
	}
	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: jsonModel
		query: "$.data[*]"

		function isOnline(pgpId)
		{
			var qr = "$.data[?(@.pgp_id=='"+pgpId+"')].locations[*].is_online"
			var locArr = JSONPath.jsonPath(JSON.parse(jsonModel.json), qr)
			return locArr.reduce(function(cur,acc){return cur || acc}, false)
		}
	}

	ListView
	{
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: bottomButton.top
		model: jsonModel.model
		delegate: Item
		{
			height: 30
			width: parent.width

			Image
			{
				id: statusImage
				source: jsonModel.isOnline(model.pgp_id) ?
							"icons/state-ok.png" :
							"icons/state-offline.png"

				height: parent.height - 4
				fillMode: Image.PreserveAspectFit
				anchors.leftMargin: 6
				anchors.verticalCenter: parent.verticalCenter
			}
			Text
			{
				text: model.name
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: statusImage.right
				anchors.leftMargin: 10
			}
			Image
			{
				source: "icons/remove-link.png"

				height: parent.height - 6
				fillMode: Image.PreserveAspectFit

				anchors.right: parent.right
				anchors.rightMargin: 2
				anchors.verticalCenter: parent.verticalCenter

				MouseArea
				{
					height: parent.height
					width: parent.width
					onClicked:
					{
						deleteDialog.nodeName = model.name
						deleteDialog.nodeId = model.pgp_id
						deleteDialog.visible = true
					}
				}
			}
		}
	}

	Dialog
	{
		id: deleteDialog
		property string nodeName
		property string nodeId
		standardButtons: StandardButton.Yes | StandardButton.No
		visible: false
		onYes:
		{
			rsApi.request("/peers/"+nodeId+"/delete")
			trustedNodesView.refreshData()
			trustedNodesView.forceActiveFocus()
		}
		onNo: trustedNodesView.forceActiveFocus()
		Text
		{
			text: "Are you sure to delete " + deleteDialog.nodeName + " ("+
				  deleteDialog.nodeId +") ?"

			width: parent.width - 2
			wrapMode: Text.Wrap
		}
	}

	Button
	{
		id: bottomButton
		text: "Add Trusted Node"
		anchors.bottom: parent.bottom
		onClicked: stackView.push({item:"qrc:/qml/AddTrustedNode.qml"})
		width: parent.width
	}

	Timer
	{
		interval: 800
		repeat: true
		onTriggered: if(trustedNodesView.visible) trustedNodesView.refreshData()
		Component.onCompleted: start()
	}
}
