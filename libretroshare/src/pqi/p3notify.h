#ifndef RS_P3_NOTIFY_INTERFACE_H
#define RS_P3_NOTIFY_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3notify.h
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

#include "rsiface/rsnotify.h"
#include "pqi/pqinotify.h"

#include "util/rsthreads.h"

class p3NotifySysMsg
{
	public:

	uint32_t sysid;
	uint32_t type;
	std::string title;
	std::string msg;
};

class p3NotifyLogMsg
{
	public:

	uint32_t sysid;
	uint32_t type;
	std::string title;
	std::string msg;
};

class p3NotifyPopupMsg
{
	public:

	uint32_t type;
	std::string name;
	std::string msg;
};


class p3Notify: public RsNotify, public pqiNotify
{
	public:

	p3Notify() { return; }
virtual ~p3Notify() { return; }

	/* Output for retroshare-gui */
virtual bool NotifySysMessage(uint32_t &sysid, uint32_t &type, 
					std::string &title, std::string &msg);
virtual bool NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &msg);
virtual bool NotifyLogMessage(uint32_t &sysid, uint32_t &type, std::string &title, std::string &msg);

	/* Control over Messages */
virtual bool GetSysMessageList(std::map<uint32_t, std::string> &list);
virtual bool GetPopupMessageList(std::map<uint32_t, std::string> &list);

virtual bool SetSysMessageMode(uint32_t sysid, uint32_t mode);
virtual bool SetPopupMessageMode(uint32_t ptype, uint32_t mode);

virtual bool GetFeedItem(RsFeedItem &item);

	/* Overloaded from pqiNotify */
virtual bool AddPopupMessage(uint32_t ptype, std::string name, std::string msg);
virtual bool AddSysMessage(uint32_t sysid, uint32_t type, std::string title, std::string msg);
virtual bool AddLogMessage(uint32_t sysid, uint32_t type, std::string title, std::string msg);
virtual bool AddFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3);
virtual bool ClearFeedItems(uint32_t type);

	private:

	RsMutex noteMtx;

	std::list<p3NotifySysMsg> pendingSysMsgs;
	std::list<p3NotifyLogMsg> pendingLogMsgs;
	std::list<p3NotifyPopupMsg> pendingPopupMsgs;
	std::list<RsFeedItem>  pendingNewsFeed;
};


#endif
