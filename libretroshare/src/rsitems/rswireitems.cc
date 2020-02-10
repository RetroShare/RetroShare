/*******************************************************************************
 * libretroshare/src/rsitems: rswireitems.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include <iostream>

#include "rswireitems.h"
#include "serialiser/rstypeserializer.h"

#define WIRE_DEBUG	1


RsItem *RsGxsWireSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
    if(service != RS_SERVICE_GXS_TYPE_WIRE)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_WIRE_GROUP_ITEM: return new RsGxsWireGroupItem();
    case RS_PKT_SUBTYPE_WIRE_PULSE_ITEM: return new RsGxsWirePulseItem();
    default:
        return NULL ;
    }
}

void RsGxsWireGroupItem::clear()
{
	group.mDescription.clear();
}

void RsGxsWireGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,group.mDescription,"group.mDescription") ;
}

void RsGxsWirePulseItem::clear()
{
	pulse.mPulseText.clear();
	pulse.mHashTags.clear();
}

void RsGxsWirePulseItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,pulse.mPulseText,"pulse.mPulseText") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG,pulse.mHashTags,"pulse.mHashTags") ;
}

