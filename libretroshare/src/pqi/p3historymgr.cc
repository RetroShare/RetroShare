/*
 * libretroshare/src/services: p3HistoryMgr.cc
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

#include "p3historymgr.h"
#include "serialiser/rshistoryitems.h"
#include "serialiser/rsconfigitems.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "serialiser/rsmsgitems.h"
#include "util/rsstring.h"

// clean too old messages every 5 minutes
//
#define MSG_HISTORY_CLEANING_PERIOD  300

RsHistory *rsHistory = NULL;

p3HistoryMgr::p3HistoryMgr()
	: p3Config(CONFIG_TYPE_HISTORY), mHistoryMtx("p3HistoryMgr")
{
	nextMsgId = 1;

	mPublicEnable = false;
	mPrivateEnable = true;
	mLobbyEnable = true;

	mPublicSaveCount  = 0;
	mLobbySaveCount   = 0;
	mPrivateSaveCount = 0;
	mLastCleanTime = 0 ;

	mMaxStorageDurationSeconds = 10*86400 ; // store for 10 days at most.
}

p3HistoryMgr::~p3HistoryMgr()
{
}

/***** p3HistoryMgr *****/

void p3HistoryMgr::addMessage(bool incoming, const std::string &chatPeerId, const std::string &peerId, const RsChatMsgItem *chatItem)
{
	uint32_t addMsgId = 0;

	time_t now = time(NULL) ;

	if(mLastCleanTime + MSG_HISTORY_CLEANING_PERIOD < now)
	{
		cleanOldMessages() ;
		mLastCleanTime = now ;
	}

	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

		if (mPublicEnable == false && chatPeerId.empty()) {
			// public chat not enabled
			return;
		}

		const RsChatLobbyMsgItem *cli = dynamic_cast<const RsChatLobbyMsgItem*>(chatItem);

		if (cli) 
		{
			if (mLobbyEnable == false && !chatPeerId.empty()) // lobby chat not enabled
				return;
		}
		else 
		{
			if (mPrivateEnable == false && !chatPeerId.empty()) // private chat not enabled
				return;
		}

		RsHistoryMsgItem* item = new RsHistoryMsgItem;
		item->chatPeerId = chatPeerId;
		item->incoming = incoming;
		item->peerId = peerId;
		item->peerName = cli ? cli->nick : rsPeers->getPeerName(item->peerId);
		item->sendTime = chatItem->sendTime;
		item->recvTime = chatItem->recvTime;

		if (cli) {
			// disable save to disc for chat lobbies until they are saved
			item->saveToDisc = false;
		}

		librs::util::ConvertUtf16ToUtf8(chatItem->message, item->message);

		std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(item->chatPeerId);
		if (mit != mMessages.end()) {
			item->msgId = nextMsgId++;
			mit->second.insert(std::make_pair(item->msgId, item));
			addMsgId = item->msgId;

			// check the limit
			uint32_t limit;
			if (chatPeerId.empty()) 
				limit = mPublicSaveCount;
			else if (cli) 
				limit = mLobbySaveCount;
			else 
				limit = mPrivateSaveCount;

			if (limit) {
				while (mit->second.size() > limit) {
					delete(mit->second.begin()->second);
					mit->second.erase(mit->second.begin());
				}
			}
		} else {
			std::map<uint32_t, RsHistoryMsgItem*> msgs;
			item->msgId = nextMsgId++;
			msgs.insert(std::make_pair(item->msgId, item));
			mMessages.insert(std::make_pair(item->chatPeerId, msgs));
			addMsgId = item->msgId;

			// no need to check the limit
		}

		IndicateConfigChanged();
	}

	if (addMsgId) {
		rsicontrol->getNotify().notifyHistoryChanged(addMsgId, NOTIFY_TYPE_ADD);
	}
}

