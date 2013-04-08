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
#include <math.h>

#include "util/rsdir.h"
#include "util/rsaes.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"
#include "turtle/p3turtle.h"
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

static const int 		CONNECTION_CHALLENGE_MAX_COUNT 	  =   20 ; // sends a connexion challenge every 20 messages
static const time_t	CONNECTION_CHALLENGE_MAX_MSG_AGE	  =   30 ; // maximum age of a message to be used in a connexion challenge
static const int 		CONNECTION_CHALLENGE_MIN_DELAY 	  =   15 ; // sends a connexion at most every 15 seconds
static const int 		LOBBY_CACHE_CLEANING_PERIOD    	  =   10 ; // clean lobby caches every 10 secs (remove old messages)
static const time_t 	MAX_KEEP_MSG_RECORD 					  = 1200 ; // keep msg record for 1200 secs max.
static const time_t 	MAX_KEEP_INACTIVE_NICKNAME         =  180 ; // keep inactive nicknames for 3 mn max.
static const time_t  MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE =  120 ; // send keep alive packet every 2 minutes.
static const time_t 	MAX_KEEP_PUBLIC_LOBBY_RECORD       =   60 ; // keep inactive lobbies records for 60 secs max.
static const time_t 	MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ =   20 ; // don't ask for lobby list more than once every 30 secs.

p3ChatService::p3ChatService(p3LinkMgr *lm, p3HistoryMgr *historyMgr)
	:p3Service(RS_SERVICE_TYPE_CHAT), p3Config(CONFIG_TYPE_CHAT), mChatMtx("p3ChatService"), mLinkMgr(lm) , mHistoryMgr(historyMgr)
{
	_serializer = new RsChatSerialiser() ;
	_own_avatar = NULL ;
	_custom_status_string = "" ;
	_time_shift_average = 0.0f ;
	_default_nick_name = rsPeers->getPeerName(rsPeers->getOwnId());
	_should_reset_lobby_counts = false ;
	mTurtle = NULL ;
	
	last_visible_lobby_info_request_time = 0 ;

	addSerialType(_serializer) ;
}

void p3ChatService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	tr->registerTunnelService(this) ;
}

int	p3ChatService::tick()
{
	if(receivedItems()) 
		receiveChatQueue();

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
	ChatLobbyId lobby_id ;
	if(isLobbyId(id,lobby_id))
		sendLobbyStatusString(lobby_id,status_string) ;
	else
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
}

