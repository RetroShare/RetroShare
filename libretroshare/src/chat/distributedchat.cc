/*******************************************************************************
 * libretroshare/src/chat: distributedchat.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <iomanip>
#include <math.h>
#include <sstream>
#include <unistd.h>

#include "util/rsprint.h"
#include "util/rsmemory.h"
#include "distributedchat.h"

#include "pqi/p3historymgr.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsidentity.h"
#include "rsserver/p3face.h"
#include "gxs/rsgixs.h"
#include "services/p3idservice.h"

//#define DEBUG_CHAT_LOBBIES 1

static const int 		CONNECTION_CHALLENGE_MAX_COUNT   =   20 ; // sends a connection challenge every 20 messages
static const rstime_t	CONNECTION_CHALLENGE_MAX_MSG_AGE =   30 ; // maximum age of a message to be used in a connection challenge
static const int 		CONNECTION_CHALLENGE_MIN_DELAY   =   15 ; // sends a connection at most every 15 seconds
static const int 		LOBBY_CACHE_CLEANING_PERIOD      =   10 ; // clean lobby caches every 10 secs (remove old messages)

static const rstime_t 	MAX_KEEP_MSG_RECORD                = 1200 ; // keep msg record for 1200 secs max.
static const rstime_t 	MAX_KEEP_INACTIVE_NICKNAME         =  180 ; // keep inactive nicknames for 3 mn max.
static const rstime_t  	MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE =  120 ; // send keep alive packet every 2 minutes.
static const rstime_t 	MAX_KEEP_PUBLIC_LOBBY_RECORD       =   60 ; // keep inactive lobbies records for 60 secs max.
static const rstime_t 	MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ =   20 ; // don't ask for lobby list more than once every 30 secs.
static const rstime_t 	LOBBY_LIST_AUTO_UPDATE_TIME        =  121 ; // regularly ask for available lobbies every 5 minutes, to allow auto-subscribe to work

static const uint32_t MAX_ALLOWED_LOBBIES_IN_LIST_WARNING = 50 ;
//static const uint32_t MAX_MESSAGES_PER_SECONDS_NUMBER   =  5 ; // max number of messages from a given peer in a window for duration below
static const uint32_t MAX_MESSAGES_PER_SECONDS_PERIOD     = 10 ; // duration window for max number of messages before messages get dropped.

#define        IS_PUBLIC_LOBBY(flags) (flags & RS_CHAT_LOBBY_FLAGS_PUBLIC    )
#define    IS_PGP_SIGNED_LOBBY(flags) (flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)
#define IS_CONNEXION_CHALLENGE(flags) (flags & RS_CHAT_LOBBY_FLAGS_CHALLENGE )

#define  EXTRACT_PRIVACY_FLAGS(flags) (ChatLobbyFlags(flags.toUInt32()) * (RS_CHAT_LOBBY_FLAGS_PUBLIC | RS_CHAT_LOBBY_FLAGS_PGP_SIGNED))

DistributedChatService::DistributedChatService(uint32_t serv_type,p3ServiceControl *sc,p3HistoryMgr *hm, RsGixs *is)
    : mServType(serv_type),mDistributedChatMtx("Distributed Chat"), mServControl(sc), mHistMgr(hm),mGixs(is)
{
    _time_shift_average = 0.0f ;
    _should_reset_lobby_counts = false ;
    last_visible_lobby_info_request_time = 0 ;
}

void DistributedChatService::flush()
{
	static rstime_t last_clean_time_lobby = 0 ;
	static rstime_t last_req_chat_lobby_list = 0 ;

	rstime_t now = time(NULL) ;

	if(last_clean_time_lobby + LOBBY_CACHE_CLEANING_PERIOD < now)
	{
		cleanLobbyCaches() ;
        last_clean_time_lobby = now ;

        // also make sure that the default identity is not null

        if(_default_identity.isNull())
        {
            std::list<RsGxsId> ids ;
            mGixs->getOwnIds(ids) ;

            if(!ids.empty())
            {
                _default_identity = ids.front() ;
                triggerConfigSave() ;
            }
        }
	}
	if(last_req_chat_lobby_list + LOBBY_LIST_AUTO_UPDATE_TIME < now)
	{
		std::vector<VisibleChatLobbyRecord> visible_lobbies_tmp ;
		getListOfNearbyChatLobbies(visible_lobbies_tmp) ;

		if (visible_lobbies_tmp.empty()){
			last_req_chat_lobby_list = now-LOBBY_LIST_AUTO_UPDATE_TIME+MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ;
		} else {
			last_req_chat_lobby_list = now ;
		}
	}
}

bool DistributedChatService::handleRecvChatLobbyMsgItem(RsChatMsgItem *ci)
{
    // check if it's a lobby msg, in which case we replace the peer id by the lobby's virtual peer id.
    //
    RsChatLobbyMsgItem *cli = dynamic_cast<RsChatLobbyMsgItem*>(ci) ;

    if(cli == NULL)
        return true ;	// the item is handled correctly if it's not a lobby item ;-)

    rstime_t now = time(NULL) ;

    if(now+100 > (rstime_t) cli->sendTime + MAX_KEEP_MSG_RECORD)	// the message is older than the max cache keep plus 100 seconds ! It's too old, and is going to make an echo!
    {
        std::cerr << "Received severely outdated lobby event item (" << now - (rstime_t)cli->sendTime << " in the past)! Dropping it!" << std::endl;
        std::cerr << "Message item is:" << std::endl;
        cli->print(std::cerr) ;
        std::cerr << std::endl;
        return false ;
    }
    if(now+600 < (rstime_t) cli->sendTime)	// the message is from the future. Drop it. more than 10 minutes
    {
        std::cerr << "Received event item from the future (" << (rstime_t)cli->sendTime - now << " seconds in the future)! Dropping it!" << std::endl;
        std::cerr << "Message item is:" << std::endl;
        cli->print(std::cerr) ;
        std::cerr << std::endl;
        return false ;
    }
    
	if( rsReputations->overallReputationLevel(cli->signature.keyId) ==
	        RsReputationLevel::LOCALLY_NEGATIVE )
    {
        std::cerr << "(WW) Received lobby msg/item from banned identity " << cli->signature.keyId << ". Dropping it." << std::endl;
        return false ;
    }
    if(!checkSignature(cli,cli->PeerId()))	// check the object's signature and possibly request missing keys
    {
        std::cerr << "Signature mismatched for this lobby event item. Item will be dropped: " << std::endl;
        cli->print(std::cerr) ;
        std::cerr << std::endl;
        return false;
    }
    
    // add a routing clue for this peer/GXSid combination. This is quite reliable since the lobby transport is almost instantaneous
    
    rsGRouter->addRoutingClue(GRouterKeyId(cli->signature.keyId),cli->PeerId()) ;
    
    ChatLobbyFlags fl ;
    
    // delete items that are not for us, as early as possible.
    {
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        // send upward for display

        std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it =  _chat_lobbys.find(cli->lobby_id) ;
        
        if(it == _chat_lobbys.end())
        {
#ifdef DEBUG_CHAT_LOBBIES
            std::cerr << "Chatlobby for id " << std::hex << cli->lobby_id << " has no record. Dropping the msg." << std::dec << std::endl;
#endif
            return false;
        }
        fl = it->second.lobby_flags ;
    }
    if(IS_PGP_SIGNED_LOBBY(fl))
    {
	    RsIdentityDetails details;

	    if(!rsIdentity->getIdDetails(cli->signature.keyId,details))
	    {
#ifdef DEBUG_CHAT_LOBBIES
		    std::cerr << "(WW) cannot get ID " << cli->signature.keyId << " for checking signature  of lobby item." << std::endl;
#endif
		    return false;
	    }

	    if(!(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
	    {
		    std::cerr << "(WW) Received a lobby msg/item that is not PGP-authed (id=" << cli->signature.keyId << "), whereas the lobby flags require it. Rejecting!" << std::endl;

		    return false ;
	    }
    }
    
    if(!bounceLobbyObject(cli,cli->PeerId()))	// forwards the message to friends, keeps track of subscribers, etc.
        return false;

    // setup the peer id to the virtual peer id of the lobby.
    //
    RsPeerId virtual_peer_id ;
    getVirtualPeerId(cli->lobby_id,virtual_peer_id) ;
    cli->PeerId(virtual_peer_id) ;

    //name = cli->nick;
    //popupChatFlag = RS_POPUP_CHATLOBBY;

    RsServer::notify()->AddPopupMessage(RS_POPUP_CHATLOBBY, ChatId(cli->lobby_id).toStdString(), cli->signature.keyId.toStdString(), cli->message); /* notify private chat message */

    return true ;
}

