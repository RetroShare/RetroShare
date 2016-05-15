/*
 * libretroshare/src/pqi: pqimonitor.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 Robert Fernie.
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

#include "pqi/pqimonitor.h"
#include "pqi/pqinetwork.h"
#include "pqi/pqiipset.h"
#include "util/rsprint.h"

/***** DUMMY Connect CB for testing *****/

#include <iostream>


pqiConnectCbDummy::pqiConnectCbDummy()
{
	std::cerr << "pqiConnectCbDummy()" << std::endl;
	return;
}

pqiConnectCbDummy::~pqiConnectCbDummy() 
{ 
	return; 
}

void    pqiConnectCbDummy::peerStatus(const RsPeerId& id, const pqiIpAddrSet &addrs,
                       uint32_t type, uint32_t mode, uint32_t source)
{
	std::cerr << "pqiConnectCbDummy::peerStatus()";
	std::cerr << " id: " << id;
	std::cerr << " type: " << type;
	std::cerr << " mode: " << mode;
	std::cerr << " source: " << source;
	std::cerr << std::endl;

	std::cerr << " addrs: ";
	std::cerr << std::endl;
	std::string out;
	addrs.printAddrs(out);
	std::cerr << out << std::endl;
}

void    pqiConnectCbDummy::peerConnectRequest(const RsPeerId& id, 
                        	const struct sockaddr_storage &raddr, uint32_t source)
{
	std::cerr << "pqiConnectCbDummy::peerConnectRequest()";
	std::cerr << " id: " << id;
	std::cerr << " raddr: " << sockaddr_storage_tostring(raddr);
	std::cerr << " source: " << source;
    std::cerr << std::endl;
}

void pqiConnectCbDummy::peerConnectRequest(const RsPeerId &id, const sockaddr_storage &raddr, const sockaddr_storage &, const sockaddr_storage &, uint32_t source, uint32_t, uint32_t, uint32_t) {
    peerConnectRequest(id, raddr, source);
}

void pqiMonitor::disconnectPeer(const RsPeerId &peer)
{
    std::cerr << "(EE) pqiMonitor::disconnectPeer() shouldn't be called!!!"<< std::endl;
}

#if 0
void    pqiConnectCbDummy::stunStatus(std::string id, const struct sockaddr_storage *raddr, 
                                      uint32_t type, uint32_t flags)
{
	std::cerr << "pqiConnectCbDummy::stunStatus()";
	std::cerr << " idhash: " << RsUtil::BinToHex(id) << " raddr: " << sockaddr_storage_tostring(raddr);
	std::cerr << " type: " << type;
	std::cerr << " flags: " << flags;
	std::cerr << std::endl;
}
#endif