void p3ChatService::checkSizeAndSendMessage_deprecated(RsChatMsgItem *msg)
{
	// We check the message item, and possibly split it into multiple messages, if the message is too big.

	static const uint32_t MAX_STRING_SIZE = 15000 ;

	while(msg->message.size() > MAX_STRING_SIZE)
	{
		// chop off the first 15000 wchars

		RsChatMsgItem *item = new RsChatMsgItem(*msg) ;

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
// This function should be used for all types of chat messages. But this requires a non backward compatible change in 
// chat protocol. To be done for version 0.6
//
void p3ChatService::checkSizeAndSendMessage(RsChatLobbyMsgItem *msg)
{
	// We check the message item, and possibly split it into multiple messages, if the message is too big.

	static const uint32_t MAX_STRING_SIZE = 15000 ;
	int n=0 ;

	while(msg->message.size() > MAX_STRING_SIZE)
	{
		// chop off the first 15000 wchars

		RsChatLobbyMsgItem *item = new RsChatLobbyMsgItem(*msg) ;

		item->message = item->message.substr(0,MAX_STRING_SIZE) ;
		msg->message = msg->message.substr(MAX_STRING_SIZE,msg->message.size()-MAX_STRING_SIZE) ;

		// Clear out any one time flags that should not be copied into multiple objects. This is 
		// a precaution, in case the receivign peer does not yet handle split messages transparently.
		//
		item->chatFlags &= (RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_PUBLIC | RS_CHAT_FLAG_LOBBY) ;

		// Indicate that the message is to be continued.
		//
		item->chatFlags |= RS_CHAT_FLAG_PARTIAL_MESSAGE ;
		item->subpacket_id = n++ ;

		sendItem(item) ;
	}
	msg->subpacket_id = n ;
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
	time_t now = time(NULL) ;

	for( std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin()) ;it!=_chat_lobbys.end();++it)
	{
		std::cerr << "   Lobby id\t\t: " << std::hex << it->first << std::dec << std::endl;
		std::cerr << "   Lobby name\t\t: " << it->second.lobby_name << std::endl;
    std::cerr << "   Lobby topic\t\t: " << it->second.lobby_topic << std::endl;
		std::cerr << "   nick name\t\t: " << it->second.nick_name << std::endl;
		std::cerr << "   Lobby type\t\t: " << ((it->second.lobby_privacy_level==RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC)?"Public":"private") << std::endl;
		std::cerr << "   Lobby peer id\t: " << it->second.virtual_peer_id << std::endl;
		std::cerr << "   Challenge count\t: " << it->second.connexion_challenge_count << std::endl;
		std::cerr << "   Last activity\t: " << now - it->second.last_activity << " seconds ago." << std::endl;
		std::cerr << "   Cached messages\t: " << it->second.msg_cache.size() << std::endl;

		for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
			std::cerr << "       " << std::hex << it2->first << std::dec << "  time=" << now - it2->second << " secs ago" << std::endl;

		std::cerr << "   Participating friends: " << std::endl;

		for(std::set<std::string>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "       " << *it2 << std::endl;

		std::cerr << "   Participating nick names: " << std::endl;

		for(std::map<std::string,time_t>::const_iterator it2(it->second.nick_names.begin());it2!=it->second.nick_names.end();++it2)
			std::cerr << "       " << it2->first << ": " << now - it2->second << " secs ago" << std::endl;

	}

	std::cerr << "Recorded lobby names: " << std::endl;

	for( std::map<std::string,ChatLobbyId>::const_iterator it(_lobby_ids.begin()) ;it!=_lobby_ids.end();++it)
		std::cerr << "   \"" << it->first << "\" id = " << std::hex << it->second << std::dec << std::endl;

	std::cerr << "Visible public lobbies: " << std::endl;

	for( std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.begin()) ;it!=_visible_lobbies.end();++it)
	{
		std::cerr << "   " << std::hex << it->first << " name = " << std::dec << it->second.lobby_name << it->second.lobby_topic << std::endl;
		for(std::set<std::string>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "    With friend: " << *it2 << std::endl;
	}
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

	lobby_id = 0;
	return false ;
}


bool     p3ChatService::sendPrivateChat(const std::string &id, const std::wstring &msg)
{
	// look into ID. Is it a peer, or a chat lobby?

	ChatLobbyId lobby_id ;

	if(isLobbyId(id,lobby_id))
		return sendLobbyChat(id,msg,lobby_id) ;

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

	checkSizeAndSendMessage_deprecated(ci);

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

bool p3ChatService::locked_checkAndRebuildPartialMessage_deprecated(RsChatMsgItem *ci)
{
	// Check is the item is ending an incomplete item.
	//
	std::map<std::string,RsChatMsgItem*>::iterator it = _pendingPartialMessages.find(ci->PeerId()) ;

	bool ci_is_incomplete = ci->chatFlags & RS_CHAT_FLAG_PARTIAL_MESSAGE ;

	if(it != _pendingPartialMessages.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "Pending message found. Happending it." << std::endl;
#endif
		// Yes, there is. Append the item to ci.

		ci->message = it->second->message + ci->message ;
		ci->chatFlags |= it->second->chatFlags ;

		delete it->second ;

		if(!ci_is_incomplete)
			_pendingPartialMessages.erase(it) ;
	}

	if(ci_is_incomplete)
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
bool p3ChatService::locked_checkAndRebuildPartialMessage(RsChatLobbyMsgItem *ci)
{
	// Check is the item is ending an incomplete item.
	//
	std::map<ChatLobbyMsgId,std::vector<RsChatLobbyMsgItem*> >::iterator it = _pendingPartialLobbyMessages.find(ci->msg_id) ;

#ifdef CHAT_DEBUG
	std::cerr << "Checking chat message for completeness:" << std::endl;
#endif
	bool ci_is_incomplete = ci->chatFlags & RS_CHAT_FLAG_PARTIAL_MESSAGE ;

	if(it != _pendingPartialLobbyMessages.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "  Pending message found. Happending it." << std::endl;
#endif
		// Yes, there is. Add the item to the list of stored sub-items

		if(ci->subpacket_id >= it->second.size() )
			it->second.resize(ci->subpacket_id+1,NULL) ;

		it->second[ci->subpacket_id] = ci ;
#ifdef CHAT_DEBUG
		std::cerr << "  Checking for completeness." << std::endl;
#endif
		// Now check wether we have a complete item or not.
		//
		bool complete = true ;
		for(uint32_t i=0;i<it->second.size() && complete;++i)
			complete = complete && (it->second[i] != NULL) ;

		complete = complete && !(it->second.back()->chatFlags & RS_CHAT_FLAG_PARTIAL_MESSAGE) ;

		if(complete)
		{
#ifdef CHAT_DEBUG
			std::cerr << "  Message is complete ! Re-forming it and returning true." << std::endl;
#endif
			std::wstring msg ;
			uint32_t flags = 0 ;

			for(uint32_t i=0;i<it->second.size();++i)
			{
				msg += it->second[i]->message ;
				flags |= it->second[i]->chatFlags ;

				if(i != ci->subpacket_id)	// don't delete ci itself !!
					delete it->second[i] ;
			}
			_pendingPartialLobbyMessages.erase(it) ;

			ci->chatFlags = flags ;
			ci->message = msg ;

			return true ;
		}
		else
		{
#ifdef CHAT_DEBUG
			std::cerr << "  Not complete: returning" << std::endl ;
#endif
			return false ;
		}
	}
	else if(ci_is_incomplete || ci->subpacket_id > 0)	// the message id might not yet be recorded
	{
#ifdef CHAT_DEBUG
		std::cerr << "  Message is partial, but not recorded. Adding it. " << std::endl;
#endif

		_pendingPartialLobbyMessages[ci->msg_id].resize(ci->subpacket_id+1,NULL) ;
		_pendingPartialLobbyMessages[ci->msg_id][ci->subpacket_id] = ci ;

		return false ;
	}
	else
	{
#ifdef CHAT_DEBUG
		std::cerr << "  Message is not partial. Returning it as is." << std::endl;
#endif
		return true ;
	}
}

void p3ChatService::receiveChatQueue()
{
	RsItem *item ;

	while(NULL != (item=recvItem()))
		handleIncomingItem(item) ;
}

void p3ChatService::handleIncomingItem(RsItem *item)
{
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::receiveChatQueue() Item:" << (void*)item << std::endl ;
#endif
	// RsChatMsgItems needs dynamic_cast, since they have derived siblings.
	//
	RsChatMsgItem *ci = dynamic_cast<RsChatMsgItem*>(item) ; 
	if(ci != NULL) 
	{
		if(!  handleRecvChatMsgItem(ci))
			delete ci ;

		return ;	// don't delete! It's handled by handleRecvChatMsgItem in some specific cases only.
	}

	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_CHAT_STATUS:             handleRecvChatStatusItem      (dynamic_cast<RsChatStatusItem               *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_AVATAR:             handleRecvChatAvatarItem      (dynamic_cast<RsChatAvatarItem               *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE:       handleRecvLobbyInvite         (dynamic_cast<RsChatLobbyInviteItem          *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE:    handleConnectionChallenge     (dynamic_cast<RsChatLobbyConnectChallengeItem*>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT:        handleRecvChatLobbyEventItem  (dynamic_cast<RsChatLobbyEventItem           *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE:  handleFriendUnsubscribeLobby  (dynamic_cast<RsChatLobbyUnsubscribeItem     *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST: handleRecvChatLobbyListRequest(dynamic_cast<RsChatLobbyListRequestItem     *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST:         handleRecvChatLobbyList       (dynamic_cast<RsChatLobbyListItem            *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated: handleRecvChatLobbyList       (dynamic_cast<RsChatLobbyListItem_deprecated *>(item)) ; break ;

		default:
																		std::cerr << "Unhandled item subtype " << item->PacketSubType() << " in p3ChatService: " << std::endl;
	}
	delete item ;
}

void p3ChatService::handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *clr)
{
	// make a lobby list item
	//
	RsChatLobbyListItem *item = new RsChatLobbyListItem;

#ifdef CHAT_DEBUG
	std::cerr << "Peer " << clr->PeerId() << " requested the list of public chat lobbies." << std::endl;
#endif

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
		{
			const ChatLobbyEntry& lobby(it->second) ;

			if(lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC 
					|| (lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE 
						&& (lobby.previously_known_peers.find(clr->PeerId()) != lobby.previously_known_peers.end() 
							||lobby.participating_friends.find(clr->PeerId()) != lobby.participating_friends.end()) ))
			{
#ifdef CHAT_DEBUG
				std::cerr << "  Adding lobby " << std::hex << it->first << std::dec << " \"" << it->second.lobby_name << it->second.lobby_topic << "\" count=" << it->second.nick_names.size() << std::endl;
#endif

				item->lobby_ids.push_back(it->first) ;
				item->lobby_names.push_back(it->second.lobby_name) ;
				item->lobby_topics.push_back(it->second.lobby_topic) ;
				item->lobby_counts.push_back(it->second.nick_names.size()) ;
				item->lobby_privacy_levels.push_back(it->second.lobby_privacy_level) ;
			}
#ifdef CHAT_DEBUG
			else
				std::cerr << "  Not adding private lobby " << std::hex << it->first << std::dec << std::endl ;
#endif
		}
	}

	item->PeerId(clr->PeerId()) ;

#ifdef CHAT_DEBUG
	std::cerr << "  Sending list to " << clr->PeerId() << std::endl;
#endif
	sendItem(item);

	// *********** Also send an item in old formats. To be removed.
	
	RsChatLobbyListItem_deprecated *itemd = new RsChatLobbyListItem_deprecated;
	RsChatLobbyListItem_deprecated2 *itemd2 = new RsChatLobbyListItem_deprecated2;

	for(uint32_t i=0;i<item->lobby_ids.size();++i)
		if(item->lobby_privacy_levels[i] == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC)
		{
			itemd->lobby_ids.push_back(item->lobby_ids[i]) ;
			itemd->lobby_names.push_back(item->lobby_names[i]) ;
			itemd->lobby_counts.push_back(item->lobby_counts[i]) ;

			itemd2->lobby_ids.push_back(item->lobby_ids[i]) ;
			itemd2->lobby_names.push_back(item->lobby_names[i]) ;
			itemd2->lobby_counts.push_back(item->lobby_counts[i]) ;
			itemd2->lobby_topics.push_back(item->lobby_topics[i]) ;
		}

	itemd->PeerId(clr->PeerId()) ;
	itemd2->PeerId(clr->PeerId()) ;

	sendItem(itemd) ;
	sendItem(itemd2) ;

	// End of part to remove in future versions. *************
}
void p3ChatService::handleRecvChatLobbyList(RsChatLobbyListItem_deprecated *item)
{
	{
		time_t now = time(NULL) ;

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(uint32_t i=0;i<item->lobby_ids.size();++i)
		{
			VisibleChatLobbyRecord& rec(_visible_lobbies[item->lobby_ids[i]]) ;

			rec.lobby_id = item->lobby_ids[i] ;
			rec.lobby_name = item->lobby_names[i] ;
			rec.participating_friends.insert(item->PeerId()) ;

			if(_should_reset_lobby_counts)
				rec.total_number_of_peers = item->lobby_counts[i] ;
			else
				rec.total_number_of_peers = std::max(rec.total_number_of_peers,item->lobby_counts[i]) ;

			rec.last_report_time = now ;
			rec.lobby_privacy_level = RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC ;
		}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	_should_reset_lobby_counts = false ;
}
void p3ChatService::handleRecvChatLobbyList(RsChatLobbyListItem_deprecated2 *item)
{
	{
		time_t now = time(NULL) ;

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(uint32_t i=0;i<item->lobby_ids.size();++i)
		{
			VisibleChatLobbyRecord& rec(_visible_lobbies[item->lobby_ids[i]]) ;

			rec.lobby_id = item->lobby_ids[i] ;
			rec.lobby_name = item->lobby_names[i] ;
      rec.lobby_topic = item->lobby_topics[i] ;
			rec.participating_friends.insert(item->PeerId()) ;

			if(_should_reset_lobby_counts)
				rec.total_number_of_peers = item->lobby_counts[i] ;
			else
				rec.total_number_of_peers = std::max(rec.total_number_of_peers,item->lobby_counts[i]) ;

			rec.last_report_time = now ;
			rec.lobby_privacy_level = RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC ;
		}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	_should_reset_lobby_counts = false ;
}
void p3ChatService::handleRecvChatLobbyList(RsChatLobbyListItem *item)
{
	{
		time_t now = time(NULL) ;

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(uint32_t i=0;i<item->lobby_ids.size();++i)
		{
			VisibleChatLobbyRecord& rec(_visible_lobbies[item->lobby_ids[i]]) ;

			rec.lobby_id = item->lobby_ids[i] ;
			rec.lobby_name = item->lobby_names[i] ;
      rec.lobby_topic = item->lobby_topics[i] ;
			rec.participating_friends.insert(item->PeerId()) ;

			if(_should_reset_lobby_counts)
				rec.total_number_of_peers = item->lobby_counts[i] ;
			else
				rec.total_number_of_peers = std::max(rec.total_number_of_peers,item->lobby_counts[i]) ;

			rec.last_report_time = now ;
			rec.lobby_privacy_level = item->lobby_privacy_levels[i] ;
		}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	_should_reset_lobby_counts = false ;
}

void p3ChatService::addTimeShiftStatistics(int D)
{
	static const int S = 50 ; // accuracy up to 2^50 second. Quite conservative!
	static int total = 0 ;
	static std::vector<int> log_delay_histogram(S,0) ;

	int delay = (D<0)?(-D):D ;

	if(delay < 0)
		delay = -delay ;

	// compute log2.
	int l = 0 ;
	while(delay > 0) delay >>= 1, ++l ;

	int bin = std::min(S-1,l) ;
	++log_delay_histogram[bin] ;
	++total ;

#ifdef CHAT_DEBUG
	std::cerr << "New delay stat item. delay=" << D << ", log=" << bin << " total=" << total << ", histogram = " ;

	for(int i=0;i<S;++i)
		std::cerr << log_delay_histogram[i] << " " ;
#endif

	if(total > 30)
	{
		float t = 0.0f ;
		int i=0 ;
		for(;i<S && t<0.5*total;++i)
			t += log_delay_histogram[i] ;

		if(i == 0) return ;									// cannot happen, since total>0 so i is incremented
		if(log_delay_histogram[i-1] == 0) return ;	// cannot happen, either, but let's be cautious.

		float expected = ( i * (log_delay_histogram[i-1] - t + total*0.5) + (i-1) * (t - total*0.5) ) / (float)log_delay_histogram[i-1] - 1;

#ifdef CHAT_DEBUG
		std::cerr << ". Expected delay: " << expected << std::endl ;
#endif

		if(expected > 9)	// if more than 20 samples
			rsicontrol->getNotify().notifyChatLobbyTimeShift( (int)pow(2.0f,expected)) ;

		total = 0.0f ;
		log_delay_histogram.clear() ;
		log_delay_histogram.resize(S,0) ;
	}
#ifdef CHAT_DEBUG
	else
		std::cerr << std::endl;
#endif
}

void p3ChatService::handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item)
{
#ifdef CHAT_DEBUG
	std::cerr << "Received ChatLobbyEvent item of type " << (int)(item->event_type) << ", and string=" << item->string1 << std::endl;
#endif
	time_t now = time(NULL) ;

	addTimeShiftStatistics((int)now - (int)item->sendTime) ;

	if(now+100 > (time_t) item->sendTime + MAX_KEEP_MSG_RECORD)	// the message is older than the max cache keep minus 100 seconds ! It's too old, and is going to make an echo!
	{
		std::cerr << "Received severely outdated lobby event item (" << now - (time_t)item->sendTime << " in the past)! Dropping it!" << std::endl;
		std::cerr << "Message item is:" << std::endl;
		item->print(std::cerr) ;
		std::cerr << std::endl;
		return ;
	}
	if(now+600 < (time_t) item->sendTime)	// the message is from the future more than 10 minutes
	{
		std::cerr << "Received event item from the future (" << (time_t)item->sendTime - now << " seconds in the future)! Dropping it!" << std::endl;
		std::cerr << "Message item is:" << std::endl;
		item->print(std::cerr) ;
		std::cerr << std::endl;
		return ;
	}
	if(! bounceLobbyObject(item,item->PeerId()))
		return ;

#ifdef CHAT_DEBUG
	std::cerr << "  doing specific job for this status item." << std::endl;
#endif

	if(item->event_type == RS_CHAT_LOBBY_EVENT_PEER_LEFT)		// if a peer left. Remove its nickname from the list.
	{
#ifdef CHAT_DEBUG
		std::cerr << "  removing nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
			std::map<std::string,time_t>::iterator it2(it->second.nick_names.find(item->nick)) ;

			if(it2 != it->second.nick_names.end())
			{
				it->second.nick_names.erase(it2) ;
#ifdef CHAT_DEBUG
				std::cerr << "  removed nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
			}
#ifdef CHAT_DEBUG
			else
				std::cerr << "  (EE) nickname " << item->nick << " not in participant nicknames list!" << std::endl;
#endif
		}
	}
	else if(item->event_type == RS_CHAT_LOBBY_EVENT_PEER_JOINED)		// if a joined left. Add its nickname to the list.
	{
#ifdef CHAT_DEBUG
		std::cerr << "  adding nickname " << item->nick << " to lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
			it->second.nick_names[item->nick] = time(NULL) ;
#ifdef CHAT_DEBUG
			std::cerr << "  added nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
		}
	}
	else if(item->event_type == RS_CHAT_LOBBY_EVENT_KEEP_ALIVE)		// keep alive packet. 
	{
#ifdef CHAT_DEBUG
		std::cerr << "  adding nickname " << item->nick << " to lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
			it->second.nick_names[item->nick] = time(NULL) ;
#ifdef CHAT_DEBUG
			std::cerr << "  added nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
		}
	}
	rsicontrol->getNotify().notifyChatLobbyEvent(item->lobby_id,item->event_type,item->nick,item->string1) ;
}

void p3ChatService::handleRecvChatAvatarItem(RsChatAvatarItem *ca)
{
	receiveAvatarJpegData(ca) ;

#ifdef CHAT_DEBUG
	std::cerr << "Received avatar data for peer " << ca->PeerId() << ". Notifying." << std::endl ;
#endif
	rsicontrol->getNotify().notifyPeerHasNewAvatar(ca->PeerId()) ;
}

bool p3ChatService::handleRecvChatMsgItem(RsChatMsgItem *ci)
{
	bool publicChanged = false;
	bool privateChanged = false;

	time_t now = time(NULL);
	std::string name;
	uint32_t popupChatFlag = RS_POPUP_CHAT;

	// check if it's a lobby msg, in which case we replace the peer id by the lobby's virtual peer id.
	//
	RsChatLobbyMsgItem *cli = dynamic_cast<RsChatLobbyMsgItem*>(ci) ;

	if(cli != NULL)
	{
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			if(!locked_checkAndRebuildPartialMessage(cli))
				return true ;
		}

		if(now+100 > (time_t) cli->sendTime + MAX_KEEP_MSG_RECORD)	// the message is older than the max cache keep plus 100 seconds ! It's too old, and is going to make an echo!
		{
			std::cerr << "Received severely outdated lobby event item (" << now - (time_t)cli->sendTime << " in the past)! Dropping it!" << std::endl;
			std::cerr << "Message item is:" << std::endl;
			cli->print(std::cerr) ;
			std::cerr << std::endl;
			return false ;
		}
		if(now+600 < (time_t) cli->sendTime)	// the message is from the future. Drop it. more than 10 minutes
		{
			std::cerr << "Received event item from the future (" << (time_t)cli->sendTime - now << " seconds in the future)! Dropping it!" << std::endl;
			std::cerr << "Message item is:" << std::endl;
			cli->print(std::cerr) ;
			std::cerr << std::endl;
			return false ;
		}
		if(!bounceLobbyObject(cli,cli->PeerId()))	// forwards the message to friends, keeps track of subscribers, etc.
			return false;

		// setup the peer id to the virtual peer id of the lobby.
		//
		std::string virtual_peer_id ;
		getVirtualPeerId(cli->lobby_id,virtual_peer_id) ;
		cli->PeerId(virtual_peer_id) ;
		name = cli->nick;
		popupChatFlag = RS_POPUP_CHATLOBBY;
	}	
	else 
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		if(!locked_checkAndRebuildPartialMessage_deprecated(ci)) 	// Don't delete ! This function is not handled propoerly for chat lobby msgs, so
			return true ;															// we don't use it in this case.
	}

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
		return false ;
	}
	else														// normal msg. Return it normally.
	{
		// Check if new avatar is available at peer's. If so, send a request to get the avatar.
		if(ci->chatFlags & RS_CHAT_FLAG_AVATAR_AVAILABLE) 
		{
#ifdef CHAT_DEBUG
			std::cerr << "New avatar is available for peer " << ci->PeerId() << ", sending request" << std::endl ;
#endif
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
#ifdef CHAT_DEBUG
			std::cerr << "Avatar is new for peer. ending info above" << std::endl ;
#endif
			ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
		}

		std::string message;
		librs::util::ConvertUtf16ToUtf8(ci->message, message);
		if (ci->chatFlags & RS_CHAT_FLAG_PRIVATE) {
			/* notify private chat message */
			getPqiNotify()->AddPopupMessage(popupChatFlag, ci->PeerId(), name, message);
		} else {
			/* notify public chat message */
			getPqiNotify()->AddPopupMessage(RS_POPUP_GROUPCHAT, ci->PeerId(), "", message);
			getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAT_NEW, ci->PeerId(), message, "");
		}

		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			ci->recvTime = now;

			if (ci->chatFlags & RS_CHAT_FLAG_PRIVATE) {
#ifdef CHAT_DEBUG
				std::cerr << "Adding msg 0x" << std::hex << (void*)ci << std::dec << " to private chat incoming list." << std::endl;
#endif
				privateChanged = true;
				privateIncomingList.push_back(ci);	// don't delete the item !!
			} else {
#ifdef CHAT_DEBUG
				std::cerr << "Adding msg 0x" << std::hex << (void*)ci << std::dec << " to public chat incoming list." << std::endl;
#endif
				publicChanged = true;
				publicList.push_back(ci);	// don't delete the item !!

				if (ci->PeerId() != mLinkMgr->getOwnId()) {
					/* not from loop back */
					mHistoryMgr->addMessage(true, "", ci->PeerId(), ci);
				}
			}
		} /* UNLOCK */
	}

	if (publicChanged) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PUBLIC_CHAT, NOTIFY_TYPE_ADD);
	}
	if (privateChanged) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);

		IndicateConfigChanged(); // only private chat messages are saved
	}
	return true ;
}

