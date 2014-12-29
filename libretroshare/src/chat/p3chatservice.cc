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
#include <sstream>
#include <unistd.h>

#include "util/rsdir.h"
#include "util/radix64.h"
#include "util/rsaes.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsstatus.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3historymgr.h"
#include "rsserver/p3face.h"
#include "services/p3idservice.h"

#include "chat/p3chatservice.h"
#include "serialiser/rsconfigitems.h"

/****
 * #define CHAT_DEBUG 1
 ****/

static const uint32_t MAX_MESSAGE_SECURITY_SIZE         = 6000 ; // Max message size to forward other friends
static const uint32_t MAX_AVATAR_JPEG_SIZE              = 32767; // Maximum size in bytes for an avatar. Too large packets 
                                                                 // don't transfer correctly and can kill the system.
																					  // Images are 96x96, which makes approx. 27000 bytes uncompressed.

p3ChatService::p3ChatService(p3ServiceControl *sc,p3IdService *pids, p3LinkMgr *lm, p3HistoryMgr *historyMgr)
    :p3Service(), p3Config(), mChatMtx("p3ChatService"),DistantChatService(pids),DistributedChatService(getServiceInfo().mServiceType,sc,historyMgr),mServiceCtrl(sc), mLinkMgr(lm) , mHistoryMgr(historyMgr)
{
	_serializer = new RsChatSerialiser() ;

	_own_avatar = NULL ;
	_custom_status_string = "" ;

	addSerialType(_serializer) ;
}

const std::string CHAT_APP_NAME = "chat";
const uint16_t CHAT_APP_MAJOR_VERSION	= 	1;
const uint16_t CHAT_APP_MINOR_VERSION  = 	0;
const uint16_t CHAT_MIN_MAJOR_VERSION  = 	1;
const uint16_t CHAT_MIN_MINOR_VERSION	=	0;

RsServiceInfo p3ChatService::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_CHAT, 
		CHAT_APP_NAME,
		CHAT_APP_MAJOR_VERSION, 
		CHAT_APP_MINOR_VERSION, 
		CHAT_MIN_MAJOR_VERSION, 
		CHAT_MIN_MINOR_VERSION);
}

int	p3ChatService::tick()
{
	if(receivedItems()) 
		receiveChatQueue();

	DistributedChatService::flush() ;
	DistantChatService::flush() ;

	return 0;
}

/***************** Chat Stuff **********************/

void p3ChatService::sendPublicChat(const std::string &msg)
{
	/* go through all the peers */

	std::set<RsPeerId> ids;
	std::set<RsPeerId>::iterator it;
	mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, ids);

	/* add in own id -> so get reflection */
	RsPeerId ownId = mServiceCtrl->getOwnId();
	ids.insert(ownId);

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat()";
	std::cerr << std::endl;
#endif

	for(it = ids.begin(); it != ids.end(); ++it)
	{
		RsChatMsgItem *ci = new RsChatMsgItem();

		ci->PeerId(*it);
		ci->chatFlags = RS_CHAT_FLAG_PUBLIC;
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
            //mHistoryMgr->addMessage(false, RsPeerId(), ownId, ci);
            ChatMessage message;
            initChatMessage(ci, message);
            message.incoming = false;
            message.online = true;
            RsServer::notify()->notifyChatMessage(message);
            mHistoryMgr->addMessage(message);
		}
		sendItem(ci);
	}
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
	std::set<RsPeerId> ids;
	mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, ids);

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat(): sending group chat status string: " << status_string << std::endl ;
	std::cerr << std::endl;
#endif

	for(std::set<RsPeerId>::iterator it = ids.begin(); it != ids.end(); ++it)
	{
		RsChatStatusItem *cs = new RsChatStatusItem ;

		cs->status_string = status_string ;
		cs->flags = RS_CHAT_FLAG_PUBLIC ;

		cs->PeerId(*it);

		sendItem(cs);
	}
}