void p3HistoryMgr::cleanOldMessages()
{
	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

	std::cerr << "****** cleaning old messages." << std::endl;
	time_t now = time(NULL) ;
	bool changed = false ;

	for(std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.begin(); mit != mMessages.end();) 
	{
		if (mMaxStorageDurationSeconds > 0)
		{
			for(std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.begin();lit!=mit->second.end();)
				if(lit->second->recvTime + mMaxStorageDurationSeconds < now)
				{
					std::map<uint32_t, RsHistoryMsgItem*>::iterator lit2 = lit ;
					++lit2 ;

					std::cerr << "   removing msg id " << lit->first << ", for peer id " << mit->first << std::endl;
					delete lit->second ;

					mit->second.erase(lit) ;
					lit = lit2 ;

					changed = true ;
				}
				else
					++lit ;
		}

		if(mit->second.empty())
		{
			std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit2 = mit ;
			++mit2 ;
				std::cerr << "   removing peer id " << mit->first << ", since it has no messages" << std::endl;
			mMessages.erase(mit) ;
			mit = mit2 ;

			changed = true ;
		}
		else
			++mit ;
	}

	if(changed)
		IndicateConfigChanged() ;
}

/***** p3Config *****/

RsSerialiser* p3HistoryMgr::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser;
	rss->addSerialType(new RsHistorySerialiser);
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}

bool p3HistoryMgr::saveList(bool& cleanup, std::list<RsItem*>& saveData)
{
	cleanup = false;

	mHistoryMtx.lock(); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
	std::map<uint32_t, RsHistoryMsgItem*>::iterator lit;
	for (mit = mMessages.begin(); mit != mMessages.end(); mit++) {
		for (lit = mit->second.begin(); lit != mit->second.end(); lit++) {
			if (lit->second->saveToDisc) {
				saveData.push_back(lit->second);
			}
		}
	}

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet;

	RsTlvKeyValue kv;
	kv.key = "PUBLIC_ENABLE";
	kv.value = mPublicEnable ? "TRUE" : "FALSE";
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "PRIVATE_ENABLE";
	kv.value = mPrivateEnable ? "TRUE" : "FALSE";
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "LOBBY_ENABLE";
	kv.value = mLobbyEnable ? "TRUE" : "FALSE";
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "MAX_STORAGE_TIME";
	rs_sprintf(kv.value,"%d",mMaxStorageDurationSeconds) ;
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "LOBBY_SAVECOUNT";
	rs_sprintf(kv.value, "%lu", mLobbySaveCount);
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "PUBLIC_SAVECOUNT";
	rs_sprintf(kv.value, "%lu", mPublicSaveCount);
	vitem->tlvkvs.pairs.push_back(kv);

	kv.key = "PRIVATE_SAVECOUNT";
	rs_sprintf(kv.value, "%lu", mPrivateSaveCount);
	vitem->tlvkvs.pairs.push_back(kv);

	saveData.push_back(vitem);
	saveCleanupList.push_back(vitem);

	return true;
}

void p3HistoryMgr::saveDone()
{
	/* clean up the save List */
	std::list<RsItem*>::iterator it;
	for (it = saveCleanupList.begin(); it != saveCleanupList.end(); ++it) {
		delete (*it);
	}

	saveCleanupList.clear();

	/* unlock mutex */
	mHistoryMtx.unlock(); /****** MUTEX UNLOCKED *******/
}