bool DistributedChatService::checkSignature(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id)
{
    // Always request the key. If not present, it will be asked to the peer id who sent this item.

    std::list<RsPeerId> peer_list ;
    peer_list.push_back(peer_id) ;

    // network pre-request key to allow message authentication.

    mGixs->requestKey(obj->signature.keyId,peer_list,RsIdentityUsage(RS_SERVICE_TYPE_CHAT,RsIdentityUsage::CHAT_LOBBY_MSG_VALIDATION,RsGxsGroupId(),RsGxsMessageId(),obj->lobby_id));

    uint32_t size = RsChatSerialiser(RsSerializationFlags::SIGNATURE)
            .size(dynamic_cast<RsItem*>(obj));
    RsTemporaryMemory memory(size) ;

#ifdef DEBUG_CHAT_LOBBIES
    std::cerr << "Checking object signature: " << std::endl;
    std::cerr << "   signature id: " << obj->signature.keyId << std::endl;
#endif

	if( !RsChatSerialiser(RsSerializationFlags::SIGNATURE)
	        .serialise(dynamic_cast<RsItem*>(obj),memory,&size) )
    {
	    std::cerr << "  (EE) Cannot serialise message item. " << std::endl;
	    return false ;
    }

    uint32_t error_status ;
    RsIdentityUsage use_info(RS_SERVICE_TYPE_CHAT,RsIdentityUsage::CHAT_LOBBY_MSG_VALIDATION,RsGxsGroupId(),RsGxsMessageId(),obj->lobby_id) ;

    if(!mGixs->validateData(memory,size,obj->signature,false,use_info,error_status))
    {
	    bool res = false ;

	    switch(error_status)
	    {
	    case RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE:
#ifdef DEBUG_CHAT_LOBBIES
		    std::cerr << "(WW) Key is not is cache. Cannot verify." << std::endl;
#endif
		    res =true ;
		    break ;
	    case RsGixs::RS_GIXS_ERROR_SIGNATURE_MISMATCH: std::cerr << "(EE) Signature mismatch. Spoofing/MITM?." << std::endl;
		    res =false ;
		    break ;
	    default: break ;
	    }
	    return res;
    }


#ifdef DEBUG_CHAT_LOBBIES
    std::cerr << "  signature: CHECKS" << std::endl;
#endif

    return true ;
}

bool DistributedChatService::getVirtualPeerId(const ChatLobbyId& id,ChatLobbyVirtualPeerId& vpid) 
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Was asked for virtual peer name of " << std::hex << id << std::dec<< std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.find(id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "   not found!! " << std::endl;
#endif
		return false ;
	}

	vpid = it->second.virtual_peer_id ;
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "   returning " << vpid << std::endl;
#endif
	return true ;
}

void DistributedChatService::locked_printDebugInfo() const
{
	std::cerr << "Recorded lobbies: " << std::endl;
	rstime_t now = time(NULL) ;

	for( std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin()) ;it!=_chat_lobbys.end();++it)
	{
		std::cerr << "   Lobby id\t\t: " << std::hex << it->first << std::dec << std::endl;
		std::cerr << "   Lobby name\t\t: " << it->second.lobby_name << std::endl;
		std::cerr << "   Lobby topic\t\t: " << it->second.lobby_topic << std::endl;
		std::cerr << "   nick name\t\t: " << it->second.gxs_id << std::endl;
		std::cerr << "   Lobby type\t\t: " << ((IS_PUBLIC_LOBBY(it->second.lobby_flags))?"Public":"Private") << std::endl;
		std::cerr << "   Lobby security\t\t: " << ((IS_PGP_SIGNED_LOBBY(it->second.lobby_flags))?"PGP-signed IDs required":"Anon IDs accepted") << std::endl;
		std::cerr << "   Lobby peer id\t: " << it->second.virtual_peer_id << std::endl;
		std::cerr << "   Challenge count\t: " << it->second.connexion_challenge_count << std::endl;
		std::cerr << "   Last activity\t: " << now - it->second.last_activity << " seconds ago." << std::endl;
		std::cerr << "   Cached messages\t: " << it->second.msg_cache.size() << std::endl;

		for(std::map<ChatLobbyMsgId,rstime_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
			std::cerr << "       " << std::hex << it2->first << std::dec << "  time=" << now - it2->second << " secs ago" << std::endl;

		std::cerr << "   Participating friends: " << std::endl;

		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "       " << *it2 << std::endl;

		std::cerr << "   Participating nick names: " << std::endl;

		for(std::map<RsGxsId,rstime_t>::const_iterator it2(it->second.gxs_ids.begin());it2!=it->second.gxs_ids.end();++it2)
			std::cerr << "       " << it2->first << ": " << now - it2->second << " secs ago" << std::endl;

	}

	std::cerr << "Visible public lobbies: " << std::endl;

	for( std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.begin()) ;it!=_visible_lobbies.end();++it)
	{
		std::cerr << "   " << std::hex << it->first << " name = " << std::dec << it->second.lobby_name << it->second.lobby_topic << std::endl;
		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "    With friend: " << *it2 << std::endl;
	}

    std::cerr << "Chat lobby flags: " << std::endl;

    for( std::map<ChatLobbyId,ChatLobbyFlags>::const_iterator it(_known_lobbies_flags.begin()) ;it!=_known_lobbies_flags.end();++it)
        std::cerr << "   \"" << std::hex << it->first << "\" flags = " << it->second << std::dec << std::endl;
}

bool DistributedChatService::locked_bouncingObjectCheck(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id,uint32_t lobby_count)
{
	static std::map<std::string, std::list<rstime_t> > message_counts ;

	std::ostringstream os ;
	os << obj->lobby_id ;

	std::string pid = peer_id.toStdString() + "_" + os.str() ;

	// Check for the number of peers in the lobby. First look into visible lobbies, because the number
	// of peers there is more accurate. If non existant (because it's a private lobby), take the count from
	// the current lobby list.
	//
	std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it = _visible_lobbies.find(obj->lobby_id) ;

	if(it != _visible_lobbies.end())
		lobby_count = it->second.total_number_of_peers ;
	else
	{
		std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it = _chat_lobbys.find(obj->lobby_id) ;

		if(it != _chat_lobbys.end())
            lobby_count = it->second.gxs_ids.size() ;
		else
		{
			std::cerr << "DistributedChatService::locked_bouncingObjectCheck(): weird situation: cannot find lobby in visible lobbies nor current lobbies. Dropping message. If you see this, contact the developers." << std::endl;
			return false ;
		}
	}

	// max objects per second: lobby_count * 1/MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE objects per second.
	// So in cache, there is in average that number times MAX_MESSAGES_PER_SECONDS_PERIOD
	//
	float max_cnt = std::max(10.0f, 4*lobby_count / (float)MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE * MAX_MESSAGES_PER_SECONDS_PERIOD) ;

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "lobby_count=" << lobby_count << std::endl;
	std::cerr << "Got msg for peer " << pid << std::dec << ". Limit is " << max_cnt << ". List is " ;
	for(std::list<rstime_t>::const_iterator it(message_counts[pid].begin());it!=message_counts[pid].end();++it)
		std::cerr << *it << " " ;
	std::cerr << std::endl;
#endif

	rstime_t now = time(NULL) ;

	std::list<rstime_t>& lst = message_counts[pid] ;
	
	// Clean old messages time stamps from the list.
	//
	while(!lst.empty() && lst.front() + MAX_MESSAGES_PER_SECONDS_PERIOD < now)
		lst.pop_front() ;

	if(lst.size() > max_cnt)
	{
		std::cerr << "Too many messages from peer " << pid << ". Someone (name=" << obj->nick << ") is trying to flood this lobby. Message will not be forwarded." << std::endl;
		return false;
	}
	else
		lst.push_back(now) ;

	return true ;
}

// This function should be used for all types of chat messages. But this requires a non backward compatible change in
// chat protocol. To be done for version 0.6
//
void DistributedChatService::checkSizeAndSendLobbyMessage(RsChatItem *msg)
{
    // Multiple-parts messaging has been disabled in lobbies, because of the following issues:
    //    1 - it breaks signatures because the subid of each sub-item is changed (can be fixed)
    //    2 - it breaks (can be fixed as well)
    //    3 - it is unreliable since items are not guarrantied to all arrive in the end (cannot be fixed)
    //    4 - large objects can be used to corrupt end peers (cannot be fixed)
    //
    static const uint32_t MAX_ITEM_SIZE = 32000 ;

    if(RsChatSerialiser().size(msg) > MAX_ITEM_SIZE)
    {
        std::cerr << "(EE) Chat item exceeds maximum serial size. It will be dropped." << std::endl;
        delete msg ;
    return ;
    }
    sendChatItem(msg) ;
}

