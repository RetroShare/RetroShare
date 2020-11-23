/*******************************************************************************
 * libretroshare/src/rsserver: p3history.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Thunder.                                                  *
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
#ifndef RS_P3HISTORY_INTERFACE_H
#define RS_P3HISTORY_INTERFACE_H

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

	virtual bool chatIdToVirtualPeerId(const ChatId &chat_id, RsPeerId &peer_id);
	virtual bool getMessages(const ChatId &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount);
	virtual bool getMessage(uint32_t msgId, HistoryMsg &msg);
	virtual void removeMessages(const std::list<uint32_t> &msgIds);
	virtual void clear(const ChatId &chatPeerId);

	virtual bool getEnable(uint32_t chat_type);
	virtual void setEnable(uint32_t chat_type, bool enable);

	virtual uint32_t getMaxStorageDuration();
	virtual void     setMaxStorageDuration(uint32_t seconds);

	// 0 = no limit, >0 count of saved messages
	virtual uint32_t getSaveCount(uint32_t chat_type);
	virtual void     setSaveCount(uint32_t chat_type, uint32_t count);

private:
	p3HistoryMgr* mHistoryMgr;
};

#endif /* RS_P3HISTORY_INTERFACE_H */
