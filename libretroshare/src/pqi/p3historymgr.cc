/*******************************************************************************
 * libretroshare/src/pqi: p3historymgr.cc                                      *
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
#include "util/rstime.h"

#include "p3historymgr.h"
#include "rsitems/rshistoryitems.h"
#include "rsitems/rsconfigitems.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "rsitems/rsmsgitems.h"
#include "rsserver/p3face.h"
#include "util/rsstring.h"

/****
 * #define HISTMGR_DEBUG 1
 ***/

// clean too old messages every 5 minutes
//
#define MSG_HISTORY_CLEANING_PERIOD  300

RsHistory *rsHistory = NULL;

p3HistoryMgr::p3HistoryMgr()
	: p3Config(), mHistoryMtx("p3HistoryMgr")
{
	nextMsgId = 1;

	mPublicEnable = false;
	mPrivateEnable = true;
	mLobbyEnable = true;
	mDistantEnable = true;

	mPublicSaveCount  = 0;
	mLobbySaveCount   = 0;
	mPrivateSaveCount = 0;
	mDistantSaveCount = 0;

	mLastCleanTime = 0 ;

	mMaxStorageDurationSeconds = 10*86400 ; // store for 10 days at most.
}

p3HistoryMgr::~p3HistoryMgr()
{
}

/***** p3HistoryMgr *****/

void p3HistoryMgr::addMessage(const ChatMessage& cm)
{
	uint32_t addMsgId = 0;

	rstime_t now = time(NULL) ;

	if(mLastCleanTime + MSG_HISTORY_CLEANING_PERIOD < now)
	{
		cleanOldMessages() ;
		mLastCleanTime = now ;
	}

	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/


		RsPeerId msgPeerId; // id of sending peer
		RsPeerId chatPeerId; // id of chat endpoint
		std::string peerName; //name of sending peer

		bool enabled = false;
		if (cm.chat_id.isBroadcast() && mPublicEnable == true) {
			peerName = rsPeers->getPeerName(cm.broadcast_peer_id);
			enabled = true;
		}
		if (cm.chat_id.isPeerId() && mPrivateEnable == true) {
			msgPeerId = cm.incoming ? cm.chat_id.toPeerId() : rsPeers->getOwnId();
			peerName = rsPeers->getPeerName(msgPeerId);
			enabled = true;
		}
		if (cm.chat_id.isLobbyId() && mLobbyEnable == true) {
			peerName = cm.lobby_peer_gxs_id.toStdString();
            msgPeerId = RsPeerId(cm.lobby_peer_gxs_id);
            enabled = true;
		}

		if(cm.chat_id.isDistantChatId()&& mDistantEnable == true)
		{
			DistantChatPeerInfo dcpinfo;
			if (rsMsgs->getDistantChatStatus(cm.chat_id.toDistantChatId(), dcpinfo))
            {
                RsIdentityDetails det;
                RsGxsId writer_id = cm.incoming?(dcpinfo.to_id):(dcpinfo.own_id);

                if(rsIdentity->getIdDetails(writer_id,det))
					peerName = det.mNickname;
                else
					peerName = writer_id.toStdString();

                msgPeerId = cm.incoming?RsPeerId(dcpinfo.own_id):RsPeerId(dcpinfo.to_id);
            }
            else
            {
                RsErr() << "Cannot retrieve friend name for distant chat " << cm.chat_id.toDistantChatId() << std::endl;
				peerName = "";
            }

			enabled = true;
		}

		if(enabled == false)
			return;

		if(!chatIdToVirtualPeerId(cm.chat_id, chatPeerId))
			return;

		RsHistoryMsgItem* item = new RsHistoryMsgItem;
		item->chatPeerId = chatPeerId;
		item->incoming = cm.incoming;
		item->msgPeerId = msgPeerId;
		item->peerName = peerName;
		item->sendTime = cm.sendTime;
		item->recvTime = cm.recvTime;

		item->message = cm.msg ;
		//librs::util::ConvertUtf16ToUtf8(chatItem->message, item->message);

		std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(item->chatPeerId);
		if (mit != mMessages.end()) {
			item->msgId = nextMsgId++;
			mit->second.insert(std::make_pair(item->msgId, item));
			addMsgId = item->msgId;

			// check the limit
			uint32_t limit;
			if (chatPeerId.isNull()) 
				limit = mPublicSaveCount;
			else if (cm.chat_id.isLobbyId())
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
		RsServer::notify()->notifyHistoryChanged(addMsgId, NOTIFY_TYPE_ADD);
	}
}

