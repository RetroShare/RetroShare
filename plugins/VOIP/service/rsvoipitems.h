#ifndef RS_VOIP_ITEMS_H
#define RS_VOIP_ITEMS_H

/*
 * libretroshare/src/serialiser: rsvoipitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie.
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

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

/**************************************************************************/

const uint8_t RS_PKT_SUBTYPE_VOIP_PING = 0x01;
const uint8_t RS_PKT_SUBTYPE_VOIP_PONG = 0x02;

class RsVoipItem: public RsItem
{
	public:
		RsVoipItem(uint8_t chat_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_VOIP,chat_subtype) 
	{ setPriorityLevel(QOS_PRIORITY_RS_VOIP_PING) ;}	// should be refined later.

		virtual ~RsVoipItem() {};
		virtual void clear() {};
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;
};

class RsVoipPingItem: public RsVoipItem
{
	public:
		RsVoipPingItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_PING) {}

		virtual ~RsVoipPingItem();
		virtual void clear();
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsVoipPongItem: public RsVoipItem
{
	public:
		RsVoipPongItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_PONG) {}

		virtual ~RsVoipPongItem();
		virtual void clear();
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};


class RsVoipSerialiser: public RsSerialType
{
	public:
	RsVoipSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_VOIP)
	{ return; }
	
virtual     ~RsVoipSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeVoipPingItem(RsVoipPingItem *);
virtual	bool        serialiseVoipPingItem  (RsVoipPingItem *item, void *data, uint32_t *size);
virtual	RsVoipPingItem *deserialiseVoipPingItem(void *data, uint32_t *size);

virtual	uint32_t    sizeVoipPongItem(RsVoipPongItem *);
virtual	bool        serialiseVoipPongItem  (RsVoipPongItem *item, void *data, uint32_t *size);
virtual	RsVoipPongItem *deserialiseVoipPongItem(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_VOIP_ITEMS_H */


