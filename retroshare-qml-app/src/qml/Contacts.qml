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
	function refreshData() { rsApi.request("/identity/*/", "") }

	onFocusChanged: focus && refreshData()

	LibresapiLocalClient
	{
		id: rsApi
		onGoodResponseReceived: locationsModel.json = msg
		Component.onCompleted: { openConnection(apiSocketPath) }
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
		height: 300
		model: locationsModel.model
		delegate: Text { text: model.name }
	}

	Text { text: "Contacts View"; anchors.bottom: parent.bottom }
}
