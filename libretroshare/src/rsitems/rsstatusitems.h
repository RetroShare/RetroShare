/*******************************************************************************
 * libretroshare/src/rsitems: rsstatusitems.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Vinny Do.                                            *
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
#ifndef RS_STATUS_ITEMS_H
#define RS_STATUS_ITEMS_H

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


