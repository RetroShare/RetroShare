#ifndef RS_BANLIST_ITEMS_H
#define RS_BANLIST_ITEMS_H

/*
 * libretroshare/src/serialiser: rsbanlistitems.h
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

#include "rsitems/rsserviceids.h"
#include "serialiser/rstlvbanlist.h"
#include "serialization/rsserializer.h"

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
	void serial_process(RsItem::SerializeJob j,SerializeContext& ctx);

	RsTlvBanList	peerList;
};

class RsBanListConfigItem: public RsItem
{
public:
    RsBanListConfigItem()
            :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST, RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM) {}

    virtual ~RsBanListConfigItem(){}
    virtual void clear() { banned_peers.TlvClear() ; }

	void serial_process(RsItem::SerializeJob j,SerializeContext& ctx);

    uint32_t		type ;
    RsPeerId  		peerId ;
    time_t			update_time ;
    RsTlvBanList	banned_peers;
};

class RsBanListSerialiser: public RsSerializer
{
public:
		RsBanListSerialiser() :RsSerializer(RS_SERVICE_TYPE_BANLIST) {}

		virtual RsItem *create_item(uint16_t service_id,uint8_t item_sub_id) const ;
};

/**************************************************************************/

#endif /* RS_BANLIST_ITEMS_H */


