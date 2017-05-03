/*
 * libretroshare/src/serialiser: rshistoryitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Thunder.
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
    RsTypeSerializer::serial_process          (j,ctx,peerId,"peerId") ;
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