void p3ChatService::handleRecvChatStatusItem(RsChatStatusItem *cs)
{
#ifdef CHAT_DEBUG
	std::cerr << "Received status string \"" << cs->status_string << "\"" << std::endl ;
#endif

	if(cs->flags & RS_CHAT_FLAG_REQUEST_CUSTOM_STATE) 	// no state here just a request.
		sendCustomState(cs->PeerId()) ;
	else if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE)		// Check if new custom string is available at peer's. 
	{ 																	// If so, send a request to get the custom string.
		receiveStateString(cs->PeerId(),cs->status_string) ;	// store it
		rsicontrol->getNotify().notifyCustomState(cs->PeerId(), cs->status_string) ;
	}
	else if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE)
	{
#ifdef CHAT_DEBUG
		std::cerr << "New custom state is available for peer " << cs->PeerId() << ", sending request" << std::endl ;
#endif
		sendCustomStateRequest(cs->PeerId()) ;
	}
	else if(cs->flags & RS_CHAT_FLAG_PRIVATE)
		rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,true) ;
	else if(cs->flags & RS_CHAT_FLAG_PUBLIC)
		rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,false) ;
}

void p3ChatService::getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& visible_lobbies)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		visible_lobbies.clear() ;

		for(std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.begin());it!=_visible_lobbies.end();++it)
			visible_lobbies.push_back(it->second) ;
	}

	time_t now = time(NULL) ;

	if(now > MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ + last_visible_lobby_info_request_time)
	{
		std::list<std::string> ids ;
		mLinkMgr->getOnlineList(ids);

		for(std::list<std::string>::const_iterator it(ids.begin());it!=ids.end();++it)
		{
#ifdef CHAT_DEBUG
			std::cerr << "  asking list of public lobbies to " << *it << std::endl;
#endif
			RsChatLobbyListRequestItem *item = new RsChatLobbyListRequestItem ;
			item->PeerId(*it) ;

			sendItem(item);
		}
		last_visible_lobby_info_request_time = now ;
		_should_reset_lobby_counts = true ;
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

				it = list->erase(it);

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

	// has it. Return it strait away.
	//
	if(it!=_state_strings.end())
	{
	   it->second._peer_is_new = false ;
	   return it->second._custom_status_string ;
	}

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
	std::list<std::string> ssl_peers;
	mLinkMgr->getFriendList(ssl_peers);

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
				if (std::find(ssl_peers.begin(), ssl_peers.end(), citem->configPeerId) != ssl_peers.end()) {
					RsChatMsgItem *ci = new RsChatMsgItem();
					citem->get(ci);

					if (citem->configFlags & RS_CHATMSG_CONFIGFLAG_INCOMING) {
						privateIncomingList.push_back(ci);
					} else {
						privateOutgoingList.push_back(ci);
					}
				} else {
					// no friends
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
#ifdef CHAT_DEBUG
					std::cerr << "Loaded config default nick name for chat: " << kit->value << std::endl ;
#endif
					if (!kit->value.empty())
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

							checkSizeAndSendMessage_deprecated(c); // delete item

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
		} else if (it->actions & RS_PEER_MOVED) {
			/* now handle remove */
			clearPrivateChatQueue(true, it->id);
			clearPrivateChatQueue(false, it->id);
			mHistoryMgr->clear(it->id);
		}
	}
}

