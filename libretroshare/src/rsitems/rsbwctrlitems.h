/*******************************************************************************
 * libretroshare/src/rsitems: rsbwctrlitems.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef RS_BANDWIDTH_CONTROL_ITEMS_H
#define RS_BANDWIDTH_CONTROL_ITEMS_H

#include <map>

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserializer.h"
#include "serialiser/rstypeserializer.h"

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

	void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint32_t	allowedBw; // Units are bytes/sec => 4Gb/s;
};


class RsBwCtrlSerialiser: public RsServiceSerializer
{
public:
	RsBwCtrlSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_BWCTRL) {}
	virtual     ~RsBwCtrlSerialiser() {}

	RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */) const;
};

/**************************************************************************/

#endif /* RS_BANDWIDTH_CONTROL_ITEMS_H */

