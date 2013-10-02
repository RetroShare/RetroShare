/*
 * libretroshare/src/pqi pqiservice.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "pqi/pqiservice.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"

const int pqiservicezone = 60478;

/****
 * #define SERVICE_DEBUG 1
 ****/

void pqiService::setServiceServer(p3ServiceServer *server)
{
	mServiceServer = server;
}

bool pqiService::send(RsRawItem *item)
{
	return mServiceServer->sendItem(item);
}


p3ServiceServer::p3ServiceServer(pqiPublisher *pub) : mPublisher(pub), srvMtx("p3ServiceServer") 
{
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::p3ServiceServer()");
#endif

	return;
}

int	p3ServiceServer::addService(pqiService *ts)
{
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::addService()");
#endif


	std::map<uint32_t, pqiService *>::iterator it;
	it = services.find(ts -> getType());
	if (it != services.end())
	{
		std::cerr << "p3ServiceServer::addService(): Service already added with id " << ts->getType() << "!" << std::endl;
		// it exists already!
		return -1;
	}

	ts->setServiceServer(this);
	services[ts -> getType()] = ts;

	return 1;
}

bool	p3ServiceServer::recvItem(RsRawItem *item)
{
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::incoming()");

	{
		std::string out;
		rs_sprintf(out, "p3ServiceServer::incoming() PacketId: %x\nLooking for Service: %x\nItem:\n", item -> PacketId(), (item -> PacketId() & 0xffffff00));
		item -> print_string(out);
		pqioutput(PQL_DEBUG_BASIC, pqiservicezone, out);
	}
#endif

	std::map<uint32_t, pqiService *>::iterator it;
	it = services.find(item -> PacketId() & 0xffffff00);
	if (it == services.end())
	{
#ifdef  SERVICE_DEBUG
		pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::incoming() Service: No Service - deleting");
#endif
		delete item;
		return false;
	}

	{
#ifdef  SERVICE_DEBUG
		std::string out;
		rs_sprintf(out, "p3ServiceServer::incoming() Sending to %p", it -> second);
		pqioutput(PQL_DEBUG_BASIC, pqiservicezone, out);
#endif

		return (it->second) -> recv(item);
	}

	delete item;
	return false;
}



bool p3ServiceServer::sendItem(RsRawItem *item)
{
#ifdef  SERVICE_DEBUG
	std::cerr << "p3ServiceServer::sendItem()";
	std::cerr << std::endl;
	item -> print_string(out);
	std::cerr << std::endl;
#endif

	/* any filtering ??? */

	mPublisher->sendItem(item);
	return true;
}



int	p3ServiceServer::tick()
{

	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqiservicezone, 
		"p3ServiceServer::tick()");
#endif

	std::map<uint32_t, pqiService *>::iterator it;

	// from the beginning to where we started.
	for(it = services.begin();it != services.end(); it++)
	{

#ifdef  SERVICE_DEBUG
		std::string out;
		rs_sprintf(out, "p3ServiceServer::service id: %u -> Service: %p", it -> first, it -> second);
		pqioutput(PQL_DEBUG_ALL, pqiservicezone, out);
#endif

		// now we should actually tick the service.
		(it -> second) -> tick();
	}
	return 1;
}



