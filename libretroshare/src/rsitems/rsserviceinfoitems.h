#ifndef RS_SERVICE_INFO_ITEMS_H
#define RS_SERVICE_INFO_ITEMS_H

/*
 * libretroshare/src/serialiser: rsserviceinfoitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 by Robert Fernie.
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

// Provides serialiser for p3ServiceControl & p3ServiceInfo.

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rstlvgenericmap.h"
#include "retroshare/rsservicecontrol.h"

#define RS_PKT_SUBTYPE_SERVICELIST_ITEM       		0x01
#define RS_PKT_SUBTYPE_SERVICEPERMISSIONS_ITEM  	0x02

/**************************************************************************/
#define SERVICE_INFO_MAP	0x01
#define SERVICE_INFO_KEY	0x01
#define SERVICE_ID	0x01
#define SERVICE_INFO	0x01

class RsTlvServiceInfoMapRef: public RsTlvGenericMapRef<uint32_t, RsServiceInfo>
{
public:
        RsTlvServiceInfoMapRef(std::map<uint32_t, RsServiceInfo> &refmap)
        :RsTlvGenericMapRef<uint32_t, RsServiceInfo>(
		SERVICE_INFO_MAP,
		SERVICE_INFO_KEY,
		SERVICE_ID,
		SERVICE_INFO, 
		refmap)
	{ 
		return;
	}
};

class RsServiceInfoListItem: public RsItem
{
	public:
	RsServiceInfoListItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_SERVICEINFO, RS_PKT_SUBTYPE_SERVICELIST_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_SERVICE_INFO_ITEM);
		return; 
	}

    virtual ~RsServiceInfoListItem(){}
	virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	std::map<uint32_t, RsServiceInfo> mServiceInfo;
};

class RsServiceInfoPermissionsItem: public RsItem
{
	public:
	RsServiceInfoPermissionsItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_SERVICEINFO,  RS_PKT_SUBTYPE_SERVICEPERMISSIONS_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_SERVICE_INFO_ITEM);
		return; 
	}

    virtual ~RsServiceInfoPermissionsItem(){}
    virtual void clear(){}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint32_t	allowedBw; // Units are bytes/sec => 4Gb/s; 
};

class RsServiceInfoSerialiser: public RsServiceSerializer
{
	public:
	RsServiceInfoSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_SERVICEINFO) {}
	virtual     ~RsServiceInfoSerialiser() {}
	
	virtual RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */) const;
};

/**************************************************************************/

#endif /* RS_SERVICE_INFO_ITEMS_H */
