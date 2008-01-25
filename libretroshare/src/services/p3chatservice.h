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

/* 
 * The basic Chat service.
 *
 */

#include <list>
#include <string>

#include "serialiser/rsmsgitems.h"
#include "services/p3service.h"
#include "pqi/p3connmgr.h"


class p3ChatService: public p3Service
{
	public:
	p3ChatService(p3ConnectMgr *cm);

	/* overloaded */
virtual int   tick();
virtual int   status();

int	sendChat(std::string msg);
int	sendPrivateChat(std::string msg, std::string id);

std::list<RsChatItem *> getChatQueue(); 

	private:
	p3ConnectMgr *mConnMgr;
};

#endif // SERVICE_CHAT_HEADER
