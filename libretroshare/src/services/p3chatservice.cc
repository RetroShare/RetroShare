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
#include "pqi/pqidebug.h"
#include <sstream>

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/



const int p3chatzone = 1745;

p3ChatService::p3ChatService()
	:p3Service(RS_SERVICE_TYPE_CHAT) 
{
	addSerialType(new RsChatSerialiser());
}

int	p3ChatService::tick()
{
	pqioutput(PQL_DEBUG_BASIC, p3chatzone, 
		"p3ChatService::tick()");
	return 0;
}

int	p3ChatService::status()
{
	pqioutput(PQL_DEBUG_BASIC, p3chatzone, 
		"p3ChatService::status()");
	return 1;
}

/***************** Chat Stuff **********************/

int     p3ChatService::sendChat(std::string msg)
{
	/* go through all the peers */
	sslroot *sslr = getSSLRoot();

	std::list<cert *>::iterator it;
	std::list<cert *> &certs = sslr -> getCertList();

	for(it = certs.begin(); it != certs.end(); it++)
	{
		pqioutput(PQL_DEBUG_BASIC, p3chatzone, 
			"p3ChatService::sendChat()");

		RsChatItem *ci = new RsChatItem();

		ci->PeerId((*it)->PeerId());
		ci->chatFlags = 0;
		ci->sendTime = time(NULL);
		ci->message = msg;
	
		{
		  std::ostringstream out;
		  out << "Chat Item we are sending:" << std::endl;
		  ci -> print(out);
		  pqioutput(PQL_DEBUG_BASIC, p3chatzone, out.str());
		}
	
		sendItem(ci);
	}

	return 1;
}

int     p3ChatService::sendPrivateChat(std::string msg, std::string id)
{
	// make chat item....
	pqioutput(PQL_DEBUG_BASIC, p3chatzone, 
		"p3ChatService::sendPrivateChat()");

	RsChatItem *ci = new RsChatItem();

	ci->PeerId(id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE;
	ci->sendTime = time(NULL);
	ci->message = msg;

	{
	  std::ostringstream out;
	  out << "Private Chat Item we are sending:" << std::endl;
	  ci -> print(out);
	  out << "Sending to:" << id << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, p3chatzone, out.str());
	}

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


