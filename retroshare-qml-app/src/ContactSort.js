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

function strcmp(left, right)
{ return ( left < right ? -1 : ( left > right ? 1:0 ) ) }

var unreadMessages = {}
var contactsData = {}

function cntcmp(left, right, searchText)
{
	if(typeof searchText !== 'undefined' && searchText.length > 0)
	{
		var mtc = searchText.toLowerCase()
		var lfn = left.name.toLowerCase()
		var rgn = right.name.toLowerCase()
		var lfml = lfn.indexOf(mtc)
		var rgml = rgn.indexOf(mtc)
		if ( lfml !== rgml )
		{
			lfml = lfml >= 0 ? lfml : Number.MAX_VALUE
			rgml = rgml >= 0 ? rgml : Number.MAX_VALUE
			return lfml - rgml
		}
	}

	var lfun = left.hasOwnProperty("unread_count") ? left.unread_count : 0
	var rgun = right.hasOwnProperty("unread_count") ? right.unread_count : 0
	if( lfun !== rgun ) return rgun - lfun
	var lcon = left.is_contact
	var rcon = right.is_contact
	if( lcon !== rcon ) return rcon - lcon
	var lname = left.name.toLowerCase()
	var rname = right.name.toLowerCase()
	if(lname !== rname) return strcmp(lname, rname)
	return strcmp(left.gxs_id, right.gxs_id)
}

function mergeContactsUnread()
{
	var jsonData = contactsData.data
	var dataLen = jsonData.length
	for ( var i=0; i<dataLen; ++i)
	{
		var el = jsonData[i]
		if(unreadMessages.hasOwnProperty(el.gxs_id))
			el['unread_count'] = unreadMessages[el.gxs_id]
		else el['unread_count'] = "0" // This must be string
	}
}

function parseUnread(responseStr)
{
	var jsonData = JSON.parse(responseStr).data
	var dataLen = jsonData.length
	unreadMessages = {}
	for ( var i=0; i<dataLen; ++i)
	{
		var el = jsonData[i]
		if(el.is_distant_chat_id)
			unreadMessages[el.remote_author_id] = el.unread_count
	}

	mergeContactsUnread()
}

function parseContacts(responseStr)
{
	contactsData = JSON.parse(responseStr)
	mergeContactsUnread()
}

WorkerScript.onMessage = function(message)
{
	var sortFn = cntcmp
	message.action = message.hasOwnProperty("action") ? message.action : "rSort"

	if(message.action === "refreshContacts") parseContacts(message.response)
	else if(message.action === "refreshUnread") parseUnread(message.response)
	else if(message.action === "searchContact")
		sortFn = function cmp(l,r) { return cntcmp(l,r, message.sexp) }

	contactsData.data.sort(sortFn)

	WorkerScript.sendMessage(contactsData)
}
