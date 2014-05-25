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
#include "pqi/pqiservicemonitor.h"
#include "pgp/pgphandler.h"
#include "turtle/turtleclientservice.h"
#include "retroshare/rsmsgs.h"

class p3ServiceControl;
class p3LinkMgr;
class p3HistoryMgr;
class p3turtle ;

typedef RsPeerId ChatLobbyVirtualPeerId ;

//!The basic Chat service.
 /**
  *
  * Can be used to send and receive chats, immediate status (using notify), avatars, and custom status
  * This service uses rsnotify (callbacks librs clients (e.g. rs-gui))
  * @see NotifyBase
  */
class p3ChatService: public p3Service, public p3Config, public pqiServiceMonitor, public RsTurtleClientService
{
	public:
		p3ChatService(p3ServiceControl *cs, p3LinkMgr *cm, p3HistoryMgr *historyMgr);

		virtual RsServiceInfo getServiceInfo();

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
		virtual void statusChange(const std::list<pqiServicePeer> &plist);

		/*!
		 * public chat sent to all peers
		 */
		int	sendPublicChat(const std::string &msg);

		/********* RsMsgs ***********/
		/*!
		 * chat is sent to specifc peer
		 * @param id peer to send chat msg to
		 */
		bool	sendPrivateChat(const RsPeerId &id, const std::string &msg);

		/*!
		 * can be used to send 'immediate' status msgs, these status updates are meant for immediate use by peer (not saved by rs)
		 * e.g currently used to update user when a peer 'is typing' during a chat
		 */
		void  sendStatusString(const RsPeerId& peer_id,const std::string& status_str) ;

		/*!
		 * send to all peers online
		 *@see sendStatusString()
		 */
		void  sendGroupChatStatusString(const std::string& status_str) ;

		/*!
		 * this retrieves custom status for a peers, generate a requests to the peer
		 * @param peer_id the id of the peer you want status string for
		 */
		std::string getCustomStateString(const RsPeerId& peer_id) ;

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
		void getAvatarJpegData(const RsPeerId& peer_id,unsigned char *& data,int& size) ;

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
		bool getPrivateChatQueueIds(bool incoming, std::list<RsPeerId> &ids);

		/*!
		 * This retrieves all private chat msg items for peer
		 */
		bool getPrivateChatQueue(bool incoming, const RsPeerId &id, std::list<ChatInfo> &chats);

		/*!
		 * Checks message security, especially remove billion laughs attacks
		 */

		static bool checkForMessageSecurity(RsChatMsgItem *) ;

		/*!
		 * @param clear private chat queue
		 */
		bool clearPrivateChatQueue(bool incoming, const RsPeerId &id);

