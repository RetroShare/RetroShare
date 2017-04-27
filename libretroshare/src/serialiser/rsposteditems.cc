
/*
 * libretroshare/src/gxs: rsopsteditems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012-2013 by Robert Fernie, Christopher Evi-Parker
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

#include "serialiser/rsposteditems.h"
#include "serialization/rstypeserializer.h"

void RsGxsPostedPostItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_LINK,mPost.mLink,"mPost.mLink") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG ,mPost.mNotes,"mPost.mNotes") ;
}

void RsGxsPostedGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR ,mGroup.mDescription,"mGroup.mDescription") ;
}

RsItem *RsGxsPostedSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != RS_SERVICE_GXS_TYPE_POSTED)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_POSTED_GRP_ITEM: return new RsGxsPostedGroupItem() ;
    case RS_PKT_SUBTYPE_POSTED_POST_ITEM: return new RsGxsPostedPostItem() ;
    default:
		return RsGxsCommentSerialiser::create_item(service_id,item_subtype) ;
    }
}

void RsGxsPostedPostItem::clear()
{
	mPost.mLink.clear();
	mPost.mNotes.clear();
}
void RsGxsPostedGroupItem::clear()
{
	mGroup.mDescription.clear();
}
