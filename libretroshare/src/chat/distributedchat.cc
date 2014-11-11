/*
 * libretroshare/src/chat: distributedchat.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2014 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#include <sstream>
#include <math.h>

#include "distributedchat.h"

#include "pqi/p3historymgr.h"
#include "serialiser/rsmsgitems.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsiface.h"
#include "rsserver/p3face.h"

static const int 		CONNECTION_CHALLENGE_MAX_COUNT 	  =   20 ; // sends a connection challenge every 20 messages
static const time_t	CONNECTION_CHALLENGE_MAX_MSG_AGE	  =   30 ; // maximum age of a message to be used in a connection challenge
static const int 		CONNECTION_CHALLENGE_MIN_DELAY 	  =   15 ; // sends a connection at most every 15 seconds
static const int 		LOBBY_CACHE_CLEANING_PERIOD    	  =   10 ; // clean lobby caches every 10 secs (remove old messages)

static const time_t 	MAX_KEEP_MSG_RECORD 					  = 1200 ; // keep msg record for 1200 secs max.
static const time_t 	MAX_KEEP_INACTIVE_NICKNAME         =  180 ; // keep inactive nicknames for 3 mn max.
static const time_t  MAX_DELAY_BETWEEN_LOBBY_KEEP_ALIVE =  120 ; // send keep alive packet every 2 minutes.
static const time_t 	MAX_KEEP_PUBLIC_LOBBY_RECORD       =   60 ; // keep inactive lobbies records for 60 secs max.
static const time_t 	MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ =   20 ; // don't ask for lobby list more than once every 30 secs.
static const time_t 	LOBBY_LIST_AUTO_UPDATE_TIME        =  121 ; // regularly ask for available lobbies every 5 minutes, to allow auto-subscribe to work

static const uint32_t MAX_ALLOWED_LOBBIES_IN_LIST_WARNING = 50 ;
static const uint32_t MAX_MESSAGES_PER_SECONDS_NUMBER     =  5 ; // max number of messages from a given peer in a window for duration below
static const uint32_t MAX_MESSAGES_PER_SECONDS_PERIOD     = 10 ; // duration window for max number of messages before messages get dropped.

DistributedChatService::DistributedChatService(uint32_t serv_type,p3ServiceControl *sc,p3HistoryMgr *hm)
	: mServType(serv_type),mDistributedChatMtx("Distributed Chat"), mServControl(sc), mHistMgr(hm)
{
	_time_shift_average = 0.0f ;
	_default_nick_name = rsPeers->getPeerName(rsPeers->getOwnId());
	_should_reset_lobby_counts = false ;
	
	last_visible_lobby_info_request_time = 0 ;
}

void DistributedChatService::flush()
{
	static time_t last_clean_time_lobby = 0 ;
	static time_t last_req_chat_lobby_list = 0 ;

	time_t now = time(NULL) ;

	if(last_clean_time_lobby + LOBBY_CACHE_CLEANING_PERIOD < now)
	{
		cleanLobbyCaches() ;
		last_clean_time_lobby = now ;
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

    time_t now = time(NULL) ;

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
    RsPeerId virtual_peer_id ;
    getVirtualPeerId(cli->lobby_id,virtual_peer_id) ;
    cli->PeerId(virtual_peer_id) ;
    //name = cli->nick;
    //popupChatFlag = RS_POPUP_CHATLOBBY;

    RsServer::notify()->AddPopupMessage(RS_POPUP_CHATLOBBY, ci->PeerId().toStdString(), cli->nick, cli->message); /* notify private chat message */

    return true ;
}

bool DistributedChatService::getVirtualPeerId(const ChatLobbyId& id,ChatLobbyVirtualPeerId& vpid) 
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

