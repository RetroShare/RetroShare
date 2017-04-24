#ifndef RS_P3_HISTORY_MGR_H
#define RS_P3_HISTORY_MGR_H

/*
 * libretroshare/src/services: p3historymgr.h
 *
 * RetroShare C++
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

#include <map>
#include <list>

#include "rsitems/rshistoryitems.h"
#include "retroshare/rshistory.h"
#include "pqi/p3cfgmgr.h"

class RsChatMsgItem;
class ChatMessage;

//! handles history
/*!
 * The is a retroshare service which allows peers
 * to store the history of the chat messages
 */
class p3HistoryMgr: public p3Config
{
public:
	p3HistoryMgr();
	virtual ~p3HistoryMgr();

	/******** p3HistoryMgr *********/

    void addMessage(const ChatMessage &cm);

	/********* RsHistory ***********/

    bool getMessages(const ChatId &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount);
	bool getMessage(uint32_t msgId, HistoryMsg &msg);
    void clear(const ChatId &chatPeerId);
	void removeMessages(const std::list<uint32_t> &msgIds);

	virtual bool getEnable(uint32_t chat_type);
	virtual void setEnable(uint32_t chat_type, bool enable);
	virtual uint32_t getSaveCount(uint32_t chat_type);
	virtual void setSaveCount(uint32_t chat_type, uint32_t count);
	virtual void setMaxStorageDuration(uint32_t seconds) ;
	virtual uint32_t getMaxStorageDuration() ;

	/********* p3config ************/

	virtual RsSerialiser *setupSerialiser();
	virtual bool saveList(bool& cleanup, std::list<RsItem*>& saveData);
	virtual void saveDone();
	virtual bool loadList(std::list<RsItem*>& load);

private:
    static bool chatIdToVirtualPeerId(ChatId chat_id, RsPeerId& peer_id);

	uint32_t nextMsgId;
	std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> > mMessages;

	// Removes messages stored for more than mMaxMsgStorageDurationSeconds seconds.
	// This avoids the stored list to grow crazy with time.
	//
	void cleanOldMessages() ;

	bool mPublicEnable;
	bool mLobbyEnable;
	bool mPrivateEnable;

	uint32_t mPublicSaveCount;
	uint32_t mLobbySaveCount;
	uint32_t mPrivateSaveCount;

	uint32_t mMaxStorageDurationSeconds ;
	time_t mLastCleanTime ;

	std::list<RsItem*> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */

	RsMutex mHistoryMtx;
};

#endif