void p3ChatService::sendStatusString(const ChatId& id , const std::string& status_string)
{
    if(id.isLobbyId())
        sendLobbyStatusString(id.toLobbyId(),status_string) ;
    else if(id.isBroadcast())
        sendGroupChatStatusString(status_string);
    else if(id.isPeerId() || id.isGxsId())
	{
		RsChatStatusItem *cs = new RsChatStatusItem ;

		cs->status_string = status_string ;
		cs->flags = RS_CHAT_FLAG_PRIVATE ;
        RsPeerId vpid;
        if(id.isGxsId())
            vpid = RsPeerId(id.toGxsId());
        else
            vpid = id.toPeerId();
        cs->PeerId(vpid);

#ifdef CHAT_DEBUG
		std::cerr  << "sending chat status packet:" << std::endl ;
		cs->print(std::cerr) ;
#endif
		sendChatItem(cs);
    }
    else
    {
        std::cerr << "p3ChatService::sendStatusString() Error: chat id of this type is not handled, is it empty?" << std::endl;
        return;
    }
}

void p3ChatService::sendChatItem(RsChatItem *item)
{
	if(DistantChatService::handleOutgoingItem(item))
		return ;
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChatItem(): sending to " << item->PeerId() << ": interpreted as friend peer id." << std::endl;
#endif
	sendItem(item) ;
}

void p3ChatService::checkSizeAndSendMessage(RsChatMsgItem *msg)
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
		sendChatItem(item) ;
	}
	sendChatItem(msg) ;
}


bool p3ChatService::isOnline(const RsPeerId& pid)
{
	// check if the id is a tunnel id or a peer id.
	uint32_t status ;
    if(getDistantChatStatus(RsGxsId(pid),status))
		return status == RS_DISTANT_CHAT_STATUS_CAN_TALK ;
    else
        return mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, pid);
}

