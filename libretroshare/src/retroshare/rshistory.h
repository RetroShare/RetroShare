#ifndef RS_HISTORY_INTERFACE_H
#define RS_HISTORY_INTERFACE_H

/*
 * libretroshare/src/retroshare: rshistory.h
 *
 * RetroShare C++ .
 *
 * Copyright 2011 by Thunder.
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

class RsHistory;

extern RsHistory *rsHistory;

#include <string>
#include <inttypes.h>
#include <list>

//! data object for message history
/*!
 * data object used for message history
 */
class HistoryMsg
{
public:
	HistoryMsg()
	{
		msgId = 0;
		incoming = false;
		sendTime = 0;
		recvTime = 0;
	}

public:
	uint32_t    msgId;
	std::string chatPeerId;
	bool        incoming;
	std::string peerId;
	std::string peerName;
	uint32_t    sendTime;
	uint32_t    recvTime;
	std::string message;
};

//! Interface to retroshare for message history
/*!
 * Provides an interface for retroshare's message history functionality
 */
class RsHistory
{
public:
	virtual bool getMessages(const std::string &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount) = 0;
	virtual bool getMessage(uint32_t msgId, HistoryMsg &msg) = 0;
	virtual void removeMessages(const std::list<uint32_t> &msgIds) = 0;
	virtual void clear(const std::string &chatPeerId) = 0;

	virtual bool getEnable(bool ofPublic) = 0;
	virtual void setEnable(bool forPublic, bool enable) = 0;

	// 0 = no limit, >0 count of saved messages
	virtual uint32_t getSaveCount(bool ofPublic) = 0;
	virtual void     setSaveCount(bool forPublic, uint32_t count) = 0;
};

#endif