void DistributedChatService::locked_printDebugInfo() const
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

		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
			std::cerr << "       " << *it2 << std::endl;

		std::cerr << "   Participating nick names: " << std::endl;

		for(std::map<std::string,time_t>::const_iterator it2(it->second.nick_names.begin());it2!=it->second.nick_names.end();++it2)
			std::cerr << "       " << it2->first << ": " << now - it2->second << " secs ago" << std::endl;

	}

	std::cerr << "Recorded lobby names: " << std::endl;

	for( std::map<RsPeerId,ChatLobbyId>::const_iterator it(_lobby_ids.begin()) ;it!=_lobby_ids.end();++it)
		std::cerr << "   \"" << it->first << "\" id = " << std::hex << it->second << std::dec << std::endl;

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

bool DistributedChatService::isLobbyId(const RsPeerId& virtual_peer_id,ChatLobbyId& lobby_id)
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

	std::map<ChatLobbyVirtualPeerId,ChatLobbyId>::const_iterator it(_lobby_ids.find(virtual_peer_id)) ;

	if(it != _lobby_ids.end())
	{
		lobby_id = it->second ;
		return true ;
	}

	lobby_id = 0;
	return false ;
}
bool DistributedChatService::locked_bouncingObjectCheck(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id,uint32_t lobby_count)
{
	static std::map<std::string, std::list<time_t> > message_counts ;

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
			lobby_count = it->second.nick_names.size() ;
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

#ifdef CHAT_DEBUG
	std::cerr << "lobby_count=" << lobby_count << std::endl;
	std::cerr << "Got msg for peer " << pid << std::dec << ". Limit is " << max_cnt << ". List is " ;
	for(std::list<time_t>::const_iterator it(message_counts[pid].begin());it!=message_counts[pid].end();++it)
		std::cerr << *it << " " ;
	std::cerr << std::endl;
#endif

	time_t now = time(NULL) ;

	std::list<time_t>& lst = message_counts[pid] ;
	
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
void DistributedChatService::checkSizeAndSendLobbyMessage(RsChatLobbyMsgItem *msg)
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

        sendChatItem(item) ;
    }
    msg->subpacket_id = n ;
    sendChatItem(msg) ;
}

