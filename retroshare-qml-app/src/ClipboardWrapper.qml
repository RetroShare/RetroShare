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

pragma Singleton

import QtQml 2.2
import QtQuick 2.7
import QtQuick.Controls 2.0

QtObject
{
	/// Public API
	function postToClipBoard(str)
	{
		console.log("postToClipBoard(str)", str)
		privTF.text = str
		privTF.selectAll()
		privTF.cut()
	}

	/// Public API
	function getFromClipBoard()
	{
		privTF.text = "getFromClipBoard()" // Need some text for selectAll()
		privTF.selectAll()
		privTF.paste()
		return privTF.text.toString()
	}

	/// Private
	property TextInput privTF: TextInput { visible: false }
}