bool p3ChatService::sendChat(ChatId destination, std::string msg)
{
    if(destination.isLobbyId())
        return DistributedChatService::sendLobbyChat(destination.toLobbyId(), msg);
    else if(destination.isBroadcast())
    {
        sendPublicChat(msg);
        return true;
    }
    else if(destination.isPeerId()==false && destination.isGxsId()==false)
    {
        std::cerr << "p3ChatService::sendChat() Error: chat id type not handled. Is it empty?" << std::endl;
        return false;
    }
    // destination is peer or distant
#ifdef CHAT_DEBUG
    std::cerr << "p3ChatService::sendChat()";
    std::cerr << std::endl;
#endif

    RsPeerId vpid;
    if(destination.isGxsId())
        vpid = RsPeerId(destination.toGxsId()); // convert to virtual peer id
    else
        vpid = destination.toPeerId();

    RsChatMsgItem *ci = new RsChatMsgItem();
    ci->PeerId(vpid);
    ci->chatFlags = RS_CHAT_FLAG_PRIVATE;
    ci->sendTime = time(NULL);
    ci->recvTime = ci->sendTime;
    ci->message = msg;

    ChatMessage message;
    initChatMessage(ci, message);
    message.incoming = false;
    message.online = true;

    if(!isOnline(vpid))
    {
        /* peer is offline, add to outgoing list */
        {
            RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
            privateOutgoingList.push_back(ci);
        }

        message.online = false;
        RsServer::notify()->notifyChatMessage(message);

        // use the history to load pending messages to the gui
        // this is not very nice, because the user may think the message was send, while it is still in the queue
        mHistoryMgr->addMessage(message);

        IndicateConfigChanged();
        return false;
    }

    {
        RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
        std::map<RsPeerId,AvatarInfo*>::iterator it = _avatars.find(vpid) ;

        if(it == _avatars.end())
        {
            _avatars[vpid] = new AvatarInfo ;
            it = _avatars.find(vpid) ;
        }
        if(it->second->_own_is_new)
        {
#ifdef CHAT_DEBUG
            std::cerr << "p3ChatService::sendChat: new avatar never sent to peer " << id << ". Setting <new> flag to packet." << std::endl;
#endif

            ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
            it->second->_own_is_new = false ;
        }
    }

#ifdef CHAT_DEBUG
    std::cerr << "Sending msg to (maybe virtual) peer " << id << ", flags = " << ci->chatFlags << std::endl ;
    std::cerr << "p3ChatService::sendChat() Item:";
    std::cerr << std::endl;
    ci->print(std::cerr);
    std::cerr << std::endl;
#endif

    RsServer::notify()->notifyChatMessage(message);
    mHistoryMgr->addMessage(message);

    checkSizeAndSendMessage(ci);

    // Check if custom state string has changed, in which case it should be sent to the peer.
    bool should_send_state_string = false ;
    {
        RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

        std::map<RsPeerId,StateStringInfo>::iterator it = _state_strings.find(vpid) ;

        if(it == _state_strings.end())
        {
            _state_strings[vpid] = StateStringInfo() ;
            it = _state_strings.find(vpid) ;
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
        cs->PeerId(vpid) ;
        sendChatItem(cs) ;
    }

    return true;
}

bool p3ChatService::locked_checkAndRebuildPartialMessage(RsChatMsgItem *ci)
{
	// Check is the item is ending an incomplete item.
	//
	std::map<RsPeerId,RsChatMsgItem*>::iterator it = _pendingPartialMessages.find(ci->PeerId()) ;

	bool ci_is_incomplete = ci->chatFlags & RS_CHAT_FLAG_PARTIAL_MESSAGE ;

	if(it != _pendingPartialMessages.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "Pending message found. Appending it." << std::endl;
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


void p3ChatService::receiveChatQueue()
{
	RsItem *item ;

	while(NULL != (item=recvItem()))
		handleIncomingItem(item) ;
}

class MsgCounter
{
	public:
		MsgCounter() {}

		void clean(time_t max_time)
		{
			while(!recv_times.empty() && recv_times.front() < max_time)
				recv_times.pop_front() ;
		}
		std::list<time_t> recv_times ;
};

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

	if(DistantChatService::handleRecvItem(dynamic_cast<RsChatItem*>(item)))
	{
		delete item ;
		return ;
	}

    if(DistributedChatService::handleRecvItem(dynamic_cast<RsChatItem*>(item)))
	{
		delete item ;
		return ;
	}

	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_CHAT_STATUS:                handleRecvChatStatusItem(dynamic_cast<RsChatStatusItem*>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_AVATAR:                handleRecvChatAvatarItem(dynamic_cast<RsChatAvatarItem*>(item)) ; break ;
		default:
																		{
																			static int already = false ;

																			if(!already)
																			{
																				std::cerr << "Unhandled item subtype " << (int)item->PacketSubType() << " in p3ChatService: " << std::endl;
																				already = true ;
																			}
																		}
	}
	delete item ;
}

void p3ChatService::handleRecvChatAvatarItem(RsChatAvatarItem *ca)
{
	receiveAvatarJpegData(ca) ;

#ifdef CHAT_DEBUG
	std::cerr << "Received avatar data for peer " << ca->PeerId() << ". Notifying." << std::endl ;
#endif
	RsServer::notify()->notifyPeerHasNewAvatar(ca->PeerId().toStdString()) ;
}

uint32_t p3ChatService::getMaxMessageSecuritySize(int type)
{
	switch (type)
	{
	case RS_CHAT_TYPE_PUBLIC:
	case RS_CHAT_TYPE_LOBBY:
		return MAX_MESSAGE_SECURITY_SIZE;

	case RS_CHAT_TYPE_PRIVATE:
	case RS_CHAT_TYPE_DISTANT:
		return 0; // unlimited
	}

	std::cerr << "p3ChatService::getMaxMessageSecuritySize: Unknown chat type " << type << std::endl;

	return MAX_MESSAGE_SECURITY_SIZE;
}

bool p3ChatService::checkForMessageSecurity(RsChatMsgItem *ci)
{
	// Remove too big messages
	if (ci->chatFlags & RS_CHAT_FLAG_LOBBY)
	{
		uint32_t maxMessageSize = getMaxMessageSecuritySize(RS_CHAT_TYPE_LOBBY);
		if (maxMessageSize > 0 && ci->message.length() > maxMessageSize)
		{
			std::ostringstream os;
			os << getMaxMessageSecuritySize(RS_CHAT_TYPE_LOBBY);

			ci->message = "**** Security warning: Message bigger than ";
			ci->message += os.str();
			ci->message += " characters, forwarded to you by ";
			ci->message += rsPeers->getPeerName(ci->PeerId());
			ci->message += ", dropped. ****";
			return false;
		}
	}

	// The following code has been suggested, but is kept suspended since it is a bit too much restrictive.
#ifdef SUSPENDED
	// Transform message to lowercase
	std::wstring mes(ci->message);
	std::transform( mes.begin(), mes.end(), mes.begin(), std::towlower);

	// Quick fix for svg attack and other nuisances (inline pictures)
	if (mes.find(L"<img") != std::string::npos)
	{
		ci->message = L"**** Security warning: Message contains an . ****";
		return false;
	}

	// Remove messages with too many line breaks
	size_t pos = 0;
	int count_line_breaks = 0;
	while ((pos = mes.find(L"<br", pos+1)) != std::string::npos)
	{
		count_line_breaks++;
	}
	if (count_line_breaks > 50)
	{
		ci->message = L"**** More than 50 line breaks, dropped. ****";
		return false;
	}
#endif

	// https://en.wikipedia.org/wiki/Billion_laughs
	// This should be done for all incoming HTML messages (also in forums
	// etc.) so this should be a function in some other file.

	if (ci->message.find("<!") != std::string::npos)
	{
		// Drop any message with "<!doctype" or "<!entity"...
		// TODO: check what happens with partial messages
		//
		std::cout << "handleRecvChatMsgItem: " << ci->message << std::endl;
		std::cout << "**********" << std::endl;
		std::cout << "********** entity attack by " << ci->PeerId().toStdString().c_str() << std::endl;
		std::cout << "**********" << std::endl;

		ci->message = "**** This message (from peer id " + rsPeers->getPeerName(ci->PeerId()) + ") has been removed because it contains the string \"<!\".****" ;
		return false;
	}
	// For a future whitelist:
	// things to be kept:
	// <span> <img src="data:image/png;base64,... />
	// <a href="retroshare://…>…</a>
	
	// Also check flags. Lobby msgs should have proper flags, but they can be
	// corrupted by a friend before sending them that can result in e.g. lobby
	// messages ending up in the broadcast channel, etc.
	
	uint32_t fl = ci->chatFlags & (RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_PUBLIC | RS_CHAT_FLAG_LOBBY) ;

#ifdef CHAT_DEBUG
	std::cerr << "Checking msg flags: " << std::hex << fl << std::endl;
#endif

	if(dynamic_cast<RsChatLobbyMsgItem*>(ci) != NULL)
	{
		if(fl != (RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_LOBBY))
			std::cerr << "Warning: received chat lobby message with iconsistent flags " << std::hex << fl << std::dec << " from friend peer " << ci->PeerId() << std::endl;

		ci->chatFlags &= ~RS_CHAT_FLAG_PUBLIC ;
	}
	else if(fl!=0 && !(fl == RS_CHAT_FLAG_PRIVATE || fl == RS_CHAT_FLAG_PUBLIC))	// The !=0 is normally not needed, but we keep it for 
	{																										// a while, for backward compatibility. It's not harmful.
		std::cerr << "Warning: received chat lobby message with iconsistent flags " << std::hex << fl << std::dec << " from friend peer " << ci->PeerId() << std::endl;

		std::cerr << "This message will be dropped."<< std::endl;
		return false ;
	}

	return true ;
}

bool p3ChatService::handleRecvChatMsgItem(RsChatMsgItem *ci)
{
	bool publicChanged = false;
	bool privateChanged = false;

	time_t now = time(NULL);
	std::string name;
    uint32_t popupChatFlag = RS_POPUP_CHAT;

    {
        RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

        // This crap is because chat lobby messages use a different method for chunking messages using an additional
        // subpacket ID, and a list of lobbies. We cannot just collapse the two because it would make the normal chat
        // (and chat lobbies) not backward compatible.

        if(!DistributedChatService::locked_checkAndRebuildPartialLobbyMessage(dynamic_cast<RsChatLobbyMsgItem*>(ci)))
            return true ;

        if(!locked_checkAndRebuildPartialMessage(ci))
        return true ;
    }

    // Check for security. This avoids bombing messages, and so on.

    if(!checkForMessageSecurity(ci))
        return false ;

    // If it's a lobby item, we need to bounce it and possibly check for timings etc.

    if(!DistributedChatService::handleRecvChatLobbyMsgItem(ci))
        return false ;

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::receiveChatQueue() Item:";
	std::cerr << std::endl;
	ci->print(std::cerr);
	std::cerr << std::endl;
	std::cerr << "Got msg. Flags = " << ci->chatFlags << std::endl ;
#endif

    // Now treat normal chat stuff such as avatar requests, except for chat lobbies.

    if( !(ci->chatFlags & RS_CHAT_FLAG_LOBBY))
    {
        if(ci->chatFlags & RS_CHAT_FLAG_REQUESTS_AVATAR) 	// no msg here. Just an avatar request.
        {
            sendAvatarJpegData(ci->PeerId()) ;
            return false ;
        }

        // normal msg. Return it normally.
        // Check if new avatar is available at peer's. If so, send a request to get the avatar.

        if((ci->chatFlags & RS_CHAT_FLAG_AVATAR_AVAILABLE) && !(ci->chatFlags & RS_CHAT_FLAG_LOBBY))
        {
#ifdef CHAT_DEBUG
            std::cerr << "New avatar is available for peer " << ci->PeerId() << ", sending request" << std::endl ;
#endif
            sendAvatarRequest(ci->PeerId()) ;
            ci->chatFlags &= ~RS_CHAT_FLAG_AVATAR_AVAILABLE ;
        }

        std::map<RsPeerId,AvatarInfo *>::const_iterator it = _avatars.find(ci->PeerId()) ;

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
    }

    std::string message = ci->message;

    if(!(ci->chatFlags & RS_CHAT_FLAG_LOBBY))
    {
        if(ci->chatFlags & RS_CHAT_FLAG_PRIVATE)
            RsServer::notify()->AddPopupMessage(popupChatFlag, ci->PeerId().toStdString(), name, message); /* notify private chat message */
        else
        {
            /* notify public chat message */
            RsServer::notify()->AddPopupMessage(RS_POPUP_GROUPCHAT, ci->PeerId().toStdString(), "", message);
            RsServer::notify()->AddFeedItem(RS_FEED_ITEM_CHAT_NEW, ci->PeerId().toStdString(), message, "");
        }
    }

    ci->recvTime = now;

    ChatMessage cm;
    initChatMessage(ci, cm);
    cm.incoming = true;
    cm.online = true;
    RsServer::notify()->notifyChatMessage(cm);
    mHistoryMgr->addMessage(cm);
    return true ;
}

void p3ChatService::locked_storeIncomingMsg(RsChatMsgItem *item)
{
#ifdef REMOVE
	privateIncomingList.push_back(item) ;
#endif
}

void p3ChatService::handleRecvChatStatusItem(RsChatStatusItem *cs)
{
#ifdef CHAT_DEBUG
	std::cerr << "Received status string \"" << cs->status_string << "\"" << std::endl ;
#endif

    uint32_t status;

	if(cs->flags & RS_CHAT_FLAG_REQUEST_CUSTOM_STATE) 	// no state here just a request.
		sendCustomState(cs->PeerId()) ;
	else if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE)		// Check if new custom string is available at peer's. 
	{ 																	// If so, send a request to get the custom string.
		receiveStateString(cs->PeerId(),cs->status_string) ;	// store it
		RsServer::notify()->notifyCustomState(cs->PeerId().toStdString(), cs->status_string) ;
	}
	else if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE)
	{
#ifdef CHAT_DEBUG
		std::cerr << "New custom state is available for peer " << cs->PeerId() << ", sending request" << std::endl ;
#endif
		sendCustomStateRequest(cs->PeerId()) ;
	}
    else if(DistantChatService::getDistantChatStatus(RsGxsId(cs->PeerId()), status))
    {
        RsServer::notify()->notifyChatStatus(ChatId(RsGxsId(cs->PeerId())), cs->status_string) ;
    }
	else if(cs->flags & RS_CHAT_FLAG_PRIVATE)
	{
        RsServer::notify()->notifyChatStatus(ChatId(cs->PeerId()),cs->status_string) ;
	}
	else if(cs->flags & RS_CHAT_FLAG_PUBLIC)
    {
        ChatId id = ChatId::makeBroadcastId();
        id.broadcast_status_peer_id = cs->PeerId();
        RsServer::notify()->notifyChatStatus(id, cs->status_string) ;
    }

	DistantChatService::handleRecvChatStatusItem(cs) ;
}