//********************** Chat Lobby Stuff ***********************//

// returns:
// 	true: the object is not a duplicate and should be used
// 	false: the object is a duplicate or there is an error, and it should be destroyed.
//
bool p3ChatService::bounceLobbyObject(RsChatLobbyBouncingObject *item,const std::string& peer_id)
{
	time_t now = time(NULL) ;
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
	locked_printDebugInfo() ; // debug

	std::cerr << "Handling ChatLobbyMsg " << std::hex << item->msg_id << ", lobby id " << item->lobby_id << ", from peer id " << peer_id << std::endl;
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

	if(peer_id != mLinkMgr->getOwnId())
		lobby.participating_friends.insert(peer_id) ;

	lobby.nick_names[item->nick] = now ;

	// Checks wether the msg is already recorded or not

	std::map<ChatLobbyMsgId,time_t>::iterator it2(lobby.msg_cache.find(item->msg_id)) ;

	if(it2 != lobby.msg_cache.end()) // found!
	{
#ifdef CHAT_DEBUG
		std::cerr << "  Msg already received at time " << it2->second << ". Dropping!" << std::endl ;
#endif
		it2->second = now ;	// update last msg seen time, to prevent echos.
		return false ;
	}
#ifdef CHAT_DEBUG
	std::cerr << "  Msg already not received already. Adding in cache, and forwarding!" << std::endl ;
#endif

	lobby.msg_cache[item->msg_id] = now ;
	lobby.last_activity = now ;

	bool is_message =  (NULL != dynamic_cast<RsChatLobbyMsgItem*>(item)) ;

	// Forward to allparticipating friends, except this peer.

	for(std::set<std::string>::const_iterator it(lobby.participating_friends.begin());it!=lobby.participating_friends.end();++it)
		if((*it)!=peer_id && mLinkMgr->isOnline(*it)) 
		{
			RsChatLobbyBouncingObject *obj2 = item->duplicate() ; // makes a copy
			RsChatItem *item2 = dynamic_cast<RsChatItem*>(obj2) ;

			assert(item2 != NULL) ;

			item2->PeerId(*it) ;	// replaces the virtual peer id with the actual destination.

			if(is_message)
				checkSizeAndSendMessage(static_cast<RsChatLobbyMsgItem*>(item2)) ;
			else
				sendItem(item2);
		}

	++lobby.connexion_challenge_count ;

	return true ;
}

