
/*
 * libretroshare/src/serialiser: rsrttitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011-2013 by Robert Fernie.
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

#include "rsitems/rsrttitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

#include "serialiser/rstypeserializer.h"

/*************************************************************************/

RsItem *RsRttSerialiser::create_item(uint16_t service,uint8_t type) const
{
	if(service != RS_SERVICE_TYPE_RTT)
		return NULL ;

	switch(type)
	{
	case RS_PKT_SUBTYPE_RTT_PING: return new RsRttPingItem() ; //= 0x01;
	case RS_PKT_SUBTYPE_RTT_PONG: return new RsRttPongItem() ; // = 0x02;
	default:
		return NULL ;
	}
}

void RsRttPingItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
}

void RsRttPongItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPongTS,"mPongTS") ;
}