bool DistributedChatService::handleRecvItem(RsChatItem *item)
{
	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_EVENT:      handleRecvChatLobbyEventItem     (dynamic_cast<RsChatLobbyEventItem            *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_DEPRECATED: handleRecvLobbyInvite_Deprecated (dynamic_cast<RsChatLobbyInviteItem_Deprecated*>(item)) ; break ; // to be removed (deprecated since May 2017)
		case RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE:            handleRecvLobbyInvite            (dynamic_cast<RsChatLobbyInviteItem           *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE:         handleConnectionChallenge        (dynamic_cast<RsChatLobbyConnectChallengeItem *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE:       handleFriendUnsubscribeLobby     (dynamic_cast<RsChatLobbyUnsubscribeItem      *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST:      handleRecvChatLobbyListRequest   (dynamic_cast<RsChatLobbyListRequestItem      *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST:              handleRecvChatLobbyList          (dynamic_cast<RsChatLobbyListItem             *>(item)) ; break ;
		default:                                          return false ;
	}
	return true ;
}

void DistributedChatService::handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *clr)
{
	// make a lobby list item
	//
	RsChatLobbyListItem *item = new RsChatLobbyListItem;

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Peer " << clr->PeerId() << " requested the list of public chat lobbies." << std::endl;
#endif

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
		{
			const ChatLobbyEntry& lobby(it->second) ;

            if(IS_PUBLIC_LOBBY(lobby.lobby_flags) ||
                             (lobby.previously_known_peers.find(clr->PeerId()) != lobby.previously_known_peers.end()
                            ||lobby.participating_friends.find(clr->PeerId()) != lobby.participating_friends.end()) )
			{
#ifdef DEBUG_CHAT_LOBBIES
        std::cerr << "  Adding lobby " << std::hex << it->first << std::dec << " \""
                  << it->second.lobby_name << it->second.lobby_topic << "\" count=" << it->second.gxs_ids.size() << std::endl;
#endif

                VisibleChatLobbyInfo info ;

                info.id    = it->first ;
                info.name  = it->second.lobby_name ;
                info.topic = it->second.lobby_topic ;
                info.count = it->second.gxs_ids.size() ;
                info.flags = ChatLobbyFlags(EXTRACT_PRIVACY_FLAGS(it->second.lobby_flags)) ;

                item->lobbies.push_back(info) ;
			}
#ifdef DEBUG_CHAT_LOBBIES
			else
				std::cerr << "  Not adding private lobby " << std::hex << it->first << std::dec << std::endl ;
#endif
		}
	}

	item->PeerId(clr->PeerId()) ;

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "  Sending list to " << clr->PeerId() << std::endl;
#endif
	sendChatItem(item);
}

void DistributedChatService::handleRecvChatLobbyList(RsChatLobbyListItem *item)
{
    if(item->lobbies.size() > MAX_ALLOWED_LOBBIES_IN_LIST_WARNING)
        std::cerr << "Warning: Peer " << item->PeerId() << "(" << rsPeers->getPeerName(item->PeerId()) << ") is sending a lobby list of " << item->lobbies.size() << " lobbies. This is unusual, and probably a attempt to crash you." << std::endl;

    std::list<ChatLobbyId> chatLobbyToSubscribe;
	 std::list<ChatLobbyId> invitationNeeded ;

#ifdef DEBUG_CHAT_LOBBIES
     std::cerr << "Received chat lobby list from friend " << item->PeerId() << ", " << item->lobbies.size() << " elements." << std::endl;
#endif
	{
		rstime_t now = time(NULL) ;

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        for(uint32_t i=0;i<item->lobbies.size() && i < MAX_ALLOWED_LOBBIES_IN_LIST_WARNING;++i)
		{
            VisibleChatLobbyRecord& rec(_visible_lobbies[item->lobbies[i].id]) ;

            rec.lobby_id = item->lobbies[i].id ;
            rec.lobby_name = item->lobbies[i].name ;
            rec.lobby_topic = item->lobbies[i].topic ;
			rec.participating_friends.insert(item->PeerId()) ;

			if(_should_reset_lobby_counts)
                rec.total_number_of_peers = item->lobbies[i].count ;
			else
                rec.total_number_of_peers = std::max(rec.total_number_of_peers,item->lobbies[i].count) ;

			rec.last_report_time = now ;
            rec.lobby_flags = EXTRACT_PRIVACY_FLAGS(item->lobbies[i].flags) ;

            std::map<ChatLobbyId,ChatLobbyFlags>::const_iterator it(_known_lobbies_flags.find(item->lobbies[i].id)) ;

#ifdef DEBUG_CHAT_LOBBIES
            std::cerr << "  lobby id " << std::hex << item->lobbies[i].id << std::dec << ", " << item->lobbies[i].name
                      << ", " << item->lobbies[i].count << " participants" << std::endl;
#endif
			if(it != _known_lobbies_flags.end() && (it->second & RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE))
			{
#ifdef DEBUG_CHAT_LOBBIES
				std::cerr << "    lobby is flagged as autosubscribed. Adding it to subscribe list." << std::endl;
#endif
				ChatLobbyId clid = item->lobbies[i].id;
				if(_chat_lobbys.find(clid) == _chat_lobbys.end())
					chatLobbyToSubscribe.push_back(clid);
			}

			// for subscribed lobbies, check that item->PeerId() is among the participating friends. If not, add him!

            std::map<ChatLobbyId,ChatLobbyEntry>::iterator it2 = _chat_lobbys.find(item->lobbies[i].id) ;

			if(it2 != _chat_lobbys.end() && it2->second.participating_friends.find(item->PeerId()) == it2->second.participating_friends.end())
			{
#ifdef DEBUG_CHAT_LOBBIES
				std::cerr << "    lobby is currently subscribed but friend is not participating already -> adding to partipating friends and sending invite." << std::endl;
#endif
				it2->second.participating_friends.insert(item->PeerId()) ; 
                invitationNeeded.push_back(item->lobbies[i].id) ;
			}
		}
	}

	std::list<ChatLobbyId>::iterator it;
	for (it = chatLobbyToSubscribe.begin(); it != chatLobbyToSubscribe.end(); ++it)
	{
		RsGxsId gxsId = _lobby_default_identity[*it];
		if (gxsId.isNull())
			gxsId = _default_identity;

		//Check if gxsId can connect to this lobby
		ChatLobbyFlags flags(0);
		std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator vlIt = _visible_lobbies.find(*it) ;
		if(vlIt != _visible_lobbies.end())
			flags = vlIt->second.lobby_flags;
		else
		{
			std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator clIt = _chat_lobbys.find(*it) ;

			if(clIt != _chat_lobbys.end())
				flags = clIt->second.lobby_flags;
		}

		RsIdentityDetails idd ;
		if(!rsIdentity->getIdDetails(gxsId,idd))
			std::cerr << "(EE) Lobby auto-subscribe: Can't get Id detail for:" << gxsId.toStdString().c_str() << std::endl;
		else
		{
			if(IS_PGP_SIGNED_LOBBY(flags)
			   && !(idd.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED) )
			{
				std::cerr << "(EE) Attempt to auto-subscribe to signed lobby with non signed Id. Remove it." << std::endl;
				setLobbyAutoSubscribe(*it, false);
			} else {
				joinVisibleChatLobby(*it,gxsId);
			}
		}
	}

	 for(std::list<ChatLobbyId>::const_iterator it = invitationNeeded.begin();it!=invitationNeeded.end();++it)
		 invitePeerToLobby(*it,item->PeerId(),false) ;

	 RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	_should_reset_lobby_counts = false ;
}

void DistributedChatService::addTimeShiftStatistics(int D)
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

#ifdef DEBUG_CHAT_LOBBIES
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

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << ". Expected delay: " << expected << std::endl ;
#endif

		if(expected > 9)	// if more than 20 samples
			RsServer::notify()->notifyChatLobbyTimeShift( (int)pow(2.0f,expected)) ;

		total = 0.0f ;
		log_delay_histogram.clear() ;
		log_delay_histogram.resize(S,0) ;
	}
#ifdef DEBUG_CHAT_LOBBIES
	else
		std::cerr << std::endl;
#endif
}

