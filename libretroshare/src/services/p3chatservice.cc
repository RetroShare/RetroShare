/*
 * "$Id: p3ChatService.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#include "util/rsdir.h"
#include "util/rsrandom.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "pqi/pqibin.h"
#include "pqi/pqinotify.h"
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3historymgr.h"

#include "services/p3chatservice.h"
#include "serialiser/rsconfigitems.h"

/****
 * #define CHAT_DEBUG 1
 ****/

static const int 		CONNECTION_CHALLENGE_MAX_COUNT 	= 15  ;	// sends a connexion challenge every 15 messages
static const int 		LOBBY_CACHE_CLEANING_PERIOD    	= 10  ;	// sends a connexion challenge every 15 messages
static const time_t 	MAX_KEEP_MSG_RECORD 					= 240 ;  // keep msg record for 240 secs max.


p3ChatService::p3ChatService(p3LinkMgr *lm, p3HistoryMgr *historyMgr)
	:p3Service(RS_SERVICE_TYPE_CHAT), p3Config(CONFIG_TYPE_CHAT), mChatMtx("p3ChatService"), mLinkMgr(lm) , mHistoryMgr(historyMgr)
{
	addSerialType(new RsChatSerialiser());

	_own_avatar = NULL ;
	_custom_status_string = "" ;
	_default_nick_name = rsPeers->getPeerName(rsPeers->getOwnId());
}

int	p3ChatService::tick()
{
	if (receivedItems()) {
		receiveChatQueue();
	}

	static time_t last_clean_time = 0 ;
	time_t now = time(NULL) ;

	if(last_clean_time + LOBBY_CACHE_CLEANING_PERIOD < now)
	{
		cleanLobbyCaches() ;
		last_clean_time = now ;
	}

	return 0;
}

int	p3ChatService::status()
{
	return 1;
}

/***************** Chat Stuff **********************/

int     p3ChatService::sendPublicChat(const std::wstring &msg)
{
	/* go through all the peers */

	std::list<std::string> ids;
	std::list<std::string>::iterator it;
	mLinkMgr->getOnlineList(ids);

	/* add in own id -> so get reflection */
	std::string ownId = mLinkMgr->getOwnId();
	ids.push_back(ownId);

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat()";
	std::cerr << std::endl;
#endif

	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsChatMsgItem *ci = new RsChatMsgItem();

		ci->PeerId(*it);
		ci->chatFlags = 0;
		ci->sendTime = time(NULL);
		ci->recvTime = ci->sendTime;
		ci->message = msg;

#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::sendChat() Item:";
		std::cerr << std::endl;
		ci->print(std::cerr);
		std::cerr << std::endl;
#endif

		if (*it == ownId) {
			mHistoryMgr->addMessage(false, "", ownId, ci);
		}
		sendItem(ci);
	}

	return 1;
}


class p3ChatService::AvatarInfo
{
   public: 
	  AvatarInfo() 
	  {
		  _image_size = 0 ;
		  _image_data = NULL ;
		  _peer_is_new = false ;			// true when the peer has a new avatar
		  _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
	  }

	  ~AvatarInfo()
	  {
		  delete[] _image_data ;
		  _image_data = NULL ;
		  _image_size = 0 ;
	  }

	  AvatarInfo(const AvatarInfo& ai)
	  {
		  init(ai._image_data,ai._image_size) ;
	  }

	  void init(const unsigned char *jpeg_data,int size)
	  {
		  _image_size = size ;
		  _image_data = new unsigned char[size] ;
		  memcpy(_image_data,jpeg_data,size) ;
	  }
	  AvatarInfo(const unsigned char *jpeg_data,int size)
	  {
		  init(jpeg_data,size) ;
	  }

	  void toUnsignedChar(unsigned char *& data,uint32_t& size) const
	  {
		  data = new unsigned char[_image_size] ;
		  size = _image_size ;
		  memcpy(data,_image_data,size*sizeof(unsigned char)) ;
	  }

	  uint32_t _image_size ;
	  unsigned char *_image_data ;
	  int _peer_is_new ;			// true when the peer has a new avatar
	  int _own_is_new ;			// true when I myself a new avatar to send to this peer.
};

void p3ChatService::sendGroupChatStatusString(const std::string& status_string)
{
	std::list<std::string> ids;
	mLinkMgr->getOnlineList(ids);

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat(): sending group chat status string: " << status_string << std::endl ;
	std::cerr << std::endl;
#endif

	for(std::list<std::string>::iterator it = ids.begin(); it != ids.end(); ++it)
	{
		RsChatStatusItem *cs = new RsChatStatusItem ;

		cs->status_string = status_string ;
		cs->flags = RS_CHAT_FLAG_PUBLIC ;

		cs->PeerId(*it);

		sendItem(cs);
	}
}

void p3ChatService::sendStatusString( const std::string& id , const std::string& status_string)
{
	RsChatStatusItem *cs = new RsChatStatusItem ;

	cs->status_string = status_string ;
	cs->flags = RS_CHAT_FLAG_PRIVATE ;
	cs->PeerId(id);

#ifdef CHAT_DEBUG
	std::cerr  << "sending chat status packet:" << std::endl ;
	cs->print(std::cerr) ;
#endif
	sendItem(cs);
}

void p3ChatService::checkSizeAndSendMessage(RsChatMsgItem *msg)
{
	// We check the message item, and possibly split it into multiple messages, if the message is too big.

	static const uint32_t MAX_STRING_SIZE = 15000 ;

	while(msg->message.size() > MAX_STRING_SIZE)
	{
		// chop off the first 15000 wchars

		RsChatMsgItem *item = msg->duplicate() ;

		item->message = item->message.substr(0,MAX_STRING_SIZE) ;
		msg->message = msg->message.substr(MAX_STRING_SIZE,msg->message.size()-MAX_STRING_SIZE) ;

		// Clear out any one time flags that should not be copied into multiple objects. This is 
		// a precaution, in case the receivign peer does not yet handle split messages transparently.
		//
		item->chatFlags &= (RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_PUBLIC | RS_CHAT_FLAG_LOBBY) ;

		// Indicate that the message is to be continued.
		//
		item->chatFlags |= RS_CHAT_FLAG_PARTIAL_MESSAGE ;
		sendItem(item) ;
	}
	sendItem(msg) ;
}

bool p3ChatService::getVirtualPeerId(const ChatLobbyId& id,std::string& vpid) 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
	std::cerr << "Was asked for virtual peer name of " << std::hex << id << std::dec<< std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.find(id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "   not found!! " << std::endl;
#endif
		return false ;
	}

	vpid = it->second.virtual_peer_id ;