void p3ChatService::sendLobbyStatusString(const ChatLobbyId& lobby_id,const std::string& status_string)
{
	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_STATUS,status_string) ; 
}

/** 
 * Inform other Clients of a nickname change
 * 
 * as example for updating their ChatLobby Blocklist for muted peers
 *  */
void p3ChatService::sendLobbyStatusPeerChangedNickname(const ChatLobbyId& lobby_id, const std::string& newnick)
{
	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_CHANGE_NICKNAME, newnick) ; 
}


void p3ChatService::sendLobbyStatusPeerLiving(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_LEFT,nick) ; 
}

void p3ChatService::sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_JOINED,nick) ; 
}

void p3ChatService::sendLobbyStatusKeepAlive(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,nick) ; 
}

void p3ChatService::sendLobbyStatusItem(const ChatLobbyId& lobby_id,int type,const std::string& status_string) 
{
	RsChatLobbyEventItem item ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		locked_initLobbyBouncableObject(lobby_id,item) ;

		item.event_type = type ;
		item.string1 = status_string ;
		item.sendTime = time(NULL) ;
	}
	std::string ownId = mLinkMgr->getOwnId();
	bounceLobbyObject(&item,ownId) ;
}

void p3ChatService::locked_initLobbyBouncableObject(const ChatLobbyId& lobby_id,RsChatLobbyBouncingObject& item)
{
	// get a pointer to the info for that chat lobby.
	//
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(lobby_id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "Chatlobby for id " << std::hex << lobby_id << " has no record. This is a serious error!!" << std::dec << std::endl;
#endif
		return ;
	}
	ChatLobbyEntry& lobby(it->second) ;

	// chat lobby stuff
	//
	do 
	{ 
		item.msg_id	= RSRandom::random_u64(); 
	} 
	while( lobby.msg_cache.find(item.msg_id) != lobby.msg_cache.end() ) ;

	item.lobby_id = lobby_id ;
	item.nick = lobby.nick_name ;
}