void DistributedChatService::handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item)
{
    ChatLobbyFlags fl ;
    
    // delete items that are not for us, as early as possible.
    {
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        // send upward for display

        std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it =  _chat_lobbys.find(item->lobby_id) ;
        
        if(it == _chat_lobbys.end())
        {
#ifdef DEBUG_CHAT_LOBBIES
            std::cerr << "Chatlobby for id " << std::hex << item->lobby_id << " has no record. Dropping the msg." << std::dec << std::endl;
#endif
            return ;
        }
        fl = it->second.lobby_flags ;
    }


#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Received ChatLobbyEvent item of type " << (int)(item->event_type) << ", and string=" << item->string1 << std::endl;
#endif
	rstime_t now = time(nullptr);

	if( rsReputations->overallReputationLevel(item->signature.keyId) ==
	         RsReputationLevel::LOCALLY_NEGATIVE )
    {
        std::cerr << "(WW) Received lobby msg/item from banned identity " << item->signature.keyId << ". Dropping it." << std::endl;
        return ;
    }
    if(!checkSignature(item,item->PeerId()))	// check the object's signature and possibly request missing keys
    {
        std::cerr << "Signature mismatched for this lobby event item: " << std::endl;
        item->print(std::cerr) ;
        std::cerr << std::endl;
        return ;
    }
        
    if(IS_PGP_SIGNED_LOBBY(fl))
    {
	    RsIdentityDetails details;

	    if(!rsIdentity->getIdDetails(item->signature.keyId,details))
	    {
#ifdef DEBUG_CHAT_LOBBIES
		    std::cerr << "(WW) cannot get ID " << item->signature.keyId << " for checking signature  of lobby item." << std::endl;
#endif
		    return ;
	    }

	    if(!(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
	    {
		    std::cerr << "(WW) Received a lobby msg/item that is not PGP-authed (ID=" << item->signature.keyId << "), whereas the lobby flags require it. Rejecting!" << std::endl;

		    return ;
	    }
    }
    addTimeShiftStatistics((int)now - (int)item->sendTime) ;

	if(now+100 > (rstime_t) item->sendTime + MAX_KEEP_MSG_RECORD)	// the message is older than the max cache keep minus 100 seconds ! It's too old, and is going to make an echo!
	{
		std::cerr << "Received severely outdated lobby event item (" << now - (rstime_t)item->sendTime << " in the past)! Dropping it!" << std::endl;
		std::cerr << "Message item is:" << std::endl;
		item->print(std::cerr) ;
		std::cerr << std::endl;
		return ;
	}
	if(now+600 < (rstime_t) item->sendTime)	// the message is from the future more than 10 minutes
	{
		std::cerr << "Received event item from the future (" << (rstime_t)item->sendTime - now << " seconds in the future)! Dropping it!" << std::endl;
		std::cerr << "Message item is:" << std::endl;
		item->print(std::cerr) ;
		std::cerr << std::endl;
		return ;
	}
    // add a routing clue for this peer/GXSid combination. This is quite reliable since the lobby transport is almost instantaneous
    rsGRouter->addRoutingClue(GRouterKeyId(item->signature.keyId),item->PeerId()) ;
    
    if(! bounceLobbyObject(item,item->PeerId()))
		return ;

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "  doing specific job for this status item." << std::endl;
#endif

	if(item->event_type == RS_CHAT_LOBBY_EVENT_PEER_LEFT)		// if a peer left. Remove its nickname from the list.
    {
#ifdef DEBUG_CHAT_LOBBIES
        std::cerr << "  removing nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

        if(it != _chat_lobbys.end())
        {
            std::map<RsGxsId,rstime_t>::iterator it2(it->second.gxs_ids.find(item->signature.keyId)) ;

            if(it2 != it->second.gxs_ids.end())
            {
                it->second.gxs_ids.erase(it2) ;
#ifdef DEBUG_CHAT_LOBBIES
                std::cerr << "  removed id " << item->signature.keyId << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
            }
#ifdef DEBUG_CHAT_LOBBIES
            else
                std::cerr << "  (EE) nickname " << item->nick << " not in participant nicknames list!" << std::endl;
#endif
        }
    }
	else if(item->event_type == RS_CHAT_LOBBY_EVENT_PEER_JOINED)		// if a joined left. Add its nickname to the list.
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  adding nickname " << item->nick << " to lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
            it->second.gxs_ids[item->signature.keyId] = time(NULL) ;
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  added nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
		}
	}
	else if(item->event_type == RS_CHAT_LOBBY_EVENT_KEEP_ALIVE)		// keep alive packet. 
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  adding nickname " << item->nick << " to lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
            it->second.gxs_ids[item->signature.keyId] = time(NULL) ;
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  added nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
		}
	}
    RsServer::notify()->notifyChatLobbyEvent(item->lobby_id,item->event_type,item->signature.keyId,item->string1) ;
}
void DistributedChatService::getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& visible_lobbies)
{
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		visible_lobbies.clear() ;

		for(std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.begin());it!=_visible_lobbies.end();++it)
			visible_lobbies.push_back(it->second) ;
	}

	rstime_t now = time(NULL) ;

	if(now > MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ + last_visible_lobby_info_request_time)
	{
		std::set<RsPeerId> ids ;
		mServControl->getPeersConnected(mServType, ids);

		for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
		{
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  asking list of public lobbies to " << *it << std::endl;
#endif
			RsChatLobbyListRequestItem *item = new RsChatLobbyListRequestItem ;
			item->PeerId(*it) ;

			sendChatItem(item);
		}
		last_visible_lobby_info_request_time = now ;
		_should_reset_lobby_counts = true ;
	}
}

// returns:
// 	true: the object is not a duplicate and should be used
// 	false: the object is a duplicate or there is an error, and it should be destroyed.
//
bool DistributedChatService::bounceLobbyObject(RsChatLobbyBouncingObject *item,const RsPeerId& peer_id)
{
	rstime_t now = time(NULL) ;
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
#ifdef DEBUG_CHAT_LOBBIES
	locked_printDebugInfo() ; // debug

	std::cerr << "Handling ChatLobbyMsg " << std::hex << item->msg_id << ", lobby id " << item->lobby_id << ", from peer id " << peer_id << std::endl;
#endif

	// send upward for display

	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(item->lobby_id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "Chatlobby for id " << std::hex << item->lobby_id << " has no record. Dropping the msg." << std::dec << std::endl;
#endif
		return false ;
	}

	ChatLobbyEntry& lobby(it->second) ;

	// Adds the peer id to the list of friend participants, even if it's not original msg source

	if(peer_id != mServControl->getOwnId())
		lobby.participating_friends.insert(peer_id) ;

    lobby.gxs_ids[item->signature.keyId] = now ;

	// Checks wether the msg is already recorded or not

	std::map<ChatLobbyMsgId,rstime_t>::iterator it2(lobby.msg_cache.find(item->msg_id)) ;

	if(it2 != lobby.msg_cache.end()) // found!
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  Msg already received at time " << it2->second << ". Dropping!" << std::endl ;
#endif
		it2->second = now ;	// update last msg seen time, to prevent echos.
		return false ;
	}
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "  Msg not received already. Adding in cache, and forwarding!" << std::endl ;
#endif

	lobby.msg_cache[item->msg_id] = now ;
	lobby.last_activity = now ;

	// Check that if we have a lobby bouncing object, it's not flooding the lobby
	if(!locked_bouncingObjectCheck(item,peer_id,lobby.participating_friends.size()))
		return false;

	// Forward to allparticipating friends, except this peer.

	for(std::set<RsPeerId>::const_iterator it(lobby.participating_friends.begin());it!=lobby.participating_friends.end();++it)
		if((*it)!=peer_id && mServControl->isPeerConnected(mServType, *it)) 
		{
			RsChatLobbyBouncingObject *obj2 = item->duplicate() ; // makes a copy
			RsChatItem *item2 = dynamic_cast<RsChatItem*>(obj2) ;

			assert(item2 != NULL) ;

			item2->PeerId(*it) ;	// replaces the virtual peer id with the actual destination.

            checkSizeAndSendLobbyMessage(item2) ;
		}

	++lobby.connexion_challenge_count ;

	return true ;
}

void DistributedChatService::sendLobbyStatusString(const ChatLobbyId& lobby_id,const std::string& status_string)
{
	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_STATUS,status_string) ; 
}

/** 
 * Inform other Clients of a nickname change
 * 
 * as example for updating their ChatLobby Blocklist for muted peers
 *  */
void DistributedChatService::sendLobbyStatusPeerChangedNickname(const ChatLobbyId& lobby_id, const std::string& newnick)
{
	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_CHANGE_NICKNAME, newnick) ; 
}


void DistributedChatService::sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id)
{
    sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_LEFT,std::string()) ;
}

void DistributedChatService::sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id)
{
    sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_JOINED,std::string()) ;
}

void DistributedChatService::sendLobbyStatusKeepAlive(const ChatLobbyId& lobby_id)
{
    sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,std::string()) ;
}

void DistributedChatService::sendLobbyStatusItem(const ChatLobbyId& lobby_id,int type,const std::string& status_string) 
{
    RsChatLobbyEventItem item ;

    {
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        item.event_type = type ;
        item.string1 = status_string ;
        item.sendTime = time(NULL) ;

        if(! locked_initLobbyBouncableObject(lobby_id,item))
            return ;
    }
    RsPeerId ownId = mServControl->getOwnId();
    bounceLobbyObject(&item,ownId) ;
}