bool p3HistoryMgr::loadList(std::list<RsItem*>& load)
{
	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

	RsHistoryMsgItem *msgItem;
	std::list<RsItem*>::iterator it;

	for (it = load.begin(); it != load.end(); it++) {
		if (NULL != (msgItem = dynamic_cast<RsHistoryMsgItem*>(*it))) {

			std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(msgItem->chatPeerId);
			msgItem->msgId = nextMsgId++;

			std::cerr << "Loading msg history item: peer id=" << msgItem->chatPeerId << "), msg id =" << msgItem->msgId  << std::endl;

			if (mit != mMessages.end()) {
				mit->second.insert(std::make_pair(msgItem->msgId, msgItem));
			} else {
				std::map<uint32_t, RsHistoryMsgItem*> msgs;
				msgs.insert(std::make_pair(msgItem->msgId, msgItem));
				mMessages.insert(std::make_pair(msgItem->chatPeerId, msgs));
			}

			// don't delete the item !!

			continue;
		}

		RsConfigKeyValueSet *rskv ;
		if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet*>(*it))) {
			for (std::list<RsTlvKeyValue>::const_iterator kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); kit++) {
				if (kit->key == "PUBLIC_ENABLE") {
					mPublicEnable = (kit->value == "TRUE") ? true : false;
					continue;
				}

				if (kit->key == "PRIVATE_ENABLE") {
					mPrivateEnable = (kit->value == "TRUE") ? true : false;
					continue;
				}

				if (kit->key == "LOBBY_ENABLE") {
					mLobbyEnable = (kit->value == "TRUE") ? true : false;
					continue;
				}

				if (kit->key == "MAX_STORAGE_TIME") {
					uint32_t val ;
					if (sscanf(kit->value.c_str(), "%d", &val) == 1)
						mMaxStorageDurationSeconds = val ;

					std::cerr << "Loaded max storage time for history = " << val << " seconds" << std::endl;
					continue;
				}

				if (kit->key == "PUBLIC_SAVECOUNT") {
					mPublicSaveCount = atoi(kit->value.c_str());
					continue;
				}
				if (kit->key == "PRIVATE_SAVECOUNT") {
					mPrivateSaveCount = atoi(kit->value.c_str());
					continue;
				}
				if (kit->key == "LOBBY_SAVECOUNT") {
					mLobbySaveCount = atoi(kit->value.c_str());
					continue;
				}
			}

			delete (*it);
			continue;
		}

		// delete unknown items
		delete (*it);
	}

	return true;
}

/***** p3History *****/

static void convertMsg(const RsHistoryMsgItem* item, HistoryMsg &msg)
{
	msg.msgId = item->msgId;
	msg.chatPeerId = item->chatPeerId;
	msg.incoming = item->incoming;
	msg.peerId = item->peerId;
	msg.peerName = item->peerName;
	msg.sendTime = item->sendTime;
	msg.recvTime = item->recvTime;
	msg.message = item->message;
}

bool p3HistoryMgr::getMessages(const std::string &chatPeerId, std::list<HistoryMsg> &msgs, uint32_t loadCount)
{
	msgs.clear();

	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

	std::cerr << "Getting history for peer " << chatPeerId << std::endl;

	if (mPublicEnable == false && chatPeerId.empty()) {	// chatPeerId.empty() means it's public chat
		// public chat not enabled
		return false;
	}

	if (mPrivateEnable == false && chatPeerId.empty() == false) // private chat not enabled
		return false;

	if (mLobbyEnable == false && chatPeerId.empty() == false) // private chat not enabled
		return false;

	uint32_t foundCount = 0;

	std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(chatPeerId);

	if (mit != mMessages.end()) 
	{
		std::map<uint32_t, RsHistoryMsgItem*>::reverse_iterator lit;

		for (lit = mit->second.rbegin(); lit != mit->second.rend(); lit++) 
		{
			HistoryMsg msg;
			convertMsg(lit->second, msg);
			msgs.insert(msgs.begin(), msg);
			foundCount++;
			if (loadCount && foundCount >= loadCount) {
				break;
			}
		}
	}
	std::cerr << msgs.size() << " messages added." << std::endl;

	return true;
}

bool p3HistoryMgr::getMessage(uint32_t msgId, HistoryMsg &msg)
{
	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
	for (mit = mMessages.begin(); mit != mMessages.end(); mit++) {
		std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.find(msgId);
		if (lit != mit->second.end()) {
			convertMsg(lit->second, msg);
			return true;
		}
	}

	return false;
}

void p3HistoryMgr::clear(const std::string &chatPeerId)
{
	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

		std::cerr << "********** p3History::clear()called for peer id " << chatPeerId << std::endl;

		std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(chatPeerId);
		if (mit == mMessages.end()) {
			return;
		}

		std::map<uint32_t, RsHistoryMsgItem*>::iterator lit;
		for (lit = mit->second.begin(); lit != mit->second.end(); lit++) {
			delete(lit->second);
		}
		mit->second.clear();
		mMessages.erase(mit);

		IndicateConfigChanged();
	}

	rsicontrol->getNotify().notifyHistoryChanged(0, NOTIFY_TYPE_MOD);
}

