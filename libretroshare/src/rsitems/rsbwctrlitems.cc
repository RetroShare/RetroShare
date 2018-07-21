/*******************************************************************************
 * libretroshare/src/rsitems: rsbwctrlitems.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 by Robert Fernie <retroshare@lunamutt.com>                   *
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



