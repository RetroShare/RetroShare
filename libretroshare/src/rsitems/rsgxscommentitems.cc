/*
 * libretroshare/src/serialiser: rsgxscommentitems.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2013 by Robert Fernie
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

#include "rsgxscommentitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstypeserializer.h"

//#define GXSCOMMENT_DEBUG	1

RsItem *RsGxsCommentSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != getRsItemService(PacketId()))
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM: return new RsGxsCommentItem(getRsItemService(PacketId())) ;
    case RS_PKT_SUBTYPE_GXSCOMMENT_VOTE_ITEM:    return new RsGxsVoteItem(getRsItemService(PacketId()));
    default:
		return NULL ;
    }
}


void RsGxsCommentItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,1,mMsg.mComment,"mMsg.mComment") ;
}

void RsGxsVoteItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mMsg.mVoteType,"mMsg.mVoteType") ;
}