void p3HistoryMgr::removeMessages(const std::list<uint32_t> &msgIds)
{
	std::list<uint32_t> ids = msgIds;
	std::list<uint32_t> removedIds;
	std::list<uint32_t>::iterator iit;

		std::cerr << "********** p3History::removeMessages called()" << std::endl;
	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

		std::map<std::string, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
		for (mit = mMessages.begin(); mit != mMessages.end(); ++mit) {
			iit = ids.begin();
			while (iit != ids.end()) {
				std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.find(*iit);
				if (lit != mit->second.end()) {
					delete(lit->second);
					mit->second.erase(lit);

		std::cerr << "**** Removing " << mit->first << " msg id = " << lit->first << std::endl;
					removedIds.push_back(*iit);
					iit = ids.erase(iit);

					continue;
				}

				++iit;
			}

			if (ids.empty()) {
				break;
			}
		}
	}

	if (removedIds.empty() == false) {
		IndicateConfigChanged();

		for (iit = removedIds.begin(); iit != removedIds.end(); ++iit) {
			rsicontrol->getNotify().notifyHistoryChanged(*iit, NOTIFY_TYPE_DEL);
		}
	}
}

bool p3HistoryMgr::getEnable(uint32_t chat_type)
{
	switch(chat_type)
	{
		case RS_HISTORY_TYPE_PUBLIC : return mPublicEnable ;
		case RS_HISTORY_TYPE_LOBBY  : return mLobbyEnable ;
		case RS_HISTORY_TYPE_PRIVATE: return mPrivateEnable ;
		default:
											  std::cerr << "Unexpected value " << chat_type<< " in p3HistoryMgr::getEnable(): this is a bug." << std::endl;
											  return 0 ;
	}
}

uint32_t p3HistoryMgr::getMaxStorageDuration()
{
	return mMaxStorageDurationSeconds ;
}


void p3HistoryMgr::setMaxStorageDuration(uint32_t seconds)
{
	if(mMaxStorageDurationSeconds != seconds)
		IndicateConfigChanged() ;

	mMaxStorageDurationSeconds = seconds ;
}

void p3HistoryMgr::setEnable(uint32_t chat_type, bool enable)
{
	bool oldValue;

	switch(chat_type)
	{
		case RS_HISTORY_TYPE_PUBLIC : oldValue = mPublicEnable ;
											  mPublicEnable = enable ; 
											  break ;

		case RS_HISTORY_TYPE_LOBBY  : oldValue = mLobbyEnable ; 
											  mLobbyEnable = enable;
											  break ;

		case RS_HISTORY_TYPE_PRIVATE: oldValue = mPrivateEnable ;
											  mPrivateEnable = enable ;
											  break ;
	}

	if (oldValue != enable) 
		IndicateConfigChanged();
}

uint32_t p3HistoryMgr::getSaveCount(uint32_t chat_type)
{
	switch(chat_type)
	{
		case RS_HISTORY_TYPE_PUBLIC : return mPublicSaveCount ;
		case RS_HISTORY_TYPE_LOBBY  : return mLobbySaveCount ;
		case RS_HISTORY_TYPE_PRIVATE: return mPrivateSaveCount ;
		default:
											  std::cerr << "Unexpected value " << chat_type<< " in p3HistoryMgr::getSaveCount(): this is a bug." << std::endl;
											  return 0 ;
	}
}

void p3HistoryMgr::setSaveCount(uint32_t chat_type, uint32_t count)
{
	uint32_t oldValue;

	switch(chat_type)
	{
		case RS_HISTORY_TYPE_PUBLIC : oldValue = mPublicSaveCount ;
											  mPublicSaveCount = count ; 
											  break ;

		case RS_HISTORY_TYPE_LOBBY  : oldValue = mLobbySaveCount ; 
											  mLobbySaveCount = count;
											  break ;

		case RS_HISTORY_TYPE_PRIVATE: oldValue = mPrivateSaveCount ;
											  mPrivateSaveCount = count ;
											  break ;
	}

	if (oldValue != count) 
		IndicateConfigChanged();
}
