#ifndef RS_MSG_GUI_INTERFACE_H
#define RS_MSG_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsmsgs.h
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


#include <list>
#include <iostream>
#include <string>

#include "rstypes.h"

/********************** For Messages and Channels *****************/

#define RS_MSG_BOXMASK   0x000f   /* Mask for determining Box */

#define RS_MSG_OUTGOING        0x0001   /* !Inbox */
#define RS_MSG_PENDING         0x0002   /* OutBox */
#define RS_MSG_DRAFT           0x0004   /* Draft  */

/* ORs of above */
#define RS_MSG_INBOX           0x00     /* Inbox */
#define RS_MSG_SENTBOX         0x01     /* Sentbox */
#define RS_MSG_OUTBOX          0x03     /* Outbox */
#define RS_MSG_DRAFTBOX        0x05     /* Draftbox */

#define RS_MSG_NEW             0x0010   /* New */
#define RS_MSG_TRASH           0x0020   /* Trash */
#define RS_MSG_UNREAD_BY_USER  0x0040   /* Unread by user */

#define RS_MSGTAGTYPE_IMPORTANT  1
#define RS_MSGTAGTYPE_WORK       2
#define RS_MSGTAGTYPE_PERSONAL   3
#define RS_MSGTAGTYPE_TODO       4
#define RS_MSGTAGTYPE_LATER      5
#define RS_MSGTAGTYPE_USER       100

class MessageInfo 
{
	public:
	MessageInfo() {}
	std::string msgId;
	std::string srcId;

	unsigned int msgflags;

	std::list<std::string> msgto;
	std::list<std::string> msgcc;
	std::list<std::string> msgbcc;

	std::wstring title;
	std::wstring msg;

	std::wstring attach_title;
	std::wstring attach_comment;
	std::list<FileInfo> files;
	int size;  /* total of files */
	int count; /* file count     */

	int ts;
};

class MsgInfoSummary 
{
	public:
	MsgInfoSummary() {}

	std::string msgId;
	std::string srcId;

	uint32_t msgflags;

	std::wstring title;
	int count; /* file count     */
	time_t ts;

};

class MsgTagInfo
{
public:
	MsgTagInfo() {}

	std::string msgId;
	std::list<uint32_t> tagIds;
};

class MsgTagType
{
public:
	MsgTagType() {}

	/* map containing tagId -> pair (text, rgb color) */
	std::map<uint32_t, std::pair<std::string, uint32_t> > types;

};

#define RS_CHAT_PUBLIC 				0x0001
#define RS_CHAT_PRIVATE 			0x0002
#define RS_CHAT_AVATAR_AVAILABLE 	0x0004

class ChatInfo 
{
	public:
	std::string rsid;
	unsigned int chatflags;
	std::string name;
	std::wstring msg;
};

std::ostream &operator<<(std::ostream &out, const MessageInfo &info);
std::ostream &operator<<(std::ostream &out, const ChatInfo &info);
//std::ostream &operator<<(std::ostream &out, const MsgTagInfo);
//std::ostream &operator<<(std::ostream &out, const MsgTagType);


class RsMsgs;
extern RsMsgs   *rsMsgs;

class RsMsgs 
{
	public:

	RsMsgs() { return; }
virtual ~RsMsgs() { return; }

/****************************************/
	/* Message Items */

virtual bool getMessageSummaries(std::list<MsgInfoSummary> &msgList) = 0;
virtual bool getMessage(std::string mId, MessageInfo &msg)  = 0;
virtual void getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox) = 0;

virtual	bool MessageSend(MessageInfo &info)                 = 0;
virtual bool MessageToDraft(MessageInfo &info)              = 0;
virtual bool MessageToTrash(std::string mid, bool bTrash)   = 0;

virtual bool MessageDelete(std::string mid)                 = 0;
virtual bool MessageRead(std::string mid, bool bUnreadByUser) = 0;

/* message tagging */

virtual bool getMessageTagTypes(MsgTagType& tags) = 0;
/* set == false && tagId == 0 --> remove all */
virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color) = 0;
virtual bool removeMessageTagType(uint32_t tagId) = 0;

virtual bool getMessageTag(std::string msgId, MsgTagInfo& info) = 0;
virtual bool setMessageTag(std::string msgId, uint32_t tagId, bool set) = 0;

virtual bool resetMessageStandardTagTypes(MsgTagType& tags) = 0;

/****************************************/
	/* Chat */
virtual bool    chatAvailable() 			   = 0;
virtual	bool 	ChatSend(ChatInfo &ci)                     = 0;
virtual	bool	getNewChat(std::list<ChatInfo> &chats)	   = 0;
virtual void   sendStatusString(const std::string& id,const std::string& status_string) = 0 ;
virtual void   sendGroupChatStatusString(const std::string& status_string) = 0 ;

virtual void   setCustomStateString(const std::string&  status_string) = 0 ;
virtual std::string getCustomStateString() = 0 ;
virtual std::string getCustomStateString(const std::string& peer_id) = 0 ;

// get avatar data for peer pid
virtual void getAvatarData(std::string pid,unsigned char *& data,int& size) = 0 ;
// set own avatar data 
virtual void setOwnAvatarData(const unsigned char *data,int size) = 0 ;
virtual void getOwnAvatarData(unsigned char *& data,int& size) = 0 ;

/****************************************/

};


#endif
