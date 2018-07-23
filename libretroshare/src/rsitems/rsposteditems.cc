/*******************************************************************************
 * libretroshare/src/rsitems: rsposteditems.h                                  *
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
#include "rsitems/rsposteditems.h"
#include "serialiser/rstypeserializer.h"

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
