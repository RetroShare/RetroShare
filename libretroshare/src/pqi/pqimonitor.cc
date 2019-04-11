/*******************************************************************************
 * libretroshare/src/pqi: pqimonitor.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie.                                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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

void pqiConnectCbDummy::peerConnectRequest(const RsPeerId &id, const sockaddr_storage &raddr
                                         , const sockaddr_storage &proxyaddr, const sockaddr_storage &srcaddr
                                         , uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth)
{
	std::cerr << "pqiConnectCbDummy::peerConnectRequest()";
	std::cerr << " id: " << id;
	std::cerr << " raddr: " << sockaddr_storage_tostring(raddr);
	std::cerr << " proxyaddr: " << sockaddr_storage_tostring(proxyaddr);
	std::cerr << " srcaddr: " << sockaddr_storage_tostring(srcaddr);
	std::cerr << " source: " << source;
	std::cerr << " flags: " << flags;
	std::cerr << " delay: " << delay;
	std::cerr << " bandwidth: " << bandwidth;
	std::cerr << std::endl;
}

void pqiMonitor::disconnectPeer(const RsPeerId &/*peer*/)
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


