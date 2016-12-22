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
import "jsonpath.js" as JSONPath

Item
{
	function refreshData() { rsApi.request("/peers/*", "", function(par) { jsonModel.json = par.response }) }
	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: jsonModel
		query: "$.data[*]"
	}

	ListView
	{
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: bottomButton.top
		model: jsonModel.model
		delegate: Item
		{
			height: 50
			Row
			{
				height: 30
				Text
				{
					text: model.name
					onTextChanged: color = JSONPath.jsonPath(JSON.parse(jsonModel.json), "$.data[?(@.pgp_id=='"+model.pgp_id+"')].locations[*].is_online").reduce(function(cur,acc){return cur || acc}, false) ? "lime" : "darkslategray"
				}

				Rectangle
				{
					height: parent.height
					width: parent.height
					color: "red"

					MouseArea
					{
						height: parent.height
						width: parent.height
						onClicked: rsApi.request("/peers/"+model.pgp_id+"/delete")
					}
				}
			}
		}
	}

	Button
	{
		id: bottomButton
		text: "Add Trusted Node"
		anchors.bottom: parent.bottom
		onClicked: swipeView.currentIndex = 3
	}
}
