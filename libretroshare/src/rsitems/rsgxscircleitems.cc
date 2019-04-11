/*******************************************************************************
 * libretroshare/src/rsitems: rsgxscircleitems.cc                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <iostream>

#include "rsgxscircleitems.h"

#include "serialiser/rstypeserializer.h"

//#define CIRCLE_DEBUG	1

RsItem *RsGxsCircleSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_GXS_TYPE_GXSCIRCLE)
        return NULL ;

    switch(item_sub_id)
    {
    case RS_PKT_SUBTYPE_GXSCIRCLE_GROUP_ITEM: return new RsGxsCircleGroupItem();
    case RS_PKT_SUBTYPE_GXSCIRCLE_MSG_ITEM:   return new RsGxsCircleMsgItem();
    case RS_PKT_SUBTYPE_GXSCIRCLE_SUBSCRIPTION_REQUEST_ITEM: return new RsGxsCircleSubscriptionRequestItem();
    default:
        return NULL ;
    }
}

void RsGxsCircleSubscriptionRequestItem::clear()
{
    time_stamp = 0 ;
    time_out   = 0 ;
    subscription_type = SUBSCRIPTION_REQUEST_UNKNOWN;
}

void RsGxsCircleMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	//RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,mMsg.stuff,"mMsg.stuff") ;//Should be this but not retrocompatible...
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,mMsg.stuff,"msg.stuff") ;
}

void RsGxsCircleSubscriptionRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,time_stamp,"time_stamp") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,time_out  ,"time_out") ;
    RsTypeSerializer::serial_process<uint8_t> (j,ctx,subscription_type  ,"subscription_type") ;
}

void RsGxsCircleGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,pgpIdSet,"pgpIdSet") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,gxsIdSet,"gxsIdSet") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,subCircleSet,"subCircleSet") ;
}

void RsGxsCircleMsgItem::clear()
{
	mMsg.stuff.clear();
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

