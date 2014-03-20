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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"
#include "retroshare/rsgxsifacetypes.h"

#define RS_PKT_SUBTYPE_GXSREPUTATION_CONFIG_ITEM 	0x01
#define RS_PKT_SUBTYPE_GXSREPUTATION_SET_ITEM 		0x02
#define RS_PKT_SUBTYPE_GXSREPUTATION_UPDATE_ITEM 	0x03
#define RS_PKT_SUBTYPE_GXSREPUTATION_REQUEST_ITEM       0x04

/**************************************************************************/

class RsGxsReputationConfigItem: public RsItem
{
	public:
	RsGxsReputationConfigItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_REPUTATION, 
		RS_PKT_SUBTYPE_GXSREPUTATION_CONFIG_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_GXSREPUTATION_ITEM);
		return; 
	}

virtual ~RsGxsReputationConfigItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	std::string mPeerId;
	uint32_t mLatestUpdate; // timestamp they returned.
	uint32_t mLastQuery;	// when we sent out.
};

class RsGxsReputationSetItem: public RsItem
{
	public:
	RsGxsReputationSetItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_REPUTATION, 
		RS_PKT_SUBTYPE_GXSREPUTATION_SET_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_GXSREPUTATION_ITEM);
		return; 
	}

virtual ~RsGxsReputationSetItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	std::string mGxsId;
	uint32_t mOwnOpinion;
	uint32_t mOwnOpinionTs;
	uint32_t mReputation;
	std::map<std::string, uint32_t> mOpinions; // RsPeerId -> Opinion.
};

class RsGxsReputationUpdateItem: public RsItem
{
	public:
	RsGxsReputationUpdateItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_REPUTATION, 
		RS_PKT_SUBTYPE_GXSREPUTATION_UPDATE_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_GXSREPUTATION_ITEM);
		return; 
	}

virtual ~RsGxsReputationUpdateItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	std::map<RsGxsId, uint32_t> mOpinions; // GxsId -> Opinion.
	uint32_t mLatestUpdate; 
};

class RsGxsReputationRequestItem: public RsItem
{
	public:
	RsGxsReputationRequestItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_REPUTATION,
		RS_PKT_SUBTYPE_GXSREPUTATION_REQUEST_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_GXSREPUTATION_ITEM);
		return; 
	}

virtual ~RsGxsReputationRequestItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint32_t mLastUpdate;
};


class RsGxsReputationSerialiser: public RsSerialType
{
	public:
	RsGxsReputationSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_REPUTATION)
	{ return; }
virtual     ~RsGxsReputationSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeConfig(RsGxsReputationConfigItem *);
virtual	bool        serialiseConfig(RsGxsReputationConfigItem *item, void *data, uint32_t *size);
virtual	RsGxsReputationConfigItem *deserialiseConfig(void *data, uint32_t *size);


virtual	uint32_t    sizeSet(RsGxsReputationSetItem *);
virtual	bool        serialiseSet(RsGxsReputationSetItem *item, void *data, uint32_t *size);
virtual	RsGxsReputationSetItem *deserialiseSet(void *data, uint32_t *size);


virtual	uint32_t    sizeUpdate(RsGxsReputationUpdateItem *);
virtual	bool        serialiseUpdate(RsGxsReputationUpdateItem *item, void *data, uint32_t *size);
virtual	RsGxsReputationUpdateItem *deserialiseUpdate(void *data, uint32_t *size);


virtual	uint32_t    sizeRequest(RsGxsReputationRequestItem *);
virtual	bool        serialiseRequest(RsGxsReputationRequestItem *item, void *data, uint32_t *size);
virtual	RsGxsReputationRequestItem *deserialiseRequest(void *data, uint32_t *size);


};

/**************************************************************************/

#endif /* RS_GXSREPUTATION_ITEMS_H */


