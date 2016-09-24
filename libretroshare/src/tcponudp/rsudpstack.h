#ifndef RS_UDP_STACK_RECEIVER_H
#define RS_UDP_STACK_RECEIVER_H

/*
 * tcponudp/rsudpstack.h
 *
 * libretroshare.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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


/*******************************************************/
#ifdef RS_USE_BITDHT

#include "udp/udpstack.h"


#else

class UdpStack
{
	public:
		UdpStack(int testmode, struct sockaddr_in &local) { return; }
	UdpStack(struct sockaddr_in &local) { return; }

	/* from pqiNetListener */
virtual bool resetAddress(struct sockaddr_in &local) { return false; }

};

#endif
/*******************************************************/

#include "pqi/pqimonitor.h"
#include <iostream>

class rsUdpStack: public UdpStack, public pqiNetListener
{
	public:
	rsUdpStack(struct sockaddr_in &local)
	:UdpStack(local) { return; }

        rsUdpStack(int testmode, struct sockaddr_in &local)
	:UdpStack(testmode, local) { return; }

	/* from pqiNetListener */
virtual bool resetListener(const struct sockaddr_storage &local)
	{
		//std::cerr << "rsUdpStack::resetListener(" << sockaddr_storage_tostring(local) << ")";
		//std::cerr << std::endl;

		if (local.ss_family != AF_INET)
		{
			std::cerr << "rsUdpStack::resetListener() NOT IPv4 ERROR";
			std::cerr << std::endl;
			abort();
		}

		struct sockaddr_in *addr = (struct sockaddr_in *) &local;
		return resetAddress(*addr);
	}

};

class rsFixedUdpStack: public UdpStack, public pqiNetListener
{
	public:
	rsFixedUdpStack(struct sockaddr_in &local)
	:UdpStack(local) { return; }

        rsFixedUdpStack(int testmode, struct sockaddr_in &local)
	:UdpStack(testmode, local) { return; }

	/* from pqiNetListener */
virtual bool resetListener(const struct sockaddr_storage & /* local */)
	{
		struct sockaddr_in addr;
		getLocalAddress(addr);

		// The const_cast below is not so nice but without it, the compiler can't
		// find the correct operator<<(). No idea why!
		//std::cerr << "rsFixedUdpStack::resetListener(" << sockaddr_storage_tostring(local) << ")";
		//std::cerr << " Resetting with original addr: " << const_cast<const struct sockaddr_in &>(addr);
		//std::cerr << std::endl;

		return resetAddress(addr);
	}

};

#endif
