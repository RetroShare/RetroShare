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

#define GXSCOMMENT_DEBUG	1


uint32_t RsGxsCommentSerialiser::size(RsItem *item)
{
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::size()" << std::endl;
#endif

	RsGxsCommentItem* com_item = NULL;
	RsGxsVoteItem* vote_item = NULL;

	if((com_item = dynamic_cast<RsGxsCommentItem*>(item)) != NULL)
	{
		return sizeGxsCommentItem(com_item);
	}
	else if((vote_item = dynamic_cast<RsGxsVoteItem*>(item)) != NULL)
	{
		return sizeGxsVoteItem(vote_item);
	}
	std::cerr << "RsGxsCommentSerialiser::size() ERROR invalid item" << std::endl;
	return 0;
}

bool RsGxsCommentSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::serialise()" << std::endl;
#endif

	RsGxsCommentItem* com_item = NULL;
	RsGxsVoteItem* vote_item = NULL;

	if((com_item = dynamic_cast<RsGxsCommentItem*>(item)) != NULL)
	{
		return serialiseGxsCommentItem(com_item, data, size);
	}
	else if((vote_item = dynamic_cast<RsGxsVoteItem*>(item)) != NULL)
	{
		return serialiseGxsVoteItem(vote_item, data, size);
	}
	std::cerr << "RsGxsCommentSerialiser::serialise() ERROR invalid item" << std::endl;
	return false;
}

RsItem* RsGxsCommentSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(getRsItemService(PacketId()) != getRsItemService(rstype)))
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialise() ERROR Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM:
			return deserialiseGxsCommentItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSCOMMENT_VOTE_ITEM:
			return deserialiseGxsVoteItem(data, size);
			break;
		default:
#ifdef GXSCOMMENT_DEBUG
			std::cerr << "RsGxsCommentSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}




/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsCommentItem::clear()
{
	mMsg.mComment.clear();
}

std::ostream& RsGxsCommentItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsCommentItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Comment: " << mMsg.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsCommentItem", indent);
	return out;
}


uint32_t RsGxsCommentSerialiser::sizeGxsCommentItem(RsGxsCommentItem *item)
{

	const RsGxsComment& msg = item->mMsg;
	uint32_t s = 8; // header

	s += GetTlvStringSize(msg.mComment); // mMsg.

#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::sizeGxsCommentItem() is: " << s << std::endl;
#endif

	return s;
}

bool RsGxsCommentSerialiser::serialiseGxsCommentItem(RsGxsCommentItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::serialiseGxsCommentItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsCommentItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsCommentItem() Failed size too small" << std::endl;
#endif

		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsCommentItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mMsg.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsCommentItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCOMMENT_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsCommentItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsCommentItem* RsGxsCommentSerialiser::deserialiseGxsCommentItem(void *data, uint32_t *size)
{
	
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::deserialiseGxsCommentItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(getRsItemService(PacketId()) != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsCommentItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsCommentItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsCommentItem* item = new RsGxsCommentItem(getRsItemService(PacketId()));
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->mMsg.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsCommentItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsCommentItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsVoteItem::clear()
{
	mMsg.mVoteType = 0;
}

std::ostream& RsGxsVoteItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsVoteItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "VoteType: " << mMsg.mVoteType << std::endl;
  
	printRsItemEnd(out ,"RsGxsVoteItem", indent);
	return out;
}


uint32_t RsGxsCommentSerialiser::sizeGxsVoteItem(RsGxsVoteItem *item)
{

	const RsGxsVote& msg = item->mMsg;
	uint32_t s = 8; // header

	s += 4; // vote flags.

	return s;
}

bool RsGxsCommentSerialiser::serialiseGxsVoteItem(RsGxsVoteItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::serialiseGxsVoteItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsVoteItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsVoteItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsVoteItem */
        ok &= setRawUInt32(data, tlvsize, &offset, item->mMsg.mVoteType);
	
	if(offset != tlvsize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsVoteItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCOMMENT_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsCommentSerialiser::serialiseGxsVoteItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsVoteItem* RsGxsCommentSerialiser::deserialiseGxsVoteItem(void *data, uint32_t *size)
{
	
#ifdef GXSCOMMENT_DEBUG
	std::cerr << "RsGxsCommentSerialiser::deserialiseGxsVoteItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(getRsItemService(PacketId()) != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCOMMENT_VOTE_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsVoteItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsVoteItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsVoteItem* item = new RsGxsVoteItem(getRsItemService(PacketId()));
	/* skip the header */
	offset += 8;

        ok &= getRawUInt32(data, rssize, &offset, &(item->mMsg.mVoteType));
	
	if (offset != rssize)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsVoteItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCOMMENT_DEBUG
		std::cerr << "RsGxsCommentSerialiser::deserialiseGxsVoteItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