#ifdef CHAT_DEBUG
	std::cerr << "   returning " << vpid << std::endl;
#endif
	return true ;
}

void p3ChatService::locked_printDebugInfo() const
{
	std::cerr << "Recorded lobbies: " << std::endl;

	for( std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin()) ;it!=_chat_lobbys.end();++it)
	{
		std::cerr << "   Lobby id\t\t: " << std::hex << it->first << std::dec << std::endl;
		std::cerr << "   Lobby name\t\t: " << it->second.lobby_name << std::endl;
		std::cerr << "   nick name\t\t: " << it->second.nick_name << std::endl;
		std::cerr << "   Lobby peer id\t: " << it->second.virtual_peer_id << std::endl;
		std::cerr << "   Challenge count\t: " << it->second.connexion_challenge_count << std::endl;
		std::cerr << "   Cached messages\t: " << it->second.msg_cache.size() << std::endl;

		for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
			std::cerr << "       " << std::hex << it2->first << std::dec << "  time=" << it2->second << std::endl;

		std::cerr << "   Participating friends: " << std::endl;

		for(std::set<std::string>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "       " << *it2 << std::endl;

		std::cerr << "   Participating nick names: " << std::endl;

		for(std::set<std::string>::const_iterator it2(it->second.nick_names.begin());it2!=it->second.nick_names.end();++it2)
			std::cerr << "       " << *it2 << std::endl;

	}

	std::cerr << "Recorded lobby names: " << std::endl;

	for( std::map<std::string,ChatLobbyId>::const_iterator it(_lobby_ids.begin()) ;it!=_lobby_ids.end();++it)
		std::cerr << "   \"" << it->first << "\" id = " << std::hex << it->second << std::dec << std::endl;
}

bool p3ChatService::isLobbyId(const std::string& id,ChatLobbyId& lobby_id) 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,ChatLobbyId>::const_iterator it(_lobby_ids.find(id)) ;

	if(it != _lobby_ids.end())
	{
		lobby_id = it->second ;
		return true ;
	}
	else
		return false ;
}

bool     p3ChatService::sendPrivateChat(const std::string &id, const std::wstring &msg)
{
	// look into ID. Is it a peer, or a chat lobby?

	ChatLobbyId lobby_id ;

	if(isLobbyId(id,lobby_id))
		return sendLobbyChat(msg,lobby_id) ;

	// make chat item....
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendPrivateChat()";
	std::cerr << std::endl;
#endif

	RsChatMsgItem *ci = new RsChatMsgItem();

	ci->PeerId(id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE;
	ci->sendTime = time(NULL);
	ci->recvTime = ci->sendTime;
	ci->message = msg;

	if (!mLinkMgr->isOnline(id)) {
		/* peer is offline, add to outgoing list */
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
			privateOutgoingList.push_back(ci);
		}

		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_ADD);

		IndicateConfigChanged();

		return false;
	}

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<std::string,AvatarInfo*>::iterator it = _avatars.find(id) ; 

		if(it == _avatars.end())
		{
			_avatars[id] = new AvatarInfo ;
			it = _avatars.find(id) ;
		}
		if(it->second->_own_is_new)
		{
#ifdef CHAT_DEBUG
			std::cerr << "p3ChatService::sendPrivateChat: new avatar never sent to peer " << id << ". Setting <new> flag to packet." << std::endl; 
#endif

			ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
			it->second->_own_is_new = false ;
		}
	}

#ifdef CHAT_DEBUG
	std::cerr << "Sending msg to peer " << id << ", flags = " << ci->chatFlags << std::endl ;
	std::cerr << "p3ChatService::sendPrivateChat() Item:";
	std::cerr << std::endl;
	ci->print(std::cerr);
	std::cerr << std::endl;
#endif

	mHistoryMgr->addMessage(false, id, mLinkMgr->getOwnId(), ci);

	checkSizeAndSendMessage(ci);

	// Check if custom state string has changed, in which case it should be sent to the peer.
	bool should_send_state_string = false ;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<std::string,StateStringInfo>::iterator it = _state_strings.find(id) ; 

		if(it == _state_strings.end())
		{
			_state_strings[id] = StateStringInfo() ;
			it = _state_strings.find(id) ;
			it->second._own_is_new = true ;
		}
		if(it->second._own_is_new)
		{
			should_send_state_string = true ;
			it->second._own_is_new = false ;
		}
	}

	if(should_send_state_string)
	{
#ifdef CHAT_DEBUG
		std::cerr << "own status string is new for peer " << id << ": sending it." << std::endl ;
#endif
		RsChatStatusItem *cs = makeOwnCustomStateStringItem() ;
		cs->PeerId(id) ;
		sendItem(cs) ;
	}

	return true;
}

bool p3ChatService::checkAndRebuildPartialMessage(RsChatMsgItem *ci)
{
	// Check is the item is ending an incomplete item.
	//
	std::map<std::string,RsChatMsgItem*>::iterator it = _pendingPartialMessages.find(ci->PeerId()) ;

	bool ci_is_partial = ci->chatFlags & RS_CHAT_FLAG_PARTIAL_MESSAGE ;

	if(it != _pendingPartialMessages.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "Pending message found. Happending it." << std::endl;
#endif
		// Yes, there is. Append the item to ci.

		ci->message = it->second->message + ci->message ;
		ci->chatFlags |= it->second->chatFlags ;

		delete it->second ;

		if(!ci_is_partial)
			_pendingPartialMessages.erase(it) ;
	}

	if(ci_is_partial)
	{
#ifdef CHAT_DEBUG
		std::cerr << "Message is partial, storing for later." << std::endl;
#endif
		// The item is a partial message. Push it, and wait for the rest.
		//
		_pendingPartialMessages[ci->PeerId()] = ci ;
		return false ;
	}
	else
	{
#ifdef CHAT_DEBUG
		std::cerr << "Message is complete, using it now." << std::endl;
#endif
		return true ;
	}
}