bool p3ChatService::sendLobbyChat(const std::string &id, const std::wstring& msg, const ChatLobbyId& lobby_id)
{
#ifdef CHAT_DEBUG
	std::cerr << "Sending chat lobby message to lobby " << std::hex << lobby_id << std::dec << std::endl;
	std::cerr << "msg:" << std::endl;
	std::wcerr << msg << std::endl;
#endif

	RsChatLobbyMsgItem item ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		// gives a random msg id, setup the nickname
		locked_initLobbyBouncableObject(lobby_id,item) ;

		// chat msg stuff
		//
		item.chatFlags = RS_CHAT_FLAG_LOBBY | RS_CHAT_FLAG_PRIVATE;
		item.sendTime = time(NULL);
		item.recvTime = item.sendTime;
		item.message = msg;
	}

	std::string ownId = rsPeers->getOwnId();

	mHistoryMgr->addMessage(false, id, ownId, &item);

	bounceLobbyObject(&item, ownId) ;

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

	time_t now = time(NULL) ;
	ChatLobbyId lobby_id ;
	std::string ownId = rsPeers->getOwnId();
	bool found = false ;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end() && !found;++it)
			for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end() && !found;++it2)
				if(it2->second + CONNECTION_CHALLENGE_MAX_MSG_AGE + 5 > now)  // any msg not older than 5 seconds plus max challenge count is fine.
				{
					uint64_t code = makeConnexionChallengeCode(ownId,it->first,it2->first) ;
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
		invitePeerToLobby(lobby_id, item->PeerId(),true) ;
#ifdef CHAT_DEBUG
	else
		std::cerr << "    Challenge denied: no existing cached msg has matching Id." << std::endl;
#endif
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
	ChatLobbyMsgId msg_id = 0 ;

	for(std::map<ChatLobbyMsgId,time_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
		if(it2->second + CONNECTION_CHALLENGE_MAX_MSG_AGE > now)  // any msg not older than 20 seconds is fine.
		{
			msg_id = it2->first ;
#ifdef CHAT_DEBUG
			std::cerr << "  Using msg id 0x" << std::hex << it2->first << std::dec << std::endl; 
#endif
			break ;
		}

	if(msg_id == 0)
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

		uint64_t code = makeConnexionChallengeCode(*it,lobby_id,msg_id) ;

#ifdef CHAT_DEBUG
		std::cerr << "  Sending collexion challenge code 0x" << std::hex << code <<  std::dec << " to peer " << *it << std::endl; 
#endif
		item->PeerId(*it) ;
		item->challenge_code = code ;

		sendItem(item);
	}
}

uint64_t p3ChatService::makeConnexionChallengeCode(const std::string& peer_id,ChatLobbyId lobby_id,ChatLobbyMsgId msg_id)
{
	uint64_t result = 0 ;

	for(uint32_t i=0;i<peer_id.size();++i)
	{
		result += msg_id ;
		result ^= result >> 35 ;
		result += result <<  6 ;
		result ^= peer_id[i] * lobby_id ;
		result += result << 26 ;
		result ^= result >> 13 ;
	}
	return result ;
}

void p3ChatService::getChatLobbyList(std::list<ChatLobbyInfo>& linfos) 
{
	// fill up a dummy list for now.

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	linfos.clear() ;

	for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
		linfos.push_back(it->second) ;
}
void p3ChatService::invitePeerToLobby(const ChatLobbyId& lobby_id, const std::string& peer_id,bool connexion_challenge) 
{
#ifdef CHAT_DEBUG
	if(connexion_challenge)
		std::cerr << "Sending connexion challenge accept to peer " << peer_id << " for lobby "<< std::hex << lobby_id << std::dec << std::endl;
	else
		std::cerr << "Sending invitation to peer " << peer_id << " to lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif

	RsChatLobbyInviteItem *item = new RsChatLobbyInviteItem ;

	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "  invitation send: canceled. Lobby " << lobby_id << " not found!" << std::endl;
#endif
		return ;
	}
	item->lobby_id = lobby_id ;
	item->lobby_name = it->second.lobby_name ;
	item->lobby_topic = it->second.lobby_topic ;
	item->lobby_privacy_level = connexion_challenge?RS_CHAT_LOBBY_PRIVACY_LEVEL_CHALLENGE:(it->second.lobby_privacy_level) ;
	item->PeerId(peer_id) ;

	sendItem(item) ;
}
void p3ChatService::handleRecvLobbyInvite(RsChatLobbyInviteItem *item) 
{
#ifdef CHAT_DEBUG
	std::cerr << "Received invite to lobby from " << item->PeerId() << " to lobby " << std::hex << item->lobby_id << std::dec << ", named " << item->lobby_name << item->lobby_topic << std::endl;
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
#ifdef CHAT_DEBUG
			std::cerr << "  Lobby already exists. " << std::endl;
			std::cerr << "     privacy levels: " << item->lobby_privacy_level << " vs. " << it->second.lobby_privacy_level ;
#endif

			if(item->lobby_privacy_level != RS_CHAT_LOBBY_PRIVACY_LEVEL_CHALLENGE && item->lobby_privacy_level != it->second.lobby_privacy_level)
			{
				std::cerr << " : Don't match. Cancelling." << std::endl;
				return ;
			}
#ifdef CHAT_DEBUG
			else
				std::cerr << " : Match!" << std::endl;

			std::cerr << "  Addign new friend " << item->PeerId() << " to lobby." << std::endl;
#endif

			it->second.participating_friends.insert(item->PeerId()) ;
			return ;
		}

		// Don't record the invitation if it's a challenge response item or a lobby we don't have.
		//
		if(item->lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_CHALLENGE)	
			return ;

		// no, then create a new invitation entry in the cache.
		
		ChatLobbyInvite invite ;
		invite.lobby_id = item->lobby_id ;
		invite.peer_id = item->PeerId() ;
		invite.lobby_name = item->lobby_name ;
		invite.lobby_topic = item->lobby_topic ;
		invite.lobby_privacy_level = item->lobby_privacy_level ;

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
		time_t now = time(NULL) ;

		ChatLobbyEntry entry ;
		entry.participating_friends.insert(it->second.peer_id) ;
		entry.lobby_privacy_level = it->second.lobby_privacy_level ;
		entry.nick_name = _default_nick_name ;	
		entry.lobby_id = lobby_id ;
		entry.lobby_name = it->second.lobby_name ;
		entry.lobby_topic = it->second.lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ;
		entry.last_connexion_challenge_time = now ;
		entry.last_keep_alive_packet_time = now ;

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
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD);

	// send AKN item
	sendLobbyStatusNewPeer(lobby_id) ;

	return true ;
}

