/*
 * libretroshare/src/services: p3portservice.h
 *
 * Services for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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


#ifndef SERVICE_PORT_FORWARD_HEADER
#define SERVICE_PORT_FORWARD_HEADER

/* 
 * The start of a port forwarding service.
 *
 * This is just a very rough example of what might be required for version 1.
 *
 * FIRST VERSION:
 *
 * listen to a single port and send data with a single RS peer.
 *
 *
 * SECOND VERSION:
 *
 * enable multiple port forwardings.
 *
 * each forwarding consists of a 'port, peerId & Connection Id'.
 *
 * THIRD VERSION:
 *
 * add broadcast/multicast forwardings. 
 * i.e. data gets sent to multiple peers.
 *
 * each forwarding with then consist of 'port, connectionId + list of peerIds'
 * NOTE: version 3 needs some thought - might require a master host
 * which distributes to all others.... or other more complicated systems.
 *
 */

#include <list>
#include <string>

#include "serialiser/rsbaseitems.h"
#include "services/p3service.h"

class p3LinkMgr;


class p3PortService: public p3Service
{
	public:
	p3PortService(p3LinkMgr *lm);

	/* example setup functions */
bool	enablePortForwarding(uint32_t port, std::string peerId);

	/* overloaded */
virtual int   tick();

	private:

	p3LinkMgr *mLinkMgr;

	bool mEnabled;
	bool mPeerOnline;

	uint32_t mPort;
	std::string mPeerId;

};

#endif // SERVICE_PORT_FORWARD_HEADER