void p3ChatService::receiveChatQueue()
{
	bool publicChanged = false;
	bool privateChanged = false;

	time_t now = time(NULL);
	RsItem *item ;

	while(NULL != (item=recvItem()))
	{
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::receiveChatQueue() Item:" << (void*)item << std::endl ;
#endif
		RsChatMsgItem *ci = dynamic_cast<RsChatMsgItem*>(item) ;

		if(ci != NULL)	// real chat message
		{
			// check if it's a lobby msg, in which case we replace the peer id by the lobby's virtual peer id.
			//
			RsChatLobbyMsgItem *cli = dynamic_cast<RsChatLobbyMsgItem*>(ci) ;

			if(cli != NULL)
			{
				if(!recvLobbyChat(cli,cli->PeerId()))	// forwards the message to friends, keeps track of subscribers, etc.
				{
					delete ci ;
					continue ;
				}
			}	
			else if(!checkAndRebuildPartialMessage(ci)) 	// Don't delete ! This function is not handled propoerly for chat lobby msgs, so
				continue ;											// we don't use it in this case.

#ifdef CHAT_DEBUG
			std::cerr << "p3ChatService::receiveChatQueue() Item:";
			std::cerr << std::endl;
			ci->print(std::cerr);
			std::cerr << std::endl;
			std::cerr << "Got msg. Flags = " << ci->chatFlags << std::endl ;
#endif

			if(ci->chatFlags & RS_CHAT_FLAG_REQUESTS_AVATAR)		// no msg here. Just an avatar request.
			{
				sendAvatarJpegData(ci->PeerId()) ;
				delete item ;
				continue ;
			}
			else														// normal msg. Return it normally.
			{
				// Check if new avatar is available at peer's. If so, send a request to get the avatar.
				if(ci->chatFlags & RS_CHAT_FLAG_AVATAR_AVAILABLE) 
				{
					std::cerr << "New avatar is available for peer " << ci->PeerId() << ", sending request" << std::endl ;
					sendAvatarRequest(ci->PeerId()) ;
					ci->chatFlags &= ~RS_CHAT_FLAG_AVATAR_AVAILABLE ;
				}

				std::map<std::string,AvatarInfo *>::const_iterator it = _avatars.find(ci->PeerId()) ; 

#ifdef CHAT_DEBUG
				std::cerr << "p3chatservice:: avatar requested from above. " << std::endl ;
#endif
				// has avatar. Return it strait away.
				//
				if(it!=_avatars.end() && it->second->_peer_is_new)
				{
					std::cerr << "Avatar is new for peer. ending info above" << std::endl ;
					ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
				}

				if ((ci->chatFlags & RS_CHAT_FLAG_PRIVATE) == 0) {
					/* notify public chat message */
					std::string message;
					message.assign(ci->message.begin(), ci->message.end());
					getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAT_NEW, ci->PeerId(), message, "");
				}

				{
					RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

					ci->recvTime = now;

					if (ci->chatFlags & RS_CHAT_FLAG_PRIVATE) {
						std::cerr << "Adding msg 0x" << std::hex << (void*)ci << std::dec << " to private chat incoming list." << std::endl;
						privateChanged = true;
						privateIncomingList.push_back(ci);	// don't delete the item !!
					} else {
						std::cerr << "Adding msg 0x" << std::hex << (void*)ci << std::dec << " to public chat incoming list." << std::endl;
						publicChanged = true;
						publicList.push_back(ci);	// don't delete the item !!

						if (ci->PeerId() != mLinkMgr->getOwnId()) {
							/* not from loop back */
							mHistoryMgr->addMessage(true, "", ci->PeerId(), ci);
						}
					}
				} /* UNLOCK */
			}
			continue ;
		}

		RsChatStatusItem *cs = dynamic_cast<RsChatStatusItem*>(item) ;

		if(cs != NULL)
		{
#ifdef CHAT_DEBUG
			std::cerr << "Received status string \"" << cs->status_string << "\"" << std::endl ;
#endif

			if(cs->flags & RS_CHAT_FLAG_REQUEST_CUSTOM_STATE){ // no state here just a request.
				sendCustomState(cs->PeerId()) ;

			}
			else // Check if new custom string is available at peer's. If so, send a request to get the custom string.
				if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE)
				{
					receiveStateString(cs->PeerId(),cs->status_string) ;	// store it
					rsicontrol->getNotify().notifyCustomState(cs->PeerId(), cs->status_string) ;
				}else
					if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE){

					std::cerr << "New custom state is available for peer " << cs->PeerId() << ", sending request" << std::endl ;
					sendCustomStateRequest(cs->PeerId()) ;
					}else
						if(cs->flags & RS_CHAT_FLAG_PRIVATE)
							rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,true) ;
						else
							if(cs->flags & RS_CHAT_FLAG_PUBLIC)
								rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,false) ;

			delete item;
			continue ;
		}

		RsChatAvatarItem *ca = dynamic_cast<RsChatAvatarItem*>(item) ;

		if(ca != NULL)
		{
			receiveAvatarJpegData(ca) ;

#ifdef CHAT_DEBUG
			std::cerr << "Received avatar data for peer " << ca->PeerId() << ". Notifying." << std::endl ;
#endif
			rsicontrol->getNotify().notifyPeerHasNewAvatar(ca->PeerId()) ;

			delete item ;
			continue ;
		}

		RsChatLobbyInviteItem *cl = dynamic_cast<RsChatLobbyInviteItem*>(item) ;

		if(cl != NULL)
		{
			handleRecvLobbyInvite(cl) ;
			delete item ;
			continue ;
		}

		RsChatLobbyConnectChallengeItem *cn = dynamic_cast<RsChatLobbyConnectChallengeItem*>(item) ;

		if(cn != NULL)
		{
			handleConnectionChallenge(cn) ;
			delete item ;
			continue ;
		}

		RsChatLobbyUnsubscribeItem *cu = dynamic_cast<RsChatLobbyUnsubscribeItem*>(item) ;

		if(cu != NULL)
		{
			handleFriendUnsubscribeLobby(cu) ;
			delete item ;
			continue ;
		}


		std::cerr << "Received ChatItem of unhandled type: " << std::endl;
		item->print(std::cerr,0) ;
	}

	if (publicChanged) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PUBLIC_CHAT, NOTIFY_TYPE_ADD);
	}
	if (privateChanged) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);

		IndicateConfigChanged(); // only private chat messages are saved
	}
}

int p3ChatService::getPublicChatQueueCount()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	return publicList.size();
}

bool p3ChatService::getPublicChatQueue(std::list<ChatInfo> &chats)
{
	bool changed = false;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		// get the items from the public list.
		if (publicList.size() == 0) {
			return false;
		}

		std::list<RsChatMsgItem *>::iterator it;
		while (publicList.size()) {
			RsChatMsgItem *c = publicList.front();
			publicList.pop_front();

			ChatInfo ci;
			initRsChatInfo(c, ci);
			chats.push_back(ci);

			changed = true;

			delete c;
		}
	} /* UNLOCKED */

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PUBLIC_CHAT, NOTIFY_TYPE_DEL);
	}
	
	return true;
}

