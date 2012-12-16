/*
 * libretroshare/src/serialiser: rsgxsforumitems.cc
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

#include "rsgxsforumitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define GXSFORUM_DEBUG	1


uint32_t RsGxsForumSerialiser::size(RsItem *item)
{
	RsGxsForumGroupItem* grp_item = NULL;
	RsGxsForumMsgItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsForumGroupItem*>(item)) != NULL)
	{
		return sizeGxsForumGroupItem(grp_item);
	}
	else if((op_item = dynamic_cast<RsGxsForumMsgItem*>(item)) != NULL)
	{
		return sizeGxsForumMsgItem(op_item);
	}
	std::cerr << "RsGxsForumSerialiser::size() ERROR invalid item" << std::endl;
	return 0;
}

bool RsGxsForumSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsForumGroupItem* grp_item = NULL;
	RsGxsForumMsgItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsForumGroupItem*>(item)) != NULL)
	{
		return serialiseGxsForumGroupItem(grp_item, data, size);
	}
	else if((op_item = dynamic_cast<RsGxsForumMsgItem*>(item)) != NULL)
	{
		return serialiseGxsForumMsgItem(op_item, data, size);
	}
	std::cerr << "RsGxsForumSerialiser::serialise() ERROR invalid item" << std::endl;
	return false;
}

RsItem* RsGxsForumSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef GXSFORUM_DEBUG
	std::cerr << "RsGxsForumSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_FORUMS != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM:
			return deserialiseGxsForumGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM:
			return deserialiseGxsForumMsgItem(data, size);
			break;
		default:
#ifdef GXSFORUM_DEBUG
			std::cerr << "RsGxsForumSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsForumGroupItem::clear()
{
	mGroup.mDescription.clear();
}

std::ostream& RsGxsForumGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsForumGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << mGroup.mDescription << std::endl;
  
	printRsItemEnd(out ,"RsGxsForumGroupItem", indent);
	return out;
}


uint32_t RsGxsForumSerialiser::sizeGxsForumGroupItem(RsGxsForumGroupItem *item)
{

	const RsGxsForumGroup& group = item->mGroup;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mDescription);

	return s;
}

bool RsGxsForumSerialiser::serialiseGxsForumGroupItem(RsGxsForumGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSFORUM_DEBUG
	std::cerr << "RsGxsForumSerialiser::serialiseGxsForumGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsForumGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumGroupItem() Size too small" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsForumGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mGroup.mDescription);
	
	if(offset != tlvsize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSFORUM_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsForumGroupItem* RsGxsForumSerialiser::deserialiseGxsForumGroupItem(void *data, uint32_t *size)
{
	
#ifdef GXSFORUM_DEBUG
	std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_FORUMS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsForumGroupItem* item = new RsGxsForumGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->mGroup.mDescription);
	
	if (offset != rssize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsForumMsgItem::clear()
{
	mMsg.mMsg.clear();
}

std::ostream& RsGxsForumMsgItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsForumMsgItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Msg: " << mMsg.mMsg << std::endl;
  
	printRsItemEnd(out ,"RsGxsForumMsgItem", indent);
	return out;
}


uint32_t RsGxsForumSerialiser::sizeGxsForumMsgItem(RsGxsForumMsgItem *item)
{

	const RsGxsForumMsg& msg = item->mMsg;
	uint32_t s = 8; // header

	s += GetTlvStringSize(msg.mMsg); // mMsg.

	return s;
}

bool RsGxsForumSerialiser::serialiseGxsForumMsgItem(RsGxsForumMsgItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSFORUM_DEBUG
	std::cerr << "RsGxsForumSerialiser::serialiseGxsForumMsgItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsForumMsgItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumMsgItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsForumMsgItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mMsg.mMsg);
	
	if(offset != tlvsize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumMsgItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSFORUM_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsForumSerialiser::serialiseGxsForumGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsForumMsgItem* RsGxsForumSerialiser::deserialiseGxsForumMsgItem(void *data, uint32_t *size)
{
	
#ifdef GXSFORUM_DEBUG
	std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumMsgItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_FORUMS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumMsgItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumMsgItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsForumMsgItem* item = new RsGxsForumMsgItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->mMsg.mMsg);
	
	if (offset != rssize)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumMsgItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSFORUM_DEBUG
		std::cerr << "RsGxsForumSerialiser::deserialiseGxsForumMsgItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

