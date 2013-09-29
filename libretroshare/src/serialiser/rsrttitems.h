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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

/**************************************************************************/

const uint8_t RS_PKT_SUBTYPE_RTT_PING = 0x01;
const uint8_t RS_PKT_SUBTYPE_RTT_PONG = 0x02;

class RsRttItem: public RsItem
{
	public:
		RsRttItem(uint8_t chat_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_RTT,chat_subtype) 
	{ setPriorityLevel(QOS_PRIORITY_RS_RTT_PING) ;}	// should be refined later.

		virtual ~RsRttItem() {};
		virtual void clear() {};
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;
};

class RsRttPingItem: public RsRttItem
{
	public:
		RsRttPingItem() :RsRttItem(RS_PKT_SUBTYPE_RTT_PING) {}

		virtual ~RsRttPingItem();
		virtual void clear();
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsRttPongItem: public RsRttItem
{
	public:
		RsRttPongItem() :RsRttItem(RS_PKT_SUBTYPE_RTT_PONG) {}

		virtual ~RsRttPongItem();
		virtual void clear();
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};


class RsRttSerialiser: public RsSerialType
{
	public:
	RsRttSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_RTT)
	{ return; }
	
virtual     ~RsRttSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeRttPingItem(RsRttPingItem *);
virtual	bool        serialiseRttPingItem  (RsRttPingItem *item, void *data, uint32_t *size);
virtual	RsRttPingItem *deserialiseRttPingItem(void *data, uint32_t *size);

virtual	uint32_t    sizeRttPongItem(RsRttPongItem *);
virtual	bool        serialiseRttPongItem  (RsRttPongItem *item, void *data, uint32_t *size);
virtual	RsRttPongItem *deserialiseRttPongItem(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_RTT_ITEMS_H */