int p3ChatService::getPrivateChatQueueCount(bool incoming)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	if (incoming) {
		return privateIncomingList.size();
	}

	return privateOutgoingList.size();
}

bool p3ChatService::getPrivateChatQueueIds(bool incoming, std::list<std::string> &ids)
{
	ids.clear();

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::list<RsChatMsgItem *> *list;

	if (incoming) {
		list = &privateIncomingList;
	} else {
		list = &privateOutgoingList;
	}
	
	// get the items from the private list.
	if (list->size() == 0) {
		return false;
	}

	std::list<RsChatMsgItem *>::iterator it;
	for (it = list->begin(); it != list->end(); it++) {
		RsChatMsgItem *c = *it;

		if (std::find(ids.begin(), ids.end(), c->PeerId()) == ids.end()) {
			ids.push_back(c->PeerId());
		}
	}

	return true;
}

bool p3ChatService::getPrivateChatQueue(bool incoming, const std::string &id, std::list<ChatInfo> &chats)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::list<RsChatMsgItem *> *list;

	if (incoming) {
		list = &privateIncomingList;
	} else {
		list = &privateOutgoingList;
	}

	// get the items from the private list.
	if (list->size() == 0) {
		return false;
	}

	std::list<RsChatMsgItem *>::iterator it;
	for (it = list->begin(); it != list->end(); it++) {
		RsChatMsgItem *c = *it;

		if (c->PeerId() == id) {
			ChatInfo ci;
			initRsChatInfo(c, ci);
			chats.push_back(ci);
		}
	}

	return (chats.size() > 0);
}

bool p3ChatService::clearPrivateChatQueue(bool incoming, const std::string &id)
{
	bool changed = false;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::list<RsChatMsgItem *> *list;

		if (incoming) {
			list = &privateIncomingList;
		} else {
			list = &privateOutgoingList;
		}

		// get the items from the private list.
		if (list->size() == 0) {
			return false;
		}

		std::list<RsChatMsgItem *>::iterator it = list->begin();
		while (it != list->end()) {
			RsChatMsgItem *c = *it;

			if (c->PeerId() == id) {
				if (incoming) {
					mHistoryMgr->addMessage(true, c->PeerId(), c->PeerId(), c);
				}

				delete c;
				changed = true;

				std::list<RsChatMsgItem *>::iterator it1 = it;
				it++;
				list->erase(it1);

				continue;
			}

			it++;
		}
	} /* UNLOCKED */

	if (changed) {
		rsicontrol->getNotify().notifyListChange(incoming ? NOTIFY_LIST_PRIVATE_INCOMING_CHAT : NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_DEL);

		IndicateConfigChanged();
	}

	return true;
}

void p3ChatService::initRsChatInfo(RsChatMsgItem *c, ChatInfo &i)
{
	i.rsid = c->PeerId();
	i.chatflags = 0;
	i.sendTime = c->sendTime;
	i.recvTime = c->recvTime;
	i.msg  = c->message;

	RsChatLobbyMsgItem *lobbyItem = dynamic_cast<RsChatLobbyMsgItem*>(c) ;

	if(lobbyItem != NULL)
		i.peer_nickname = lobbyItem->nick;

	if (c -> chatFlags & RS_CHAT_FLAG_PRIVATE)
		i.chatflags |= RS_CHAT_PRIVATE;
	else
		i.chatflags |= RS_CHAT_PUBLIC;
}

void p3ChatService::setOwnCustomStateString(const std::string& s)
{
	std::list<std::string> onlineList;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
		std::cerr << "p3chatservice: Setting own state string to new value : " << s << std::endl ;
#endif
		_custom_status_string = s ;

		for(std::map<std::string,StateStringInfo>::iterator it(_state_strings.begin());it!=_state_strings.end();++it)
			it->second._own_is_new = true ;

		mLinkMgr->getOnlineList(onlineList);
	}

	rsicontrol->getNotify().notifyOwnStatusMessageChanged() ;

	// alert your online peers to your newly set status
	std::list<std::string>::iterator it(onlineList.begin());
	for(; it != onlineList.end(); it++){

		RsChatStatusItem *cs = new RsChatStatusItem();
		cs->flags = RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE;
		cs->status_string = "";
		cs->PeerId(*it);
		sendItem(cs);
	}

	IndicateConfigChanged();
}

void p3ChatService::setOwnAvatarJpegData(const unsigned char *data,int size)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
		std::cerr << "p3chatservice: Setting own avatar to new image." << std::endl ;
#endif

		if(_own_avatar != NULL)
			delete _own_avatar ;

		_own_avatar = new AvatarInfo(data,size) ;

		// set the info that our avatar is new, for all peers
		for(std::map<std::string,AvatarInfo *>::iterator it(_avatars.begin());it!=_avatars.end();++it)
			it->second->_own_is_new = true ;
	}
	IndicateConfigChanged();

	rsicontrol->getNotify().notifyOwnAvatarChanged() ;

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:setOwnAvatarJpegData() done." << std::endl ;
#endif

}

void p3ChatService::receiveStateString(const std::string& id,const std::string& s)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: received custom state string for peer " << id << ". Storing it." << std::endl ;
#endif

   bool new_peer = (_state_strings.find(id) == _state_strings.end()) ;

   _state_strings[id]._custom_status_string = s ;
   _state_strings[id]._peer_is_new = true ;
   _state_strings[id]._own_is_new = new_peer ;
}

void p3ChatService::receiveAvatarJpegData(RsChatAvatarItem *ci)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: received avatar jpeg data for peer " << ci->PeerId() << ". Storing it." << std::endl ;
#endif

   bool new_peer = (_avatars.find(ci->PeerId()) == _avatars.end()) ;

   if (new_peer == false && _avatars[ci->PeerId()]) {
       delete _avatars[ci->PeerId()];
   }
   _avatars[ci->PeerId()] = new AvatarInfo(ci->image_data,ci->image_size) ; 
   _avatars[ci->PeerId()]->_peer_is_new = true ;
   _avatars[ci->PeerId()]->_own_is_new = new_peer ;
}

std::string p3ChatService::getOwnCustomStateString() 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	return _custom_status_string ;
}
void p3ChatService::getOwnAvatarJpegData(unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	uint32_t s = 0 ;
#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: own avatar requested from above. " << std::endl ;
#endif
	// has avatar. Return it strait away.
	//
	if(_own_avatar != NULL)
	{
	   _own_avatar->toUnsignedChar(data,s) ;
		size = s ;
	}
	else
	{
		data=NULL ;
		size=0 ;
	}
}

