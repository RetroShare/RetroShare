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

//#define CIRCLE_DEBUG	1

uint32_t RsGxsCircleSerialiser::size(RsItem *item)
{
	RsGxsCircleGroupItem* grp_item = NULL;
	RsGxsCircleMsgItem* snap_item = NULL;
	RsGxsCircleSubscriptionRequestItem* subr_item = NULL;

	if((grp_item = dynamic_cast<RsGxsCircleGroupItem*>(item)) != NULL)
	{
		return sizeGxsCircleGroupItem(grp_item);
	}
	else if((snap_item = dynamic_cast<RsGxsCircleMsgItem*>(item)) != NULL)
	{
		return sizeGxsCircleMsgItem(snap_item);
    }
	else if((subr_item = dynamic_cast<RsGxsCircleSubscriptionRequestItem*>(item)) != NULL)
	{
		return sizeGxsCircleSubscriptionRequestItem(subr_item);
    }
    else
        return 0 ;
}

bool RsGxsCircleSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsCircleGroupItem* grp_item = NULL;
	RsGxsCircleMsgItem* snap_item = NULL;
	RsGxsCircleSubscriptionRequestItem* subr_item = NULL;

	if((grp_item = dynamic_cast<RsGxsCircleGroupItem*>(item)) != NULL)
	{
		return serialiseGxsCircleGroupItem(grp_item, data, size);
	}
	else if((snap_item = dynamic_cast<RsGxsCircleMsgItem*>(item)) != NULL)
	{
		return serialiseGxsCircleMsgItem(snap_item, data, size);
	}
	else if((subr_item = dynamic_cast<RsGxsCircleSubscriptionRequestItem*>(item)) != NULL)
	{
		return serialiseGxsCircleSubscriptionRequestItem(subr_item, data, size);
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
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_GXS_TYPE_GXSCIRCLE != getRsItemService(rstype)))
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
		case RS_PKT_SUBTYPE_GXSCIRCLE_SUBSCRIPTION_REQUEST_ITEM:
			return deserialiseGxsCircleSubscriptionRequestItem(data, size);
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

void RsGxsCircleSubscriptionRequestItem::clear()
{
    time_stamp = 0 ;
    time_out   = 0 ;
    subscription_type = SUBSCRIPTION_REQUEST_UNKNOWN;
}

std::ostream& RsGxsCircleSubscriptionRequestItem::print(std::ostream& out, uint16_t indent)
{ 
	printRsItemBase(out, "RsGxsCircleSubscriptionRequestItem", indent);
	uint16_t int_Indent = indent + 2;

	printRsItemBase(out, "time stmp: ", indent); out << time_stamp ;
	printRsItemBase(out, "time out : ", indent); out << time_out ;
	printRsItemBase(out, "Subs type: ", indent); out << subscription_type ;

	printRsItemEnd(out ,"RsGxsCircleSubscriptionRequestItem", indent);
	return out;
}

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
		pgpIdSet.ids = group.mLocalFriends;
	}
	else
	{
		gxsIdSet.ids = group.mInvitedMembers;
	}

	subCircleSet.ids = group.mSubCircles;
	return true;
}

bool RsGxsCircleGroupItem::convertTo(RsGxsCircleGroup &group) const
{
	group.mMeta = meta;

	// Enforce the local rules.
	if (meta.mCircleType ==  GXS_CIRCLE_TYPE_LOCAL)
	{
		group.mLocalFriends = pgpIdSet.ids;
	}
	else
	{
		group.mInvitedMembers = gxsIdSet.ids;
	}

	group.mSubCircles = subCircleSet.ids;
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

uint32_t RsGxsCircleSerialiser::sizeGxsCircleSubscriptionRequestItem(RsGxsCircleSubscriptionRequestItem * /* item */)
{
    uint32_t s=8 ; // header
    
    s += 4 ; // time_stamp serialised as uint32_t;
    s += 4 ; // time_out serialised as uint32_t;
    s += 1 ; //	subscription_type ;
    
    return s ;
}

uint32_t RsGxsCircleSerialiser::sizeGxsCircleGroupItem(RsGxsCircleGroupItem *item)
{
	uint32_t s = 8; // header

	s += item->pgpIdSet.TlvSize();
	s += item->gxsIdSet.TlvSize();
	s += item->subCircleSet.TlvSize();

	return s;
}

bool RsGxsCircleSerialiser::serialiseGxsCircleSubscriptionRequestItem(RsGxsCircleSubscriptionRequestItem *item, void *data, uint32_t *size)
{

#ifdef CIRCLE_DEBUG
    std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleSubscriptionRequestItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsCircleSubscriptionRequestItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize)
    {
#ifdef CIRCLE_DEBUG
	    std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleSubscriptionRequestItem()" << std::endl;
#endif
	    return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsCircleGroupItem */
    ok &= setRawUInt32(data,tlvsize,&offset,item->time_stamp) ;
    ok &= setRawUInt32(data,tlvsize,&offset,item->time_out) ;
    ok &= setRawUInt8(data,tlvsize,&offset,item->subscription_type) ;

    if(offset != tlvsize)
    {
	    //#ifdef CIRCLE_DEBUG
	    std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleSubscriptionRequestItem() FAIL Size Error! " << std::endl;
	    //#endif
	    ok = false;
    }

    //#ifdef CIRCLE_DEBUG
    if (!ok)
	    std::cerr << "RsGxsCircleSerialiser::serialiseGxsCircleSubscriptionRequestItem() NOK" << std::endl;
    //#endif

    return ok;
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
	
RsGxsCircleSubscriptionRequestItem *RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem(void *data, uint32_t *size)
{
	
#ifdef CIRCLE_DEBUG
	std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCIRCLE_SUBSCRIPTION_REQUEST_ITEM != getRsItemSubType(rstype)))
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsCircleSubscriptionRequestItem* item = new RsGxsCircleSubscriptionRequestItem();
	/* skip the header */
	offset += 8;
	
    	uint32_t tmp ;
    	ok &= getRawUInt32(data,rssize,&offset,&tmp) ; item->time_stamp = tmp ;
    	ok &= getRawUInt32(data,rssize,&offset,&tmp) ; item->time_out   = tmp ;
    	ok &= getRawUInt8(data,rssize,&offset,&item->subscription_type) ;
    
	if (offset != rssize)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef CIRCLE_DEBUG
		std::cerr << "RsGxsCircleSerialiser::deserialiseGxsCircleSubscriptionRequestItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
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
		(RS_SERVICE_GXS_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
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
		(RS_SERVICE_GXS_TYPE_GXSCIRCLE != getRsItemService(rstype)) ||
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

