
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

#include "util/rsdir.h"
#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

#include "rsiface/rstypes.h"
#include "rsserver/p3msgs.h"

#include "services/p3msgservice.h"
#include "services/p3chatservice.h"

#include "pqi/authgpg.h"

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
	//std::cerr << "p3Msgs::MessageDelete() ";
	//std::cerr << "mid: " << mid << std::endl;

	mMsgSrv -> removeMsgId(mid);

	return 1;
}

bool p3Msgs::MessageRead(std::string mid)
{
	//std::cerr << "p3Msgs::MessageRead() ";
	//std::cerr << "mid: " << mid << std::endl;

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

void p3Msgs::sendGroupChatStatusString(const std::string& status_string) 
{
	mChatSrv->sendGroupChatStatusString(status_string);
}
void p3Msgs::sendStatusString(const std::string& peer_id,const std::string& status_string) 
{
	mChatSrv->sendStatusString(peer_id,status_string);
}

bool    p3Msgs::chatAvailable()
{
	return mChatSrv->receivedItems();
}

bool	p3Msgs::getNewChat(std::list<ChatInfo> &chats)
{
	/* get any messages and push them to iface */

	// get the items from the list.
	std::list<RsChatMsgItem *> clist = mChatSrv -> getChatQueue();
	if (clist.size() < 1)
	{
		return false;
	}

	std::list<RsChatMsgItem *>::iterator it;
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

void p3Msgs::initRsChatInfo(RsChatMsgItem *c, ChatInfo &i)
{
	i.rsid = c -> PeerId();
        i.name = rsPeers->getPeerName(c -> PeerId());
	i.chatflags = 0 ;
	i.msg  = c -> message;

	if (c -> chatFlags & RS_CHAT_FLAG_PRIVATE)
	{
		i.chatflags |= RS_CHAT_PRIVATE;
		//std::cerr << "RsServer::initRsChatInfo() Chat Private!!!";
	}
	else
	{
		i.chatflags |= RS_CHAT_PUBLIC;
		//std::cerr << "RsServer::initRsChatInfo() Chat Public!!!";
	}
}

void p3Msgs::getOwnAvatarData(unsigned char *& data,int& size)
{
	mChatSrv->getOwnAvatarJpegData(data,size) ;
}

void p3Msgs::setOwnAvatarData(const unsigned char *data,int size)
{
	mChatSrv->setOwnAvatarJpegData(data,size) ;
}

void p3Msgs::getAvatarData(std::string pid,unsigned char *& data,int& size)
{
	mChatSrv->getAvatarJpegData(pid,data,size) ;
}

std::string p3Msgs::getCustomStateString(const std::string& peer_id)
{
	return mChatSrv->getCustomStateString(peer_id) ;
}

std::string p3Msgs::getCustomStateString()
{
	return mChatSrv->getOwnCustomStateString() ;
}

void p3Msgs::setCustomStateString(const std::string& state_string)
{
	mChatSrv->setOwnCustomStateString(state_string) ;
}

