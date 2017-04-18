/*
 * libretroshare/src/serialiser: rsgxscircleitems.h
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

#ifndef RS_GXSCIRCLE_ITEMS_H
#define RS_GXSCIRCLE_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvstring.h"
#include "serialiser/rstlvidset.h"

#include "rsgxsitems.h"
#include "retroshare/rsgxscircles.h"

const uint8_t RS_PKT_SUBTYPE_GXSCIRCLE_GROUP_ITEM                = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSCIRCLE_MSG_ITEM                  = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXSCIRCLE_SUBSCRIPTION_REQUEST_ITEM = 0x04;

const uint16_t GXSCIRCLE_PGPIDSET	= 0x0001;
const uint16_t GXSCIRCLE_GXSIDSET	= 0x0002;
const uint16_t GXSCIRCLE_SUBCIRCLESET	= 0x0003;

// These classes are a mess. Needs proper item serialisation etc.

class RsGxsCircleGroupItem : public RsGxsGrpItem
{

public:

	RsGxsCircleGroupItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_GXSCIRCLE,
			RS_PKT_SUBTYPE_GXSCIRCLE_GROUP_ITEM)
#if 0
			pgpIdSet(GXSCIRCLE_PGPIDSET), 
			gxsIdSet(GXSCIRCLE_GXSIDSET), 
			subCircleSet(GXSCIRCLE_SUBCIRCLESET) 
#endif
			{ return;}
        virtual ~RsGxsCircleGroupItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	bool convertFrom(const RsGxsCircleGroup &group);
	bool convertTo(RsGxsCircleGroup &group) const;

	// DIFFERENT FROM OTHER ONES, as stupid serialisation otherwise.
	RsTlvPgpIdSet pgpIdSet; // For Local Groups.
	RsTlvGxsIdSet gxsIdSet; // For External Groups.
	RsTlvGxsCircleIdSet subCircleSet;
};

class RsGxsCircleMsgItem : public RsGxsMsgItem
{
public:

	RsGxsCircleMsgItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_GXSCIRCLE,
			RS_PKT_SUBTYPE_GXSCIRCLE_MSG_ITEM) {return; }
        virtual ~RsGxsCircleMsgItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsGxsCircleMsg msg;
};

class RsGxsCircleSubscriptionRequestItem: public RsGxsMsgItem
{
public:
    
    RsGxsCircleSubscriptionRequestItem() : RsGxsMsgItem(RS_SERVICE_GXS_TYPE_GXSCIRCLE, RS_PKT_SUBTYPE_GXSCIRCLE_SUBSCRIPTION_REQUEST_ITEM) { }
    virtual ~RsGxsCircleSubscriptionRequestItem() {}
    
    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);
    
    enum {
        SUBSCRIPTION_REQUEST_UNKNOWN     = 0x00, 
        SUBSCRIPTION_REQUEST_SUBSCRIBE   = 0x01,
        SUBSCRIPTION_REQUEST_UNSUBSCRIBE = 0x02
    };
    
    time_t           time_stamp ;
    time_t           time_out ;           
    uint8_t          subscription_type ;
};

class RsGxsCircleSerialiser : public RsSerialType
{
public:

	RsGxsCircleSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXS_TYPE_GXSCIRCLE)
	{ return; }
	virtual     ~RsGxsCircleSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsCircleSubscriptionRequestItem(RsGxsCircleSubscriptionRequestItem *item);
	bool        serialiseGxsCircleSubscriptionRequestItem  (RsGxsCircleSubscriptionRequestItem *item, void *data, uint32_t *size);
	RsGxsCircleSubscriptionRequestItem *    deserialiseGxsCircleSubscriptionRequestItem(void *data, uint32_t *size);

	uint32_t    sizeGxsCircleGroupItem(RsGxsCircleGroupItem *item);
	bool        serialiseGxsCircleGroupItem  (RsGxsCircleGroupItem *item, void *data, uint32_t *size);
	RsGxsCircleGroupItem *    deserialiseGxsCircleGroupItem(void *data, uint32_t *size);

	uint32_t    sizeGxsCircleMsgItem(RsGxsCircleMsgItem *item);
	bool        serialiseGxsCircleMsgItem  (RsGxsCircleMsgItem *item, void *data, uint32_t *size);
	RsGxsCircleMsgItem *    deserialiseGxsCircleMsgItem(void *data, uint32_t *size);
};

#endif /* RS_GXSCIRCLE_ITEMS_H */
