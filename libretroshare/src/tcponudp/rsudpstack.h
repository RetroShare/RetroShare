/*******************************************************************************
 * libretroshare/src/tcponudp: rsudpstack.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare.project@gmail.com>         *
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

#ifndef RS_UDP_STACK_RECEIVER_H
#define RS_UDP_STACK_RECEIVER_H

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
#include "util/rsnet.h"
#include "util/stacktrace.h"

#include <iostream>

class rsUdpStack: public UdpStack, public pqiNetListener
{
public:
	rsUdpStack(struct sockaddr_in &local) : UdpStack(local) {}

	rsUdpStack(int testmode, struct sockaddr_in &local) :
	    UdpStack(testmode, local) {}

	/// @see pqiNetListener
	virtual bool resetListener(const sockaddr_storage& local)
	{
		sockaddr_storage temp;
		sockaddr_storage_copy(local, temp);

		if (!sockaddr_storage_ipv6_to_ipv4(temp))
		{
			std::cerr << __PRETTY_FUNCTION__ << " Got non IPv4 address ERROR"
			          << std::endl;
			sockaddr_storage_dump(local);
			print_stacktrace();
			return -EINVAL;
		}

		sockaddr_in *addr = reinterpret_cast<sockaddr_in*>(&temp);
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
