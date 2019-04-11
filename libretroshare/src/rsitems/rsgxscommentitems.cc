/*******************************************************************************
 * libretroshare/src/rsitems: rsgxscommentitems.cc                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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