void p3HistoryMgr::cleanOldMessages()
{
	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

#ifdef HISTMGR_DEBUG
	std::cerr << "****** cleaning old messages." << std::endl;
#endif
	rstime_t now = time(NULL) ;
	bool changed = false ;

	for(std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.begin(); mit != mMessages.end();) 
	{
		if (mMaxStorageDurationSeconds > 0)
		{
			for(std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.begin();lit!=mit->second.end();)
				if(lit->second->recvTime + mMaxStorageDurationSeconds < now)
				{
					std::map<uint32_t, RsHistoryMsgItem*>::iterator lit2 = lit ;
					++lit2 ;

#ifdef HISTMGR_DEBUG
					std::cerr << "   removing msg id " << lit->first << ", for peer id " << mit->first << std::endl;
#endif
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
			std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit2 = mit ;
			++mit2 ;
#ifdef HISTMGR_DEBUG
			std::cerr << "   removing peer id " << mit->first << ", since it has no messages" << std::endl;
#endif
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

	std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
	std::map<uint32_t, RsHistoryMsgItem*>::iterator lit;
	for (mit = mMessages.begin(); mit != mMessages.end(); ++mit) {
		for (lit = mit->second.begin(); lit != mit->second.end(); ++lit) {
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
	
	kv.key = "DISTANT_ENABLE";
	kv.value = mDistantEnable ? "TRUE" : "FALSE";
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
	
	kv.key = "DISTANT_SAVECOUNT";
	rs_sprintf(kv.value, "%lu", mDistantSaveCount);
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

	for (it = load.begin(); it != load.end(); ++it) 
   	 {
		if (NULL != (msgItem = dynamic_cast<RsHistoryMsgItem*>(*it))) {

			std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(msgItem->chatPeerId);
			msgItem->msgId = nextMsgId++;

#ifdef HISTMGR_DEBUG
			std::cerr << "Loading msg history item: peer id=" << msgItem->chatPeerId << "), msg id =" << msgItem->msgId  << std::endl;
#endif

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
			for (std::list<RsTlvKeyValue>::const_iterator kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); ++kit) {
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
				
				if (kit->key == "DISTANT_ENABLE") {
					mDistantEnable = (kit->value == "TRUE") ? true : false;
					continue;
				}

				if (kit->key == "MAX_STORAGE_TIME") {
					uint32_t val ;
					if (sscanf(kit->value.c_str(), "%u", &val) == 1)
						mMaxStorageDurationSeconds = val ;

#ifdef HISTMGR_DEBUG
					std::cerr << "Loaded max storage time for history = " << val << " seconds" << std::endl;
#endif
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
				if (kit->key == "DISTANT_SAVECOUNT") {
					mDistantSaveCount = atoi(kit->value.c_str());
					continue;
				}
			}

			delete (*it);
			continue;
		}

		// delete unknown items
		delete (*it);
	}

    load.clear() ;
	return true;
}

// have to convert to virtual peer id, to be able to use existing serialiser and file format
bool p3HistoryMgr::chatIdToVirtualPeerId(const ChatId& chat_id, RsPeerId &peer_id)
{
    if (chat_id.isBroadcast()) {
        peer_id = RsPeerId();
        return true;
    }
    if (chat_id.isPeerId()) {
        peer_id = chat_id.toPeerId();
        return true;
    }
    if (chat_id.isLobbyId()) {
        if(sizeof(ChatLobbyId) > RsPeerId::SIZE_IN_BYTES){
            std::cerr << "p3HistoryMgr::chatIdToVirtualPeerId() ERROR: ChatLobbyId does not fit into virtual peer id. Please report this error." << std::endl;
            return false;
        }
        uint8_t bytes[RsPeerId::SIZE_IN_BYTES] ;
        memset(bytes,0,RsPeerId::SIZE_IN_BYTES) ;
        ChatLobbyId lobby_id = chat_id.toLobbyId();
        memcpy(bytes,&lobby_id,sizeof(ChatLobbyId));
        peer_id = RsPeerId(bytes);
        return true;
    }

    if (chat_id.isDistantChatId()) {
        peer_id = RsPeerId(chat_id.toDistantChatId());
        return true;
    }

    return false;
}

/***** p3History *****/

static void convertMsg(const RsHistoryMsgItem* item, HistoryMsg &msg)
{
	msg.msgId = item->msgId;
	msg.chatPeerId = item->chatPeerId;
	msg.incoming = item->incoming;
	msg.peerId = item->msgPeerId;
	msg.peerName = item->peerName;
	msg.sendTime = item->sendTime;
	msg.recvTime = item->recvTime;
	msg.message = item->message;
}

bool p3HistoryMgr::getMessages(const ChatId &chatId, std::list<HistoryMsg> &msgs, uint32_t loadCount)
{
	msgs.clear();

	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

    RsPeerId chatPeerId;
    bool enabled = false;
    if (chatId.isBroadcast() && mPublicEnable == true) {
        enabled = true;
    }
    if (chatId.isPeerId() && mPrivateEnable == true) {
        enabled = true;
    }
    if (chatId.isLobbyId() && mLobbyEnable == true) {
        enabled = true;
    }
    if (chatId.isDistantChatId() && mDistantEnable == true) {
        enabled = true;
    }

    if(enabled == false)
        return false;

    if(!chatIdToVirtualPeerId(chatId, chatPeerId))
        return false;

#ifdef HISTMGR_DEBUG
    std::cerr << "Getting history for virtual peer " << chatPeerId << std::endl;
#endif

	uint32_t foundCount = 0;

	std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(chatPeerId);

	if (mit != mMessages.end()) 
	{
		std::map<uint32_t, RsHistoryMsgItem*>::reverse_iterator lit;

		for (lit = mit->second.rbegin(); lit != mit->second.rend(); ++lit)
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
#ifdef HISTMGR_DEBUG
	std::cerr << msgs.size() << " messages added." << std::endl;
#endif

	return true;
}

bool p3HistoryMgr::getMessage(uint32_t msgId, HistoryMsg &msg)
{
	RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
	for (mit = mMessages.begin(); mit != mMessages.end(); ++mit) {
		std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.find(msgId);
		if (lit != mit->second.end()) {
			convertMsg(lit->second, msg);
			return true;
		}
	}

	return false;
}

void p3HistoryMgr::clear(const ChatId &chatId)
{
	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

        RsPeerId chatPeerId;
        if(!chatIdToVirtualPeerId(chatId, chatPeerId))
            return;

#ifdef HISTMGR_DEBUG
        std::cerr << "********** p3History::clear()called for virtual peer id " << chatPeerId << std::endl;
#endif

		std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit = mMessages.find(chatPeerId);
		if (mit == mMessages.end()) {
			return;
		}

		std::map<uint32_t, RsHistoryMsgItem*>::iterator lit;
		for (lit = mit->second.begin(); lit != mit->second.end(); ++lit) {
			delete(lit->second);
		}
		mit->second.clear();
		mMessages.erase(mit);

		IndicateConfigChanged();
	}

	RsServer::notify()->notifyHistoryChanged(0, NOTIFY_TYPE_MOD);
}

void p3HistoryMgr::removeMessages(const std::list<uint32_t> &msgIds)
{
	std::list<uint32_t> ids = msgIds;
	std::list<uint32_t> removedIds;
	std::list<uint32_t>::iterator iit;

#ifdef HISTMGR_DEBUG
	std::cerr << "********** p3History::removeMessages called()" << std::endl;
#endif
	{
		RsStackMutex stack(mHistoryMtx); /********** STACK LOCKED MTX ******/

		std::map<RsPeerId, std::map<uint32_t, RsHistoryMsgItem*> >::iterator mit;
		for (mit = mMessages.begin(); mit != mMessages.end(); ++mit)
		{
			iit = ids.begin();
			while ( !ids.empty() || (iit != ids.end()) )
			{
				std::map<uint32_t, RsHistoryMsgItem*>::iterator lit = mit->second.find(*iit);
				if (lit != mit->second.end())
				{
#ifdef HISTMGR_DEBUG
					std::cerr << "**** Removing " << mit->first << " msg id = " << lit->first << std::endl;
#endif

					delete(lit->second);
					mit->second.erase(lit);

					removedIds.push_back(*iit);
					iit = ids.erase(iit);

					continue;
				}

				++iit;
			}
		}
	}

	if (!removedIds.empty())
	{
		IndicateConfigChanged();

		for (iit = removedIds.begin(); iit != removedIds.end(); ++iit)
			RsServer::notify()->notifyHistoryChanged(*iit, NOTIFY_TYPE_DEL);
	}
}

bool p3HistoryMgr::getEnable(uint32_t chat_type)
{
	switch(chat_type)
	{
		case RS_HISTORY_TYPE_PUBLIC : return mPublicEnable ;
		case RS_HISTORY_TYPE_LOBBY  : return mLobbyEnable ;
		case RS_HISTORY_TYPE_PRIVATE: return mPrivateEnable ;
		case RS_HISTORY_TYPE_DISTANT: return mDistantEnable ;
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
		case RS_HISTORY_TYPE_DISTANT: oldValue = mDistantEnable ;
											  mDistantEnable = enable ;
											  break ;
		default:
			return;
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
		default:
			return;
	}

	if (oldValue != count) 
		IndicateConfigChanged();
}
