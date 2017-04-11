#ifndef RS_BANDWIDTH_CONTROL_ITEMS_H
#define RS_BANDWIDTH_CONTROL_ITEMS_H

/*
 * libretroshare/src/serialiser: rsbwctrlitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 by Robert Fernie.
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
#include "serialiser/rstlvbase.h"

#include "serialization/rsserializer.h"
#include "serialization/rstypeserializer.h"

#define RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM       0x01

/**************************************************************************/

class RsBwCtrlAllowedItem: public RsItem
{
public:
	RsBwCtrlAllowedItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BWCTRL,  RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM)
	{
		setPriorityLevel(QOS_PRIORITY_RS_BWCTRL_ALLOWED_ITEM);
		return;
	}

    virtual ~RsBwCtrlAllowedItem() {}
	virtual void clear();

	void serial_process(SerializeJob j,SerializeContext& ctx);

	uint32_t	allowedBw; // Units are bytes/sec => 4Gb/s;
};


class RsBwCtrlSerialiser: public RsSerializer
{
public:
	RsBwCtrlSerialiser() :RsSerializer(RS_SERVICE_TYPE_BWCTRL) {}
	virtual     ~RsBwCtrlSerialiser() {}

	RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */);
};

/**************************************************************************/

#endif /* RS_BANDWIDTH_CONTROL_ITEMS_H */

