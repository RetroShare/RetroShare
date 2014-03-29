/*
 * libretroshare/src/serialiser: rsgxsiditems.cc
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

#include "rsgxsiditems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvstring.h"

#define GXSID_DEBUG	1


uint32_t RsGxsIdSerialiser::size(RsItem *item)
{
	RsGxsIdGroupItem* grp_item = NULL;
#if 0
	RsGxsIdOpinionItem* op_item = NULL;
        RsGxsIdCommentItem* com_item = NULL;
#endif

	if((grp_item = dynamic_cast<RsGxsIdGroupItem*>(item)) != NULL)
	{
		return sizeGxsIdGroupItem(grp_item);
	}
#if 0
	else if((op_item = dynamic_cast<RsGxsIdOpinionItem*>(item)) != NULL)
	{
		return sizeGxsIdOpinionItem(op_item);
	}
	else if((com_item = dynamic_cast<RsGxsIdCommentItem*>(item)) != NULL)
	{
		return sizeGxsIdCommentItem(com_item);
	}
#endif
	std::cerr << "RsGxsIdSerialiser::size() ERROR invalid item" << std::endl;
	return 0;
}

bool RsGxsIdSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsIdGroupItem* grp_item = NULL;
#if 0
	RsGxsIdOpinionItem* op_item = NULL;
        RsGxsIdCommentItem* com_item = NULL;
#endif

	if((grp_item = dynamic_cast<RsGxsIdGroupItem*>(item)) != NULL)
	{
		return serialiseGxsIdGroupItem(grp_item, data, size);
	}
#if 0
	else if((op_item = dynamic_cast<RsGxsIdOpinionItem*>(item)) != NULL)
	{
		return serialiseGxsIdOpinionItem(op_item, data, size);
	}
	else if((com_item = dynamic_cast<RsGxsIdCommentItem*>(item)) != NULL)
	{
		return serialiseGxsIdCommentItem(com_item, data, size);
	}
#endif
	std::cerr << "RsGxsIdSerialiser::serialise() ERROR invalid item" << std::endl;
	return false;
}

RsItem* RsGxsIdSerialiser::deserialise(void* data, uint32_t* size)
{
		
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSID_GROUP_ITEM:
			return deserialiseGxsIdGroupItem(data, size);
			break;
#if 0
		case RS_PKT_SUBTYPE_GXSID_OPINION_ITEM:
			return deserialiseGxsIdOpinionItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM:
			return deserialiseGxsIdCommentItem(data, size);
			break;
#endif
		default:
#ifdef GXSID_DEBUG
			std::cerr << "RsGxsIdSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsIdGroupItem::clear()
{
	group.mPgpIdHash.clear();
	group.mPgpIdSign.clear();

	group.mRecognTags.clear();

        group.mPgpKnown = false;
        group.mPgpId.clear();

}


std::ostream& RsGxsIdGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "MetaData: " << meta << std::endl;
	printIndent(out, int_Indent);
	out << "PgpIdHash: " << group.mPgpIdHash << std::endl;
	printIndent(out, int_Indent);
	out << "PgpIdSign: " << group.mPgpIdSign << std::endl;
	printIndent(out, int_Indent);
	out << "RecognTags:" << std::endl;

	RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, group.mRecognTags);
	set.print(out, int_Indent + 2);
  
	printRsItemEnd(out ,"RsGxsIdGroupItem", indent);
	return out;
}


uint32_t RsGxsIdSerialiser::sizeGxsIdGroupItem(RsGxsIdGroupItem *item)
{

	const RsGxsIdGroup& group = item->group;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mPgpIdHash);
	s += GetTlvStringSize(group.mPgpIdSign);

	RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, item->group.mRecognTags);
	s += set.TlvSize();

	return s;
}

bool RsGxsIdSerialiser::serialiseGxsIdGroupItem(RsGxsIdGroupItem *item, void *data, uint32_t *size)
{
	
	
	uint32_t tlvsize = sizeGxsIdGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdGroupItem() Size too small" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsIdGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, item->group.mPgpIdHash);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_SIGN, item->group.mPgpIdSign);

	RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, item->group.mRecognTags);
	ok &= set.SetTlv(data, tlvsize, &offset);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsIdGroupItem* RsGxsIdSerialiser::deserialiseGxsIdGroupItem(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSID_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsIdGroupItem* item = new RsGxsIdGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_SHA1, item->group.mPgpIdHash);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_SIGN, item->group.mPgpIdSign);

	RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, item->group.mRecognTags);
	ok &= set.GetTlv(data, rssize, &offset);
	
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#if 0

void RsGxsIdOpinionItem::clear()
{
	opinion.mOpinion = 0;
	opinion.mReputation = 0;
	opinion.mComment = "";
}

std::ostream& RsGxsIdOpinionItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdOpinionItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Opinion: " << opinion.mOpinion << std::endl;
	printIndent(out, int_Indent);
	out << "Reputation: " << opinion.mReputation << std::endl;
	printIndent(out, int_Indent);
	out << "Comment: " << opinion.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsIdOpinionItem", indent);
	return out;
}


uint32_t RsGxsIdSerialiser::sizeGxsIdOpinionItem(RsGxsIdOpinionItem *item)
{

	const RsGxsIdOpinion& opinion = item->opinion;
	uint32_t s = 8; // header

	s += 4; // mOpinion.
	s += 4; // mReputation.
	s += GetTlvStringSize(opinion.mComment);

	return s;
}

bool RsGxsIdSerialiser::serialiseGxsIdOpinionItem(RsGxsIdOpinionItem *item, void *data, uint32_t *size)
{
	
	
	uint32_t tlvsize = sizeGxsIdOpinionItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdOpinionItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsIdOpinionItem */
	ok &= setRawUInt32(data, tlvsize, &offset, item->opinion.mOpinion);
	ok &= setRawUInt32(data, tlvsize, &offset, item->opinion.mReputation);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->opinion.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdOpinionItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsIdOpinionItem* RsGxsIdSerialiser::deserialiseGxsIdOpinionItem(void *data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSID_OPINION_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsIdOpinionItem* item = new RsGxsIdOpinionItem();
	/* skip the header */
	offset += 8;
	
	ok &= getRawUInt32(data, rssize, &offset, &(item->opinion.mOpinion));
	ok &= getRawUInt32(data, rssize, &offset, &(item->opinion.mReputation));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->opinion.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsIdCommentItem::clear()
{
	comment.mComment.clear();
}

std::ostream& RsGxsIdCommentItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdCommentItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Comment: " << comment.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsIdCommentItem", indent);
	return out;
}


uint32_t RsGxsIdSerialiser::sizeGxsIdCommentItem(RsGxsIdCommentItem *item)
{

	const RsGxsIdComment& comment = item->comment;
	uint32_t s = 8; // header

	s += GetTlvStringSize(comment.mComment);

	return s;
}

bool RsGxsIdSerialiser::serialiseGxsIdCommentItem(RsGxsIdCommentItem *item, void *data, uint32_t *size)
{
	
	uint32_t tlvsize = sizeGxsIdCommentItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdCommentItem() Not enough space" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsIdCommentItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdCommentItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
}
	
RsGxsIdCommentItem* RsGxsIdSerialiser::deserialiseGxsIdCommentItem(void *data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsIdCommentItem* item = new RsGxsIdCommentItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

#endif

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

