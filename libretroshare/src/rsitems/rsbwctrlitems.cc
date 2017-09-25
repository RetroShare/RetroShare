/*
 * libretroshare/src/serialiser: rsbwctrlitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 by Robert Fernie.
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

#include "serialiser/rsbaseserial.h"
#include "rsitems/rsbwctrlitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsItem *RsBwCtrlSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_TYPE_BWCTRL)
        return NULL ;

    switch(item_sub_id)
	{
	case RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM: return new RsBwCtrlAllowedItem();
	default:
		return NULL;
	}
}

void 	RsBwCtrlAllowedItem::clear()
{
	allowedBw = 0;
}

void RsBwCtrlAllowedItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,TLV_TYPE_UINT32_BW,allowedBw,"allowedBw") ;
}



