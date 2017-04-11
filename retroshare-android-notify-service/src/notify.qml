/*
 * RetroShare Android Autologin Service
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

import QtQml 2.2
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import "." //Needed for TokensManager singleton

QtObject
{
	id: notifyRoot

	property alias coreReady: coreWatcher.coreReady

	property AutologinManager _aM: AutologinManager { id: coreWatcher }

	onCoreReadyChanged: if(coreReady) refreshUnread()

	function refreshUnreadCallback(par)
	{
		console.log("notifyRoot.refreshUnreadCB()")
		var json = JSON.parse(par.response)
		TokensManager.registerToken(json.statetoken, refreshUnread)
		if(json.data.length > 0)
		{
			console.log("notifyRoot.refreshUnreadCB() got", json.data.length,
						"unread messages")
			//TODO: display android notification
		}
	}
	function refreshUnread()
	{
		console.log("notifyRoot.refreshUnread()")
		rsApi.request("/chat/unread_msgs", "", refreshUnreadCallback)
	}
}
