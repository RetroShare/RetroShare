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

	/* from pqiNetListener */
virtual bool resetListener(struct sockaddr_in &local)
	{
		std::cerr << "rsUdpStack::resetListener(" << local << ")";
		std::cerr << std::endl;
		return resetAddress(local);
	}

};

#endif
