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

#include "rswikiitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

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

#ifdef TO_REMOVE
uint32_t RsGxsWikiSerialiser::size(RsItem *item)
{
	RsGxsWikiCollectionItem* grp_item = NULL;
	RsGxsWikiSnapshotItem* snap_item = NULL;
        RsGxsWikiCommentItem* com_item = NULL;

	if((grp_item = dynamic_cast<RsGxsWikiCollectionItem*>(item)) != NULL)
	{
		return sizeGxsWikiCollectionItem(grp_item);
	}
	else if((snap_item = dynamic_cast<RsGxsWikiSnapshotItem*>(item)) != NULL)
	{
		return sizeGxsWikiSnapshotItem(snap_item);
	}
	else if((com_item = dynamic_cast<RsGxsWikiCommentItem*>(item)) != NULL)
	{
		return sizeGxsWikiCommentItem(com_item);
	}
    return 0;
}

bool RsGxsWikiSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsWikiCollectionItem* grp_item = NULL;
	RsGxsWikiSnapshotItem* snap_item = NULL;
        RsGxsWikiCommentItem* com_item = NULL;

	if((grp_item = dynamic_cast<RsGxsWikiCollectionItem*>(item)) != NULL)
	{
		return serialiseGxsWikiCollectionItem(grp_item, data, size);
	}
	else if((snap_item = dynamic_cast<RsGxsWikiSnapshotItem*>(item)) != NULL)
	{
		return serialiseGxsWikiSnapshotItem(snap_item, data, size);
	}
	else if((com_item = dynamic_cast<RsGxsWikiCommentItem*>(item)) != NULL)
	{
		return serialiseGxsWikiCommentItem(com_item, data, size);
	}
	return false;
}

RsItem* RsGxsWikiSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIKI != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM:
			return deserialiseGxsWikiCollectionItem(data, size);
			break;
		case RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM:
			return deserialiseGxsWikiSnapshotItem(data, size);
			break;
		case RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM:
			return deserialiseGxsWikiCommentItem(data, size);
			break;
		default:
#ifdef GXSID_DEBUG
			std::cerr << "RsGxsWikiSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#endif

void RsGxsWikiCollectionItem::clear()
{
	collection.mDescription.clear();
	collection.mCategory.clear();
	collection.mHashTags.clear();
}

#ifdef TO_REMOVE
std::ostream& RsGxsWikiCollectionItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsWikiCollectionItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << collection.mDescription << std::endl;
	printIndent(out, int_Indent);
	out << "Category: " << collection.mCategory << std::endl;
	printIndent(out, int_Indent);
	out << "HashTags: " << collection.mHashTags << std::endl;
  
	printRsItemEnd(out ,"RsGxsWikiCollectionItem", indent);
	return out;
}


uint32_t RsGxsWikiSerialiser::sizeGxsWikiCollectionItem(RsGxsWikiCollectionItem *item)
{

	const RsWikiCollection& collection = item->collection;
	uint32_t s = 8; // header

	s += GetTlvStringSize(collection.mDescription);
	s += GetTlvStringSize(collection.mCategory);
	s += GetTlvStringSize(collection.mHashTags);

	return s;
}
#endif

void RsGxsWikiCollectionItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR   ,collection.mDescription,"collection.mDescription") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_CATEGORY,collection.mCategory   ,"collection.mCategory") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG,collection.mHashTags   ,"collection.mHashTags") ;
}

#ifdef TO_REMOVE
bool RsGxsWikiSerialiser::serialiseGxsWikiCollectionItem(RsGxsWikiCollectionItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCollectionItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsWikiCollectionItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCollectionItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsWikiCollectionItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DESCR, item->collection.mDescription);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_CATEGORY, item->collection.mCategory);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_TAG, item->collection.mHashTags);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCollectionItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCollectionItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsWikiCollectionItem* RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem(void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIKI != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsWikiCollectionItem* item = new RsGxsWikiCollectionItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DESCR, item->collection.mDescription);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_CATEGORY, item->collection.mCategory);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_TAG, item->collection.mHashTags);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCollectionItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#endif

void RsGxsWikiSnapshotItem::clear()
{
	snapshot.mPage.clear();
	snapshot.mHashTags.clear();
}

#ifdef TO_REMOVE
std::ostream& RsGxsWikiSnapshotItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsWikiSnapshotItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Page: " << snapshot.mPage << std::endl;
  
	printIndent(out, int_Indent);
	out << "HashTags: " << snapshot.mHashTags << std::endl;
  
	printRsItemEnd(out ,"RsGxsWikiSnapshotItem", indent);
	return out;
}


uint32_t RsGxsWikiSerialiser::sizeGxsWikiSnapshotItem(RsGxsWikiSnapshotItem *item)
{

	const RsWikiSnapshot& snapshot = item->snapshot;
	uint32_t s = 8; // header

	s += GetTlvStringSize(snapshot.mPage);
	s += GetTlvStringSize(snapshot.mHashTags);

	return s;
}
#endif

void RsGxsWikiSnapshotItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_WIKI_PAGE,snapshot.mPage,"snapshot.mPage") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG ,snapshot.mPage,"snapshot.mHashTags") ;
}

#ifdef TO_REMOVE
bool RsGxsWikiSerialiser::serialiseGxsWikiSnapshotItem(RsGxsWikiSnapshotItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiSnapshotItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsWikiSnapshotItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiSnapshotItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsWikiSnapshotItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_WIKI_PAGE, item->snapshot.mPage);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_TAG, item->snapshot.mHashTags);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiSnapshotItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiSnapshotItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsWikiSnapshotItem* RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem(void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIKI != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsWikiSnapshotItem* item = new RsGxsWikiSnapshotItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_WIKI_PAGE, item->snapshot.mPage);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_TAG, item->snapshot.mHashTags);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiSnapshotItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#endif

void RsGxsWikiCommentItem::clear()
{
	comment.mComment.clear();
}

#ifdef TO_REMOVE
std::ostream& RsGxsWikiCommentItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsWikiCommentItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Comment: " << comment.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsWikiCommentItem", indent);
	return out;
}


uint32_t RsGxsWikiSerialiser::sizeGxsWikiCommentItem(RsGxsWikiCommentItem *item)
{

	const RsWikiComment& comment = item->comment;
	uint32_t s = 8; // header

	s += GetTlvStringSize(comment.mComment);

	return s;
}
#endif

void RsGxsWikiCommentItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_COMMENT,comment.mComment,"comment.mComment") ;
}

#ifdef TO_REMOVE
bool RsGxsWikiSerialiser::serialiseGxsWikiCommentItem(RsGxsWikiCommentItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCommentItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsWikiCommentItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCommentItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsWikiCommentItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCommentItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsWikiSerialiser::serialiseGxsWikiCommentItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsWikiCommentItem* RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem(void *data, uint32_t *size)
{
	
#ifdef GXSID_DEBUG
	std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIKI != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsWikiCommentItem* item = new RsGxsWikiCommentItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsWikiSerialiser::deserialiseGxsWikiCommentItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


#endif