bool DistributedChatService::locked_checkAndRebuildPartialLobbyMessage(RsChatLobbyMsgItem *ci)
{
    if(ci == NULL)		// null item, should be treated normally.
        return true ;

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
        std::cerr << "  Pending message found. Appending it." << std::endl;
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
            std::string msg ;
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
            ci->chatFlags &= ~RS_CHAT_FLAG_PARTIAL_MESSAGE ;	// remove partial flag form message.

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

bool DistributedChatService::handleRecvItem(RsChatItem *item)
{
	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE:          handleRecvLobbyInvite           (dynamic_cast<RsChatLobbyInviteItem           *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE:       handleConnectionChallenge       (dynamic_cast<RsChatLobbyConnectChallengeItem *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT:           handleRecvChatLobbyEventItem    (dynamic_cast<RsChatLobbyEventItem            *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE:     handleFriendUnsubscribeLobby    (dynamic_cast<RsChatLobbyUnsubscribeItem      *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST:    handleRecvChatLobbyListRequest  (dynamic_cast<RsChatLobbyListRequestItem      *>(item)) ; break ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST:            handleRecvChatLobbyList         (dynamic_cast<RsChatLobbyListItem             *>(item)) ; break ;
		default:
																		return false ;
	}
	return true ;
}

void DistributedChatService::handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *clr)
{
	// make a lobby list item
	//
	RsChatLobbyListItem *item = new RsChatLobbyListItem;

#ifdef CHAT_DEBUG
	std::cerr << "Peer " << clr->PeerId() << " requested the list of public chat lobbies." << std::endl;
#endif

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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
	sendChatItem(item);
}

void DistributedChatService::handleRecvChatLobbyList(RsChatLobbyListItem *item)
{
	if(item->lobby_ids.size() > MAX_ALLOWED_LOBBIES_IN_LIST_WARNING)
		std::cerr << "Warning: Peer " << item->PeerId() << "(" << rsPeers->getPeerName(item->PeerId()) << ") is sending a lobby list of " << item->lobby_ids.size() << " lobbies. This is unusual, and probably a attempt to crash you." << std::endl;

    std::list<ChatLobbyId> chatLobbyToSubscribe;
	 std::list<ChatLobbyId> invitationNeeded ;

#ifdef CHAT_DEBUG
	 std::cerr << "Received chat lobby list from friend " << item->PeerId() << ", " << item->lobby_ids.size() << " elements." << std::endl;
#endif
	{
		time_t now = time(NULL) ;

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		for(uint32_t i=0;i<item->lobby_ids.size() && i < MAX_ALLOWED_LOBBIES_IN_LIST_WARNING;++i)
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

			std::map<ChatLobbyId,ChatLobbyFlags>::const_iterator it(_known_lobbies_flags.find(item->lobby_ids[i])) ;

#ifdef CHAT_DEBUG
			std::cerr << "  lobby id " << std::hex << item->lobby_ids[i] << std::dec << ", " << item->lobby_names[i] << ", " << item->lobby_counts[i] << " participants" << std::endl;
#endif
			if(it != _known_lobbies_flags.end() && (it->second & RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE))
			{
#ifdef CHAT_DEBUG
				std::cerr << "    lobby is flagged as autosubscribed. Adding it to subscribe list." << std::endl;
#endif
				ChatLobbyId clid = item->lobby_ids[i];
				chatLobbyToSubscribe.push_back(clid);
			}

			// for subscribed lobbies, check that item->PeerId() is among the participating friends. If not, add him!

			std::map<ChatLobbyId,ChatLobbyEntry>::iterator it2 = _chat_lobbys.find(item->lobby_ids[i]) ;

			if(it2 != _chat_lobbys.end() && it2->second.participating_friends.find(item->PeerId()) == it2->second.participating_friends.end())
			{
#ifdef CHAT_DEBUG
				std::cerr << "    lobby is currently subscribed but friend is not participating already -> adding to partipating friends and sending invite." << std::endl;
#endif
				it2->second.participating_friends.insert(item->PeerId()) ; 
				invitationNeeded.push_back(item->lobby_ids[i]) ;
			}
		}
	}

    std::list<ChatLobbyId>::iterator it;
    for (it = chatLobbyToSubscribe.begin(); it != chatLobbyToSubscribe.end(); ++it)
        joinVisibleChatLobby(*it);

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
			RsServer::notify()->notifyChatLobbyTimeShift( (int)pow(2.0f,expected)) ;

		total = 0.0f ;
		log_delay_histogram.clear() ;
		log_delay_histogram.resize(S,0) ;
	}
#ifdef CHAT_DEBUG
	else
		std::cerr << std::endl;
#endif
}

void DistributedChatService::handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item)
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

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

		if(it != _chat_lobbys.end())
		{
			it->second.nick_names[item->nick] = time(NULL) ;
#ifdef CHAT_DEBUG
			std::cerr << "  added nickname " << item->nick << " from lobby " << std::hex << item->lobby_id << std::dec << std::endl;
#endif
		}
	}
	RsServer::notify()->notifyChatLobbyEvent(item->lobby_id,item->event_type,item->nick,item->string1) ;
}
void DistributedChatService::getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& visible_lobbies)
{
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		visible_lobbies.clear() ;

		for(std::map<ChatLobbyId,VisibleChatLobbyRecord>::const_iterator it(_visible_lobbies.begin());it!=_visible_lobbies.end();++it)
			visible_lobbies.push_back(it->second) ;
	}

	time_t now = time(NULL) ;

	if(now > MIN_DELAY_BETWEEN_PUBLIC_LOBBY_REQ + last_visible_lobby_info_request_time)
	{
		std::set<RsPeerId> ids ;
		mServControl->getPeersConnected(mServType, ids);

		for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
		{
#ifdef CHAT_DEBUG
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
	time_t now = time(NULL) ;
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
	locked_printDebugInfo() ; // debug

	std::cerr << "Handling ChatLobbyMsg " << std::hex << item->msg_id << ", lobby id " << item->lobby_id << ", from peer id " << peer_id << std::endl;
#endif

	// send upward for display

	std::map<ChatLobbyId,ChatLobbyEntry>::iterator it(_chat_lobbys.find(item->lobby_id)) ;

	if(it == _chat_lobbys.end())
	{
#ifdef CHAT_DEBUG
		std::cerr << "Chatlobby for id " << std::hex << item->lobby_id << " has no record. Dropping the msg." << std::dec << std::endl;
#endif
		return false ;
	}

	ChatLobbyEntry& lobby(it->second) ;

	// Adds the peer id to the list of friend participants, even if it's not original msg source

	if(peer_id != mServControl->getOwnId())
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
	std::cerr << "  Msg not received already. Adding in cache, and forwarding!" << std::endl ;
#endif

	lobby.msg_cache[item->msg_id] = now ;
	lobby.last_activity = now ;

	// Check that if we have a lobby bouncing object, it's not flooding the lobby
	if(!locked_bouncingObjectCheck(item,peer_id,lobby.participating_friends.size()))
		return false;

	bool is_message =  (NULL != dynamic_cast<RsChatLobbyMsgItem*>(item)) ;

	// Forward to allparticipating friends, except this peer.

	for(std::set<RsPeerId>::const_iterator it(lobby.participating_friends.begin());it!=lobby.participating_friends.end();++it)
		if((*it)!=peer_id && mServControl->isPeerConnected(mServType, *it)) 
		{
			RsChatLobbyBouncingObject *obj2 = item->duplicate() ; // makes a copy
			RsChatItem *item2 = dynamic_cast<RsChatItem*>(obj2) ;

			assert(item2 != NULL) ;

			item2->PeerId(*it) ;	// replaces the virtual peer id with the actual destination.

			if(is_message)
                checkSizeAndSendLobbyMessage(static_cast<RsChatLobbyMsgItem*>(item2)) ;
			else
				sendChatItem(item2);
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


void DistributedChatService::sendLobbyStatusPeerLiving(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_LEFT,nick) ; 
}

void DistributedChatService::sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_PEER_JOINED,nick) ; 
}

