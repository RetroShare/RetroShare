/*******************************************************************************
 * libretroshare/src/serialiser: rstlvaddr.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#pragma once

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


