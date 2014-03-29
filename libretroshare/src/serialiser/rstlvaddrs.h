#pragma once

/*
 * libretroshare/src/serialiser: rstlvaddrs.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2010 by Robert Fernie
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

/*******************************************************************
 * These are the Compound TLV structures that must be (un)packed.
 ******************************************************************/

#include "serialiser/rstlvitem.h"
#include "util/rsnet.h"
#include <list>

#include "serialiser/rstlvlist.h"

class RsTlvIpAddress: public RsTlvItem
{
	public:
	 RsTlvIpAddress();
virtual ~RsTlvIpAddress() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	struct sockaddr_storage addr; 			// Mandatory :
};


class RsTlvIpAddressInfo: public RsTlvItem
{
	public:
	 RsTlvIpAddressInfo();
virtual ~RsTlvIpAddressInfo() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	RsTlvIpAddress addr; 				// Mandatory :
	uint64_t  seenTime;				// Mandatory :
	uint32_t  source; 				// Mandatory :
};

typedef t_RsTlvList<RsTlvIpAddressInfo,TLV_TYPE_ADDRESS_SET> RsTlvIpAddrSet;

#if 0
class RsTlvIpAddrSet: public RsTlvItem
{
	public:
	 RsTlvIpAddrSet() { return; }
virtual ~RsTlvIpAddrSet() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvIpAddressInfo> addrs; 		// Mandatory :
};
#endif