bool DistributedChatService::locked_initLobbyBouncableObject(const ChatLobbyId& lobby_id,RsChatLobbyBouncingObject& item)
{
	// get a pointer to the info for that chat lobby.
	//
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(lobby_id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "Chatlobby for id " << std::hex << lobby_id << " has no record. This is a serious error!!" << std::dec << std::endl;
#endif
		return false;
	}
	ChatLobbyEntry& lobby(it->second) ;

	// chat lobby stuff
	//
	do 
	{ 
		item.msg_id	= RSRandom::random_u64(); 
	} 
	while( lobby.msg_cache.find(item.msg_id) != lobby.msg_cache.end() ) ;

    RsIdentityDetails details ;
    if(!rsIdentity || !rsIdentity->getIdDetails(lobby.gxs_id,details))
    {
        std::cerr << "(EE) Cannot send chat lobby object. Signign identity " << lobby.gxs_id << " is unknown." << std::endl;
        return false ;
    }

    item.lobby_id = lobby_id ;
    item.nick = details.mNickname ;
    item.signature.TlvClear() ;
    item.signature.keyId = lobby.gxs_id ;

    // now sign the object, if the lobby expects it

	uint32_t size = RsChatSerialiser(RsSerializationFlags::SIGNATURE)
	        .size(dynamic_cast<RsItem*>(&item));
        RsTemporaryMemory memory(size) ;

	if( !RsChatSerialiser(RsSerializationFlags::SIGNATURE)
	        .serialise(dynamic_cast<RsItem*>(&item),memory,&size) )
        {
            std::cerr << "(EE) Cannot sign message item. " << std::endl;
            return false ;
        }

    uint32_t error_status ;

    if(!mGixs->signData(memory,size,lobby.gxs_id,item.signature,error_status))
    {
        switch(error_status)
        {
            case RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE: std::cerr << "(EE) Cannot sign item: key not available for ID " << lobby.gxs_id << std::endl;
                                        break ;
            default: std::cerr << "(EE) Cannot sign item: unknown error" << std::endl;
                                        break ;
        }
        return false ;
    }

#ifdef DEBUG_CHAT_LOBBIES
    std::cerr << "  signature done." << std::endl;

    // check signature
        if(!mGixs->validateData(memory,item.signed_serial_size(),item.signature,true,error_status))
        {
            std::cerr << "(EE) Cannot check message item. " << std::endl;
            return false ;
        }
    std::cerr << "  signature checks!" << std::endl;
    std::cerr << "  Item dump:" << std::endl;
    item.print(std::cerr,2) ;
#endif

    return true ;
}

bool DistributedChatService::sendLobbyChat(const ChatLobbyId& lobby_id, const std::string& msg)
{
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Sending chat lobby message to lobby " << std::hex << lobby_id << std::dec << std::endl;
	std::cerr << "msg:" << std::endl;
	std::cerr << msg << std::endl;
#endif

	RsChatLobbyMsgItem item ;

    {
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        // chat msg stuff
        //
        item.chatFlags = RS_CHAT_FLAG_LOBBY | RS_CHAT_FLAG_PRIVATE;
        item.sendTime = time(NULL);
        item.recvTime = item.sendTime;
        item.message = msg;
        item.parent_msg_id = 0;

        // gives a random msg id, setup the nickname, and signs the item.

        if(!  locked_initLobbyBouncableObject(lobby_id,item))
            return false;
    }

    RsPeerId ownId = rsPeers->getOwnId();

    bounceLobbyObject(&item, ownId) ;

    ChatMessage message;
    message.chatflags = 0;
    message.chat_id = ChatId(lobby_id);
    message.msg = msg;
    message.lobby_peer_gxs_id = item.signature.keyId;
    message.recvTime = item.recvTime;
    message.sendTime = item.sendTime;
    message.incoming = false;
    message.online = true;
    RsServer::notify()->notifyChatMessage(message);
    mHistMgr->addMessage(message);

	return true ;
}

void DistributedChatService::handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) 
{
	// Look into message cache of all lobbys to handle the challenge.
	//
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "DistributedChatService::handleConnectionChallenge(): received connection challenge:" << std::endl;
	std::cerr << "    Challenge code = 0x" << std::hex << item->challenge_code << std::dec << std::endl;
	std::cerr << "    Peer Id        =   " << item->PeerId() << std::endl;
#endif

	rstime_t now = time(NULL) ;
	ChatLobbyId lobby_id ;
	const RsPeerId& ownId = rsPeers->getOwnId();

	bool found = false ;
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end() && !found;++it)
			for(std::map<ChatLobbyMsgId,rstime_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end() && !found;++it2)
				if(it2->second + CONNECTION_CHALLENGE_MAX_MSG_AGE + 5 > now)  // any msg not older than 5 seconds plus max challenge count is fine.
				{
					uint64_t code = makeConnexionChallengeCode(ownId,it->first,it2->first) ;
#ifdef DEBUG_CHAT_LOBBIES
					std::cerr << "    Lobby_id = 0x" << std::hex << it->first << ", msg_id = 0x" << it2->first << ": code = 0x" << code << std::dec << std::endl ;
#endif

					if(code == item->challenge_code)
					{
#ifdef DEBUG_CHAT_LOBBIES
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
#ifdef DEBUG_CHAT_LOBBIES
	else
		std::cerr << "    Challenge denied: no existing cached msg has matching Id." << std::endl;
#endif
}

void DistributedChatService::sendConnectionChallenge(ChatLobbyId lobby_id) 
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Sending connection challenge to friends for lobby 0x" << std::hex << lobby_id << std::dec << std::endl ;
#endif

	// look for a msg in cache. Any recent msg is fine.
	
	std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << "ERROR: sendConnectionChallenge(): could not find lobby 0x" << std::hex << lobby_id << std::dec << std::endl;
		return ;
	}

	rstime_t now = time(NULL) ;
	ChatLobbyMsgId msg_id = 0 ;

	for(std::map<ChatLobbyMsgId,rstime_t>::const_iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();++it2)
		if(it2->second + CONNECTION_CHALLENGE_MAX_MSG_AGE > now)  // any msg not older than 20 seconds is fine.
		{
			msg_id = it2->first ;
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  Using msg id 0x" << std::hex << it2->first << std::dec << std::endl; 
#endif
			break ;
		}

	if(msg_id == 0)
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  No suitable message found in cache. Probably not enough activity !!" << std::endl;
#endif
		return ;
	}

	// Broadcast to all direct friends

	std::set<RsPeerId> ids ;
	mServControl->getPeersConnected(mServType, ids);

	for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		RsChatLobbyConnectChallengeItem *item = new RsChatLobbyConnectChallengeItem ;

		uint64_t code = makeConnexionChallengeCode(*it,lobby_id,msg_id) ;

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  Sending collexion challenge code 0x" << std::hex << code <<  std::dec << " to peer " << *it << std::endl; 
#endif
		item->PeerId(*it) ;
		item->challenge_code = code ;

		sendChatItem(item);
	}
}

uint64_t DistributedChatService::makeConnexionChallengeCode(const RsPeerId& peer_id,ChatLobbyId lobby_id,ChatLobbyMsgId msg_id)
{
	uint64_t result = 0 ;

	for(uint32_t i=0;i<RsPeerId::SIZE_IN_BYTES;++i)
	{
		result += msg_id ;
		result ^= result >> 35 ;
		result += result <<  6 ;
		result ^= peer_id.toByteArray()[i] * lobby_id ;
		result += result << 26 ;
		result ^= result >> 13 ;
	}
	return result ;
}
void DistributedChatService::getChatLobbyList(std::list<ChatLobbyId>& clids)
{
    // fill up a dummy list for now.

    clids.clear() ;

    RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

    for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
        clids.push_back(it->first) ;
}
bool DistributedChatService::getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& info)
{
	// fill up a dummy list for now.

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/


    std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it = _chat_lobbys.find(id) ;

    if(it != _chat_lobbys.end())
    {
        info = it->second ;
        return true ;
    }
    else
        return false ;
}
void DistributedChatService::invitePeerToLobby(const ChatLobbyId& lobby_id, const RsPeerId& peer_id,bool connexion_challenge) 
{
#ifdef DEBUG_CHAT_LOBBIES
	if(connexion_challenge)
		std::cerr << "Sending connection challenge accept to peer " << peer_id << " for lobby "<< std::hex << lobby_id << std::dec << std::endl;
	else
		std::cerr << "Sending invitation to peer " << peer_id << " to lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  invitation send: canceled. Lobby " << lobby_id << " not found!" << std::endl;
#endif
		return ;
	}

	RsChatLobbyInviteItem *item = new RsChatLobbyInviteItem ;

	item->lobby_id    =  lobby_id ;
	item->lobby_name  =  it->second.lobby_name ;
	item->lobby_topic =  it->second.lobby_topic ;
	item->lobby_flags =  connexion_challenge?RS_CHAT_LOBBY_FLAGS_CHALLENGE:(it->second.lobby_flags) ;
	item->PeerId(peer_id) ;

	sendChatItem(item) ;

	//FOR BACKWARD COMPATIBILITY
	{// to be removed (deprecated since May 2017)
		RsChatLobbyInviteItem_Deprecated *item = new RsChatLobbyInviteItem_Deprecated ;

		item->lobby_id    =  lobby_id ;
		item->lobby_name  =  it->second.lobby_name ;
		item->lobby_topic =  it->second.lobby_topic ;
		item->lobby_flags =  connexion_challenge?RS_CHAT_LOBBY_FLAGS_CHALLENGE:(it->second.lobby_flags) ;
		item->PeerId(peer_id) ;

		sendChatItem(item) ;
	}
}

