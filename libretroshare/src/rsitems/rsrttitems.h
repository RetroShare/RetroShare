/*******************************************************************************
 * libretroshare/src/rsitems: rsrttitems.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_RTT_ITEMS_H
#define RS_RTT_ITEMS_H

#include <map>

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "serialiser/rsserial.h"

#include "serialiser/rsserializer.h"

/**************************************************************************/

const uint8_t RS_PKT_SUBTYPE_RTT_PING = 0x01;
const uint8_t RS_PKT_SUBTYPE_RTT_PONG = 0x02;

class RsRttItem: public RsItem
{
	public:
		explicit RsRttItem(uint8_t subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_RTT,subtype)
		{ setPriorityLevel(QOS_PRIORITY_RS_RTT_PING) ;}	// should be refined later.

		virtual ~RsRttItem() {}
		virtual void clear() {}
};

class RsRttPingItem: public RsRttItem
{
	public:
		RsRttPingItem()
		  : RsRttItem(RS_PKT_SUBTYPE_RTT_PING)
		  , mSeqNo(0), mPingTS(0)
		{}

        virtual ~RsRttPingItem(){}
        virtual void clear(){}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsRttPongItem: public RsRttItem
{
	public:
		RsRttPongItem()
		  : RsRttItem(RS_PKT_SUBTYPE_RTT_PONG)
		  , mSeqNo(0), mPingTS(0), mPongTS(0)
		{}

        virtual ~RsRttPongItem(){}
        virtual void clear(){}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};


class RsRttSerialiser: public RsServiceSerializer
{
	public:
	RsRttSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_RTT) {}
	
	virtual     ~RsRttSerialiser(){}
	
    virtual RsItem *create_item(uint16_t service,uint8_t type) const;
};

/**************************************************************************/

#endif /* RS_RTT_ITEMS_H */


