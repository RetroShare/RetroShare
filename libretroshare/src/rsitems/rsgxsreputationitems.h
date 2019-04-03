/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsreputationitems.h                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014-2014 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_GXSREPUTATION_ITEMS_H
#define RS_GXSREPUTATION_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserial.h"
#include "serialiser/rstlvidset.h"
#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rsreputations.h"

#include "serialiser/rsserializer.h"

#define RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM          0x01
#define RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM_deprecated2 0x02
#define RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM          0x03
#define RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM         0x04
#define RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM_deprecated1 0x05
#define RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM_deprecated3 0x06
#define RS_PKT_SUBTYPE_GXS_REPUTATION_BANNED_NODE_SET_ITEM 0x07
#define RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM             0x08

/**************************************************************************/
class RsReputationItem: public RsItem
{
	public:
		RsReputationItem(uint8_t reputation_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_GXS_TYPE_REPUTATION,reputation_subtype) 
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_GXSREPUTATION_ITEM);
		}

		virtual ~RsReputationItem() {}
		virtual void clear() = 0 ;
};

class RsGxsReputationConfigItem: public RsReputationItem
{
public:
	RsGxsReputationConfigItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM) {}

	virtual ~RsGxsReputationConfigItem() {}
	virtual void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) ;

	RsPeerId mPeerId;
	uint32_t mLatestUpdate; // timestamp they returned.
    uint32_t mLastQuery;	// when we sent out.
};

#ifdef TO_REMOVE
// This class should disappear. Deprecated since Jan 7, 2017. The class definition is actually not needed,
// that is why it's commented out. Kept here in order to explains how the deserialisation works.
//
class RsGxsReputationSetItem_deprecated3: public RsReputationItem
{
public:
    RsGxsReputationSetItem_deprecated3()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM_deprecated3) {}

    virtual ~RsGxsReputationSetItem_deprecated3() {}
    virtual void clear() {}

	virtual void serial_process(SerializeJob /* j */,SerializeContext& /* ctx */) ;

    RsGxsId mGxsId;
    uint32_t mOwnOpinion;
    uint32_t mOwnOpinionTS;
	uint32_t mIdentityFlags ;
    RsPgpId  mOwnerNodeId;
    std::map<RsPeerId, uint32_t> mOpinions; // RsPeerId -> Opinion.
};
#endif

class RsGxsReputationSetItem: public RsReputationItem
{
public:
    RsGxsReputationSetItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM)
    {
		mOwnOpinion = static_cast<uint32_t>(RsOpinion::NEUTRAL);
        mOwnOpinionTS = 0;
        mIdentityFlags = 0;
        mLastUsedTS = 0;
    }

    virtual ~RsGxsReputationSetItem() {}
    virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) ;

    RsGxsId mGxsId;
    uint32_t mOwnOpinion;
    uint32_t mOwnOpinionTS;
	uint32_t mIdentityFlags;
    uint32_t mLastUsedTS;
    RsPgpId  mOwnerNodeId;
    std::map<RsPeerId, uint32_t> mOpinions; // RsPeerId -> Opinion.
};
class RsGxsReputationBannedNodeSetItem: public RsReputationItem
{
public:
    RsGxsReputationBannedNodeSetItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_BANNED_NODE_SET_ITEM) {}

    virtual ~RsGxsReputationBannedNodeSetItem() {}
    virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) ;

    RsPgpId mPgpId ;
    uint32_t mLastActivityTS ;
    RsTlvGxsIdSet mKnownIdentities ;
};

class RsGxsReputationUpdateItem: public RsReputationItem
{
public:
    RsGxsReputationUpdateItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM) {}

    virtual ~RsGxsReputationUpdateItem() {}
    virtual void clear();  

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) ;

    uint32_t mLatestUpdate;
    std::map<RsGxsId, uint32_t> mOpinions; // GxsId -> Opinion.
};

class RsGxsReputationRequestItem: public RsReputationItem
{
public:
    RsGxsReputationRequestItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM) {}

    virtual ~RsGxsReputationRequestItem() {}
    virtual void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) ;

	uint32_t mLastUpdate;
};


class RsGxsReputationSerialiser: public RsServiceSerializer
{
public:
    RsGxsReputationSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_REPUTATION){}
    virtual     ~RsGxsReputationSerialiser(){}

    virtual RsItem *create_item(uint16_t service,uint8_t item_type) const;
};

/**************************************************************************/

#endif /* RS_GXSREPUTATION_ITEMS_H */


