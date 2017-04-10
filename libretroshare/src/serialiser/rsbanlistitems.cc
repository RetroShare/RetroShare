/*
 * libretroshare/src/serialiser: rsbanlist.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie.
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsbanlistitems.h"

#include "serialization/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

void 	RsBanListItem::clear()
{
	peerList.TlvClear();
}

void RsBanListItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,peerList,"peerList") ;
}

void RsBanListConfigItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,type,"type") ;
    RsTypeSerializer::serial_process          (j,ctx,peerId,"peerId") ;
    RsTypeSerializer::serial_process<time_t>  (j,ctx,update_time,"update_time") ;
    RsTypeSerializer::serial_process          (j,ctx,banned_peers,"banned_peers") ;
}
RsItem *RsBanListSerialiser::create_item(uint16_t service_id,uint8_t item_sub_id)
{
    if(service_id != RS_SERVICE_TYPE_BANLIST)
        return NULL ;

    switch(item_sub_id)
    {
    	case RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM: return new RsBanListConfigItem ;
    	case RS_PKT_SUBTYPE_BANLIST_ITEM:        return new RsBanListItem ;
    default:
        std::cerr << "(EE) unknown item subtype " << (int)item_sub_id << " in RsBanListSerialiser::create_item()" << std::endl;
        return NULL ;
    }
}