std::string p3ChatService::getCustomStateString(const std::string& peer_id) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,StateStringInfo>::iterator it = _state_strings.find(peer_id) ; 

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: status string for peer " << peer_id << " requested from above. " << std::endl ;
#endif
	// has it. Return it strait away.
	//
	if(it!=_state_strings.end())
	{
	   it->second._peer_is_new = false ;
#ifdef CHAT_DEBUG
	   std::cerr << "Already has status string. Returning it" << std::endl ;
#endif
	   return it->second._custom_status_string ;
	}


#ifdef CHAT_DEBUG
		std::cerr << "No status string for this peer. requesting it." << std::endl ;
#endif


	sendCustomStateRequest(peer_id);
	return std::string() ;
}

void p3ChatService::getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,AvatarInfo *>::const_iterator it = _avatars.find(peer_id) ; 

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: avatar for peer " << peer_id << " requested from above. " << std::endl ;
#endif
	// has avatar. Return it straight away.
	//
	if(it!=_avatars.end())
	{
		uint32_t s=0 ;
	   it->second->toUnsignedChar(data,s) ;
		size = s ;
	   it->second->_peer_is_new = false ;
#ifdef CHAT_DEBUG
	   std::cerr << "Already has avatar. Returning it" << std::endl ;
#endif
	   return ;
       } else {
           #ifdef CHAT_DEBUG
	   std::cerr << "No avatar for this peer. Requesting it by sending request packet." << std::endl ;
           #endif
       }

        sendAvatarRequest(peer_id);
}

void p3ChatService::sendAvatarRequest(const std::string& peer_id)
{
	// Doesn't have avatar. Request it.
	//
	RsChatMsgItem *ci = new RsChatMsgItem();

	ci->PeerId(peer_id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_REQUESTS_AVATAR ;
	ci->sendTime = time(NULL);
	ci->message.erase();

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sending request for avatar, to peer " << peer_id << std::endl ;
	std::cerr << std::endl;
#endif

	sendItem(ci);
}

void p3ChatService::sendCustomStateRequest(const std::string& peer_id){

	RsChatStatusItem* cs = new RsChatStatusItem;

	cs->PeerId(peer_id);
	cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_REQUEST_CUSTOM_STATE ;
	cs->status_string.erase();

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sending request for status, to peer " << peer_id << std::endl ;
	std::cerr << std::endl;
#endif

	sendItem(cs);
}

RsChatStatusItem *p3ChatService::makeOwnCustomStateStringItem()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	RsChatStatusItem *ci = new RsChatStatusItem();

	ci->flags = RS_CHAT_FLAG_CUSTOM_STATE ;
	ci->status_string = _custom_status_string ;

	return ci ;
}
RsChatAvatarItem *p3ChatService::makeOwnAvatarItem()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	RsChatAvatarItem *ci = new RsChatAvatarItem();

	_own_avatar->toUnsignedChar(ci->image_data,ci->image_size) ;

	return ci ;
}


void p3ChatService::sendAvatarJpegData(const std::string& peer_id)
{
   #ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: sending requested for peer " << peer_id << ", data=" << (void*)_own_avatar << std::endl ;
   #endif

   if(_own_avatar != NULL)
	{
		RsChatAvatarItem *ci = makeOwnAvatarItem();
		ci->PeerId(peer_id);

		// take avatar, and embed it into a std::wstring.
		//
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::sending avatar image to peer" << peer_id << ", image size = " << ci->image_size << std::endl ;
		std::cerr << std::endl;
#endif

		sendItem(ci) ;
	}
   else {
#ifdef CHAT_DEBUG
        std::cerr << "We have no avatar yet: Doing nothing" << std::endl ;
#endif
   }
}

void p3ChatService::sendCustomState(const std::string& peer_id){

#ifdef CHAT_DEBUG
std::cerr << "p3chatservice: sending requested status string for peer " << peer_id << std::endl ;
#endif

	RsChatStatusItem *cs = makeOwnCustomStateStringItem();
	cs->PeerId(peer_id);

	sendItem(cs);
}

bool p3ChatService::loadList(std::list<RsItem*>& load)
{
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
		RsChatAvatarItem *ai = NULL ;

		if(NULL != (ai = dynamic_cast<RsChatAvatarItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			_own_avatar = new AvatarInfo(ai->image_data,ai->image_size) ;

			delete *it;

			continue;
		}

		RsChatStatusItem *mitem = NULL ;

		if(NULL != (mitem = dynamic_cast<RsChatStatusItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			_custom_status_string = mitem->status_string ;

			delete *it;

			continue;
		}

		RsPrivateChatMsgConfigItem *citem = NULL ;

		if(NULL != (citem = dynamic_cast<RsPrivateChatMsgConfigItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			if (citem->chatFlags & RS_CHAT_FLAG_PRIVATE) {
				RsChatMsgItem *ci = new RsChatMsgItem();
				citem->get(ci);

				if (citem->configFlags & RS_CHATMSG_CONFIGFLAG_INCOMING) {
					privateIncomingList.push_back(ci);
				} else {
					privateOutgoingList.push_back(ci);
				}
			} else {
				// ignore all other items
			}


			delete *it;

			continue;
		}

		RsConfigKeyValueSet *vitem = NULL ;

		if(NULL != (vitem = dynamic_cast<RsConfigKeyValueSet*>(*it)))
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
				if(kit->key == "DEFAULT_NICK_NAME")
				{
					std::cerr << "Loaded config default nick name for chat: " << kit->value << std::endl ;
					_default_nick_name = kit->value ;
				}

		// delete unknown items
		delete *it;
	}
	return true;
}

bool p3ChatService::saveList(bool& cleanup, std::list<RsItem*>& list)
{
	cleanup = true;

	/* now we create a pqistore, and stream all the msgs into it */

	if(_own_avatar != NULL)
	{
		RsChatAvatarItem *ci = makeOwnAvatarItem() ;
		ci->PeerId(mLinkMgr->getOwnId());

		list.push_back(ci) ;
	}

	mChatMtx.lock(); /****** MUTEX LOCKED *******/

	RsChatStatusItem *di = new RsChatStatusItem ;
	di->status_string = _custom_status_string ;
	di->flags = RS_CHAT_FLAG_CUSTOM_STATE ;

	list.push_back(di) ;

	/* save incoming private chat messages */

	std::list<RsChatMsgItem *>::iterator it;
	for (it = privateIncomingList.begin(); it != privateIncomingList.end(); it++) {
		RsPrivateChatMsgConfigItem *ci = new RsPrivateChatMsgConfigItem;

		ci->set(*it, (*it)->PeerId(), RS_CHATMSG_CONFIGFLAG_INCOMING);

		list.push_back(ci);
	}

	/* save outgoing private chat messages */

	for (it = privateOutgoingList.begin(); it != privateOutgoingList.end(); it++) {
		RsPrivateChatMsgConfigItem *ci = new RsPrivateChatMsgConfigItem;

		ci->set(*it, (*it)->PeerId(), 0);

		list.push_back(ci);
	}

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "DEFAULT_NICK_NAME" ;
	kv.value = _default_nick_name ;
	vitem->tlvkvs.pairs.push_back(kv) ;

	list.push_back(vitem) ;

	return true;
}

void p3ChatService::saveDone()
{
	/* unlock mutex */
	mChatMtx.unlock(); /****** MUTEX UNLOCKED *******/
}

RsSerialiser *p3ChatService::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsChatSerialiser) ;
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss ;
}

