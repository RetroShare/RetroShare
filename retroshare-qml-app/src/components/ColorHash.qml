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
	property string hash

	width: height
	property int childHeight : height/2
	color: "white"

	Image
	{
		source: "qrc:/icons/edit-image-face-detect.svg"
		anchors.centerIn: parent
		height: parent.height
		width: parent.width
		sourceSize.height: height
		sourceSize.width: width
		fillMode: Image.PreserveAspectFit
	}

	Rectangle
	{
		color: '#' + hash.substring(1, 9)
		height: parent.childHeight
		width: height
		anchors.top: parent.top
		anchors.left: parent.left
	}
	Rectangle
	{
		color: '#' + hash.substring(9, 17)
		height: parent.childHeight
		width: height
		anchors.top: parent.top
		anchors.right: parent.right
	}
	Rectangle
	{
		color: '#' + hash.substring(17, 25)
		height: parent.childHeight
		width: height
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	Rectangle
	{
		color: '#' + hash.slice(-8)
		height: parent.childHeight
		width: height
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
}
