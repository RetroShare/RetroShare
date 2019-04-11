/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsreputationitems.cc                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include <stdio.h>
#include "util/rstime.h"
#include "serialiser/rsbaseserial.h"
#include "rsitems/rsgxsreputationitems.h"

#include "serialiser/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

void RsGxsReputationSetItem::clear()
{
	mOpinions.clear() ;
}

void RsGxsReputationUpdateItem::clear()
{
	mOpinions.clear() ;
}

void RsGxsReputationBannedNodeSetItem::clear()
{
    mKnownIdentities.TlvClear();
}

void RsGxsReputationConfigItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,mPeerId,"mPeerId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mLatestUpdate,"mLatestUpdate") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mLastQuery,"mLastQuery") ;
}

void RsGxsReputationBannedNodeSetItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process           (j,ctx,mPgpId,"mPgpId") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,mLastActivityTS,"mLastActivityTS") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,mKnownIdentities,"mKnownIdentities") ;
}

void RsGxsReputationSetItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,mGxsId,"mGxsId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mOwnOpinion,"mOwnOpinion") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mOwnOpinionTS,"mOwnOpinionTS") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mIdentityFlags,"mIdentityFlags") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mLastUsedTS,"mLastUsedTS") ;
    RsTypeSerializer::serial_process          (j,ctx,mOwnerNodeId,"mOwnerNodeId") ;
    RsTypeSerializer::serial_process          (j,ctx,mOpinions,"mOpinions") ;
}

void RsGxsReputationUpdateItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mLatestUpdate,"mLatestUpdate") ;
    RsTypeSerializer::serial_process          (j,ctx,mOpinions,"mOpinions") ;
}
void RsGxsReputationRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mLastUpdate,"mLastUpdate") ;
}

/*************************************************************************/

RsItem *RsGxsReputationSerialiser::create_item(uint16_t service,uint8_t subtype) const
{
    if(service != RS_SERVICE_GXS_TYPE_REPUTATION)
        return NULL ;

    switch(subtype)
    {
    case 	RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM              : return new RsGxsReputationSetItem() ;
    case    RS_PKT_SUBTYPE_GXS_REPUTATION_BANNED_NODE_SET_ITEM  : return new RsGxsReputationBannedNodeSetItem();
    case    RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM         	: return new RsGxsReputationUpdateItem();
    case    RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM        	: return new RsGxsReputationRequestItem() ;
    case    RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM         	: return new RsGxsReputationConfigItem () ;
    default:
        std::cerr << "(EE) RsGxsReputationSerialiser::create_item(): unhandled item type " << subtype << std::endl;
        return NULL ;
    }
}

