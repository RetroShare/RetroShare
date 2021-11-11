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
	        std::set<RsMailIdRecipientIdPair>& trackingIds =
	            RS_DEFAULT_STORAGE_PARAM(std::set<RsMailIdRecipientIdPair>),
	        std::string& errorMsg =
	            RS_DEFAULT_STORAGE_PARAM(std::string) ) override;

	  /****************************************/
	  /* Message Items */

	  /*!
	   * @param msgList ref to list summarising client's msgs
	   */
      virtual bool getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList)override ;
      virtual bool getMessage(const std::string &mId, Rs::Msgs::MessageInfo &msg)override ;
      virtual void getMessageCount(uint32_t &nInbox, uint32_t &nInboxNew, uint32_t &nOutbox, uint32_t &nDraftbox, uint32_t &nSentbox, uint32_t &nTrashbox)override ;

	RS_DEPRECATED_FOR(sendMail)
      virtual bool MessageSend(Rs::Msgs::MessageInfo &info)override ;
      virtual bool SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag)override ;
      virtual bool MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId)override ;
      virtual bool MessageToTrash(const std::string &mid, bool bTrash)override ;
      virtual bool MessageDelete(const std::string &mid)override ;
      virtual bool MessageRead(const std::string &mid, bool unreadByUser)override ;
      virtual bool MessageReplied(const std::string &mid, bool replied)override ;
      virtual bool MessageForwarded(const std::string &mid, bool forwarded)override ;
      virtual bool MessageStar(const std::string &mid, bool star)override ;
      virtual bool MessageJunk(const std::string &mid, bool junk)override ;
      virtual bool MessageLoadEmbeddedImages(const std::string &mid, bool load)override ;
      virtual bool getMsgParentId(const std::string &msgId, std::string &msgParentId)override ;

      virtual bool getMessageTagTypes(Rs::Msgs::MsgTagType& tags)override ;
      virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color)override ;
      virtual bool removeMessageTagType(uint32_t tagId)override ;

      virtual bool getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info)override ;
	  /* set == false && tagId == 0 --> remove all */
      virtual bool setMessageTag(const std::string &msgId, uint32_t tagId, bool set)override ;

      virtual bool resetMessageStandardTagTypes(Rs::Msgs::MsgTagType& tags)override ;

      virtual uint32_t getDistantMessagingPermissionFlags() override ;
      virtual void setDistantMessagingPermissionFlags(uint32_t flags) override ;

	  /*!
	   * gets avatar from peer, image data in jpeg format
	   */
      virtual void getAvatarData(const RsPeerId& pid,unsigned char *& data,int& size)override ;

	  /*!
	   * sets clients avatar, image data should be in jpeg format
	   */
      virtual void setOwnAvatarData(const unsigned char *data,int size)override ;

	  /*!
	   * retrieve clients avatar, image data in jpeg format
	   */
      virtual void getOwnAvatarData(unsigned char *& data,int& size)override ;

	  /*!
	   * sets clients custom status (e.g. "i'm tired")
	   */
      virtual void setCustomStateString(const std::string&  status_string) override ;

	  /*!
	   * retrieves client's custom status
	   */
      virtual std::string getCustomStateString() override ;

	  /*!
	   * retrieves peer's custom status
	   */
      virtual std::string getCustomStateString(const RsPeerId& peer_id) override ;
	  

	  /*!
       * Send a chat message.
       * @param destination where to send the chat message
       * @param msg the message
       * @see ChatId
	   */
      virtual bool sendChat(ChatId destination, std::string msg) override ;

	  /*!
	   * Return the max message size for security forwarding
	   */
      virtual uint32_t getMaxMessageSecuritySize(int type)override ;

    /*!
     * sends immediate status string to a specific peer, e.g. in a private chat
     * @param chat_id chat id to send status string to
     * @param status_string immediate status to send
     */
    virtual void    sendStatusString(const ChatId& id, const std::string& status_string) override ;

    /**
     * @brief clearChatLobby: Signal chat was cleared by GUI.
     * @param id: Chat id cleared.
     */
    virtual void clearChatLobby(const ChatId &id)override ;

	  /****************************************/

      virtual bool joinVisibleChatLobby(const ChatLobbyId& id, const RsGxsId &own_id) override ;
      virtual void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) override ;
      virtual void getChatLobbyList(std::list<ChatLobbyId>& cl_list) override ;
      virtual bool getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& info) override ;
      virtual void invitePeerToLobby(const ChatLobbyId&, const RsPeerId&) override ;
      virtual bool acceptLobbyInvite(const ChatLobbyId& id, const RsGxsId &gxs_id) override ;
      virtual bool denyLobbyInvite(const ChatLobbyId& id) override ;
      virtual void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) override ;
      virtual void unsubscribeChatLobby(const ChatLobbyId& lobby_id) override ;
      virtual void sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id)override ;
      virtual bool setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId&) override ;
      virtual bool getIdentityForChatLobby(const ChatLobbyId&,RsGxsId& nick) override ;
      virtual bool setDefaultIdentityForChatLobby(const RsGxsId&) override ;
      virtual void getDefaultIdentityForChatLobby(RsGxsId& nick) override ;
    virtual void setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe)override ;
    virtual bool getLobbyAutoSubscribe(const ChatLobbyId& lobby_id)override ;
      virtual ChatLobbyId createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic,const std::set<RsPeerId>& invited_friends,ChatLobbyFlags privacy_type) override ;

	virtual bool initiateDistantChatConnexion(
		          const RsGxsId& to_gxs_id, const RsGxsId& from_gxs_id,
		          DistantChatPeerId &pid, uint32_t& error_code,
                  bool notify = true )override ;

      virtual bool getDistantChatStatus(const DistantChatPeerId& gxs_id,DistantChatPeerInfo& info)override ;
      virtual bool closeDistantChatConnexion(const DistantChatPeerId &pid) override ;

    virtual uint32_t getDistantChatPermissionFlags() override ;
    virtual bool setDistantChatPermissionFlags(uint32_t flags) override ;
    
   private:

	  p3MsgService  *mMsgSrv;
	  p3ChatService *mChatSrv;
};
