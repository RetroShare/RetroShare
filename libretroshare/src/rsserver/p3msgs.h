/*******************************************************************************
 * libretroshare/src/rsserver: p3msgs.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include "retroshare/rsmsgs.h"
#include "retroshare/rsgxsifacetypes.h"

class p3MsgService;
class p3ChatService;

class RsChatMsgItem;

//! provides retroshares chatservice and messaging service
/*!
 * Provides rs with the ability to send/receive messages, immediate status,
 * custom status, avatar and
 * chats (public(group) and private) to peers
 */
class p3Msgs: public RsMsgs 
{
public:

	p3Msgs(p3MsgService *p3m, p3ChatService *p3c) :
	    mMsgSrv(p3m), mChatSrv(p3c) {}
	~p3Msgs() override = default;

	/// @see RsMsgs
	uint32_t sendMail(
	        const RsGxsId from,
	        const std::string& subject,
	        const std::string& body,
	        const std::set<RsGxsId>& to = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& cc = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& bcc = std::set<RsGxsId>(),
	        const std::vector<FileInfo>& attachments = std::vector<FileInfo>(),
	        std::set<RsMailTrackId>& trackingIds =
	            RS_DEFAULT_STORAGE_PARAM(std::set<RsMailTrackId>),
	        std::string& errorMsg =
	            RS_DEFAULT_STORAGE_PARAM(std::string) ) override;

	  /****************************************/
	  /* Message Items */

	  /*!
	   * @param msgList ref to list summarising client's msgs
	   */
	  virtual bool getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList);
	  virtual bool getMessage(const std::string &mId, Rs::Msgs::MessageInfo &msg);
	  virtual void getMessageCount(uint32_t &nInbox, uint32_t &nInboxNew, uint32_t &nOutbox, uint32_t &nDraftbox, uint32_t &nSentbox, uint32_t &nTrashbox);

	RS_DEPRECATED_FOR(sendMail)
	  virtual bool MessageSend(Rs::Msgs::MessageInfo &info);
	  virtual bool SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag);
	  virtual bool MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId);
	  virtual bool MessageToTrash(const std::string &mid, bool bTrash);
	  virtual bool MessageDelete(const std::string &mid);
	  virtual bool MessageRead(const std::string &mid, bool unreadByUser);
	  virtual bool MessageReplied(const std::string &mid, bool replied);
	  virtual bool MessageForwarded(const std::string &mid, bool forwarded);
	  virtual bool MessageStar(const std::string &mid, bool star);
	  virtual bool MessageLoadEmbeddedImages(const std::string &mid, bool load);
	  virtual bool getMsgParentId(const std::string &msgId, std::string &msgParentId);

	  virtual bool getMessageTagTypes(Rs::Msgs::MsgTagType& tags);
	  virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
	  virtual bool removeMessageTagType(uint32_t tagId);

	  virtual bool getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info);
	  /* set == false && tagId == 0 --> remove all */
	  virtual bool setMessageTag(const std::string &msgId, uint32_t tagId, bool set);

	  virtual bool resetMessageStandardTagTypes(Rs::Msgs::MsgTagType& tags);

	  virtual uint32_t getDistantMessagingPermissionFlags() ;
	  virtual void setDistantMessagingPermissionFlags(uint32_t flags) ;

	  /*!
	   * gets avatar from peer, image data in jpeg format
	   */
	  virtual void getAvatarData(const RsPeerId& pid,unsigned char *& data,int& size);

	  /*!
	   * sets clients avatar, image data should be in jpeg format
	   */
	  virtual void setOwnAvatarData(const unsigned char *data,int size);

	  /*!
	   * retrieve clients avatar, image data in jpeg format
	   */
	  virtual void getOwnAvatarData(unsigned char *& data,int& size);

	  /*!
	   * sets clients custom status (e.g. "i'm tired")
	   */
	  virtual void setCustomStateString(const std::string&  status_string) ;

	  /*!
	   * retrieves client's custom status
	   */
	  virtual std::string getCustomStateString() ;

	  /*!
	   * retrieves peer's custom status
	   */
	  virtual std::string getCustomStateString(const RsPeerId& peer_id) ;
	  

	  /*!
       * Send a chat message.
       * @param destination where to send the chat message
       * @param msg the message
       * @see ChatId
	   */
      virtual bool sendChat(ChatId destination, std::string msg) ;

	  /*!
	   * Return the max message size for security forwarding
	   */
	  virtual uint32_t getMaxMessageSecuritySize(int type);

    /*!
     * sends immediate status string to a specific peer, e.g. in a private chat
     * @param chat_id chat id to send status string to
     * @param status_string immediate status to send
     */
    virtual void    sendStatusString(const ChatId& id, const std::string& status_string) ;

    /**
     * @brief clearChatLobby: Signal chat was cleared by GUI.
     * @param id: Chat id cleared.
     */
    virtual void clearChatLobby(const ChatId &id);

	  /****************************************/

      virtual bool joinVisibleChatLobby(const ChatLobbyId& id, const RsGxsId &own_id) ;
	  virtual void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) ;
      virtual void getChatLobbyList(std::list<ChatLobbyId>& cl_list) ;
      virtual bool getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& info) ;
      virtual void invitePeerToLobby(const ChatLobbyId&, const RsPeerId&) ;
      virtual bool acceptLobbyInvite(const ChatLobbyId& id, const RsGxsId &gxs_id) ;
	  virtual void denyLobbyInvite(const ChatLobbyId& id) ;
	  virtual void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) ;
	  virtual void unsubscribeChatLobby(const ChatLobbyId& lobby_id) ;
	  virtual void sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id);
      virtual bool setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId&) ;
      virtual bool getIdentityForChatLobby(const ChatLobbyId&,RsGxsId& nick) ;
      virtual bool setDefaultIdentityForChatLobby(const RsGxsId&) ;
      virtual void getDefaultIdentityForChatLobby(RsGxsId& nick) ;
    virtual void setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe);
    virtual bool getLobbyAutoSubscribe(const ChatLobbyId& lobby_id);
      virtual ChatLobbyId createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic,const std::set<RsPeerId>& invited_friends,ChatLobbyFlags privacy_type) ;

	virtual bool initiateDistantChatConnexion(
		          const RsGxsId& to_gxs_id, const RsGxsId& from_gxs_id,
		          DistantChatPeerId &pid, uint32_t& error_code,
		          bool notify = true );

      virtual bool getDistantChatStatus(const DistantChatPeerId& gxs_id,DistantChatPeerInfo& info);
      virtual bool closeDistantChatConnexion(const DistantChatPeerId &pid) ;

    virtual uint32_t getDistantChatPermissionFlags() ;
    virtual bool setDistantChatPermissionFlags(uint32_t flags) ;
    
   private:

	  p3MsgService  *mMsgSrv;
	  p3ChatService *mChatSrv;
};