void DistributedChatService::sendLobbyStatusKeepAlive(const ChatLobbyId& lobby_id)
{
	std::string nick ;
	getNickNameForChatLobby(lobby_id,nick) ;

	sendLobbyStatusItem(lobby_id,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,nick) ; 
}

void DistributedChatService::sendLobbyStatusItem(const ChatLobbyId& lobby_id,int type,const std::string& status_string) 
{
	RsChatLobbyEventItem item ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

		if(! locked_initLobbyBouncableObject(lobby_id,item))
			return ;

		item.event_type = type ;
		item.string1 = status_string ;
		item.sendTime = time(NULL) ;
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
#ifdef CHAT_DEBUG
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

	item.lobby_id = lobby_id ;
	item.nick = lobby.nick_name ;

	return true ;
}

bool DistributedChatService::sendLobbyChat(const RsPeerId &id, const std::string& msg, const ChatLobbyId& lobby_id)
{
#ifdef CHAT_DEBUG
	std::cerr << "Sending chat lobby message to lobby " << std::hex << lobby_id << std::dec << std::endl;
	std::cerr << "msg:" << std::endl;
	std::cerr << msg << std::endl;
#endif

	RsChatLobbyMsgItem item ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
		// gives a random msg id, setup the nickname

		if(!  locked_initLobbyBouncableObject(lobby_id,item))
			return false;

		// chat msg stuff
		//
		item.chatFlags = RS_CHAT_FLAG_LOBBY | RS_CHAT_FLAG_PRIVATE;
		item.sendTime = time(NULL);
		item.recvTime = item.sendTime;
		item.message = msg;
	}

	RsPeerId ownId = rsPeers->getOwnId();

	mHistMgr->addMessage(false, id, ownId, &item);

	bounceLobbyObject(&item, ownId) ;

	return true ;
}

