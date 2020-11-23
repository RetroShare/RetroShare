/*******************************************************************************
 * libretroshare/src/rsitems: rsrttitems.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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



