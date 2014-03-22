/*
 * libretroshare/src/pqi pqiservice.h
 *
 * 3P/PQI network interface for RetroShare.
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


#ifndef PQI_SERVICE_HEADER
#define PQI_SERVICE_HEADER

#include "pqi/pqi.h"
#include "pqi/pqi_base.h"
#include "util/rsthreads.h"

#include "retroshare/rsservicecontrol.h"
#include "pqi/p3servicecontrol.h"

// PQI Service, is a generic lower layer on which services can run on.
// 
// these packets get passed through the 
// server, to a service that is registered.
//
// example services:
// 	proxytunnel. -> p3proxy.
// 	sockettunnel
// 		-> either broadcast (attach to 
// 				open socket instead
// 				of broadcast address)
// 		-> or requested/signon.
//
// 	games,
// 	voice
// 	video
//
//
// DataType is defined in the serialiser directory.

class RsRawItem;
class p3ServiceServer;


class pqiService
{
	protected:

	pqiService() // our type of packets.
	:mServiceServer(NULL) { return; }

virtual ~pqiService() { return; }

	public:
void 	setServiceServer(p3ServiceServer *server);
	// 
virtual bool	recv(RsRawItem *) = 0;
virtual bool	send(RsRawItem *item);

virtual RsServiceInfo getServiceInfo() = 0;

virtual int	tick() { return 0; }

	private:
	p3ServiceServer *mServiceServer; // const, no need for mutex.
};

#include <map>

/* We are pushing the packets back through p3ServiceServer, 
 * so that we can filter services at this level later...
 * if we decide not to do this, pqiService can call through
 * to the base level pqiPublisher instead.
 */

class p3ServiceServer
{
public:
	p3ServiceServer(pqiPublisher *pub, p3ServiceControl *ctrl);

int	addService(pqiService *, bool defaultOn);

bool	recvItem(RsRawItem *);
bool	sendItem(RsRawItem *);

int	tick();

private:

	pqiPublisher *mPublisher;	// constant no need for mutex.
	p3ServiceControl *mServiceControl;

	RsMutex srvMtx; 
	std::map<uint32_t, pqiService *> services;

};


#endif // PQI_SERVICE_HEADER