/*************** pqiMonitor callback ***********************/

void p3ChatService::statusChange(const std::list<pqipeer> &plist)
{
	std::list<pqipeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); it++) {
		if (it->state & RS_PEER_S_FRIEND) {
			if (it->actions & RS_PEER_CONNECTED) {
				/* send the saved outgoing messages */
				bool changed = false;

				if (privateOutgoingList.size()) {
					RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

					std::string ownId = mLinkMgr->getOwnId();

					std::list<RsChatMsgItem *>::iterator cit = privateOutgoingList.begin();
					while (cit != privateOutgoingList.end()) {
						RsChatMsgItem *c = *cit;

						if (c->PeerId() == it->id) {
							mHistoryMgr->addMessage(false, c->PeerId(), ownId, c);

							checkSizeAndSendMessage(c); // delete item

							changed = true;

							cit = privateOutgoingList.erase(cit);

							continue;
						}

						cit++;
					}
				} /* UNLOCKED */

				if (changed) {
					rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_DEL);

					IndicateConfigChanged();
				}
			}
		}
	}
}

//********************** Chat Lobby Stuff ***********************//

// returns:
// 	true: the message is not a duplicate and should be shown
// 	false: the message is a duplicate or there is an error, and should be destroyed.
//
bool p3ChatService::recvLobbyChat(RsChatLobbyMsgItem *item,const std::string& peer_id)
{
	bool send_challenge = false ;
	std::string lobby_virtual_peer_id ;
	ChatLobbyId send_challenge_lobby ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		locked_printDebugInfo() ; // debug
#ifdef CHAT_DEBUG
		std::cerr << "Handling ChatLobbyMsg " << std::hex << item->msg_id << ", lobby id " << item->lobby_id << ", from peer id " << item->PeerId() << std::endl;
#endif

		// send upward for display

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(item->lobby_id)) ;

		if(it == _chat_lobbys.end())
		{
			std::cerr << "Chatlobby for id " << std::hex << item->lobby_id << " has no record. Dropping the msg." << std::dec << std::endl;
			return false ;
		}
		ChatLobbyEntry& lobby(it->second) ;

		// Adds the peer id to the list of friend participants, even if it's not original msg source

		lobby.participating_friends.insert(peer_id) ;

		if(item->nick != "Lobby management")		// not nice ! We need a lobby management flag.
			lobby.nick_names.insert(item->nick) ;

		lobby_virtual_peer_id = lobby.virtual_peer_id ;

		// Checks wether the msg is already recorded or not

		std::map<ChatLobbyMsgId,time_t>::const_iterator it2(lobby.msg_cache.find(item->msg_id)) ;

		if(it2 != lobby.msg_cache.end()) // found!
		{
			std::cerr << "  Msg already received at time " << it2->second << ". Dropping!" << std::endl ;
			return false ;
		}
#ifdef CHAT_DEBUG
		std::cerr << "  Msg already not received already. Adding in cache, and forwarding!" << std::endl ;
#endif

		lobby.msg_cache[item->msg_id] = time(NULL) ;

		// Forward to allparticipating friends, except this peer.

		for(std::set<std::string>::const_iterator it(lobby.participating_friends.begin());it!=lobby.participating_friends.end();++it)
			if((*it)!=peer_id && mLinkMgr->isOnline(*it)) 
			{
				RsChatLobbyMsgItem *item2 = new RsChatLobbyMsgItem(*item) ;	// copy almost everything

				item2->PeerId(*it) ;	// replaces the virtual peer id with the actual destination.

				sendItem(item2);
			}

		if(++lobby.connexion_challenge_count > CONNECTION_CHALLENGE_MAX_COUNT) 
		{
			lobby.connexion_challenge_count = 0 ;
			 send_challenge_lobby = item->lobby_id ;
			 send_challenge = true ;
		}
	}

	if(send_challenge)
		sendConnectionChallenge(send_challenge_lobby) ;

	item->PeerId(lobby_virtual_peer_id) ;	// updates the peer id for proper display
	return true ;
}

bool p3ChatService::sendLobbyChat(const std::wstring& msg, const ChatLobbyId& lobby_id,bool management) 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
	std::cerr << "Sending chat lobby message to lobby " << std::hex << lobby_id << std::dec << std::endl;
	std::cerr << "msg:" << std::endl;
	std::wcerr << msg << std::endl;
#endif

	// get a pointer to the info for that chat lobby.
	//
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(lobby_id)) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << "Chatlobby for id " << std::hex << lobby_id << " has no record. This is a serious error!!" << std::dec << std::endl;
		return false ;
	}
	ChatLobbyEntry& lobby(it->second) ;

	RsChatLobbyMsgItem item ;

	// chat lobby stuff
	//
	do { item.msg_id	= RSRandom::random_u64(); } while( lobby.msg_cache.find(item.msg_id) != lobby.msg_cache.end() ) ;

	lobby.msg_cache[item.msg_id] = time(NULL) ;	// put the msg in cache!

	item.lobby_id = lobby_id ;

	if(management)
		item.nick = "Lobby management" ;
	else
		item.nick = lobby.nick_name ;

	// chat msg stuff
	//
	item.chatFlags = RS_CHAT_FLAG_LOBBY | RS_CHAT_FLAG_PRIVATE;
	item.sendTime = time(NULL);
	item.recvTime = item.sendTime;
	item.message = msg;

	for(std::set<std::string>::const_iterator it(lobby.participating_friends.begin());it!=lobby.participating_friends.end();++it)
		if(mLinkMgr->isOnline(*it)) 
		{
			RsChatLobbyMsgItem *sitem = new RsChatLobbyMsgItem(item) ;	// copies almost everything

			sitem->PeerId(*it) ;

			sendItem(sitem);
		}

	locked_printDebugInfo() ; // debug
	return true ;
}

