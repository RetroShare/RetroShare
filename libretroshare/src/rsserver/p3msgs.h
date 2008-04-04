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

#include "rsiface/rsmsgs.h"

class p3AuthMgr;
class p3MsgService;
class p3ChatService;

class p3Msgs: public RsMsgs 
{
	public:

	p3Msgs(p3AuthMgr *p3a, p3MsgService *p3m, p3ChatService *p3c)
	:mAuthMgr(p3a), mMsgSrv(p3m), mChatSrv(p3c) { return; }
virtual ~p3Msgs() { return; }

/****************************************/
	/* Message Items */

virtual bool getMessageSummaries(std::list<MsgInfoSummary> &msgList);
virtual bool getMessage(std::string mId, MessageInfo &msg);

virtual	bool MessageSend(MessageInfo &info);
virtual bool MessageDelete(std::string mid);
virtual bool MessageRead(std::string mid);

/****************************************/
	/* Chat */
virtual bool    chatAvailable();
virtual	bool 	ChatSend(ChatInfo &ci);
virtual	bool	getNewChat(std::list<ChatInfo> &chats);

/****************************************/


	private:

void initRsChatInfo(RsChatItem *c, ChatInfo &i);

	p3AuthMgr     *mAuthMgr;
	p3MsgService  *mMsgSrv;
	p3ChatService *mChatSrv;
};


#endif