// to be removed (deprecated since May 2017)
void DistributedChatService::handleRecvLobbyInvite_Deprecated(RsChatLobbyInviteItem_Deprecated *item)
{
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Received deprecated invite to lobby from " << item->PeerId() << " to lobby " << std::hex << item->lobby_id << std::dec << ", named " << item->lobby_name << item->lobby_topic << std::endl;
#endif
	RsChatLobbyInviteItem newItem ;

	newItem.lobby_id = item->lobby_id ;
	newItem.lobby_name = item->lobby_name ;
	newItem.lobby_topic = item->lobby_topic ;
	newItem.lobby_flags = item->lobby_flags ;
	newItem.PeerId( item->PeerId() );

	handleRecvLobbyInvite(&newItem);	// The item is not deleted inside this function.
}

void DistributedChatService::handleRecvLobbyInvite(RsChatLobbyInviteItem *item) 
{
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Received invite to lobby from " << item->PeerId() << " to lobby " << std::hex << item->lobby_id << std::dec << ", named " << item->lobby_name << item->lobby_topic << std::endl;
#endif

	// 1 - store invite in a cache
	//
	// 1.1 - if the lobby is already setup, add the peer to the communicating peers.
	//
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  Lobby already exists. " << std::endl;
			std::cerr << "     privacy levels: " << item->lobby_flags << " vs. " << it->second.lobby_flags ;
#endif

			if ((!IS_CONNEXION_CHALLENGE(item->lobby_flags)) && EXTRACT_PRIVACY_FLAGS(item->lobby_flags) != EXTRACT_PRIVACY_FLAGS(it->second.lobby_flags))
			{
				std::cerr << " : Don't match. Cancelling." << std::endl;
				return ;
			}
#ifdef DEBUG_CHAT_LOBBIES
			else
				std::cerr << " : Match!" << std::endl;

			std::cerr << "  Adding new friend " << item->PeerId() << " to lobby." << std::endl;
#endif

			// to be removed (deprecated since May 2017)
			{ //Update Topics if have received deprecated before (withou topic)
				if(it->second.lobby_topic.empty() && !item->lobby_topic.empty())
					it->second.lobby_topic = item->lobby_topic;
			}

			it->second.participating_friends.insert(item->PeerId()) ;
			return ;
		}

		// to be removed (deprecated since May 2017)
		{//check if invitation is already received by deprecated version
			std::map<ChatLobbyId,ChatLobbyInvite>::const_iterator it(_lobby_invites_queue.find( item->lobby_id)) ;
			if(it != _lobby_invites_queue.end())
				return ;
		}
		// Don't record the invitation if it's a challenge response item or a lobby we don't have.
		//
        if(IS_CONNEXION_CHALLENGE(item->lobby_flags))
			return ;

		// no, then create a new invitation entry in the cache.
		
		ChatLobbyInvite invite ;
		invite.lobby_id = item->lobby_id ;
		invite.peer_id = item->PeerId() ;
		invite.lobby_name = item->lobby_name ;
		invite.lobby_topic = item->lobby_topic ;
		invite.lobby_flags = item->lobby_flags ;

		_lobby_invites_queue[item->lobby_id] = invite ;
	}
	// 2 - notify the gui to ask the user.
	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_INVITATION, NOTIFY_TYPE_ADD);
}

void DistributedChatService::getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites)
{
	invites.clear() ;

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

	for(std::map<ChatLobbyId,ChatLobbyInvite>::const_iterator it(_lobby_invites_queue.begin());it!=_lobby_invites_queue.end();++it)
		invites.push_back(it->second) ;
}


bool DistributedChatService::acceptLobbyInvite(const ChatLobbyId& lobby_id,const RsGxsId& identity)
{
    {
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "Accepting chat lobby "<< lobby_id << std::endl;
#endif

		std::map<ChatLobbyId,ChatLobbyInvite>::iterator it = _lobby_invites_queue.find(lobby_id) ;

		if(it == _lobby_invites_queue.end())
		{
			std::cerr << " (EE) lobby invite not in cache!!" << std::endl;
			return false;
		}

		//std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator vid = _visible_lobbies.find(lobby_id) ;

		//When invited to new Lobby, it is not visible.
		//if(_visible_lobbies.end() == vid)
		//{
		//	std::cerr << " (EE) Cannot subscribe a non visible chat lobby!!" << std::endl;
		//	return false ;
		//}

		RsIdentityDetails det ;
		if( (!rsIdentity->getIdDetails(identity,det)) || !(det.mFlags & RS_IDENTITY_FLAGS_IS_OWN_ID))
		{
			std::cerr << " (EE) Cannot subscribe with identity " << identity << " because it is not ours! Something's wrong here." << std::endl;
			return false ;
		}

		if( (it->second.lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED ) && !(det.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
		{
			std::cerr << " (EE) Cannot subscribe with identity " << identity << " because it is unsigned and the lobby requires signed ids only." << std::endl;
			return false ;
		}

		if(_chat_lobbys.find(lobby_id) != _chat_lobbys.end())
		{
			std::cerr << "  (II) Lobby already exists. Weird." << std::endl;
			return true ;
		}

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  Creating new Lobby entry." << std::endl;
#endif
		rstime_t now = time(NULL) ;

		ChatLobbyEntry entry ;
		entry.participating_friends.insert(it->second.peer_id) ;
		entry.lobby_flags = it->second.lobby_flags ;
		entry.gxs_id = identity ;
		entry.lobby_id = lobby_id ;
		entry.lobby_name = it->second.lobby_name ;
		entry.lobby_topic = it->second.lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ;
		entry.last_connexion_challenge_time = now ;
		entry.last_keep_alive_packet_time = now ;

		_chat_lobbys[lobby_id] = entry ;

		_lobby_invites_queue.erase(it) ;		// remove the invite from cache.

		// we should also send a message to the lobby to tell we're here.

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  Pushing new msg item to incoming msgs." << std::endl;
#endif
		RsChatLobbyMsgItem *item = new RsChatLobbyMsgItem;
		item->lobby_id = entry.lobby_id ;
		item->msg_id = 0 ;
        item->parent_msg_id = 0 ;
        item->nick = "Chat room management" ;
		item->message = std::string("Welcome to chat lobby") ;
		item->PeerId(entry.virtual_peer_id) ;
		item->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_LOBBY ;

		locked_storeIncomingMsg(item) ;
	}
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "  Notifying of new recvd msg." << std::endl ;
#endif

	RsServer::notify()->notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);
	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD);

	// send AKN item
	sendLobbyStatusNewPeer(lobby_id) ;

	return true ;
}

ChatLobbyVirtualPeerId DistributedChatService::makeVirtualPeerId(ChatLobbyId lobby_id)
{
	uint8_t bytes[RsPeerId::SIZE_IN_BYTES] ;
	memset(bytes,0,RsPeerId::SIZE_IN_BYTES) ;
	memcpy(bytes,&lobby_id,sizeof(lobby_id)) ;

	return ChatLobbyVirtualPeerId(bytes) ;
}


void DistributedChatService::denyLobbyInvite(const ChatLobbyId& lobby_id) 
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CHAT_LOBBIES
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

