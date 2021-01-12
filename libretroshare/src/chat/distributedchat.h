/*******************************************************************************
 * libretroshare/src/chat: distributedchat.h                                   *
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
#pragma once

#include <retroshare/rstypes.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsservicecontrol.h>

typedef RsPeerId ChatLobbyVirtualPeerId ;

struct RsItem;
class p3HistoryMgr ;
class p3IdService ;
class p3ServiceControl;
class RsChatLobbyItem ;
class RsChatLobbyListRequestItem ;
class RsChatLobbyListItem ;
class RsChatLobbyEventItem ;
class RsChatLobbyBouncingObject ;
class RsChatLobbyInviteItem_Deprecated ; // to be removed (deprecated since May 2017)
class RsChatLobbyInviteItem ;
class RsChatLobbyMsgItem ;
class RsChatLobbyConnectChallengeItem ;
class RsChatLobbyUnsubscribeItem ;

class RsChatItem ;
class RsChatMsgItem ;
class RsGixs ;

class DistributedChatService
{
	public:
		DistributedChatService(uint32_t service_type,p3ServiceControl *sc,p3HistoryMgr *hm,RsGixs *is) ;

		virtual ~DistributedChatService() {}

		void flush() ;

		// Interface part to communicate with
		//
		bool getVirtualPeerId(const ChatLobbyId& lobby_id, RsPeerId& virtual_peer_id) ;
		void getChatLobbyList(std::list<ChatLobbyId>& clids) ;
		bool getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& clinfo) ;
		bool acceptLobbyInvite(const ChatLobbyId& id,const RsGxsId& identity) ;
		void denyLobbyInvite(const ChatLobbyId& id) ;
		void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) ;
		void invitePeerToLobby(const ChatLobbyId&, const RsPeerId& peer_id,bool connexion_challenge = false) ;
		void unsubscribeChatLobby(const ChatLobbyId& lobby_id) ;
		bool setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& nick) ;
		bool getIdentityForChatLobby(const ChatLobbyId& lobby_id,RsGxsId& nick) ;
		bool setDefaultIdentityForChatLobby(const RsGxsId& nick) ;
		void getDefaultIdentityForChatLobby(RsGxsId& nick) ;
		void setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe);
		bool getLobbyAutoSubscribe(const ChatLobbyId& lobby_id);
		void sendLobbyStatusString(const ChatLobbyId& id,const std::string& status_string) ;
		void sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id) ;

		ChatLobbyId createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic, const std::set<RsPeerId>& invited_friends,ChatLobbyFlags flags) ;

		void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) ;
		bool joinVisibleChatLobby(const ChatLobbyId& id, const RsGxsId &gxs_id) ;

	protected:
		bool handleRecvItem(RsChatItem *) ;

		virtual void sendChatItem(RsChatItem *) =0 ;
		virtual void locked_storeIncomingMsg(RsChatMsgItem *) =0 ;
		virtual void triggerConfigSave() =0;

		void addToSaveList(std::list<RsItem*>& list) const ;
		bool processLoadListItem(const RsItem *item) ;

		void checkSizeAndSendLobbyMessage(RsChatItem *) ;

		bool sendLobbyChat(const ChatLobbyId &lobby_id, const std::string&) ;
		bool handleRecvChatLobbyMsgItem(RsChatMsgItem *item) ;

		bool checkSignature(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id) ;

	private:
		/// make some statistics about time shifts, to prevent various issues. 
		void addTimeShiftStatistics(int shift) ;

		void handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *item) ;
		void handleRecvChatLobbyList(RsChatLobbyListItem *item) ;
		void handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item) ;


		/// Checks that the lobby object is not flooding a lobby.
		bool locked_bouncingObjectCheck(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id,uint32_t lobby_count) ;

		/// receive and handle chat lobby item
		bool recvLobbyChat(RsChatLobbyMsgItem*,const RsPeerId& src_peer_id) ;
		void handleRecvLobbyInvite_Deprecated(RsChatLobbyInviteItem_Deprecated*) ; // to be removed (deprecated since May 2017)
		void handleRecvLobbyInvite(RsChatLobbyInviteItem*) ;
		void checkAndRedirectMsgToLobby(RsChatMsgItem*) ;
		void handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) ;
		void sendConnectionChallenge(ChatLobbyId id) ;
		void handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem*) ;
		void cleanLobbyCaches() ;
		bool bounceLobbyObject(RsChatLobbyBouncingObject *obj, const RsPeerId& peer_id) ;

		void sendLobbyStatusItem(const ChatLobbyId&, int type, const std::string& status_string) ;
		void sendLobbyStatusPeerChangedNickname(const ChatLobbyId& lobby_id, const std::string& newnick) ;

		void sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id) ;
		void sendLobbyStatusKeepAlive(const ChatLobbyId&) ;

		bool locked_initLobbyBouncableObject(const ChatLobbyId& id,RsChatLobbyBouncingObject&) ;
		void locked_printDebugInfo() const ;
		RsGxsId locked_getDefaultIdentity();

		static ChatLobbyVirtualPeerId makeVirtualPeerId(ChatLobbyId) ;
		static uint64_t makeConnexionChallengeCode(const RsPeerId& peer_id,ChatLobbyId lobby_id,ChatLobbyMsgId msg_id) ;

		class ChatLobbyEntry: public ChatLobbyInfo
		{
			public:
				std::map<ChatLobbyMsgId,rstime_t> msg_cache ;
				RsPeerId virtual_peer_id ;
				int connexion_challenge_count ;
				rstime_t last_connexion_challenge_time ;
				bool joined_lobby_packet_sent;
				rstime_t last_keep_alive_packet_time ;
				std::set<RsPeerId> previously_known_peers ;
		};

		std::map<ChatLobbyId,ChatLobbyEntry> _chat_lobbys ;
		std::map<ChatLobbyId,ChatLobbyInvite> _lobby_invites_queue ;
		std::map<ChatLobbyId,VisibleChatLobbyRecord> _visible_lobbies ;
		std::map<ChatLobbyId,ChatLobbyFlags> _known_lobbies_flags ;	// flags for all lobbies, including the ones that are not known. So we can't
		std::map<ChatLobbyId,std::vector<RsChatLobbyMsgItem*> > _pendingPartialLobbyMessages ;	// store them in _chat_lobbies (subscribed lobbies) nor _visible_lobbies.
													// Known flags:
													// 		RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE

		float _time_shift_average ;
		rstime_t last_lobby_challenge_time ; 					// prevents bruteforce attack
		rstime_t last_visible_lobby_info_request_time ;	// allows to ask for updates
		bool _should_reset_lobby_counts ;
		RsGxsId _default_identity;
		std::map<ChatLobbyId,RsGxsId> _lobby_default_identity;

		uint32_t mServType ;
		RsMutex mDistributedChatMtx ;

		p3ServiceControl *mServControl; 
		p3HistoryMgr *mHistMgr;
		RsGixs *mGixs ;
};
