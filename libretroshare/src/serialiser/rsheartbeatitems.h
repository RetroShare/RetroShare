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

#include "serialiser/rsserial.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
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

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);
};

class RsHeartbeatSerialiser: public RsSerialType
{
        public:
        RsHeartbeatSerialiser()
        :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_HEARTBEAT)
        { return; }

virtual     ~RsHeartbeatSerialiser() { return; }

virtual uint32_t    size(RsItem *);
virtual bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual uint32_t        sizeHeartbeat(RsHeartbeatItem *);
virtual bool            serialiseHeartbeat(RsHeartbeatItem *item, void *data, uint32_t *size);
virtual RsHeartbeatItem *deserialiseHeartbeat(void *data, uint32_t *size);

};


#endif // RS_DISC_ITEMS_H

