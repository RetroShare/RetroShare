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

