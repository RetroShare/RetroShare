/*******************************************************************************
 * libretroshare/src/rsitems: rsheartbeatitems.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_HEARTBEAT_ITEMS_H
#define RS_HEARTBEAT_ITEMS_H

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

const uint8_t RS_PKT_SUBTYPE_HEARTBEAT_PULSE    = 0x01;

class RsHeartbeatItem: public RsItem
{
public:
    RsHeartbeatItem() :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_HEARTBEAT, RS_PKT_SUBTYPE_HEARTBEAT_PULSE)
    { 
		setPriorityLevel(QOS_PRIORITY_RS_HEARTBEAT_PULSE) ;
    }
    virtual ~RsHeartbeatItem() {}
    virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) {}

    virtual void clear(){}
};

class RsHeartbeatSerialiser: public RsServiceSerializer
{
public:
        RsHeartbeatSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_HEARTBEAT) {}

		virtual     ~RsHeartbeatSerialiser() {}

        virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const
        {
            if(service == RS_SERVICE_TYPE_HEARTBEAT && item_subtype == RS_PKT_SUBTYPE_HEARTBEAT_PULSE)
                return new RsHeartbeatItem() ;
            else
                return NULL ;
        }
};


#endif // RS_DISC_ITEMS_H