bool DistributedChatService::joinVisibleChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& gxs_id)
{
	RsIdentityDetails det ;
	if( (!rsIdentity->getIdDetails(gxs_id,det)) || !(det.mFlags & RS_IDENTITY_FLAGS_IS_OWN_ID))
	{
		std::cerr << " (EE) Cannot subscribe with identity " << gxs_id << " because it is not ours! Something's wrong here." << std::endl;
		return false ;
	}

#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Joining public chat lobby " << std::hex << lobby_id << std::dec << std::endl;
#endif
	std::list<RsPeerId> invited_friends ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		// create a unique id.
		//
		std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.find(lobby_id)) ;

		if(it == _visible_lobbies.end())
		{
			std::cerr << "  lobby is not a known public chat lobby. Sorry!" << std::endl;
			return false ;
		}

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  lobby found. Initiating join sequence..." << std::endl;
#endif

		if(_chat_lobbys.find(lobby_id) != _chat_lobbys.end())
		{
#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "  lobby already in participating list. Returning!" << std::endl;
#endif
			return true ;
		}

		if( (it->second.lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED ) && !(det.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
		{
			std::cerr << " (EE) Cannot subscribe with identity " << gxs_id << " because it is unsigned and the lobby requires signed ids only." << std::endl;
			return false ;
		}

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  Creating new lobby entry." << std::endl;
#endif
		rstime_t now = time(NULL) ;

        ChatLobbyEntry entry ;

        entry.lobby_flags = it->second.lobby_flags ;//RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC ;
		entry.participating_friends.clear() ;
        entry.gxs_id = gxs_id ;
		entry.lobby_id = lobby_id ;
		entry.lobby_name = it->second.lobby_name ;
		entry.lobby_topic = it->second.lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ; 
		entry.last_connexion_challenge_time = now ; 
		entry.last_keep_alive_packet_time = now ;

		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		{
			invited_friends.push_back(*it2) ;
			entry.participating_friends.insert(*it2) ;
		}
		_chat_lobbys[lobby_id] = entry ;
	}
    setLobbyAutoSubscribe(lobby_id,true);

    triggerConfigSave();	// so that we save the subscribed lobbies

	for(std::list<RsPeerId>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	sendLobbyStatusNewPeer(lobby_id) ;

	return true ;
}

ChatLobbyId DistributedChatService::createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic,const std::set<RsPeerId>& invited_friends,ChatLobbyFlags lobby_flags)
{
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Creating a new Chat lobby !!" << std::endl;
#endif
	ChatLobbyId lobby_id ;
	{
		if (!rsIdentity->isOwnId(lobby_identity))
		{
			RsErr() << __PRETTY_FUNCTION__ << " lobby_identity RsGxsId id must be own" << std::endl;
			return 0;
		}

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		// create a unique id.
		//
		do { lobby_id = RSRandom::random_u64() ; } while(_chat_lobbys.find(lobby_id) != _chat_lobbys.end()) ;

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "  New (unique) ID: " << std::hex << lobby_id << std::dec << std::endl;
#endif
		rstime_t now = time(NULL) ;

		ChatLobbyEntry entry ;
        entry.lobby_flags = lobby_flags ;
		entry.participating_friends.clear() ;
        entry.gxs_id = lobby_identity ;	// to be changed. For debug only!!
		entry.lobby_id = lobby_id ;
		entry.lobby_name = lobby_name ;
		entry.lobby_topic = lobby_topic ;
		entry.virtual_peer_id = makeVirtualPeerId(lobby_id) ;
		entry.connexion_challenge_count = 0 ;
		entry.last_activity = now ;
		entry.last_connexion_challenge_time = now ;
		entry.last_keep_alive_packet_time = now ;

		_chat_lobbys[lobby_id] = entry ;
	}

    for(std::set<RsPeerId>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;

    triggerConfigSave();

	return lobby_id ;
}

void DistributedChatService::handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem *item)
{
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

#ifdef DEBUG_CHAT_LOBBIES
		std::cerr << "Received unsubscribed to lobby " << std::hex << item->lobby_id << std::dec << ", from friend " << item->PeerId() << std::endl;
#endif

		if(it == _chat_lobbys.end())
		{
			std::cerr << "Chat lobby " << std::hex << item->lobby_id << std::dec << " does not exist ! Can't unsubscribe friend!" << std::endl;
			return ;
		}

		for(std::set<RsPeerId>::iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			if(*it2 == item->PeerId())
			{
#ifdef DEBUG_CHAT_LOBBIES
				std::cerr << "  removing peer id " << item->PeerId() << " from participant list of lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
				it->second.previously_known_peers.insert(*it2) ;
				it->second.participating_friends.erase(it2) ;
				break ;
			}
	}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_MOD) ;
}

void DistributedChatService::unsubscribeChatLobby(const ChatLobbyId& id)
{
	// send AKN item
	sendLobbyStatusPeerLeaving(id) ;
	setLobbyAutoSubscribe(id, false);

	{
		RS_STACK_MUTEX(mDistributedChatMtx);

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(id) ;

		if(it == _chat_lobbys.end())
		{
			std::cerr << "Chat lobby " << id << " does not exist ! Can't unsubscribe!" << std::endl;
			return ;
		}

		// send a lobby leaving packet to all friends

		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		{
			RsChatLobbyUnsubscribeItem *item = new RsChatLobbyUnsubscribeItem ;

			item->lobby_id = id ;
			item->PeerId(*it2) ;

#ifdef DEBUG_CHAT_LOBBIES
			std::cerr << "Sending unsubscribe item to friend " << *it2 << std::endl;
#endif

			sendChatItem(item) ;
		}

		// remove history

		//mHistoryMgr->clear(it->second.virtual_peer_id);

		// remove lobby information

		_chat_lobbys.erase(it) ;
	}

    triggerConfigSave();	// so that we save the subscribed lobbies
	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_DEL) ;

	// done!
}
bool DistributedChatService::setDefaultIdentityForChatLobby(const RsGxsId& nick)
{
    if (nick.isNull())
	{
		std::cerr << "Ignore empty nickname for chat lobby " << std::endl;
		return false;
	}

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
        _default_identity = nick;
	}

	triggerConfigSave() ;
	return true ;
}
void DistributedChatService::getDefaultIdentityForChatLobby(RsGxsId& nick)
{
    RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

    nick = locked_getDefaultIdentity() ;
}

RsGxsId DistributedChatService::locked_getDefaultIdentity()
{
    if(_default_identity.isNull() && rsIdentity!=NULL)
    {
        std::list<RsGxsId> own_ids ;
        rsIdentity->getOwnIds(own_ids) ;

        if(!own_ids.empty())
        {
            _default_identity = own_ids.front() ;
            triggerConfigSave();
        }
    }
    return _default_identity ;
}
bool DistributedChatService::getIdentityForChatLobby(const ChatLobbyId& lobby_id,RsGxsId& nick)
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
	
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "getting nickname for chat lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif
	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(lobby_id) ;

	if(it == _chat_lobbys.end())
	{
		std::cerr << " (EE) lobby does not exist!!" << std::endl;
		return false ;
	}

    nick = it->second.gxs_id ;
	return true ;
}

