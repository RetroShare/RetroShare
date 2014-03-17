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

#include "rsgxscircleitems.h"
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
		(RS_SERVICE_GXSV2_TYPE_GXSCIRCLE != getRsItemService(rstype)))
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
	pgpIdSet.TlvClear();
	gxsIdSet.TlvClear();
	subCircleSet.TlvClear();
}

bool RsGxsCircleGroupItem::convertFrom(const RsGxsCircleGroup &group)
{
	clear();

	meta = group.mMeta;

	// Enforce the local rules.
	if (meta.mCircleType == GXS_CIRCLE_TYPE_LOCAL)
	{
		std::list<RsPgpId>::const_iterator it = group.mLocalFriends.begin();

		for(; it != group.mLocalFriends.end(); it++)
			pgpIdSet.ids.push_back(it->toStdString());
	}
	else
	{
		std::list<RsGxsId>::const_iterator it = group.mInvitedMembers.begin();
		for(; it != group.mInvitedMembers.end(); it++)
		{
			gxsIdSet.ids.push_back(it->toStdString());
		}
	}
	const std::list<RsGxsCircleId>& scl = group.mSubCircles;

	std::list<RsGxsCircleId>::const_iterator cit = scl.begin();
	subCircleSet.ids.clear();
	for(; cit != scl.end(); cit++)
		subCircleSet.ids.push_back(cit->toStdString());

	return true;
}

bool RsGxsCircleGroupItem::convertTo(RsGxsCircleGroup &group) const
{
	group.mMeta = meta;

	// Enforce the local rules.
	if (meta.mCircleType ==  GXS_CIRCLE_TYPE_LOCAL)
	{
		std::list<std::string>::const_iterator it = pgpIdSet.ids.begin();
		for(; it != pgpIdSet.ids.end();  it++)
			group.mLocalFriends.push_back(RsPgpId(*it));
		group.mInvitedMembers.clear();
	}
	else
	{
		group.mLocalFriends.clear();
		std::list<std::string>::const_iterator cit = gxsIdSet.ids.begin();
		for(; cit != gxsIdSet.ids.end(); cit++)
			group.mInvitedMembers.push_back((RsGxsId(*cit)));

	}

	const std::list<std::string> scs = subCircleSet.ids;
	std::list<std::string>::const_iterator cit = scs.begin();

	group.mSubCircles.clear();

	for(; cit != scs.end(); cit++)
		group.mSubCircles.push_back(RsGxsCircleId(*cit));
	return true;
}


std::ostream& RsGxsCircleGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsCircleGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	if (meta.mCircleType == GXS_CIRCLE_TYPE_LOCAL)
	{
		printRsItemBase(out, "Local Circle: PGP Ids:", indent);
 		pgpIdSet.print(out, int_Indent);
		printRsItemBase(out, "GXS Ids (should be empty):", indent);
 		gxsIdSet.print(out, int_Indent);
	}
	else
	{
		printRsItemBase(out, "External Circle: GXS Ids", indent);
 		gxsIdSet.print(out, int_Indent);
		printRsItemBase(out, "PGP Ids (should be empty):", indent);
 		pgpIdSet.print(out, int_Indent);
	}

 	subCircleSet.print(out, int_Indent);
	printRsItemEnd(out ,"RsGxsCircleGroupItem", indent);
	return out;
}


uint32_t RsGxsCircleSerialiser::sizeGxsCircleGroupItem(RsGxsCircleGroupItem *item)
{
	uint32_t s = 8; // header

	s += item->pgpIdSet.TlvSize();
	s += item->gxsIdSet.TlvSize();
	s += item->subCircleSet.TlvSize();

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
	ok &= item->pgpIdSet.SetTlv(data, tlvsize, &offset);
	ok &= item->gxsIdSet.SetTlv(data, tlvsize, &offset);
	ok &= item->subCircleSet.SetTlv(data, tlvsize, &offset);
	
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
		(RS_SERVICE_GXSV2_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
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
	
	ok &= item->pgpIdSet.GetTlv(data, rssize, &offset);
	ok &= item->gxsIdSet.GetTlv(data, rssize, &offset);
	ok &= item->subCircleSet.GetTlv(data, rssize, &offset);
	
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
	msg.stuff.clear();
}

std::ostream& RsGxsCircleMsgItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsCircleMsgItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Stuff: " << msg.stuff << std::endl;
  
	printRsItemEnd(out ,"RsGxsCircleMsgItem", indent);
	return out;
}


uint32_t RsGxsCircleSerialiser::sizeGxsCircleMsgItem(RsGxsCircleMsgItem *item)
{

	const RsGxsCircleMsg &msg = item->msg;
	uint32_t s = 8; // header

	s += GetTlvStringSize(msg.stuff);

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
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->msg.stuff);
	
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
		(RS_SERVICE_GXSV2_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
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
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->msg.stuff);
	
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

