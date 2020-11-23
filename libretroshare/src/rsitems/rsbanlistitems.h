/*******************************************************************************
 * libretroshare/src/rsitems: rsbanlistitems.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef RS_BANLIST_ITEMS_H
#define RS_BANLIST_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"
#include "serialiser/rstlvbanlist.h"
#include "serialiser/rsserializer.h"

#define RS_PKT_SUBTYPE_BANLIST_ITEM_deprecated  		0x01
#define RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM_deprecated 	0x02
#define RS_PKT_SUBTYPE_BANLIST_ITEM				        0x03
#define RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM   			0x04

/**************************************************************************/

class RsBanListItem: public RsItem
{
	public:
	RsBanListItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST,  RS_PKT_SUBTYPE_BANLIST_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_BANLIST_ITEM);
		return; 
	}

    virtual ~RsBanListItem(){}
    virtual void clear();
	void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsTlvBanList	peerList;
};

class RsBanListConfigItem: public RsItem
{
public:
	RsBanListConfigItem()
	  : RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST, RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM)
	  , banListType(0), update_time(0)
	{}

	virtual ~RsBanListConfigItem(){}
	virtual void clear() { banned_peers.TlvClear() ; }

	void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint32_t      banListType ;
	RsPeerId      banListPeerId ;
	rstime_t        update_time ;
	RsTlvBanList  banned_peers;
};

class RsBanListSerialiser: public RsServiceSerializer
{
public:
		RsBanListSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_BANLIST) {}

		virtual RsItem *create_item(uint16_t service_id,uint8_t item_sub_id) const ;
};

/**************************************************************************/

#endif /* RS_BANLIST_ITEMS_H */


