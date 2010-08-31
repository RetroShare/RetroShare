#ifndef RS_P3MSG_INTERFACE_H
#define RS_P3MSG_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3msgs.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "retroshare/rsmsgs.h"

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

          p3Msgs(p3MsgService *p3m, p3ChatService *p3c)
                 :mMsgSrv(p3m), mChatSrv(p3c) { return; }
	  virtual ~p3Msgs() { return; }

	  /****************************************/
	  /* Message Items */

	  /*!
	   * @param msgList ref to list summarising client's msgs
	   */
	  virtual bool getMessageSummaries(std::list<MsgInfoSummary> &msgList);
	  virtual bool getMessage(std::string mId, MessageInfo &msg);
	  virtual void getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox);

	  virtual bool MessageSend(MessageInfo &info);
	  virtual bool MessageToDraft(MessageInfo &info);
	  virtual bool MessageToTrash(std::string mid, bool bTrash);
	  virtual bool MessageDelete(std::string mid);
	  virtual bool MessageRead(std::string mid, bool bUnreadByUser);

	  virtual bool getMessageTagTypes(MsgTagType& tags);
	  virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
	  virtual bool removeMessageTagType(uint32_t tagId);

	  virtual bool getMessageTag(std::string msgId, MsgTagInfo& info);
	  /* set == false && tagId == 0 --> remove all */
	  virtual bool setMessageTag(std::string msgId, uint32_t tagId, bool set);

	  virtual bool resetMessageStandardTagTypes(MsgTagType& tags);

	  /*!
	   * gets avatar from peer, image data in jpeg format
	   */
	  virtual void getAvatarData(std::string pid,unsigned char *& data,int& size);

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
	  virtual std::string getCustomStateString(const std::string& peer_id) ;
	  

	  /****************************************/
	  /* Chat */
	  /*!
	   * sends chat (public and private)
	   * @param ci chat info
	   */
	  virtual	bool 	ChatSend(ChatInfo &ci);

	  /*!
	   * @param chats ref to list of received chats is stored here
	   */
	  virtual	bool	getNewChat(std::list<ChatInfo> &chats);

	  /*!
	   * sends immediate status string to a specific peer, e.g. in a private chat
	   * @param peer_id peer to send status string to
	   * @param status_string immediate status to send
	   */
	  virtual void    sendStatusString(const std::string& peer_id,const std::string& status_string) ;

	  /*!
	   * sends immediate status to all peers
	   * @param status_string immediate status to send
	   */
	  virtual void    sendGroupChatStatusString(const std::string& status_string) ;

	  /****************************************/


   private:

	  p3MsgService  *mMsgSrv;
	  p3ChatService *mChatSrv;
};


#endif
