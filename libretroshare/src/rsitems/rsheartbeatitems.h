/*
 * libretroshare/src/serialiser: rsheartbeatitems.h
 *
 * Serialiser for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
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