std::string p3ChatService::makeVirtualPeerId(ChatLobbyId lobby_id)
{
	std::string s;
	rs_sprintf(s, "Chat Lobby 0x%llx", lobby_id);

	return s ;
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

bool p3ChatService::joinVisibleChatLobby(const ChatLobbyId& lobby_id)
{
#ifdef CHAT_DEBUG
	std::cerr << "Joining public chat lobby " << std::hex << lobby_id << std::dec << std::endl;
#endif
	std::list<std::string> invited_friends ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		// create a unique id.
		//
		std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.find(lobby_id)) ;

		if(it == _visible_lobbies.end())
		{
			std::cerr << "  lobby is not a known public chat lobby. Sorry!" << std::endl;
			return false ;
		}

#ifdef CHAT_DEBUG
		std::cerr << "  lobby found. Initiating join sequence..." << std::endl;
#endif

		if(_chat_lobbys.find(lobby_id) != _chat_lobbys.end())
		{
#ifdef CHAT_DEBUG
			std::cerr << "  lobby already in participating list. Returning!" << std::endl;
#endif
			return true ;
		}

#ifdef CHAT_DEBUG
		std::cerr << "  Creating new lobby entry." << std::endl;
#endif
		time_t now = time(NULL) ;

		ChatLobbyEntry entry ;
		entry.lobby_privacy_level = it->second.lobby_privacy_level ;//RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC ;
		entry.participating_friends.clear() ;
		entry.nick_name = _default_nick_name ;	
		entry.lobby_id = lobby_id ;
		entry.lobby_name = it->second.lobby_name ;
		entry.lobby_topic = it->second.lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ; 
		entry.last_connexion_challenge_time = now ; 
		entry.last_keep_alive_packet_time = now ;

		_lobby_ids[entry.virtual_peer_id] = lobby_id ;

		for(std::set<std::string>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		{
			invited_friends.push_back(*it2) ;
			entry.participating_friends.insert(*it2) ;
		}
		_chat_lobbys[lobby_id] = entry ;
	}

	for(std::list<std::string>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	sendLobbyStatusNewPeer(lobby_id) ;

	return true ;
}

ChatLobbyId p3ChatService::createChatLobby(const std::string& lobby_name,const std::string& lobby_topic,const std::list<std::string>& invited_friends,uint32_t privacy_level)
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
		time_t now = time(NULL) ;

		ChatLobbyEntry entry ;
		entry.lobby_privacy_level = privacy_level ;
		entry.participating_friends.clear() ;
		entry.nick_name = _default_nick_name ;	// to be changed. For debug only!!
		entry.lobby_id = lobby_id ;
		entry.lobby_name = lobby_name ;
		entry.lobby_topic = lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ;
		entry.last_connexion_challenge_time = now ;
		entry.last_keep_alive_packet_time = now ;

		_lobby_ids[entry.virtual_peer_id] = lobby_id ;
		_chat_lobbys[lobby_id] = entry ;
	}

	for(std::list<std::string>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;

	return lobby_id ;
}

void p3ChatService::handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem *item)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

#ifdef CHAT_DEBUG
		std::cerr << "Received unsubscribed to lobby " << std::hex << item->lobby_id << std::dec << ", from friend " << item->PeerId() << std::endl;
#endif

		if(it == _chat_lobbys.end())
		{
			std::cerr << "Chat lobby " << std::hex << item->lobby_id << std::dec << " does not exist ! Can't unsubscribe friend!" << std::endl;
			return ;
		}

		for(std::set<std::string>::iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			if(*it2 == item->PeerId())
			{
#ifdef CHAT_DEBUG
				std::cerr << "  removing peer id " << item->PeerId() << " from participant list of lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
				it->second.previously_known_peers.insert(*it2) ;
				it->second.participating_friends.erase(it2) ;
				break ;
			}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_MOD) ;
}

void p3ChatService::unsubscribeChatLobby(const ChatLobbyId& id)
{
	// send AKN item
	sendLobbyStatusPeerLiving(id) ;

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

#ifdef CHAT_DEBUG
			std::cerr << "Sending unsubscribe item to friend " << *it2 << std::endl;
#endif

			sendItem(item) ;
		}

		// remove history

		mHistoryMgr->clear(it->second.virtual_peer_id);

		// remove lobby information

		_chat_lobbys.erase(it) ;

		for(std::map<std::string,ChatLobbyId>::iterator it2(_lobby_ids.begin());it2!=_lobby_ids.end();++it2)
			if(it2->second == id)
			{
				_lobby_ids.erase(it2) ;
				break ;
			}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_DEL) ;

	// done!
}
bool p3ChatService::setDefaultNickNameForChatLobby(const std::string& nick)
{
	if (nick.empty())
	{
		std::cerr << "Ignore empty nickname for chat lobby " << std::endl;
		return false;
	}

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		_default_nick_name = nick;
	}

	IndicateConfigChanged() ;
	return true ;
}
bool p3ChatService::getDefaultNickNameForChatLobby(std::string& nick)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
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
	if (nick.empty())
	{
		std::cerr << "Ignore empty nickname for chat lobby " << std::hex << lobby_id << std::dec << std::endl;
		return false;
	}

	// first check for change and send status peer changed nick name
	bool changed = false;
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
		std::cerr << "Changing nickname for chat lobby " << std::hex << lobby_id << std::dec << " to " << nick << std::endl;
#endif
		it = _chat_lobbys.find(lobby_id) ;

		if(it == _chat_lobbys.end())
		{
			std::cerr << " (EE) lobby does not exist!!" << std::endl;
			return false;
		}

		if (it->second.nick_name != nick)
		{
			changed = true;
		}
	}

	if (changed)
	{
		// Inform other peers of change the Nickname
		sendLobbyStatusPeerChangedNickname(lobby_id, nick) ;

		// set new nick name
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		it = _chat_lobbys.find(lobby_id) ;

		if(it == _chat_lobbys.end())
		{
			std::cerr << " (EE) lobby does not exist!!" << std::endl;
			return false;
		}

		it->second.nick_name = nick ;
	}

	return true ;
}

void p3ChatService::cleanLobbyCaches()
{
#ifdef CHAT_DEBUG
	std::cerr << "Cleaning chat lobby caches." << std::endl;
#endif

	std::list<ChatLobbyId> keep_alive_lobby_ids ;
	std::list<ChatLobbyId> changed_lobbies ;
	std::list<ChatLobbyId> send_challenge_lobbies ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		time_t now = time(NULL) ;

		// 1 - clean cache of all lobbies and participating nicknames.
		//
		for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.begin();it!=_chat_lobbys.end();++it)
		{
			for(std::map<ChatLobbyMsgId,time_t>::iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();)
				if(it2->second + MAX_KEEP_MSG_RECORD < now)
				{
#ifdef CHAT_DEBUG
					std::cerr << "  removing old msg 0x" << std::hex << it2->first << ", time=" << std::dec << now - it2->second << " secs ago" << std::endl;
#endif

					std::map<ChatLobbyMsgId,time_t>::iterator tmp(it2) ;
					++tmp ;
					it->second.msg_cache.erase(it2) ;
					it2 = tmp ;
				}
				else
					++it2 ;

			bool changed = false ;

			for(std::map<std::string,time_t>::iterator it2(it->second.nick_names.begin());it2!=it->second.nick_names.end();)
				if(it2->second + MAX_KEEP_INACTIVE_NICKNAME < now)
				{
#ifdef CHAT_DEBUG
					std::cerr << "  removing inactive nickname 0x" << std::hex << it2->first << ", time=" << std::dec << now - it2->second << " secs ago" << std::endl;
#endif

					std::map<std::string,time_t>::iterator tmp(it2) ;
					++tmp ;
					it->second.nick_names.erase(it2) ;
					it2 = tmp ;
					changed = true ;
				}
				else
					++it2 ;

			if(changed)
				changed_lobbies.push_back(it->first) ;

			// 3 - send lobby keep-alive packets to all lobbies
			//
			if(it->second.last_keep_alive_packet_time + MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE < now)
			{
				keep_alive_lobby_ids.push_back(it->first) ;
				it->second.last_keep_alive_packet_time = now ;
			}

			// 4 - look at lobby activity and possibly send connexion challenge
			//
			if(++it->second.connexion_challenge_count > CONNECTION_CHALLENGE_MAX_COUNT && now > it->second.last_connexion_challenge_time + CONNECTION_CHALLENGE_MIN_DELAY) 
			{
				it->second.connexion_challenge_count = 0 ;
				it->second.last_connexion_challenge_time = now ;

				send_challenge_lobbies.push_back(it->first);
			}
		}

		// 2 - clean deprecated public chat lobby records
		// 

		for(std::map<ChatLobbyId,VisibleChatLobbyRecord>::iterator it(_visible_lobbies.begin());it!=_visible_lobbies.end();)
			if(it->second.last_report_time + MAX_KEEP_PUBLIC_LOBBY_RECORD < now)	// this lobby record is too late.
			{
#ifdef CHAT_DEBUG
				std::cerr << "  removing old public lobby record 0x" << std::hex << it->first << ", time=" << std::dec << now - it->second.last_report_time << " secs ago" << std::endl;
#endif

				std::map<ChatLobbyMsgId,VisibleChatLobbyRecord>::iterator tmp(it) ;
				++tmp ;
				_visible_lobbies.erase(it) ;
				it = tmp ;
			}
			else
				++it ;
	}

	for(std::list<ChatLobbyId>::const_iterator it(keep_alive_lobby_ids.begin());it!=keep_alive_lobby_ids.end();++it)
		sendLobbyStatusKeepAlive(*it) ;

	// update the gui
	for(std::list<ChatLobbyId>::const_iterator it(changed_lobbies.begin());it!=changed_lobbies.end();++it)
		rsicontrol->getNotify().notifyChatLobbyEvent(*it,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,"------------","") ;

	// send connexion challenges
	//
	for(std::list<ChatLobbyId>::const_iterator it(send_challenge_lobbies.begin());it!=send_challenge_lobbies.end();++it)
		sendConnectionChallenge(*it) ;
}

