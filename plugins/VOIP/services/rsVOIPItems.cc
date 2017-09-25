/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <stdexcept>
#include "serialiser/rstypeserializer.h"

#include "services/rsVOIPItems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

RsItem *RsVOIPSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
	if(service != RS_SERVICE_TYPE_VOIP_PLUGIN)
		return NULL ;

	switch(item_subtype)
	{
	case RS_PKT_SUBTYPE_VOIP_PING: 		return new RsVOIPPingItem();
	case RS_PKT_SUBTYPE_VOIP_PONG: 		return new RsVOIPPongItem();
	case RS_PKT_SUBTYPE_VOIP_PROTOCOL: 	return new RsVOIPProtocolItem();
	case RS_PKT_SUBTYPE_VOIP_DATA: 		return new RsVOIPDataItem();
    default:
        return NULL ;
	}
}

void RsVOIPProtocolItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,protocol,"protocol") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,flags   ,"flags") ;
}

void RsVOIPPingItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
}
void RsVOIPPongItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPongTS,"mPongTS") ;
}
void RsVOIPDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,flags   ,"flags") ;

    RsTypeSerializer::TlvMemBlock_proxy prox((uint8_t*&)voip_data,data_size) ;

    RsTypeSerializer::serial_process(j,ctx,prox,"data") ;
}

/*************************************************************************/

