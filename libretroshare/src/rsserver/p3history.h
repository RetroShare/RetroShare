#ifndef RS_P3HISTORY_INTERFACE_H
#define RS_P3HISTORY_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3history.h
 *
 * RetroShare C++ Interface.
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

#include "retroshare/rshistory.h"

class p3HistoryMgr;

//! Implements abstract interface rsHistory
/*!
 *	Interfaces with p3HistoryMsg
 */
class p3History : public RsHistory
{
public:

	p3History(p3HistoryMgr* historyMgr);
	virtual ~p3History();

	virtual bool getMessages(const RsPeerId &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount);
	virtual bool getMessage(uint32_t msgId, HistoryMsg &msg);
	virtual void removeMessages(const std::list<uint32_t> &msgIds);
	virtual void clear(const RsPeerId &chatPeerId);
	virtual bool getEnable(uint32_t chat_type);
	virtual void setEnable(uint32_t chat_type, bool enable);
	virtual uint32_t getSaveCount(uint32_t chat_type);
	virtual void setSaveCount(uint32_t chat_type, uint32_t count);
	virtual void setMaxStorageDuration(uint32_t seconds) ;
	virtual uint32_t getMaxStorageDuration() ;

private:
	p3HistoryMgr* mHistoryMgr;
};

#endif /* RS_P3HISTORY_INTERFACE_H */
