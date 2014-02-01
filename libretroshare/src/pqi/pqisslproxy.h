/*
 * pqisslproxy.h
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



#ifndef MRK_PQI_SSL_PROXY_HEADER
#define MRK_PQI_SSL_PROXY_HEADER

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqissl.h"

 /* pqisslproxy uses SOCKS5 proxy to hidden your own address and connect to peers.
  * It uses the Domain Name interface of SOCKS5, as opposed to an IP address.
  */

class pqisslproxy;
class cert;

/* This provides a NetBinInterface, which is 
 * primarily inherited from pqissl.
 * fns declared here are different -> all others are identical.
 */

class pqisslproxy: public pqissl
{
public:
        pqisslproxy(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm);
virtual ~pqisslproxy();

	// NetInterface. Is the same.
	// BinInterface. Is the same.

virtual bool connect_parameter(uint32_t type, const std::string &value);
virtual bool connect_parameter(uint32_t type, uint32_t value);

protected:

//Initiate is the same - except it uses the Proxy Address rather than the Peer Address.
// minor tweaks to setup data state.
virtual int Initiate_Connection(); 

// The real overloading is done in Basic Connection Complete.
// Instead of just checking for an open socket, we need to communicate with the SOCKS5 proxy.
virtual int Basic_Connection_Complete();

// These are the internal steps in setting up the Proxy Connection.
virtual int Proxy_Send_Method();
virtual int Proxy_Method_Response();
virtual int Proxy_Send_Address();
virtual int Proxy_Connection_Complete();

private:

	uint32_t mProxyState;

	std::string mDomainAddress;
        uint16_t mRemotePort;
};

#endif // MRK_PQI_SSL_PROXY_HEADER
