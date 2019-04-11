/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsforumitems.cc                               *
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

    // This is for backward compatibility: normally all members are serialized, but in the previous version, these members are missing.

    if(j == RsGenericSerializer::DESERIALIZE && ctx.mOffset == ctx.mSize)
        return ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,mGroup.mAdminList  ,"admin_list"  ) ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,mGroup.mPinnedPosts,"pinned_posts") ;
}

void RsGxsForumMsgItem::clear()
{
	mMsg.mMsg.clear();
}

void RsGxsForumMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,mMsg.mMsg,"mGroup.Description");
}



