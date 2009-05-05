/*
 * libretroshare/src/rsserver: p3notify.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "pqi/p3notify.h"

/* external reference point */
RsNotify *rsNotify = NULL;

pqiNotify *getPqiNotify()
{
	return ((p3Notify *) rsNotify);
}

	/* Output for retroshare-gui */
bool p3Notify::NotifySysMessage(uint32_t &sysid, uint32_t &type, 
					std::string &title, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingSysMsgs.size() > 0)
	{
		p3NotifySysMsg smsg = pendingSysMsgs.front();
		pendingSysMsgs.pop_front();

		sysid = smsg.sysid;
		type = smsg.type;
		title = smsg.title;
		msg = smsg.msg;

		return true;
	}

	return false;
}

	/* Output for retroshare-gui */
bool p3Notify::NotifyLogMessage(uint32_t &sysid, uint32_t &type,
					std::string &title, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingLogMsgs.size() > 0)
	{
		p3NotifyLogMsg smsg = pendingLogMsgs.front();
		pendingLogMsgs.pop_front();

		sysid = smsg.sysid;
		type = smsg.type;
		title = smsg.title;
		msg = smsg.msg;

		return true;
	}

	return false;
}


bool p3Notify::NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingPopupMsgs.size() > 0)
	{
		p3NotifyPopupMsg pmsg = pendingPopupMsgs.front();
		pendingPopupMsgs.pop_front();

		ptype = pmsg.type;
		name = pmsg.name;
		msg = pmsg.msg;

		return true;
	}

	return false;
}


	/* Control over Messages */
bool p3Notify::GetSysMessageList(std::map<uint32_t, std::string> &list)
{
	return false;
}

bool p3Notify::GetPopupMessageList(std::map<uint32_t, std::string> &list)
{
	return false;
}


bool p3Notify::SetSysMessageMode(uint32_t sysid, uint32_t mode)
{
	return false;
}

bool p3Notify::SetPopupMessageMode(uint32_t ptype, uint32_t mode)
{
	return false;
}


	/* Input from libretroshare */
bool p3Notify::AddPopupMessage(uint32_t ptype, std::string name, std::string msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifyPopupMsg pmsg;

	pmsg.type = ptype;
	pmsg.name = name;
	pmsg.msg = msg;

	pendingPopupMsgs.push_back(pmsg);

	return true;
}


bool p3Notify::AddSysMessage(uint32_t sysid, uint32_t type, 
					std::string title, std::string msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifySysMsg smsg;

	smsg.sysid = sysid;
	smsg.type = type;
	smsg.title = title;
	smsg.msg = msg;

	pendingSysMsgs.push_back(smsg);

	return true;
}

bool p3Notify::AddLogMessage(uint32_t sysid, uint32_t type,
					std::string title, std::string msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifyLogMsg smsg;

	smsg.sysid = sysid;
	smsg.type = type;
	smsg.title = title;
	smsg.msg = msg;

	pendingLogMsgs.push_back(smsg);

	return true;
}


bool p3Notify::GetFeedItem(RsFeedItem &item)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingNewsFeed.size() > 0)
	{
		item = pendingNewsFeed.front();
		pendingNewsFeed.pop_front();

		return true;
	}

	return false;
}


bool p3Notify::AddFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	pendingNewsFeed.push_back(RsFeedItem(type, id1, id2, id3));

	return true;
}

