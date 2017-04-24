#ifndef RS_RTT_ITEMS_H
#define RS_RTT_ITEMS_H

/*
 * libretroshare/src/serialiser: rsrttitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011-2013 by Robert Fernie.
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

#include "serialization/rsserializer.h"

/**************************************************************************/

const uint8_t RS_PKT_SUBTYPE_RTT_PING = 0x01;
const uint8_t RS_PKT_SUBTYPE_RTT_PONG = 0x02;

class RsRttItem: public RsItem
{
	public:
		RsRttItem(uint8_t subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_RTT,subtype)
	{ setPriorityLevel(QOS_PRIORITY_RS_RTT_PING) ;}	// should be refined later.

		virtual ~RsRttItem() {};
		virtual void clear() {};
};

class RsRttPingItem: public RsRttItem
{
	public:
		RsRttPingItem() :RsRttItem(RS_PKT_SUBTYPE_RTT_PING) {}

        virtual ~RsRttPingItem(){}
        virtual void clear(){}

		virtual void serial_process(SerializeJob j,SerializeContext& ctx);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsRttPongItem: public RsRttItem
{
	public:
		RsRttPongItem() :RsRttItem(RS_PKT_SUBTYPE_RTT_PONG) {}

        virtual ~RsRttPongItem(){}
        virtual void clear(){}

		virtual void serial_process(SerializeJob j,SerializeContext& ctx);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};


class RsRttSerialiser: public RsSerializer
{
	public:
	RsRttSerialiser() :RsSerializer(RS_SERVICE_TYPE_RTT) {}
	
	virtual     ~RsRttSerialiser(){}
	
    virtual RsItem *create_item(uint16_t service,uint8_t type) const;
};

/**************************************************************************/

#endif /* RS_RTT_ITEMS_H */


