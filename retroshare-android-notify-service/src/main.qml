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
	onCoreReadyChanged: if(coreReady) refreshUnread()

	property AutologinManager coreWatcher: AutologinManager { id: coreWatcher }

	function refreshUnreadCallback(par)
	{
		console.log("notifyRoot.refreshUnreadCB()")
		var json = JSON.parse(par.response)
		TokensManager.registerToken(json.statetoken, refreshUnread)
		var convCnt = json.data.length
		if(convCnt > 0)
		{
			console.log("notifyRoot.refreshUnreadCB() got", json.data.length,
						"unread conversations")
			notificationsBridge.notify(
						qsTr("New message!"),
						(convCnt > 1) ?
						qsTr("Unread messages in %1 conversations").arg(convCnt):
						qsTr("%1 Unread %2 from %3")
						    .arg(json.data[0].unread_count)
						    .arg(json.data[0].unread_count > 1 ? "messages" : "message")
						    .arg(json.data[0].remote_author_name),
						qsTr("/contacts%1").arg(
							convCnt == 1?
								"?gxsId="+json.data[0].remote_author_id +
								"&name="+json.data[0].remote_author_name
							  : "")
						)
		}
	}
	function refreshUnread()
	{
		console.log("notifyRoot.refreshUnread()")
		rsApi.request("/chat/unread_msgs", "", refreshUnreadCallback)
	}
}
