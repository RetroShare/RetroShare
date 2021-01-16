/*******************************************************************************
 * libretroshare/src/rsitems: rswireitems.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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

#ifndef RS_WIRE_ITEMS_H
#define RS_WIRE_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
//#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvimage.h"

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

	// use conversion functions to transform:
	bool fromWireGroup(RsWireGroup &group, bool moveImage);
	bool toWireGroup(RsWireGroup &group, bool moveImage);

	RsWireGroup group;
	
	std::string mTagline;
	std::string mLocation;

	RsTlvImage mHeadshotImage;
	RsTlvImage mMastheadImage;
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
