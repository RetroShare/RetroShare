/*******************************************************************************
 * libretroshare/src/rsitems: rshistoryitems.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Thunder.                                             *
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
#include "rsitems/rshistoryitems.h"
#include "rsitems/rsconfigitems.h"

#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#include "serialiser/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

void RsHistoryMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    uint16_t version=0;

    RsTypeSerializer::serial_process<uint16_t>(j,ctx,version,"version") ;
    RsTypeSerializer::serial_process          (j,ctx,chatPeerId,"chatPeerId") ;
    RsTypeSerializer::serial_process<bool>    (j,ctx,incoming,"incoming") ;
    RsTypeSerializer::serial_process          (j,ctx,msgPeerId,"peerId") ;
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_NAME,peerName,"peerName") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,sendTime,"sendTime") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,recvTime,"recvTime") ;
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_MSG,message,"message") ;
}

RsItem *RsHistorySerialiser::create_item(uint8_t item_type,uint8_t item_subtype) const
{
    if(item_type != RS_PKT_TYPE_HISTORY_CONFIG)
        return NULL ;

    if(item_subtype == RS_PKT_SUBTYPE_DEFAULT)
        return new RsHistoryMsgItem();

    return NULL ;
}


RsHistoryMsgItem::RsHistoryMsgItem() : RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_HISTORY_CONFIG, RS_PKT_SUBTYPE_DEFAULT)
{
	incoming = false;
	sendTime = 0;
	recvTime = 0;
	msgId = 0;
	saveToDisc = true;
}