void DistributedChatService::handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) 
{
	// Look into message cache of all lobbys to handle the challenge.
	//
#ifdef CHAT_DEBUG
	std::cerr << "DistributedChatService::handleConnectionChallenge(): received connection challenge:" << std::endl;
	std::cerr << "    Challenge code = 0x" << std::hex << item->challenge_code << std::dec << std::endl;
	std::cerr << "    Peer Id        =   " << item->PeerId() << std::endl;
#endif

	time_t now = time(NULL) ;
	ChatLobbyId lobby_id ;
	const RsPeerId& ownId = rsPeers->getOwnId();

	bool found = false ;
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

void DistributedChatService::sendConnectionChallenge(ChatLobbyId lobby_id) 
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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
#ifdef CHAT_DEBUG
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

#ifdef CHAT_DEBUG
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

void DistributedChatService::getChatLobbyList(std::list<ChatLobbyInfo>& cl_infos)
{
	// fill up a dummy list for now.

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

	cl_infos.clear() ;

	for(std::map<ChatLobbyId,ChatLobbyEntry>::const_iterator it(_chat_lobbys.begin());it!=_chat_lobbys.end();++it)
		cl_infos.push_back(it->second) ;
}
void DistributedChatService::invitePeerToLobby(const ChatLobbyId& lobby_id, const RsPeerId& peer_id,bool connexion_challenge) 
{
#ifdef CHAT_DEBUG
	if(connexion_challenge)
		std::cerr << "Sending connection challenge accept to peer " << peer_id << " for lobby "<< std::hex << lobby_id << std::dec << std::endl;
	else
		std::cerr << "Sending invitation to peer " << peer_id << " to lobby "<< std::hex << lobby_id << std::dec << std::endl;
#endif

	RsChatLobbyInviteItem *item = new RsChatLobbyInviteItem ;

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

	sendChatItem(item) ;
}
void DistributedChatService::handleRecvLobbyInvite(RsChatLobbyInviteItem *item) 
{
#ifdef CHAT_DEBUG
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
	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_INVITATION, NOTIFY_TYPE_ADD);
}

void DistributedChatService::getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites)
{
	invites.clear() ;

	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

	for(std::map<ChatLobbyId,ChatLobbyInvite>::const_iterator it(_lobby_invites_queue.begin());it!=_lobby_invites_queue.end();++it)
		invites.push_back(it->second) ;
}


bool DistributedChatService::acceptLobbyInvite(const ChatLobbyId& lobby_id) 
{
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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
		item->message = std::string("Welcome to chat lobby") ;
		item->PeerId(entry.virtual_peer_id) ;
		item->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_LOBBY ;

		locked_storeIncomingMsg(item) ;
	}
#ifdef CHAT_DEBUG
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

bool DistributedChatService::joinVisibleChatLobby(const ChatLobbyId& lobby_id)
{
#ifdef CHAT_DEBUG
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

		for(std::set<RsPeerId>::const_iterator it2(it->second.participating_friends.begin());it2!=it->second.participating_friends.end();++it2)
		{
			invited_friends.push_back(*it2) ;
			entry.participating_friends.insert(*it2) ;
		}
		_chat_lobbys[lobby_id] = entry ;
	}

	for(std::list<RsPeerId>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;
	sendLobbyStatusNewPeer(lobby_id) ;

	return true ;
}

ChatLobbyId DistributedChatService::createChatLobby(const std::string& lobby_name,const std::string& lobby_topic,const std::list<RsPeerId>& invited_friends,uint32_t privacy_level)
{
#ifdef CHAT_DEBUG
	std::cerr << "Creating a new Chat lobby !!" << std::endl;
#endif
	ChatLobbyId lobby_id ;
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

	for(std::list<RsPeerId>::const_iterator it(invited_friends.begin());it!=invited_friends.end();++it)
		invitePeerToLobby(lobby_id,*it) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_ADD) ;

	return lobby_id ;
}

