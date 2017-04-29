/*
 * libretroshare/src/serialiser: rswireitems.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
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

#ifndef RS_WIRE_ITEMS_H
#define RS_WIRE_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
//#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "retroshare/rswire.h"

const uint8_t RS_PKT_SUBTYPE_WIRE_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_WIRE_PULSE_ITEM = 0x03;

class RsGxsWireGroupItem : public RsGxsGrpItem
{

public:

	RsGxsWireGroupItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_WIRE, RS_PKT_SUBTYPE_WIRE_GROUP_ITEM) {}
	virtual ~RsGxsWireGroupItem() {}

	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsWireGroup group;
};

class RsGxsWirePulseItem : public RsGxsMsgItem
{
public:

	RsGxsWirePulseItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_WIRE, RS_PKT_SUBTYPE_WIRE_PULSE_ITEM) {}
	virtual ~RsGxsWirePulseItem() {}
	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsWirePulse pulse;
};

class RsGxsWireSerialiser : public RsServiceSerializer
{
public:

	RsGxsWireSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_WIRE) {}
	virtual     ~RsGxsWireSerialiser() {}

    virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const ;
};

#endif /* RS_WIKI_ITEMS_H */
