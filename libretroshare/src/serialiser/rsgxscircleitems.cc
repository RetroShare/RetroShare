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

#include "rswireitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define CIRCLE_DEBUG	1


uint32_t RsGxsCircleSerialiser::size(RsItem *item)
{
	RsGxsCircleGroupItem* grp_item = NULL;
	RsGxsCircleMsgItem* snap_item = NULL;

	if((grp_item = dynamic_cast<RsGxsCircleGroupItem*>(item)) != NULL)
	{
		return sizeGxsCircleGroupItem(grp_item);
	}
	else if((snap_item = dynamic_cast<RsGxsCircleMsgItem*>(item)) != NULL)
	{
		return sizeGxsCircleMsgItem(snap_item);
	}
	return NULL;
}

bool RsGxsCircleSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsCircleGroupItem* grp_item = NULL;
	RsGxsCircleMsgItem* snap_item = NULL;

	if((grp_item = dynamic_cast<RsGxsCircleGroupItem*>(item)) != NULL)
	{
		return serialiseGxsCircleGroupItem(grp_item, data, size);
	}
	else if((snap_item = dynamic_cast<RsGxsCircleMsgItem*>(item)) != NULL)
	{
		return serialiseGxsCircleMsgItem(snap_item, data, size);
	}
	return false;
}

RsItem* RsGxsCircleSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_GXSCIRCLE != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSCIRCLE_GROUP_ITEM:
			return deserialiseGxsCircleGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSCIRCLE_MSG_ITEM:
			return deserialiseGxsCircleMsgItem(data, size);
			break;
		default:
#ifdef CIRCLE_DEBUG
			std::cerr << "RsGxsCircleSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsCircleGroupItem::clear()
{
	group.mDescription.clear();
}

std::ostream& RsGxsCircleGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsCircleGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << group.mDescription << std::endl;
  
	printRsItemEnd(out ,"RsGxsCircleGroupItem", indent);
	return out;
}


uint32_t RsGxsCircleSerialiser::sizeGxsCircleGroupItem(RsGxsCircleGroupItem *item)
{

	const RsCircleGroup& group = item->group;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mDescription);

	return s;
}

bool RsGxsCircleSerialiser::serialiseGxsCircleGroupItem(RsGxsCircleGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsCircleGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleGroupItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsCircleGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->group.mDescription);
	
	if(offset != tlvsize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef CIRCLE_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsCircleGroupItem* RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem(void *data, uint32_t *size)
{
	
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCIRCLE_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsCircleGroupItem* item = new RsGxsCircleGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->group.mDescription);
	
	if (offset != rssize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsCircleMsgItem::clear()
{
	pulse.mPulseText.clear();
	pulse.mHashTags.clear();
}

std::ostream& RsGxsCircleMsgItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsCircleMsgItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Page: " << pulse.mPulseText << std::endl;
  
	printIndent(out, int_Indent);
	out << "HashTags: " << pulse.mHashTags << std::endl;
  
	printRsItemEnd(out ,"RsGxsCircleMsgItem", indent);
	return out;
}


uint32_t RsGxsCircleSerialiser::sizeGxsCircleMsgItem(RsGxsCircleMsgItem *item)
{

	const RsCircleMsg& pulse = item->pulse;
	uint32_t s = 8; // header

	s += GetTlvStringSize(pulse.mPulseText);
	s += GetTlvStringSize(pulse.mHashTags);

	return s;
}

bool RsGxsCircleSerialiser::serialiseGxsCircleMsgItem(RsGxsCircleMsgItem *item, void *data, uint32_t *size)
{
	
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleMsgItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsCircleMsgItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleMsgItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsCircleMsgItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->pulse.mPulseText);
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->pulse.mHashTags);
	
	if(offset != tlvsize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleMsgItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef CIRCLE_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleMsgItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsCircleMsgItem* RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem(void *data, uint32_t *size)
{
	
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV1_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCIRCLE_MSG_ITEM != getRsItemSubType(rstype)))
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsCircleMsgItem* item = new RsGxsCircleMsgItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, 1, item->pulse.mPulseText);
	ok &= GetTlvString(data, rssize, &offset, 1, item->pulse.mHashTags);
	
	if (offset != rssize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleMsgItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

