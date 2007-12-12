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

#include "pqi/pqi_base.h"

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

class pqiService
{
	protected:

	pqiService(uint32_t t) // our type of packets.
	:type(t) { return; }

virtual ~pqiService() { return; }

	public:
	// 
virtual int		receive(RsRawItem *) = 0;
virtual RsRawItem *	send() = 0;

int	getType() { return type; }

virtual int	tick() { return 0; }

	private:
	int type;
};

#include <map>


class p3ServiceServer
{
public:
	p3ServiceServer();

int	addService(pqiService *);

int	incoming(RsRawItem *);
RsRawItem *outgoing();

int	tick();

private:

std::map<int, pqiService *> services;
std::map<int, pqiService *>::iterator rrit;

};


#endif // PQI_SERVICE_HEADER
