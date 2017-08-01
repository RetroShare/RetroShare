/*
 * RetroShare Android QML App
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
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


Rectangle
{
	color: "red"

	Image
	{
		id: testImg
		height: 300
		width: 300
	}

	Component.onCompleted:
	{
		rsApi.request(
					"/peers/d441e8890164a0f335ad75acc59b5a49/avatar_image",
					"", setImgCallback )
	}
	function setImgCallback(par)
	{
		testImg.source = "data:image/png;base64," + par.response
	}
}