		bool getVirtualPeerId(const ChatLobbyId& lobby_id, RsPeerId& virtual_peer_id) ;
		bool isLobbyId(const RsPeerId& virtual_peer_id, ChatLobbyId& lobby_id) ;
		void getChatLobbyList(std::list<ChatLobbyInfo, std::allocator<ChatLobbyInfo> >& cl_infos) ;
		bool acceptLobbyInvite(const ChatLobbyId& id) ;
		void denyLobbyInvite(const ChatLobbyId& id) ;
		void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) ;
		void invitePeerToLobby(const ChatLobbyId&, const RsPeerId& peer_id,bool connexion_challenge = false) ;
		void unsubscribeChatLobby(const ChatLobbyId& lobby_id) ;
		bool setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick) ;
		bool getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick) ;
		bool setDefaultNickNameForChatLobby(const std::string& nick) ;
		bool getDefaultNickNameForChatLobby(std::string& nick) ;
		void setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe);
		bool getLobbyAutoSubscribe(const ChatLobbyId& lobby_id);
		void sendLobbyStatusString(const ChatLobbyId& id,const std::string& status_string) ;
		ChatLobbyId createChatLobby(const std::string& lobby_name,const std::string& lobby_topic, const std::list<RsPeerId>& invited_friends,uint32_t privacy_type) ;

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

		bool isOnline(const RsPeerId& id) ;
	private:
		RsMutex mChatMtx;

		class AvatarInfo ;
		class StateStringInfo ;

		// Receive chat queue
		void receiveChatQueue();
		void handleIncomingItem(RsItem *);	// called by the former, and turtle handler for incoming encrypted items

		void initRsChatInfo(RsChatMsgItem *c, ChatInfo &i);

		/// make some statistics about time shifts, to prevent various issues. 
		void addTimeShiftStatistics(int shift) ;

		/// Send avatar info to peer in jpeg format.
		void sendAvatarJpegData(const RsPeerId& peer_id) ;

		/// Send custom state info to peer
		void sendCustomState(const RsPeerId& peer_id);

		/// Receive the avatar in a chat item, with RS_CHAT_RECEIVE_AVATAR flag.
		void receiveAvatarJpegData(RsChatAvatarItem *ci) ;	// new method
		void receiveStateString(const RsPeerId& id,const std::string& s) ;

		/// methods for handling various Chat items.
		bool handleRecvChatMsgItem(RsChatMsgItem *item) ;			// returns false if the item should be deleted.
		void handleRecvChatStatusItem(RsChatStatusItem *item) ;
		void handleRecvChatAvatarItem(RsChatAvatarItem *item) ;
		void handleRecvChatLobbyListRequest(RsChatLobbyListRequestItem *item) ;
		void handleRecvChatLobbyList(RsChatLobbyListItem *item) ;
		void handleRecvChatLobbyEventItem(RsChatLobbyEventItem *item) ;

		/// Checks that the lobby object is not flooding a lobby.
		bool locked_bouncingObjectCheck(RsChatLobbyBouncingObject *obj,const RsPeerId& peer_id,uint32_t lobby_count) ;

		/// Sends a request for an avatar to the peer of given id
		void sendAvatarRequest(const RsPeerId& peer_id) ;

		/// Send a request for custom status string
		void sendCustomStateRequest(const RsPeerId& peer_id);

		/// called as a proxy to sendItem(). Possibly splits item into multiple items of size lower than the maximum item size.
		void checkSizeAndSendMessage(RsChatLobbyMsgItem *item) ;
		void checkSizeAndSendMessage_deprecated(RsChatMsgItem *item) ;	// keep for compatibility for a few weeks.

		/// Called when a RsChatMsgItem is received. The item may be collapsed with any waiting partial chat item from the same peer.
		bool locked_checkAndRebuildPartialMessage(RsChatLobbyMsgItem*) ;
		bool locked_checkAndRebuildPartialMessage_deprecated(RsChatMsgItem*) ;

		/// receive and handle chat lobby item
		bool recvLobbyChat(RsChatLobbyMsgItem*,const RsPeerId& src_peer_id) ;
		bool sendLobbyChat(const RsPeerId &id, const std::string&, const ChatLobbyId&) ;
		void handleRecvLobbyInvite(RsChatLobbyInviteItem*) ;
		void checkAndRedirectMsgToLobby(RsChatMsgItem*) ;
		void handleConnectionChallenge(RsChatLobbyConnectChallengeItem *item) ;
		void sendConnectionChallenge(ChatLobbyId id) ;
		void handleFriendUnsubscribeLobby(RsChatLobbyUnsubscribeItem*) ;
		void cleanLobbyCaches() ;
		bool bounceLobbyObject(RsChatLobbyBouncingObject *obj, const RsPeerId& peer_id) ;

		void sendLobbyStatusItem(const ChatLobbyId&, int type, const std::string& status_string) ;
		void sendLobbyStatusPeerLiving(const ChatLobbyId& lobby_id) ;
		void sendLobbyStatusPeerChangedNickname(const ChatLobbyId& lobby_id, const std::string& newnick) ;

		void sendLobbyStatusNewPeer(const ChatLobbyId& lobby_id) ;
		void sendLobbyStatusKeepAlive(const ChatLobbyId&) ;

		bool locked_initLobbyBouncableObject(const ChatLobbyId& id,RsChatLobbyBouncingObject&) ;

		static ChatLobbyVirtualPeerId makeVirtualPeerId(ChatLobbyId) ;
		static uint64_t makeConnexionChallengeCode(const RsPeerId& peer_id,ChatLobbyId lobby_id,ChatLobbyMsgId msg_id) ;

		void locked_printDebugInfo() const ;
		RsChatAvatarItem *makeOwnAvatarItem() ;
		RsChatStatusItem *makeOwnCustomStateStringItem() ;

		p3ServiceControl *mServiceCtrl;
		p3LinkMgr *mLinkMgr;
		p3HistoryMgr *mHistoryMgr;

		std::list<RsChatMsgItem *> publicList;
		std::list<RsChatMsgItem *> privateIncomingList;
		std::list<RsChatMsgItem *> privateOutgoingList;

		AvatarInfo *_own_avatar ;
		std::map<RsPeerId,AvatarInfo *> _avatars ;
		std::map<RsPeerId,RsChatMsgItem *> _pendingPartialMessages ;	
		std::map<ChatLobbyMsgId,std::vector<RsChatLobbyMsgItem*> > _pendingPartialLobbyMessages ;	// should be used for all chat msgs after version updgrade
		std::string _custom_status_string ;
		std::map<RsPeerId,StateStringInfo> _state_strings ;

		class ChatLobbyEntry: public ChatLobbyInfo
		{
			public:
				std::map<ChatLobbyMsgId,time_t> msg_cache ;
				RsPeerId virtual_peer_id ;
				int connexion_challenge_count ;
				time_t last_connexion_challenge_time ;
				time_t last_keep_alive_packet_time ;
				std::set<RsPeerId> previously_known_peers ;
				uint32_t flags ;
		};

		std::map<ChatLobbyId,ChatLobbyEntry> _chat_lobbys ;
		std::map<ChatLobbyId,ChatLobbyInvite> _lobby_invites_queue ;
		std::map<ChatLobbyId,VisibleChatLobbyRecord> _visible_lobbies ;
		std::map<ChatLobbyVirtualPeerId,ChatLobbyId> _lobby_ids ;
		std::map<ChatLobbyId,ChatLobbyFlags> _known_lobbies_flags ;	// flags for all lobbies, including the ones that are not known. So we can't
																				// store them in _chat_lobbies (subscribed lobbies) nor _visible_lobbies.
																				// Known flags:
																				// 		RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE

		std::string _default_nick_name ;
		float _time_shift_average ;
		time_t last_lobby_challenge_time ; 					// prevents bruteforce attack
		time_t last_visible_lobby_info_request_time ;	// allows to ask for updates
		bool _should_reset_lobby_counts ;

		RsChatSerialiser *_serializer ;

		// ===========================================================//
		//         Members related to anonymous distant chat.         //
		// ===========================================================//

	public:
		virtual void connectToTurtleRouter(p3turtle *) ;

		// Creates the invite if the public key of the distant peer is available.
		// Om success, stores the invite in the map above, so that we can respond to tunnel requests.
		//
		bool initiateDistantChatConnexion(const RsGxsId& gxs_id,DistantChatPeerId& pid,uint32_t& error_code) ;
		bool closeDistantChatConnexion(const DistantChatPeerId& pid) ;
		virtual bool getDistantChatStatus(const DistantChatPeerId& hash,RsGxsId& gxs_id,uint32_t& status) ;

	private:
		struct DistantChatPeerInfo 
		{
			time_t last_contact ; 			// used to send keep alive packets
			unsigned char aes_key[16] ;	// key to encrypt packets
			uint32_t status ;					// info: do we have a tunnel ?
			RsPeerId virtual_peer_id;  	// given by the turtle router. Identifies the tunnel.
			RsGxsId gxs_id ;          		// pgp id of the peer we're talking to.
			RsTurtleGenericTunnelItem::Direction direction ; // specifiec wether we are client(managing the tunnel) or server.
		};

		// This maps contains the current peers to talk to with distant chat.
		//
		std::map<TurtleFileHash,DistantChatPeerInfo> _distant_chat_peers ;

		// List of items to be sent asap. Used to store items that we cannot pass directly to 
		// sendTurtleData(), because of Mutex protection.

		std::list<RsChatItem*> pendingDistantChatItems ;

		// Overloaded from RsTurtleClientService

		virtual bool handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id) ;
		virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
		void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
		void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;
		void markDistantChatAsClosed(const TurtleVirtualPeerId& vpid) ;
		void startClientDistantChatConnection(const RsFileHash& hash,const RsGxsId& gxs_id,const unsigned char *aes_key_buf) ;
		bool getHashFromVirtualPeerId(const TurtleVirtualPeerId& pid,RsFileHash& hash) ;
		TurtleFileHash hashFromDistantChatPeerId(const DistantChatPeerId& pid) ;

		// Utility functions

		void sendTurtleData(RsChatItem *) ;
		void sendPrivateChatItem(RsChatItem *) ;

		static TurtleFileHash hashFromVirtualPeerId(const DistantChatPeerId& peerId) ;	// converts IDs so that we can talk to RsPeerId from outside
		static DistantChatPeerId virtualPeerIdFromHash(const TurtleFileHash& hash  ) ;	// ... and to a hash for p3turtle

		p3turtle *mTurtle ;
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

