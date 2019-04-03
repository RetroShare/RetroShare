/*******************************************************************************
 * libretroshare/src/rsitems: rswikiitems.cc                                   *
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

#include "rsitems/rswikiitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstypeserializer.h"

#define GXSID_DEBUG	1

RsItem *RsGxsWikiSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_GXS_TYPE_WIKI)
        return NULL ;

    switch(item_sub_id)
    {
    case RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM: return new RsGxsWikiCollectionItem();
    case RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM:    return new RsGxsWikiCommentItem();
    case RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM:   return new RsGxsWikiSnapshotItem();
    default:
        return NULL ;
    }
}

void RsGxsWikiCollectionItem::clear()
{
	collection.mDescription.clear();
	collection.mCategory.clear();
	collection.mHashTags.clear();
}

void RsGxsWikiCollectionItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR   ,collection.mDescription,"collection.mDescription") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_CATEGORY,collection.mCategory   ,"collection.mCategory") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG,collection.mHashTags   ,"collection.mHashTags") ;
}

void RsGxsWikiSnapshotItem::clear()
{
	snapshot.mPage.clear();
	snapshot.mHashTags.clear();
}

void RsGxsWikiSnapshotItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_WIKI_PAGE,snapshot.mPage,"snapshot.mPage") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG ,snapshot.mPage,"snapshot.mHashTags") ;
}

void RsGxsWikiCommentItem::clear()
{
	comment.mComment.clear();
}

void RsGxsWikiCommentItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_COMMENT,comment.mComment,"comment.mComment") ;
}