void p3ChatService::initChatMessage(RsChatMsgItem *c, ChatMessage &m)
{
    m.chat_id = ChatId(c->PeerId());
    m.chatflags = 0;
    m.sendTime = c->sendTime;
    m.recvTime = c->recvTime;
    m.msg  = c->message;

    RsChatLobbyMsgItem *lobbyItem = dynamic_cast<RsChatLobbyMsgItem*>(c) ;
    if(lobbyItem != NULL)
    {
        m.lobby_peer_nickname = lobbyItem->nick;
        m.chat_id = ChatId(lobbyItem->lobby_id);
        return;
    }

    uint32_t status;
    if(DistantChatService::getDistantChatStatus(RsGxsId(c->PeerId()), status))
        m.chat_id = ChatId(RsGxsId(c->PeerId()));

    if (c -> chatFlags & RS_CHAT_FLAG_PRIVATE)
        m.chatflags |= RS_CHAT_PRIVATE;
    else
    {
        m.chat_id = ChatId::makeBroadcastId();
        m.broadcast_peer_id = c->PeerId();
        m.chatflags |= RS_CHAT_PUBLIC;
    }
}

void p3ChatService::setOwnCustomStateString(const std::string& s)
{
	std::set<RsPeerId> onlineList;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
		std::cerr << "p3chatservice: Setting own state string to new value : " << s << std::endl ;
#endif
		_custom_status_string = s ;

		for(std::map<RsPeerId,StateStringInfo>::iterator it(_state_strings.begin());it!=_state_strings.end();++it)
			it->second._own_is_new = true ;

		mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, onlineList);
	}

	RsServer::notify()->notifyOwnStatusMessageChanged() ;

	// alert your online peers to your newly set status
	std::set<RsPeerId>::iterator it(onlineList.begin());
	for(; it != onlineList.end(); ++it){

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

		if((uint32_t)size > MAX_AVATAR_JPEG_SIZE)
		{
			std::cerr << "Supplied avatar image is too big. Maximum size is " << MAX_AVATAR_JPEG_SIZE << ", supplied image has size " << size << std::endl;
			return ;
		}
		if(_own_avatar != NULL)
			delete _own_avatar ;

		_own_avatar = new AvatarInfo(data,size) ;

		// set the info that our avatar is new, for all peers
		for(std::map<RsPeerId,AvatarInfo *>::iterator it(_avatars.begin());it!=_avatars.end();++it)
			it->second->_own_is_new = true ;
	}
	IndicateConfigChanged();

	RsServer::notify()->notifyOwnAvatarChanged() ;

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:setOwnAvatarJpegData() done." << std::endl ;
#endif

}