void DistributedChatService::handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem *item)
{
	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
		std::map<ChatLobbyId,ChatLobbyEntry>::iterator it = _chat_lobbys.find(item->lobby_id) ;

#ifdef CHAT_DEBUG
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
#ifdef CHAT_DEBUG
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
	sendLobbyStatusPeerLiving(id) ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

#ifdef CHAT_DEBUG
			std::cerr << "Sending unsubscribe item to friend " << *it2 << std::endl;
#endif

			sendChatItem(item) ;
		}

		// remove history

		//mHistoryMgr->clear(it->second.virtual_peer_id);

		// remove lobby information

		_chat_lobbys.erase(it) ;

		for(std::map<ChatLobbyVirtualPeerId,ChatLobbyId>::iterator it2(_lobby_ids.begin());it2!=_lobby_ids.end();++it2)
			if(it2->second == id)
			{
				_lobby_ids.erase(it2) ;
				break ;
			}
	}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_CHAT_LOBBY_LIST, NOTIFY_TYPE_DEL) ;

	// done!
}
bool DistributedChatService::setDefaultNickNameForChatLobby(const std::string& nick)
{
	if (nick.empty())
	{
		std::cerr << "Ignore empty nickname for chat lobby " << std::endl;
		return false;
	}

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
		_default_nick_name = nick;
	}

	triggerConfigSave() ;
	return true ;
}
bool DistributedChatService::getDefaultNickNameForChatLobby(std::string& nick)
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
	nick = _default_nick_name ;
	return true ;
}
bool DistributedChatService::getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick)
{
	RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/
	
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

bool DistributedChatService::setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick)
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
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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

void DistributedChatService::setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe)
{
	if(autoSubscribe)
		_known_lobbies_flags[lobby_id] |=  RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE ;
	else
		_known_lobbies_flags[lobby_id] &= ~RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE ;
    
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
#ifdef CHAT_DEBUG
	std::cerr << "Cleaning chat lobby caches." << std::endl;
#endif

	std::list<ChatLobbyId> keep_alive_lobby_ids ;
	std::list<ChatLobbyId> changed_lobbies ;
	std::list<ChatLobbyId> send_challenge_lobbies ;

	{
		RsStackMutex stack(mDistributedChatMtx); /********** STACK LOCKED MTX ******/

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
		RsServer::notify()->notifyChatLobbyEvent(*it,RS_CHAT_LOBBY_EVENT_KEEP_ALIVE,"------------","") ;

	// send connection challenges
	//
	for(std::list<ChatLobbyId>::const_iterator it(send_challenge_lobbies.begin());it!=send_challenge_lobbies.end();++it)
		sendConnectionChallenge(*it) ;
}

void DistributedChatService::addToSaveList(std::list<RsItem*>& list) const
{
	/* Save Lobby Auto Subscribe */
	for(std::map<ChatLobbyId,ChatLobbyFlags>::const_iterator it=_known_lobbies_flags.begin();it!=_known_lobbies_flags.end();++it)
	{
		RsChatLobbyConfigItem *clci = new RsChatLobbyConfigItem ;
		clci->lobby_Id=it->first;
		clci->flags=it->second.toUInt32();

		list.push_back(clci) ;
	}
	/* Save Default Nick Name */

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "DEFAULT_NICK_NAME" ;
	kv.value = _default_nick_name ;
	vitem->tlvkvs.pairs.push_back(kv) ;

	list.push_back(vitem) ;
}

bool DistributedChatService::processLoadListItem(const RsItem *item)
{
	const RsConfigKeyValueSet *vitem = NULL ;

	if(NULL != (vitem = dynamic_cast<const RsConfigKeyValueSet*>(item)))
		for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
			if(kit->key == "DEFAULT_NICK_NAME")
			{
#ifdef CHAT_DEBUG
				std::cerr << "Loaded config default nick name for distributed chat: " << kit->value << std::endl ;
#endif
				if (!kit->value.empty())
					_default_nick_name = kit->value ;

				return true;
			}

	const RsChatLobbyConfigItem *clci = NULL ;

	if(NULL != (clci = dynamic_cast<const RsChatLobbyConfigItem*>(item)))
	{
		_known_lobbies_flags[clci->lobby_Id] = ChatLobbyFlags(clci->flags) ;
		return true ;
	}

	return false ;
}