void p3ChatService::handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) 
{
	// Look into message cache of all lobbys to handle the challenge.
	//
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::handleConnectionChallenge(): received connexion challenge:" << std::endl;
	std::cerr << "    Challenge code = 0x" << std::hex << item->challenge_code << std::dec << std::endl;
	std::cerr << "    Peer Id        =   " << item->PeerId() << std::endl;
#endif

	ChatLobbyId lobby_id ;
	bool found = false ;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end() && !found;++it)
			for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end() && !found;++it2)
			{
				uint64_t code = makeConnexionChallengeCode(it->first,it2->first) ;
#ifdef CHAT_DEBUG
				std::cerr << "    Lobby_id = 0x" << std::hex << it->first << ", msg_id = 0x" << it2->first << ": code = 0x" << code << std::dec << std::endl ;
#endif

				if(code == item->challenge_code)
				{
#ifdef CHAT_DEBUG
					std::cerr << "    Challenge accepted for lobby " << std::hex << it->first << ", for chat msg " << it2->first << std::dec << std::endl ;
					std::cerr << "    Sending connection request to peer " << item->PeerId() << std::endl;
#endif

					lobby_id = it->first ;
					found = true ;

					// also add the peer to the list of participating friends
					it->second.participating_friends.insert(item->PeerId()) ; 
				}
			}
	}

	if(found) // send invitation. As the peer already has the lobby, the invitation will most likely be accepted.
		invitePeerToLobby(lobby_id, item->PeerId()) ;
	else
		std::cerr << "    Challenge denied: no existing cached msg has matching Id." << std::endl;
}

void p3ChatService::sendConnectionChallenge(ChatLobbyId lobby_id) 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
	std::cerr << "Sending connection challenge to friends for lobby 0x" << std::hex << lobby_id << std::dec << std::endl ;
#endif

	// look for a msg in cache. Any recent msg is fine.
	
	std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << "ERROR: sendConnectionChallenge(): could not find lobby 0x" << std::hex << lobby_id << std::dec << std::endl;
		return ;
	}

	time_t now = time(NULL) ;
	uint64_t code = 0 ;

	for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
		if(it2->second + 20 > now)  // any msg not older than 20 seconds is fine.
		{
			code = makeConnexionChallengeCode(lobby_id,it2->first) ;
#ifdef CHAT_DEBUG
			std::cerr << "  Using msg id 0x" << std::hex << it2->first << ", challenge code = " << code << std::dec << std::endl; 
#endif
			break ;
		}

	if(code == 0)
	{
		std::cerr << "  No suitable message found in cache. Weird !!" << std::endl;
		return ;
	}

	// Broadcast to all direct friends

	std::list<std::string> ids ;
	mLinkMgr->getOnlineList(ids);

	for(std::list<std::string>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		RsChatLobbyConnectChallengeItem *item = new RsChatLobbyConnectChallengeItem ;

		item->PeerId(*it) ;
		item->challenge_code = code ;

		sendItem(item);
	}
}

uint64_t p3ChatService::makeConnexionChallengeCode(ChatLobbyId lobby_id,ChatLobbyMsgId msg_id)
{
	return ((uint64_t)lobby_id) ^ (uint64_t)msg_id ;
}

void p3ChatService::getChatLobbyList(std::list<ChatLobbyInfo>& linfos) 
{
	// fill up a dummy list for now.

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	linfos.clear() ;

	for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
		linfos.push_back(it->second) ;
}
void p3ChatService::invitePeerToLobby(const ChatLobbyId& lobby_id, const std::string& peer_id) 
{
#ifdef CHAT_DEBUG
	std::cerr << "Sending invitation to peer " << peer_id << " to lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif

	RsChatLobbyInviteItem *item = new RsChatLobbyInviteItem ;

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << "  invitation send: canceled. Lobby " << lobby_id << " not found!" << std::endl;
		return ;
	}
	item->lobby_id = lobby_id ;
	item->lobby_name = it->second.lobby_name ;
	item->PeerId(peer_id) ;

	sendItem(item) ;
}
void p3ChatService::handleRecvLobbyInvite(RsChatLobbyInviteItem *item) 
{
#ifdef CHAT_DEBUG
	std::cerr << "Received invite to lobby from " << item->PeerId() << " to lobby " << item->lobby_id << ", named " << item->lobby_name << std::endl;
#endif

	// 1 - store invite in a cache
	//
	// 1.1 - if the lobby is already setup, add the peer to the communicating peers.
	//
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
			std::cerr << "  Lobby already exists. Addign new friend " << item->PeerId() << " to it" << std::endl;

			it->second.participating_friends.insert(item->PeerId()) ;
			return ;
		}
		// no, then create a new invitation entry in the cache.
		
		ChatLobbyInvite invite ;
		invite.lobby_id = item->lobby_id ;
		invite.peer_id = item->PeerId() ;
		invite.lobby_name = item->lobby_name ;

		_lobby_invites_queue[item->lobby_id] = invite ;
	}
	// 2 - notify the gui to ask the user.
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_INVITATION, NOTIFY_TYPE_ADD);
}

void p3ChatService::getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites)
{
	invites.clear() ;

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	for(std::map<ChatLobbyId,ChatLobbyInvite>::const_iterator it(_lobby_invites_queue.begin());it!=_lobby_invites_queue.end();++it)
		invites.push_back(it->second) ;
}

bool p3ChatService::acceptLobbyInvite(const ChatLobbyId& lobby_id) 
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
		std::cerr << "Accepting chat lobby "<< lobby_id << std::endl;
#endif

		std::map<ChatLobbyId,ChatLobbyInvite>::iterator it = _lobby_invites_queue.find(lobby_id) ;

		if(it == _lobby_invites_queue.end())
		{
			std::cerr << " (EE) lobby invite not in cache!!" << std::endl;
			return false;
		}

		if(_chat_lobbys.find(lobby_id) != _chat_lobbys.end())
		{
			std::cerr << "  (II) Lobby already exists. Weird." << std::endl;
			return true ;
		}

#ifdef CHAT_DEBUG
		std::cerr << "  Creating new Lobby entry." << std::endl;
#endif

		ChatLobbyEntry entry ;
		entry.participating_friends.insert(it->second.peer_id) ;
		entry.nick_name = _default_nick_name ;	// to be changed. For debug only!!
		entry.lobby_id = lobby_id ;
		entry.lobby_name = it->second.lobby_name ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;

		_lobby_ids[entry.virtual_peer_id] = lobby_id ;
		_chat_lobbys[lobby_id] = entry ;

		_lobby_invites_queue.erase(it) ;		// remove the invite from cache.

		// we should also send a message to the lobby to tell we're here.

