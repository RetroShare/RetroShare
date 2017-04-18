#ifndef RS_GXSREPUTATION_ITEMS_H
#define RS_GXSREPUTATION_ITEMS_H

/*
 * libretroshare/src/serialiser: rsgxsreputationitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie.
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

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvidset.h"
#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rsreputations.h"

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

		virtual bool serialise(void *data,uint32_t& size) const = 0 ;	
		virtual uint32_t serial_size() const = 0 ; 						

		virtual void clear() = 0 ;
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0;

	protected:
		bool serialise_header(void *data, uint32_t& pktsize, uint32_t& tlvsize, uint32_t& offset) const;
};

class RsGxsReputationConfigItem: public RsReputationItem
{
public:
	RsGxsReputationConfigItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM) {}

	virtual ~RsGxsReputationConfigItem() {}
	virtual void clear() {}
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) const ;
	virtual uint32_t serial_size() const ; 						
    	
	RsPeerId mPeerId;
	uint32_t mLatestUpdate; // timestamp they returned.
    uint32_t mLastQuery;	// when we sent out.
};

// This class should disappear. Deprecated since Jan 7, 2017. The class definition is actually not needed,
// that is why it's commented out. Kept here in order to explains how the deserialisation works.
//
class RsGxsReputationSetItem_deprecated3: public RsReputationItem
{
public:
    RsGxsReputationSetItem_deprecated3()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM_deprecated3) {}

    virtual ~RsGxsReputationSetItem_deprecated3() {}
    virtual void clear() {}
    std::ostream &print(std::ostream &out, uint16_t /*indent*/ = 0) { return out;}

    virtual bool serialise(void */*data*/,uint32_t& /*size*/) const { return false ;}
    virtual uint32_t serial_size() const { return 0;}

    RsGxsId mGxsId;
    uint32_t mOwnOpinion;
    uint32_t mOwnOpinionTS;
	uint32_t mIdentityFlags ;
    RsPgpId  mOwnerNodeId;
    std::map<RsPeerId, uint32_t> mOpinions; // RsPeerId -> Opinion.
};
class RsGxsReputationSetItem: public RsReputationItem
{
public:
    RsGxsReputationSetItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM)
    {
        mOwnOpinion = RsReputations::OPINION_NEUTRAL ;
        mOwnOpinionTS = 0;
        mIdentityFlags = 0;
        mLastUsedTS = 0;
    }

    virtual ~RsGxsReputationSetItem() {}
    virtual void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) const ;
    virtual uint32_t serial_size() const ;

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
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) const ;
    virtual uint32_t serial_size() const ;

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
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) const ;	
    virtual uint32_t serial_size() const ; 						

    uint32_t mLatestUpdate; 
    std::map<RsGxsId, uint32_t> mOpinions; // GxsId -> Opinion.
};

class RsGxsReputationRequestItem: public RsReputationItem
{
public:
    RsGxsReputationRequestItem()  :RsReputationItem(RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM) {}

    virtual ~RsGxsReputationRequestItem() {}
    virtual void clear() {}
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) const ;	
    virtual uint32_t serial_size() const ; 						

	uint32_t mLastUpdate;
};


class RsGxsReputationSerialiser: public RsSerialType
{
public:
    RsGxsReputationSerialiser() :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXS_TYPE_REPUTATION){}

    virtual     ~RsGxsReputationSerialiser(){}

    virtual	uint32_t    size(RsItem *item)
    {
	    return dynamic_cast<RsReputationItem*>(item)->serial_size() ;
    }
    virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size)
    {
	    return dynamic_cast<RsReputationItem*>(item)->serialise(data,*size) ;
    }
    virtual	RsItem *    deserialise(void *data, uint32_t *size);

private:
    static	RsGxsReputationConfigItem         *deserialiseReputationConfigItem            (void *data, uint32_t size);
    static	RsGxsReputationSetItem            *deserialiseReputationSetItem               (void *data, uint32_t size);
    static	RsGxsReputationSetItem            *deserialiseReputationSetItem_deprecated    (void *data, uint32_t size);
    static	RsGxsReputationSetItem_deprecated3 *deserialiseReputationSetItem_deprecated3  (void *data, uint32_t size);
    static	RsGxsReputationUpdateItem         *deserialiseReputationUpdateItem            (void *data, uint32_t size);
    static	RsGxsReputationRequestItem        *deserialiseReputationRequestItem           (void *data, uint32_t size);
    static	RsGxsReputationBannedNodeSetItem  *deserialiseReputationBannedNodeSetItem     (void *data, uint32_t size);
};

/**************************************************************************/

#endif /* RS_GXSREPUTATION_ITEMS_H */


