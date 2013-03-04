/*
 * libretroshare/src/serialiser: rsgxschannelitems.cc
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

#include "rsgxschannelitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define GXSCHANNEL_DEBUG	1


uint32_t RsGxsChannelSerialiser::size(RsItem *item)
{
	RsGxsChannelGroupItem* grp_item = NULL;
	RsGxsChannelPostItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsChannelGroupItem*>(item)) != NULL)
	{
		return sizeGxsChannelGroupItem(grp_item);
	}
	else if((op_item = dynamic_cast<RsGxsChannelPostItem*>(item)) != NULL)
	{
		return sizeGxsChannelPostItem(op_item);
	}
	else
	{
		RsGxsCommentSerialiser::size(item);
	}
	return 0;
}

bool RsGxsChannelSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsChannelGroupItem* grp_item = NULL;
	RsGxsChannelPostItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsChannelGroupItem*>(item)) != NULL)
	{
		return serialiseGxsChannelGroupItem(grp_item, data, size);
	}
	else if((op_item = dynamic_cast<RsGxsChannelPostItem*>(item)) != NULL)
	{
		return serialiseGxsChannelPostItem(op_item, data, size);
	}
	else
	{
		return RsGxsCommentSerialiser::serialise(item, data, size);
	}
	return false;
}

RsItem* RsGxsChannelSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_CHANNELS != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM:
			return deserialiseGxsChannelGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM:
			return deserialiseGxsChannelPostItem(data, size);
			break;
		default:
			return RsGxsCommentSerialiser::deserialise(data, size);
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsChannelGroupItem::clear()
{
	mGroup.mDescription.clear();
}

std::ostream& RsGxsChannelGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsChannelGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << mGroup.mDescription << std::endl;
  
	printRsItemEnd(out ,"RsGxsChannelGroupItem", indent);
	return out;
}


uint32_t RsGxsChannelSerialiser::sizeGxsChannelGroupItem(RsGxsChannelGroupItem *item)
{

	const RsGxsChannelGroup& group = item->mGroup;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mDescription);

	return s;
}

bool RsGxsChannelSerialiser::serialiseGxsChannelGroupItem(RsGxsChannelGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsChannelGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() Size too small" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsChannelGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mGroup.mDescription);
	
	if(offset != tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCHANNEL_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsChannelGroupItem* RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem(void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_CHANNELS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsChannelGroupItem* item = new RsGxsChannelGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->mGroup.mDescription);
	
	if (offset != rssize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsChannelPostItem::clear()
{
	mMsg.mMsg.clear();
}

std::ostream& RsGxsChannelPostItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsChannelPostItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Msg: " << mMsg.mMsg << std::endl;
  
	printRsItemEnd(out ,"RsGxsChannelPostItem", indent);
	return out;
}


uint32_t RsGxsChannelSerialiser::sizeGxsChannelPostItem(RsGxsChannelPostItem *item)
{

	const RsGxsChannelPost& msg = item->mMsg;
	uint32_t s = 8; // header

	s += GetTlvStringSize(msg.mMsg); // mMsg.

	return s;
}

bool RsGxsChannelSerialiser::serialiseGxsChannelPostItem(RsGxsChannelPostItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsChannelPostItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsChannelPostItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mMsg.mMsg);
	
	if(offset != tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCHANNEL_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsChannelPostItem* RsGxsChannelSerialiser::deserialiseGxsChannelPostItem(void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_CHANNELS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsChannelPostItem* item = new RsGxsChannelPostItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->mMsg.mMsg);
	
	if (offset != rssize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

