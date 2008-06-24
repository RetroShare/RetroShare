
/*
 * "$Id: p3face-msgs.cc,v 1.7 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#include <iostream>
#include <sstream>

#include "pqi/pqidebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

#include "rsiface/rstypes.h"
#include "util/rsdir.h"
#include "rsserver/p3msgs.h"

#include "services/p3msgservice.h"
#include "services/p3chatservice.h"

/* external reference point */
RsMsgs *rsMsgs = NULL;

/****************************************/
/****************************************/

bool p3Msgs::getMessageSummaries(std::list<MsgInfoSummary> &msgList)
{
	return mMsgSrv->getMessageSummaries(msgList);
}



bool p3Msgs::getMessage(std::string mid, MessageInfo &msg)
{
	return mMsgSrv->getMessage(mid, msg);
}



/****************************************/
/****************************************/
	/* Message Items */
bool p3Msgs::MessageSend(MessageInfo &info)
{
	return mMsgSrv->MessageSend(info);
}

/****************************************/
/****************************************/
bool p3Msgs::MessageDelete(std::string mid)
{
	std::cerr << "p3Msgs::MessageDelete() ";
	std::cerr << "mid: " << mid << std::endl;

	mMsgSrv -> removeMsgId(mid);

	return 1;
}

bool p3Msgs::MessageRead(std::string mid)
{
	std::cerr << "p3Msgs::MessageRead() ";
	std::cerr << "mid: " << mid << std::endl;

	mMsgSrv -> markMsgIdRead(mid);

	return 1;
}

/****************************************/
/****************************************/
bool 	p3Msgs::ChatSend(ChatInfo &ci)
{
	/* send a message to all for now */
	if (ci.chatflags & RS_CHAT_PRIVATE)
	{
	  mChatSrv -> sendPrivateChat(ci.msg, ci.rsid);
	}
	else
	{
	  /* global */
	  mChatSrv -> sendChat(ci.msg);
	}
	return true;
}

bool    p3Msgs::chatAvailable()
{
	return mChatSrv->receivedItems();
}

bool	p3Msgs::getNewChat(std::list<ChatInfo> &chats)
{
	/* get any messages and push them to iface */

	// get the items from the list.
	std::list<RsChatItem *> clist = mChatSrv -> getChatQueue();
	if (clist.size() < 1)
	{
		return false;
	}

	std::list<RsChatItem *>::iterator it;
	for(it = clist.begin(); it != clist.end(); it++)
	{
		ChatInfo ci;
		initRsChatInfo((*it), ci);
		chats.push_back(ci);
		delete (*it);
	}
	return true;
}

/**** HELPER FNS For Chat/Msg/Channel Lists ************
 *
 * The iface->Mutex is required to be locked
 * for intAddChannel / intAddChannelMsg.
 */

void p3Msgs::initRsChatInfo(RsChatItem *c, ChatInfo &i)
{
	i.rsid = c -> PeerId();
	i.name = mAuthMgr->getName(i.rsid);

	i.msg  = c -> message;
        if (c -> chatFlags & RS_CHAT_FLAG_PRIVATE)
	{
		std::cerr << "RsServer::initRsChatInfo() Chat Private!!!";
		i.chatflags = RS_CHAT_PRIVATE;
	}
	else
	{
		i.chatflags = RS_CHAT_PUBLIC;
		std::cerr << "RsServer::initRsChatInfo() Chat Public!!!";
	}
	std::cerr << std::endl;
}

