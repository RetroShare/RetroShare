#pragma once

/*
 * libretroshare/src/serialiser: rstlvbanlist.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie
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
#include "serialiser/rstlvaddrs.h"
#include "serialiser/rstlvlist.h"
#include "serialiser/rstlvbase.h"

#define RSDSDV_MAX_ROUTE_TABLE	1000

class RsTlvBanListEntry: public RsTlvItem
{
	public:
	 RsTlvBanListEntry();
virtual ~RsTlvBanListEntry() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	RsTlvIpAddress addr;
	uint32_t level;
	uint32_t reason;
    uint32_t age;
    uint8_t masked_bytes ;
    std::string comment ;
};

typedef t_RsTlvList<RsTlvBanListEntry,TLV_TYPE_BAN_LIST> RsTlvBanList;