void p3ChatService::receiveStateString(const RsPeerId& id,const std::string& s)
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

	if(ci->image_size > MAX_AVATAR_JPEG_SIZE)
	{
		std::cerr << "Peer " << ci->PeerId()<< " is sending a jpeg image for avatar that exceeds the admitted size (size=" << ci->image_size << ", max=" << MAX_AVATAR_JPEG_SIZE << ")"<< std::endl;
		return ;
	}
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

std::string p3ChatService::getCustomStateString(const RsPeerId& peer_id) 
{
	{
		// should be a Mutex here.
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<RsPeerId,StateStringInfo>::iterator it = _state_strings.find(peer_id) ; 

		// has it. Return it strait away.
		//
		if(it!=_state_strings.end())
		{
			it->second._peer_is_new = false ;
			return it->second._custom_status_string ;
		}
	}

	sendCustomStateRequest(peer_id);
	return std::string() ;
}

void p3ChatService::getAvatarJpegData(const RsPeerId& peer_id,unsigned char *& data,int& size) 
{
	{
		// should be a Mutex here.
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<RsPeerId,AvatarInfo *>::const_iterator it = _avatars.find(peer_id) ; 

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
	}

	sendAvatarRequest(peer_id);
}

void p3ChatService::sendAvatarRequest(const RsPeerId& peer_id)
{
	if(!isOnline(peer_id))
		return ;

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

	sendChatItem(ci);
}

