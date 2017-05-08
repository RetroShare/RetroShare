#ifndef RS_STATUS_ITEMS_H
#define RS_STATUS_ITEMS_H

/*
 * libretroshare/src/serialiser: rsstatusitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Vinny Do.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "rsitems/rsitem.h"

#include "serialiser/rstypeserializer.h"

/**************************************************************************/

class RsStatusItem: public RsItem
{
public:
	RsStatusItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_STATUS,  RS_PKT_SUBTYPE_DEFAULT)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_STATUS_ITEM); 
	}
    virtual ~RsStatusItem() {}
    virtual void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
    {
        RsTypeSerializer::serial_process<uint32_t>(j,ctx,sendTime,"sendTime") ;
        RsTypeSerializer::serial_process<uint32_t>(j,ctx,status  ,"status") ;
    }

	uint32_t sendTime;
	uint32_t status;

	/* not serialised */
	uint32_t recvTime; 
};

class RsStatusSerialiser: public RsServiceSerializer
{
public:
	RsStatusSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_STATUS) {}
	virtual     ~RsStatusSerialiser() {}

    virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const
    {
 		if(service == RS_SERVICE_TYPE_STATUS && item_subtype == RS_PKT_SUBTYPE_DEFAULT)
            return new RsStatusItem();
        else
            return NULL ;
    }
};

/**************************************************************************/

#endif /* RS_STATUS_ITEMS_H */


