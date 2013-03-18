/*
 * libretroshare/src/services chatservice.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#ifndef SERVICE_CHAT_HEADER
#define SERVICE_CHAT_HEADER

#include <list>
#include <string>
#include <vector>

#include "serialiser/rsmsgitems.h"
#include "services/p3service.h"
#include "retroshare/rsmsgs.h"

class p3LinkMgr;
class p3HistoryMgr;

//!The basic Chat service.
 /**
  *
  * Can be used to send and receive chats, immediate status (using notify), avatars, and custom status
  * This service uses rsnotify (callbacks librs clients (e.g. rs-gui))
  * @see NotifyBase
  */
class p3ChatService: public p3Service, public p3Config, public pqiMonitor
{
	public:
		p3ChatService(p3LinkMgr *cm, p3HistoryMgr *historyMgr);

		/***** overloaded from p3Service *****/
		/*!
		 * This retrieves all chat msg items and also (important!)
		 * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
		 * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
		 * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
		 * @see NotifyBase
		 */
		virtual int   tick();
		virtual int   status();

		/*************** pqiMonitor callback ***********************/
		virtual void statusChange(const std::list<pqipeer> &plist);

		/*!
		 * public chat sent to all peers
		 */
		int	sendPublicChat(const std::wstring &msg);

		/********* RsMsgs ***********/
		/*!
		 * chat is sent to specifc peer
		 * @param id peer to send chat msg to
		 */
		bool	sendPrivateChat(const std::string &id, const std::wstring &msg);

		/*!
		 * can be used to send 'immediate' status msgs, these status updates are meant for immediate use by peer (not saved by rs)
		 * e.g currently used to update user when a peer 'is typing' during a chat
		 */
		void  sendStatusString(const std::string& peer_id,const std::string& status_str) ;

		/*!
		 * send to all peers online
		 *@see sendStatusString()
		 */
		void  sendGroupChatStatusString(const std::string& status_str) ;

		/*!
		 * this retrieves custom status for a peers, generate a requests to the peer
		 * @param peer_id the id of the peer you want status string for
		 */
		std::string getCustomStateString(const std::string& peer_id) ;

		/*!
		 * sets the client's custom status, generates 'status available' item sent to all online peers
		 */
		void  setOwnCustomStateString(const std::string&) ;

		/*!
		 * @return client's custom string
		 */
		std::string getOwnCustomStateString() ;