bool p3ChatService::handleTunnelRequest(const std::string& hash,const std::string& peer_id,std::string& description_info_string) 
{
	std::map<TurtleFileHash,DistantChatInvite>::const_iterator it = _distant_chat_invites.find(hash) ;

	std::cerr << "p3ChatService::handleTunnelRequest: received tunnel request for hash " << hash << std::endl;

	if(it == _distant_chat_invites.end())
		return false ;

	std::cerr << "Responding ok." << std::endl;
	return true ;
}

void p3ChatService::receiveTurtleData(	RsTurtleGenericTunnelItem *gitem,const std::string& hash,
													const std::string& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
{
	std::cerr << "p3ChatService::receiveTurtleData(): Received turtle data. " << std::endl;
	std::cerr << "   hash = " << hash << std::endl;
	std::cerr << "   vpid = " << virtual_peer_id << std::endl;
	std::cerr << "    dir = " << virtual_peer_id << std::endl;

	RsTurtleGenericDataItem *item = dynamic_cast<RsTurtleGenericDataItem*>(gitem) ;

	if(item == NULL)
	{
		std::cerr << "(EE) item is not a data item. That is an error." << std::endl;
		return ;
	}
	std::cerr << "   size = " << item->data_size << std::endl;
	std::cerr << "   data = " << (void*)item->data_bytes << std::endl;

	uint8_t aes_key[16] ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<std::string,DistantChatPeerInfo>::iterator it = _distant_chat_peers.find(virtual_peer_id) ;

		if(it == _distant_chat_peers.end())
		{
			std::cerr << "(EE) item is not coming out of a registered tunnel. Weird. hash=" << hash << ", peer id = " << virtual_peer_id << std::endl;
			return ;
		}
		it->second.last_contact = time(NULL) ;
		memcpy(aes_key,it->second.aes_key,8) ;
	}

	// Call the AES crypto module
	// - the IV is the first 8 bytes of item->data_bytes

	if(item->data_size < 8)
	{
		std::cerr << "(EE) item encrypted data stream is too small: size = " << item->data_size << std::endl;
		return ;
	}
	uint32_t decrypted_size ;
	uint8_t *decrypted_data = new uint8_t[RsAES::get_buffer_size(item->data_size-8)];

	if(!RsAES::aes_decrypt_8_16((uint8_t*)item->data_bytes+8,item->data_size-8,aes_key,(uint8_t*)item->data_bytes,decrypted_data,decrypted_size))
	{
		std::cerr << "(EE) packet decryption failed." << std::endl;
		delete[] decrypted_data ;
		return ;
	}

	std::cerr << "(II) Decrypted data: size=" << decrypted_size << std::endl;

	// Now try deserialise the decrypted data to make an RsItem out of it.
	//
	RsItem *citem = _serializer->deserialise(decrypted_data,&decrypted_size) ;
	delete[] decrypted_data ;

	if(citem == NULL)
	{
		std::cerr << "(EE) item could not be de-serialized. That is an error." << std::endl;
		return ;
	}

	// Setup the virtual peer to be the origin, and pass it on.
	//
	citem->PeerId(virtual_peer_id) ;
	handleIncomingItem(citem) ; // Treats the item, and deletes it 
}

void p3ChatService::sendTurtleData(RsChatItem *item, const std::string& virtual_peer_id)
{
	uint32_t rssize = item->serial_size();
	uint8_t *buff = new uint8_t[rssize] ;

	if(!item->serialise(buff,rssize))
	{
		std::cerr << "(EE) p3ChatService::sendTurtleData(): Could not serialise item!" << std::endl;
		delete[] buff ;
		return ;
	}
	
	uint8_t aes_key[16] ;

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<std::string,DistantChatPeerInfo>::iterator it = _distant_chat_peers.find(virtual_peer_id) ;

		if(it == _distant_chat_peers.end())
		{
			std::cerr << "(EE) item is not coming out of a registered tunnel. Weird. peer id = " << virtual_peer_id << std::endl;
			delete[] buff ;
			return ;
		}
		it->second.last_contact = time(NULL) ;
		memcpy(aes_key,it->second.aes_key,8) ;
	}
	// Now encrypt this data using AES.
	//
	uint32_t encrypted_size ;
	uint8_t *encrypted_data = new uint8_t[RsAES::get_buffer_size(rssize)];

	uint64_t IV = RSRandom::random_u64() ; // make a random 8 bytes IV

	if(!RsAES::aes_crypt_8_16(buff,rssize,aes_key,(uint8_t*)&IV,encrypted_data,encrypted_size))
	{
		std::cerr << "(EE) packet encryption failed." << std::endl;
		delete[] encrypted_data ;
		delete[] buff ;
		return ;
	}
	delete[] buff ;

	// make a TurtleGenericData item out of it:
	//
	RsTurtleGenericDataItem *gitem = new RsTurtleGenericDataItem ;

	gitem->data_size  = encrypted_size + 8 ;
	gitem->data_bytes = malloc(gitem->data_size) ;

	memcpy(gitem->data_bytes  ,&IV,8) ;
	memcpy(gitem->data_bytes+8,encrypted_data,encrypted_size) ;

	delete[] encrypted_data ;
	delete item ;

	mTurtle->sendTurtleData(virtual_peer_id,gitem) ;
}


