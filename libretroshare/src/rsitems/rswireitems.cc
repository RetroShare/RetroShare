/*
 * libretroshare/src/serialiser: rswikiitems.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
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

void RsGxsWirePulseItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,pulse.mPulseText,"pulse.mPulseText") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG,pulse.mHashTags,"pulse.mHashTags") ;
}