		/*! gets the peer's avatar in jpeg format, if available. Null otherwise. Also asks the peer to send
		* its avatar, if not already available. Creates a new unsigned char array. It's the caller's
		* responsibility to delete this ones used.
		*/
		void getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) ;

		/*!
		 * Sets the avatar data and size for client's account
		 * @param data is copied, so should be destroyed by the caller
		 */
		void setOwnAvatarJpegData(const unsigned char *data,int size) ;

		/*!
		 * Gets the avatar data for clients account
		 * data is in jpeg format
		 */
		void getOwnAvatarJpegData(unsigned char *& data,int& size) ;

		/*!
		 * returns the count of messages in public queue
		 * @param public or private queue
		 */
		int getPublicChatQueueCount();

		/*!
		 * This retrieves all public chat msg items
		 */
		bool getPublicChatQueue(std::list<ChatInfo> &chats);

		/*!
		 * returns the count of messages in private queue
		 * @param public or private queue
		 */
		int getPrivateChatQueueCount(bool incoming);

		/*!
		* @param id's of available private chat messages
		*/
		bool getPrivateChatQueueIds(bool incoming, std::list<std::string> &ids);

		/*!
		 * This retrieves all private chat msg items for peer
		 */
		bool getPrivateChatQueue(bool incoming, const std::string &id, std::list<ChatInfo> &chats);

		/*!
		 * @param clear private chat queue
		 */
		bool clearPrivateChatQueue(bool incoming, const std::string &id);

		bool getVirtualPeerId(const ChatLobbyId&, std::string& virtual_peer_id) ;
		bool isLobbyId(const std::string&, ChatLobbyId&) ;
		void getChatLobbyList(std::list<ChatLobbyInfo, std::allocator<ChatLobbyInfo> >&) ;
		bool acceptLobbyInvite(const ChatLobbyId& id) ;
		void denyLobbyInvite(const ChatLobbyId& id) ;
		void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) ;
		void invitePeerToLobby(const ChatLobbyId&, const std::string& peer_id,bool connexion_challenge = false) ;
		void unsubscribeChatLobby(const ChatLobbyId& lobby_id) ;
		bool setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick) ;
		bool getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick) ;
		bool setDefaultNickNameForChatLobby(const std::string& nick) ;
		bool getDefaultNickNameForChatLobby(std::string& nick) ;
		void sendLobbyStatusString(const ChatLobbyId& id,const std::string& status_string) ;
		ChatLobbyId createChatLobby(const std::string& lobby_name,const std::string& lobby_topic, const std::list<std::string>& invited_friends,uint32_t privacy_type) ;

		void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) ;
		bool joinVisibleChatLobby(const ChatLobbyId& id) ;

	protected:
		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;

		/*!
		 * chat msg items and custom status are saved
		 */
		virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		virtual void saveDone();
		virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		RsMutex mChatMtx;

		class AvatarInfo ;
		class StateStringInfo ;

		// Receive chat queue
		void receiveChatQueue();

		void initRsChatInfo(RsChatMsgItem *c, ChatInfo &i);

		/// make some statistics about time shifts, to prevent various issues. 
		void addTimeShiftStatistics(int shift) ;

		/// Send avatar info to peer in jpeg format.
		void sendAvatarJpegData(const std::string& peer_id) ;

		/// Send custom state info to peer
		void sendCustomState(const std::string& peer_id);

		/// Receive the avatar in a chat item, with RS_CHAT_RECEIVE_AVATAR flag.
		void receiveAvatarJpegData(RsChatAvatarItem *ci) ;	// new method
		void receiveStateString(const std::string& id,const std::string& s) ;

		/// methods for handling various Chat items.
		bool handleRecvChatMsgItem(RsChatMsgItem *item) ;			// returns false if the item should be deleted.
		void handleRecvChatStatusItem(RsChatStatusItem *item) ;
		void handleRecvChatAvatarItem(RsChatAvatarItem *item) ;
		void handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *item) ;
		void handleRecvChatLobbyList(RsChatLobbyListItem *item) ;
		void handleRecvChatLobbyList(RsChatLobbyListItem_deprecated *item) ;
		void handleRecvChatLobbyList(RsChatLobbyListItem_deprecated2 *item) ;
		void handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item) ;

		/// Sends a request for an avatar to the peer of given id
		void sendAvatarRequest(const std::string& peer_id) ;

		/// Send a request for custom status string
		void sendCustomStateRequest(const std::string& peer_id);

		/// called as a proxy to sendItem(). Possibly splits item into multiple items of size lower than the maximum item size.
		void checkSizeAndSendMessage(RsChatLobbyMsgItem *item) ;
		void checkSizeAndSendMessage_deprecated(RsChatMsgItem *item) ;	// keep for compatibility for a few weeks.

		/// Called when a RsChatMsgItem is received. The item may be collapsed with any waiting partial chat item from the same peer.
		bool locked_checkAndRebuildPartialMessage(RsChatLobbyMsgItem*) ;
		bool locked_checkAndRebuildPartialMessage_deprecated(RsChatMsgItem*) ;

		/// receive and handle chat lobby item
		bool recvLobbyChat(RsChatLobbyMsgItem*,const std::string& src_peer_id) ;
		bool sendLobbyChat(const std::string &id, const std::wstring&, const ChatLobbyId&) ;
		void handleRecvLobbyInvite(RsChatLobbyInviteItem*) ;
		void checkAndRedirectMsgToLobby(RsChatMsgItem*) ;
		void handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) ;
		void sendConnectionChallenge(ChatLobbyId id) ;
		void handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem*) ;
		void cleanLobbyCaches() ;
		bool bounceLobbyObject(RsChatLobbyBouncingObject *obj, const std::string& peer_id) ;

		void sendLobbyStatusItem(const ChatLobbyId&, int type, const std::string& status_string) ;
		void sendLobbyStatusPeerLiving(const ChatLobbyId& lobby_id) ;
		void sendLobbyStatusPeerChangedNickname(const ChatLobbyId& lobby_id, const std::string& newnick) ;

		void sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id) ;
		void sendLobbyStatusKeepAlive(const ChatLobbyId&) ;

		void locked_initLobbyBouncableObject(const ChatLobbyId& id,RsChatLobbyBouncingObject&) ;

		static std::string makeVirtualPeerId(ChatLobbyId) ;
		static uint64_t makeConnexionChallengeCode(const std::string& peer_id,ChatLobbyId lobby_id,ChatLobbyMsgId msg_id) ;

		void locked_printDebugInfo() const ;
		RsChatAvatarItem *makeOwnAvatarItem() ;
		RsChatStatusItem *makeOwnCustomStateStringItem() ;

		p3LinkMgr *mLinkMgr;
		p3HistoryMgr *mHistoryMgr;

		std::list<RsChatMsgItem *> publicList;
		std::list<RsChatMsgItem *> privateIncomingList;
		std::list<RsChatMsgItem *> privateOutgoingList;

		AvatarInfo *_own_avatar ;
		std::map<std::string,AvatarInfo *> _avatars ;
		std::map<std::string,RsChatMsgItem *> _pendingPartialMessages ;	
		std::map<ChatLobbyMsgId,std::vector<RsChatLobbyMsgItem*> > _pendingPartialLobbyMessages ;	// should be used for all chat msgs after version updgrade
		std::string _custom_status_string ;
		std::map<std::string,StateStringInfo> _state_strings ;

		class ChatLobbyEntry: public ChatLobbyInfo
		{
			public:
				std::map<ChatLobbyMsgId,time_t> msg_cache ;
				std::string virtual_peer_id ;
				int connexion_challenge_count ;
				time_t last_connexion_challenge_time ;
				time_t last_keep_alive_packet_time ;
				std::set<std::string> previously_known_peers ;
		};

		std::map<ChatLobbyId,ChatLobbyEntry> _chat_lobbys ;
		std::map<ChatLobbyId,ChatLobbyInvite> _lobby_invites_queue ;
		std::map<ChatLobbyId,VisibleChatLobbyRecord> _visible_lobbies ;
		std::map<std::string,ChatLobbyId> _lobby_ids ;
		std::string _default_nick_name ;
		float _time_shift_average ;
		time_t last_lobby_challenge_time ; // prevents bruteforce attack
		time_t last_visible_lobby_info_request_time ;	// allows to ask for updates
		bool _should_reset_lobby_counts ;
};

class p3ChatService::StateStringInfo
{
   public:
	  StateStringInfo()
	  {
		  _custom_status_string = "" ;	// the custom status string of the peer
		  _peer_is_new = false ;			// true when the peer has a new avatar
		  _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
	  }

	  std::string _custom_status_string ;
	  int _peer_is_new ;			// true when the peer has a new avatar
	  int _own_is_new ;			// true when I myself a new avatar to send to this peer.
};

#endif // SERVICE_CHAT_HEADER