void p3ChatService::sendCustomStateRequest(const RsPeerId& peer_id){

	if(!isOnline(peer_id))
		return ;

	RsChatStatusItem* cs = new RsChatStatusItem;

	cs->PeerId(peer_id);
	cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_REQUEST_CUSTOM_STATE ;
	cs->status_string.erase();

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sending request for status, to peer " << peer_id << std::endl ;
	std::cerr << std::endl;
#endif

	sendChatItem(cs);
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


void p3ChatService::sendAvatarJpegData(const RsPeerId& peer_id)
{
   #ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: sending requested for peer " << peer_id << ", data=" << (void*)_own_avatar << std::endl ;
   #endif

   if(_own_avatar != NULL)
	{
		RsChatAvatarItem *ci = makeOwnAvatarItem();
		ci->PeerId(peer_id);

		// take avatar, and embed it into a std::string.
		//
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::sending avatar image to peer" << peer_id << ", image size = " << ci->image_size << std::endl ;
		std::cerr << std::endl;
#endif

		sendChatItem(ci) ;
	}
   else {
#ifdef CHAT_DEBUG
        std::cerr << "We have no avatar yet: Doing nothing" << std::endl ;
#endif
   }
}

void p3ChatService::sendCustomState(const RsPeerId& peer_id){

#ifdef CHAT_DEBUG
std::cerr << "p3chatservice: sending requested status string for peer " << peer_id << std::endl ;
#endif

	RsChatStatusItem *cs = makeOwnCustomStateStringItem();
	cs->PeerId(peer_id);

	sendChatItem(cs);
}

bool p3ChatService::loadList(std::list<RsItem*>& load)
{
	std::list<RsPeerId> ssl_peers;
	mLinkMgr->getFriendList(ssl_peers);

	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
		RsChatAvatarItem *ai = NULL ;

		if(NULL != (ai = dynamic_cast<RsChatAvatarItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			if(ai->image_size <= MAX_AVATAR_JPEG_SIZE)
				_own_avatar = new AvatarInfo(ai->image_data,ai->image_size) ;
			else
				std::cerr << "Dropping avatar image, because its size is " << ai->image_size << ", and the maximum allowed size is " << MAX_AVATAR_JPEG_SIZE << std::endl;

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
						locked_storeIncomingMsg(ci);
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

		if(DistributedChatService::processLoadListItem(*it))
		{
			delete *it;
			continue;
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
		ci->PeerId(mServiceCtrl->getOwnId());

		list.push_back(ci) ;
	}

	mChatMtx.lock(); /****** MUTEX LOCKED *******/

	RsChatStatusItem *di = new RsChatStatusItem ;
	di->status_string = _custom_status_string ;
	di->flags = RS_CHAT_FLAG_CUSTOM_STATE ;

	list.push_back(di) ;

	/* save outgoing private chat messages */
    std::list<RsChatMsgItem *>::iterator it;
	for (it = privateOutgoingList.begin(); it != privateOutgoingList.end(); it++) {
		RsPrivateChatMsgConfigItem *ci = new RsPrivateChatMsgConfigItem;

		ci->set(*it, (*it)->PeerId(), 0);

		list.push_back(ci);
	}

	DistributedChatService::addToSaveList(list) ;

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

void p3ChatService::statusChange(const std::list<pqiServicePeer> &plist)
{
	std::list<pqiServicePeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); ++it) {
		if (it->actions & RS_SERVICE_PEER_CONNECTED) 
		{
			/* send the saved outgoing messages */
			bool changed = false;

			std::vector<RsChatMsgItem*> to_send ;

			if (privateOutgoingList.size()) 
			{
				RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

				RsPeerId ownId = mServiceCtrl->getOwnId();

				std::list<RsChatMsgItem *>::iterator cit = privateOutgoingList.begin();
				while (cit != privateOutgoingList.end()) {
					RsChatMsgItem *c = *cit;

					if (c->PeerId() == it->id) {
                        //mHistoryMgr->addMessage(false, c->PeerId(), ownId, c);

						to_send.push_back(c) ;

						changed = true;

						cit = privateOutgoingList.erase(cit);

						continue;
					}

					++cit;
				}
			} /* UNLOCKED */

			for(uint32_t i=0;i<to_send.size();++i)
            {
                ChatMessage message;
                initChatMessage(to_send[i], message);
                message.incoming = false;
                message.online = true;
                RsServer::notify()->notifyChatMessage(message);

                checkSizeAndSendMessage(to_send[i]); // delete item
            }

			if (changed) {
				RsServer::notify()->notifyListChange(NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_DEL);

				IndicateConfigChanged();
			}
		}
		else if (it->actions & RS_SERVICE_PEER_REMOVED) 
		{
			/* now handle remove */
            mHistoryMgr->clear(ChatId(it->id));

            std::list<RsChatMsgItem *>::iterator cit = privateOutgoingList.begin();
            while (cit != privateOutgoingList.end()) {
                RsChatMsgItem *c = *cit;
                if (c->PeerId() == it->id) {
                    cit = privateOutgoingList.erase(cit);
                    continue;
                }
                ++cit;
            }
            IndicateConfigChanged();
		}
	}
}
