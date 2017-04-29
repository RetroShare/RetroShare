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

#include "serialization/rstypeserializer.h"

#define WIRE_DEBUG	1


RsItem *RsGxsWireSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
    if(service != RS_SERVICE_GXS_TYPE_WIRE)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_WIRE_GROUP_ITEM: return new RsGxsWireGroupItem();
    case RS_PKT_SUBTYPE_WIRE_PULSE_ITEM: return new RsGxsWirePulseItem();
    default:
        return NULL ;
    }
}

#ifdef TO_REMOVE
uint32_t RsGxsWireSerialiser::size(RsItem *item)
{
	RsGxsWireGroupItem* grp_item = NULL;
	RsGxsWirePulseItem* snap_item = NULL;

	if((grp_item = dynamic_cast<RsGxsWireGroupItem*>(item)) != NULL)
	{
		return sizeGxsWireGroupItem(grp_item);
	}
	else if((snap_item = dynamic_cast<RsGxsWirePulseItem*>(item)) != NULL)
	{
		return sizeGxsWirePulseItem(snap_item);
	}
    return 0;
}

bool RsGxsWireSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsWireGroupItem* grp_item = NULL;
	RsGxsWirePulseItem* snap_item = NULL;

	if((grp_item = dynamic_cast<RsGxsWireGroupItem*>(item)) != NULL)
	{
		return serialiseGxsWireGroupItem(grp_item, data, size);
	}
	else if((snap_item = dynamic_cast<RsGxsWirePulseItem*>(item)) != NULL)
	{
		return serialiseGxsWirePulseItem(snap_item, data, size);
	}
	return false;
}

RsItem* RsGxsWireSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef WIRE_DEBUG
	std::cerr << "RsGxsWireSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIRE != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_WIRE_GROUP_ITEM:
			return deserialiseGxsWireGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_WIRE_PULSE_ITEM:
			return deserialiseGxsWirePulseItem(data, size);
			break;
		default:
#ifdef WIRE_DEBUG
			std::cerr << "RsGxsWireSerialiser::deserialise(): unknown subtype";
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

void RsGxsWireGroupItem::clear()
{
	group.mDescription.clear();
}

#ifdef TO_REMOVE
std::ostream& RsGxsWireGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsWireGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << group.mDescription << std::endl;
  
	printRsItemEnd(out ,"RsGxsWireGroupItem", indent);
	return out;
}


uint32_t RsGxsWireSerialiser::sizeGxsWireGroupItem(RsGxsWireGroupItem *item)
{

	const RsWireGroup& group = item->group;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mDescription);

	return s;
}

#endif

void RsGxsWireGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,group.mDescription,"group.mDescription") ;
}

#ifdef TO_REMOVE
bool RsGxsWireSerialiser::serialiseGxsWireGroupItem(RsGxsWireGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef WIRE_DEBUG
	std::cerr << "RsGxsWireSerialiser::serialiseGxsWireGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsWireGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWireGroupItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsWireGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DESCR, item->group.mDescription);
	
	if(offset != tlvsize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWireGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef WIRE_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWireGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsWireGroupItem* RsGxsWireSerialiser::deserialiseGxsWireGroupItem(void *data, uint32_t *size)
{
	
#ifdef WIRE_DEBUG
	std::cerr << "RsGxsWireSerialiser::deserialiseGxsWireGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIRE != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_WIRE_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWireGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWireGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsWireGroupItem* item = new RsGxsWireGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DESCR, item->group.mDescription);
	
	if (offset != rssize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWireGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWireGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsWirePulseItem::clear()
{
	pulse.mPulseText.clear();
	pulse.mHashTags.clear();
}

std::ostream& RsGxsWirePulseItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsWirePulseItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Page: " << pulse.mPulseText << std::endl;
  
	printIndent(out, int_Indent);
	out << "HashTags: " << pulse.mHashTags << std::endl;
  
	printRsItemEnd(out ,"RsGxsWirePulseItem", indent);
	return out;
}


uint32_t RsGxsWireSerialiser::sizeGxsWirePulseItem(RsGxsWirePulseItem *item)
{

	const RsWirePulse& pulse = item->pulse;
	uint32_t s = 8; // header

	s += GetTlvStringSize(pulse.mPulseText);
	s += GetTlvStringSize(pulse.mHashTags);

	return s;
}

#endif

void RsGxsWirePulseItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,pulse.mPulseText,"pulse.mPulseText") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG,pulse.mHashTags,"pulse.mHashTags") ;
}

#ifdef TO_REMOVE
bool RsGxsWireSerialiser::serialiseGxsWirePulseItem(RsGxsWirePulseItem *item, void *data, uint32_t *size)
{
	
#ifdef WIRE_DEBUG
	std::cerr << "RsGxsWireSerialiser::serialiseGxsWirePulseItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsWirePulseItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWirePulseItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsWirePulseItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->pulse.mPulseText);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_TAG, item->pulse.mHashTags);
	
	if(offset != tlvsize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWirePulseItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef WIRE_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsWireSerialiser::serialiseGxsWirePulseItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsWirePulseItem* RsGxsWireSerialiser::deserialiseGxsWirePulseItem(void *data, uint32_t *size)
{
	
#ifdef WIRE_DEBUG
	std::cerr << "RsGxsWireSerialiser::deserialiseGxsWirePulseItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_WIRE != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_WIRE_PULSE_ITEM != getRsItemSubType(rstype)))
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWirePulseItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWirePulseItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsWirePulseItem* item = new RsGxsWirePulseItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->pulse.mPulseText);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_TAG, item->pulse.mHashTags);
	
	if (offset != rssize)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWirePulseItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef WIRE_DEBUG
		std::cerr << "RsGxsWireSerialiser::deserialiseGxsWirePulseItem() NOK" << std::endl;
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
