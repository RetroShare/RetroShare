#ifndef RS_NOTIFY_GUI_INTERFACE_H
#define RS_NOTIFY_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsnotify.h
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


#include <map>
#include <list>
#include <iostream>
#include <string>
#include <stdint.h>

class RsNotify;
extern RsNotify   *rsNotify;

const uint32_t RS_SYS_ERROR 	= 0x0001;
const uint32_t RS_SYS_WARNING 	= 0x0002;
const uint32_t RS_SYS_INFO 	= 0x0004;

const uint32_t RS_POPUP_MSG	= 0x0001;
const uint32_t RS_POPUP_CHAT	= 0x0002;
const uint32_t RS_POPUP_CALL	= 0x0004;
const uint32_t RS_POPUP_CONNECT = 0x0008;

/* CHAT flags are here - so they are in the same place as 
 * other Notify flags... not used by libretroshare though
 */
const uint32_t RS_CHAT_OPEN_NEW		= 0x0001;
const uint32_t RS_CHAT_REOPEN		= 0x0002;
const uint32_t RS_CHAT_FOCUS		= 0x0004;

const uint32_t RS_FEED_TYPE_PEER 	= 0x0010;
const uint32_t RS_FEED_TYPE_CHAN 	= 0x0020;
const uint32_t RS_FEED_TYPE_FORUM 	= 0x0040;
const uint32_t RS_FEED_TYPE_BLOG 	= 0x0080;
const uint32_t RS_FEED_TYPE_CHAT 	= 0x0100;
const uint32_t RS_FEED_TYPE_MSG 	= 0x0200;
const uint32_t RS_FEED_TYPE_FILES 	= 0x0400;

const uint32_t RS_FEED_ITEM_PEER_CONNECT	 = RS_FEED_TYPE_PEER  | 0x0001;
const uint32_t RS_FEED_ITEM_PEER_DISCONNECT	 = RS_FEED_TYPE_PEER  | 0x0002;
const uint32_t RS_FEED_ITEM_PEER_NEW		 = RS_FEED_TYPE_PEER  | 0x0003;
const uint32_t RS_FEED_ITEM_PEER_HELLO		 = RS_FEED_TYPE_PEER  | 0x0004;

const uint32_t RS_FEED_ITEM_CHAN_NEW		 = RS_FEED_TYPE_CHAN  | 0x0001;
const uint32_t RS_FEED_ITEM_CHAN_UPDATE		 = RS_FEED_TYPE_CHAN  | 0x0002;
const uint32_t RS_FEED_ITEM_CHAN_MSG		 = RS_FEED_TYPE_CHAN  | 0x0003;

const uint32_t RS_FEED_ITEM_FORUM_NEW		 = RS_FEED_TYPE_FORUM | 0x0001;
const uint32_t RS_FEED_ITEM_FORUM_UPDATE	 = RS_FEED_TYPE_FORUM | 0x0002;
const uint32_t RS_FEED_ITEM_FORUM_MSG		 = RS_FEED_TYPE_FORUM | 0x0003;

const uint32_t RS_FEED_ITEM_BLOG_MSG		 = RS_FEED_TYPE_BLOG  | 0x0001;
const uint32_t RS_FEED_ITEM_CHAT_NEW		 = RS_FEED_TYPE_CHAT  | 0x0001;
const uint32_t RS_FEED_ITEM_MESSAGE		 = RS_FEED_TYPE_MSG   | 0x0001;
const uint32_t RS_FEED_ITEM_FILES_NEW		 = RS_FEED_TYPE_FILES | 0x0001;


class RsFeedItem
{
public:
	RsFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3)
	:mType(type), mId1(id1), mId2(id2), mId3(id3)
	{
		return;
	}

	RsFeedItem() :mType(0) { return; }

	uint32_t mType;
	std::string mId1, mId2, mId3;
};


class RsNotify 
{
	public:

	RsNotify() { return; }
virtual ~RsNotify() { return; }

	/* Output for retroshare-gui */
virtual bool NotifySysMessage(uint32_t &sysid, uint32_t &type, 
					std::string &title, std::string &msg)		= 0;
virtual bool NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &msg) 	= 0;
virtual bool NotifyLogMessage(uint32_t &sysid, uint32_t &type,
					std::string &title, std::string &msg)		= 0;

	/* Control over Messages */
virtual bool GetSysMessageList(std::map<uint32_t, std::string> &list)  			= 0;
virtual bool GetPopupMessageList(std::map<uint32_t, std::string> &list)			= 0;

virtual bool SetSysMessageMode(uint32_t sysid, uint32_t mode)				= 0;
virtual bool SetPopupMessageMode(uint32_t ptype, uint32_t mode)				= 0;

	/* Feed Output */
virtual bool GetFeedItem(RsFeedItem &item)						= 0;

};


#endif
