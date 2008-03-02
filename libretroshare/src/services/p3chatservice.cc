/*
 * "$Id: p3ChatService.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
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


#include "services/p3chatservice.h"

/****
 * #define CHAT_DEBUG 1
 ****/

/************ NOTE *********************************
 * This Service is so simple that there is no
 * mutex protection required!
 *
 */

p3ChatService::p3ChatService(p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_CHAT), mConnMgr(cm)
{
	addSerialType(new RsChatSerialiser());
}

int	p3ChatService::tick()
{

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::tick()";
	std::cerr << std::endl;
#endif

	return 0;
}

int	p3ChatService::status()
{

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::status()";
	std::cerr << std::endl;
#endif

	return 1;
}

/***************** Chat Stuff **********************/

int     p3ChatService::sendChat(std::wstring msg)
{
	/* go through all the peers */

	std::list<std::string> ids;
	std::list<std::string>::iterator it;
	mConnMgr->getOnlineList(ids);

	/* add in own id -> so get reflection */
	ids.push_back(mConnMgr->getOwnId());

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat()";
	std::cerr << std::endl;
#endif

	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsChatItem *ci = new RsChatItem();

		ci->PeerId(*it);
		ci->chatFlags = 0;
		ci->sendTime = time(NULL);
		ci->message = msg;
	
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::sendChat() Item:";
		std::cerr << std::endl;
		ci->print(std::cerr);
		std::cerr << std::endl;
#endif

		sendItem(ci);
	}

	return 1;
}

int     p3ChatService::sendPrivateChat(std::wstring msg, std::string id)
{
	// make chat item....
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendPrivateChat()";
	std::cerr << std::endl;
#endif

	RsChatItem *ci = new RsChatItem();

	ci->PeerId(id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE;
	ci->sendTime = time(NULL);
	ci->message = msg;

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendPrivateChat() Item:";
	std::cerr << std::endl;
	ci->print(std::cerr);
	std::cerr << std::endl;
#endif

	sendItem(ci);

	return 1;
}

std::list<RsChatItem *> p3ChatService::getChatQueue()
{
	time_t now = time(NULL);

	RsChatItem *ci = NULL;
	std::list<RsChatItem *> ilist;

	while(NULL != (ci = (RsChatItem *) recvItem()))
	{
		ci->recvTime = now;
		ilist.push_back(ci);
	}

	return ilist;
}


