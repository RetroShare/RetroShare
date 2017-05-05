/*
 * libretroshare/src/serialiser: rsgxsforumitems.cc
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

#include "rsgxsforumitems.h"

#include "serialiser/rstypeserializer.h"

//#define GXSFORUM_DEBUG	1

RsItem *RsGxsForumSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != RS_SERVICE_GXS_TYPE_FORUMS)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM: return new RsGxsForumGroupItem();
    case RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM: return new RsGxsForumMsgItem();
    default:
        return NULL ;
    }
}
void RsGxsForumGroupItem::clear()
{
	mGroup.mDescription.clear();
}

void RsGxsForumGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,mGroup.mDescription,"mGroup.Description");
}
void RsGxsForumMsgItem::clear()
{
	mMsg.mMsg.clear();
}

void RsGxsForumMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,mMsg.mMsg,"mGroup.Description");
}



