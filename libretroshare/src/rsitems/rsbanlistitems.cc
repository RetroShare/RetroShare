/*******************************************************************************
 * libretroshare/src/rsitems: rsbanlistitems.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#include "serialiser/rsbaseserial.h"
#include "rsitems/rsbanlistitems.h"

#include "serialiser/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

void 	RsBanListItem::clear()
{
	peerList.TlvClear();
}

void RsBanListItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,peerList,"peerList") ;
}

void RsBanListConfigItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,banListType,"type") ;
    RsTypeSerializer::serial_process          (j,ctx,banListPeerId,"peerId") ;
    RsTypeSerializer::serial_process<rstime_t>  (j,ctx,update_time,"update_time") ;
    RsTypeSerializer::serial_process          (j,ctx,banned_peers,"banned_peers") ;
}
RsItem *RsBanListSerialiser::create_item(uint16_t service_id,uint8_t item_sub_id) const
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