bool DistributedChatService::setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& nick)
{
    if (nick.isNull())
    {
        std::cerr << "(EE) Ignore empty nickname for chat lobby " << nick << std::endl;
        return false;
    }

    // first check for change and send status peer changed nick name
    bool changed = false;
    std::map<ChatLobbyId,ChatLobbyEntry>::iterator it;

    {
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CHAT_LOBBIES
        std::cerr << "Changing nickname for chat lobby " << std::hex << lobby_id << std::dec << " to " << nick << std::endl;
#endif
        it = _chat_lobbys.find(lobby_id) ;

        if(it == _chat_lobbys.end())
        {
            std::cerr << " (EE) lobby does not exist!!" << std::endl;
            return false;
        }

        if (!it->second.gxs_id.isNull() && it->second.gxs_id != nick)
            changed = true;
    }

    if (changed)
    {
        // Inform other peers of change the Nickname
        {
            RsIdentityDetails det1,det2 ;

            // Only send a nickname change event if the two Identities are not anonymous

            if(rsIdentity->getIdDetails(nick,det1) && rsIdentity->getIdDetails(it->second.gxs_id,det2) && (det1.mFlags & det2.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
                sendLobbyStatusPeerChangedNickname(lobby_id, nick.toStdString()) ;
        }

        // set new nick name
        RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

        it = _chat_lobbys.find(lobby_id) ;

        if(it == _chat_lobbys.end())
        {
            std::cerr << " (EE) lobby does not exist!!" << std::endl;
            return false;
        }

        it->second.gxs_id = nick ;
    }

    return true ;
}

void DistributedChatService::setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe)
{

		if(autoSubscribe)
        {
            {
				RS_STACK_MUTEX(mDistributedChatMtx);
				_known_lobbies_flags[lobby_id] |=  RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE;
			}
			RsGxsId gxsId;

			if (getIdentityForChatLobby(lobby_id, gxsId))
			{
				RS_STACK_MUTEX(mDistributedChatMtx);
				_lobby_default_identity[lobby_id] = gxsId;
			}
		}
        else
        {
			RS_STACK_MUTEX(mDistributedChatMtx);
			_known_lobbies_flags[lobby_id] &= ~RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE ;
			_lobby_default_identity.erase(lobby_id);
		}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	triggerConfigSave();
}

bool DistributedChatService::getLobbyAutoSubscribe(const ChatLobbyId& lobby_id)
{
    if(_known_lobbies_flags.find(lobby_id) == _known_lobbies_flags.end()) 	// keep safe about default values form std::map
		 return false;

    return _known_lobbies_flags[lobby_id] & RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE ;
}

void DistributedChatService::cleanLobbyCaches()
{
#ifdef DEBUG_CHAT_LOBBIES
	std::cerr << "Cleaning chat lobby caches." << std::endl;
#endif

	std::list<ChatLobbyId> keep_alive_lobby_ids ;
	std::list<ChatLobbyId> changed_lobbies ;
	std::list<ChatLobbyId> send_challenge_lobbies ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		rstime_t now = time(NULL) ;

		// 1 - clean cache of all lobbies and participating nicknames.
		//
		for(std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.begin();it!=_chat_lobbys.end();++it)
		{
			for(std::map<ChatLobbyMsgId,rstime_t>::iterator it2(it->second.msg_cache.begin());it2!=it->second.msg_cache.end();)
				if(it2->second + MAX_KEEP_MSG_RECORD < now)
				{
#ifdef DEBUG_CHAT_LOBBIES
					std::cerr << "  removing old msg 0x" << std::hex << it2->first << ", time=" << std::dec << now - it2->second << " secs ago" << std::endl;
#endif

					std::map<ChatLobbyMsgId,rstime_t>::iterator tmp(it2) ;
					++tmp ;
					it->second.msg_cache.erase(it2) ;
					it2 = tmp ;
				}
				else
					++it2 ;

			bool changed = false ;

            for(std::map<RsGxsId,rstime_t>::iterator it2(it->second.gxs_ids.begin());it2!=it->second.gxs_ids.end();)
				if(it2->second + MAX_KEEP_INACTIVE_NICKNAME < now)
				{
#ifdef DEBUG_CHAT_LOBBIES
					std::cerr << "  removing inactive nickname 0x" << std::hex << it2->first << ", time=" << std::dec << now - it2->second << " secs ago" << std::endl;
#endif

                    std::map<RsGxsId,rstime_t>::iterator tmp(it2) ;
					++tmp ;
                    it->second.gxs_ids.erase(it2) ;
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

			// 4 - look at lobby activity and possibly send connection challenge
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
			if(it->second.last_report_time + MAX_KEEP_PUBLIC_LOBBY_RECORD < now && _chat_lobbys.find(it->first)==_chat_lobbys.end())	// this lobby record is too late.
			{
#ifdef DEBUG_CHAT_LOBBIES
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
        RsServer::notify()->notifyChatLobbyEvent(*it,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,RsGxsId(),"") ;

	// send connection challenges
	//
	for(std::list<ChatLobbyId>::const_iterator it(send_challenge_lobbies.begin());it!=send_challenge_lobbies.end();++it)
		sendConnectionChallenge(*it) ;
}

void DistributedChatService::addToSaveList(std::list<RsItem*>& list) const
{
	/* Save Lobby Auto Subscribe */
	for(std::map<ChatLobbyId,ChatLobbyFlags>::const_iterator it=_known_lobbies_flags.begin(); it!=_known_lobbies_flags.end(); ++it)
	{
		RsChatLobbyConfigItem *clci = new RsChatLobbyConfigItem ;
		clci->lobby_Id=it->first;
		clci->flags=it->second.toUInt32();

		list.push_back(clci) ;
	}

    for(auto it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
    {
        RsSubscribedChatLobbyConfigItem *scli = new RsSubscribedChatLobbyConfigItem;

        scli->info = it->second;	// copies the ChatLobbyInfo part only

        list.push_back(scli);
    }

	/* Save Default Nick Name */
	{
		RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
		RsTlvKeyValue kv;
		kv.key = "DEFAULT_IDENTITY";
		kv.value = _default_identity.toStdString();
		vitem->tlvkvs.pairs.push_back(kv);
		list.push_back(vitem);
	}

	/* Save Default Nick Name by Lobby*/
	for(std::map<ChatLobbyId,RsGxsId>::const_iterator it=_lobby_default_identity.begin(); it!=_lobby_default_identity.end(); ++it)
	{
		RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
		ChatLobbyId cli = it->first;
		RsGxsId gxsId = it->second;

		std::stringstream stream;
		stream << std::setfill ('0') << std::setw(sizeof(ChatLobbyId)*2)
		       << std::hex << cli;
		std::string strCli( stream.str() );

		RsTlvKeyValue kv;
		kv.key = "LOBBY_DEFAULT_IDENTITY:"+strCli;
		kv.value = gxsId.toStdString();
		vitem->tlvkvs.pairs.push_back(kv);
		list.push_back(vitem);
	}

}

bool DistributedChatService::processLoadListItem(const RsItem *item)
{
	const RsConfigKeyValueSet *vitem = NULL;
	const std::string strldID = "LOBBY_DEFAULT_IDENTITY:";

	if(NULL != (vitem = dynamic_cast<const RsConfigKeyValueSet*>(item)))
		for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit)
		{
			if( kit->key == "DEFAULT_IDENTITY" )
			{
#ifdef DEBUG_CHAT_LOBBIES
				std::cerr << "Loaded config default nick name for distributed chat: " << kit->value << std::endl ;
#endif
				if (!kit->value.empty())
				{
					_default_identity = RsGxsId(kit->value) ;
					if(_default_identity.isNull())
						std::cerr << "ERROR: default identity is malformed." << std::endl;
				}

				return true;
			}

			if( kit->key.compare(0, strldID.length(), strldID) == 0)
			{
#ifdef DEBUG_CHAT_LOBBIES
				std::cerr << "Loaded config lobby default nick name: " << kit->key << " " << kit->value << std::endl ;
#endif

				std::string strCli = kit->key.substr(strldID.length());
				std::stringstream stream;
				stream << std::hex << strCli;
				ChatLobbyId cli = 0;
				stream >> cli;

				if (!kit->value.empty() && (cli != 0))
				{
					RsGxsId gxsId(kit->value);
					if (gxsId.isNull())
						std::cerr << "ERROR: lobby default identity is malformed." << std::endl;
					else
						_lobby_default_identity[cli] = gxsId ;
				}

				return true;
			}
		}

	const RsChatLobbyConfigItem *clci = NULL ;

	if(NULL != (clci = dynamic_cast<const RsChatLobbyConfigItem*>(item)))
	{
		_known_lobbies_flags[clci->lobby_Id] = ChatLobbyFlags(clci->flags) ;
		return true ;
	}

    if(_default_identity.isNull() && rsIdentity!=NULL)
    {
        std::list<RsGxsId> own_ids ;
        rsIdentity->getOwnIds(own_ids) ;

        if(!own_ids.empty())
            _default_identity = own_ids.front() ;
    }

	const RsSubscribedChatLobbyConfigItem *scli = dynamic_cast<const RsSubscribedChatLobbyConfigItem*>(item);

    if(scli != NULL)
    {
        if(_chat_lobbys.find(scli->info.lobby_id) != _chat_lobbys.end())	// do nothing if the lobby is already subscribed
            return true;

        std::cerr << "Re-subscribing to chat lobby " << (void*)scli->info.lobby_id << ", flags = " << scli->info.lobby_flags << std::endl;

        rstime_t now = time(NULL);

        // Add the chat room into visible chat rooms
		{
			RS_STACK_MUTEX(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

			VisibleChatLobbyRecord& rec(_visible_lobbies[scli->info.lobby_id]) ;

			rec.lobby_id = scli->info.lobby_id ;
			rec.lobby_name = scli->info.lobby_name ;
			rec.lobby_topic = scli->info.lobby_topic ;
			rec.participating_friends = scli->info.participating_friends;
			rec.total_number_of_peers = 0;
			rec.last_report_time = now ;
			rec.lobby_flags = EXTRACT_PRIVACY_FLAGS(scli->info.lobby_flags) ;

			_known_lobbies_flags[scli->info.lobby_id] |=  RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE;
        }

        // Add the chat room into subscribed chat rooms

		ChatLobbyEntry entry ;
        (ChatLobbyInfo&)entry = scli->info;

		 entry.virtual_peer_id = makeVirtualPeerId(entry.lobby_id) ;	// not random, so we keep the same id at restart
		 entry.connexion_challenge_count = 0 ;
		 entry.last_activity = now ;
		 entry.last_connexion_challenge_time = now ;
		 entry.last_keep_alive_packet_time = now ;

		 {
			 RS_STACK_MUTEX(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
			 _chat_lobbys[entry.lobby_id] = entry ;
		 }

         // make the UI aware of the existing chat room

		 RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;

		 return true;
    }


	return false ;
}


