/*
 * RetroShare Android QML App
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
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
import "." //Needed for AppSettings singleton


Item
{
	Column
	{
		spacing: 5
		padding: 10

		Row
		{
			spacing: 10

			Label
			{
				text: "DHT mode:"
				anchors.verticalCenter: parent.verticalCenter
			}

			ComboBox
			{
				model: ["On", "Off", "Interactive"]
				Component.onCompleted: currentIndex = find(AppSettings.dhtMode)
				onActivated:
				{
					AppSettings.dhtMode = displayText
					focus = false
				}
			}
		}
	}
}
