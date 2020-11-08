/*******************************************************************************
 * libretroshare/src/pqi: p3historymgr.h                                       *
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
#ifndef RS_P3_HISTORY_MGR_H
#define RS_P3_HISTORY_MGR_H

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

	static bool chatIdToVirtualPeerId(const ChatId& chat_id, RsPeerId& peer_id);

private:
	uint32_t nextMsgId;
	std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> > mMessages;

	// Removes messages stored for more than mMaxMsgStorageDurationSeconds seconds.
	// This avoids the stored list to grow crazy with time.
	//
	void cleanOldMessages() ;

	bool mPublicEnable;
	bool mLobbyEnable;
	bool mPrivateEnable;
	bool mDistantEnable;

	uint32_t mPublicSaveCount;
	uint32_t mLobbySaveCount;
	uint32_t mPrivateSaveCount;
	uint32_t mDistantSaveCount;

	uint32_t mMaxStorageDurationSeconds ;
	rstime_t mLastCleanTime ;

	std::list<RsItem*> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */

	RsMutex mHistoryMtx;
};

#endif