#ifdef CHAT_DEBUG
		std::cerr << "  Pushing new msg item to incoming msgs." << std::endl;
#endif

		RsChatLobbyMsgItem *item = new RsChatLobbyMsgItem;
		item->lobby_id = entry.lobby_id ;
		item->msg_id = 0 ;
		item->nick = "Lobby management" ;
		item->message = std::wstring(L"Welcome to chat lobby") ;
		item->PeerId(entry.virtual_peer_id) ;
		item->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_LOBBY ;

		privateIncomingList.push_back(item) ;
	}
#ifdef CHAT_DEBUG
	std::cerr << "  Notifying of new recvd msg." << std::endl ;
#endif

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);

	// send AKN item
	std::wstring wmsg(_default_nick_name.length(), L' '); // Make room for characters
	// Copy string to wstring.
	std::copy(_default_nick_name.begin(), _default_nick_name.end(), wmsg.begin());

	sendLobbyChat(wmsg + L" joined the lobby",lobby_id,true) ;
	return true ;
}

std::string p3ChatService::makeVirtualPeerId(ChatLobbyId lobby_id)
{
	std::ostringstream os ;
	os << "Chat Lobby 0x" << std::hex << lobby_id << std::dec ;

	return os.str() ;
}


void p3ChatService::denyLobbyInvite(const ChatLobbyId& lobby_id) 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
	std::cerr << "Denying chat lobby invite to "<< lobby_id << std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyInvite>::iterator it = _lobby_invites_queue.find(lobby_id) ;

	if(it == _lobby_invites_queue.end())
	{
		std::cerr << " (EE) lobby invite not in cache!!" << std::endl;
		return ;
	}

	_lobby_invites_queue.erase(it) ;
}

ChatLobbyId p3ChatService::createChatLobby(const std::string& lobby_name,const std::list<std::string>& invited_friends)
{
#ifdef CHAT_DEBUG
	std::cerr << "Creating a new Chat lobby !!" << std::endl;
#endif
	ChatLobbyId lobby_id ;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		// create a unique id.
		//
		do { lobby_id = RSRandom::random_u64() ; } while(_chat_lobbys.find(lobby_id) != _chat_lobbys.end()) ;

#ifdef CHAT_DEBUG
		std::cerr << "  New (unique) ID: " << std::hex << lobby_id << std::dec << std::endl;
#endif

		ChatLobbyEntry entry ;
		entry.participating_friends.clear() ;
		entry.nick_name = _default_nick_name ;	// to be changed. For debug only!!
		entry.lobby_id = lobby_id ;
		entry.lobby_name = lobby_name ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;

		_lobby_ids[entry.virtual_peer_id] = lobby_id ;
		_chat_lobbys[lobby_id] = entry ;
	}

	for(std::list<std::string>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	return lobby_id ;
}

void p3ChatService::handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem *item)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

#ifdef CHAT_DEBUG
	std::cerr << "Received unsubscribed to lobby " << item->lobby_id << ", from friend " << item->PeerId() << std::endl;
#endif

	if(it == _chat_lobbys.end())
	{
		std::cerr << "Chat lobby " << item->lobby_id << " does not exist ! Can't unsubscribe!" << std::endl;
		return ;
	}

	for(std::set<std::string>::iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		if(*it2 == item->PeerId())
		{
#ifdef CHAT_DEBUG
			std::cerr << "  removing peer id " << item->PeerId() << " from participant list of lobby " << item->lobby_id << std::endl;
#endif
			it->second.participating_friends.erase(it2) ;
			break ;
		}
}

void p3ChatService::unsubscribeChatLobby(const ChatLobbyId& id)
{
	// send AKN item
	std::string nick ;
	getNickNameForChatLobby(id,nick) ;
	std::wstring wmsg(nick.length(), L' '); // Make room for characters
	std::copy(nick.begin(), nick.end(), wmsg.begin());
	sendLobbyChat(wmsg + L" has left the lobby",id,true) ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(id) ;

		if(it == _chat_lobbys.end())
		{
			std::cerr << "Chat lobby " << id << " does not exist ! Can't unsubscribe!" << std::endl;
			return ;
		}

		// send a lobby leaving packet to all friends

		for(std::set<std::string>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		{
			RsChatLobbyUnsubscribeItem *item = new RsChatLobbyUnsubscribeItem ;

			item->lobby_id = id ;
			item->PeerId(*it2) ;

			sendItem(item) ;
		}

		// remove lobby information

		_chat_lobbys.erase(it) ;

		for(std::map<std::string,ChatLobbyId>::iterator it2(_lobby_ids.begin());it2!=_lobby_ids.end();++it2)
			if(it2->second == id)
			{
				_lobby_ids.erase(it2) ;
				break ;
			}
	}
	// done!
}
bool p3ChatService::setDefaultNickNameForChatLobby(const std::string& nick)
{
	_default_nick_name = nick;
	IndicateConfigChanged() ;
	return true ;
}
bool p3ChatService::getDefaultNickNameForChatLobby(std::string& nick)
{
	nick = _default_nick_name ;
	return true ;
}
bool p3ChatService::getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	
#ifdef CHAT_DEBUG
	std::cerr << "getting nickname for chat lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << " (EE) lobby does not exist!!" << std::endl;
		return false ;
	}

	nick = it->second.nick_name ;
	return true ;
}

bool p3ChatService::setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	
#ifdef CHAT_DEBUG
	std::cerr << "Changing nickname for chat lobby " << std::hex << lobby_id << std::dec << " to " << nick << std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << " (EE) lobby does not exist!!" << std::endl;
		return false;
	}

	it->second.nick_name = nick ;
	return true ;
}

void p3ChatService::cleanLobbyCaches()
{
#ifdef CHAT_DEBUG
	std::cerr << "Cleaning chat lobby caches." << std::endl;
#endif

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	time_t now = time(NULL) ;

	for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.begin();it!=_chat_lobbys.end();++it)
		for(std::map<ChatLobbyMsgId,time_t>::iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();)
			if(it2->second + MAX_KEEP_MSG_RECORD < now)
			{
#ifdef CHAT_DEBUG
				std::cerr << "  removing old msg 0x" << std::hex << it2->first << ", time=" << std::dec << it2->second << std::endl;
#endif

				std::map<ChatLobbyMsgId,time_t>::iterator tmp(it2) ;
				++tmp ;
				it->second.msg_cache.erase(it2) ;
				it2 = tmp ;
			}
			else
				++it2 ;
}


