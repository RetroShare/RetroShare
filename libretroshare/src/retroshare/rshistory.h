/*******************************************************************************
 * libretroshare/src/retroshare: rshistory.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Thunder                                                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RS_HISTORY_INTERFACE_H
#define RS_HISTORY_INTERFACE_H

class RsHistory;
class ChatId;

extern RsHistory *rsHistory;

#include <string>
#include <inttypes.h>
#include <list>
#include "retroshare/rstypes.h"

//! data object for message history
/*!
 * data object used for message history
 */
static const uint32_t RS_HISTORY_TYPE_PUBLIC  = 0 ;
static const uint32_t RS_HISTORY_TYPE_PRIVATE = 1 ;
static const uint32_t RS_HISTORY_TYPE_LOBBY   = 2 ;

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
	RsPeerId chatPeerId;
	bool        incoming;
	RsPeerId peerId;
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
    virtual bool getMessages(const ChatId &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount) = 0;
	virtual bool getMessage(uint32_t msgId, HistoryMsg &msg) = 0;
	virtual void removeMessages(const std::list<uint32_t> &msgIds) = 0;
    virtual void clear(const ChatId &chatPeerId) = 0;

	virtual bool getEnable(uint32_t chat_type) = 0;
	virtual void setEnable(uint32_t chat_type, bool enable) = 0;
	virtual uint32_t getMaxStorageDuration() = 0 ;
	virtual void setMaxStorageDuration(uint32_t seconds) = 0 ;

	// 0 = no limit, >0 count of saved messages
	virtual uint32_t getSaveCount(uint32_t chat_type) = 0;
	virtual void     setSaveCount(uint32_t chat_type, uint32_t count) = 0;
};

#endif
