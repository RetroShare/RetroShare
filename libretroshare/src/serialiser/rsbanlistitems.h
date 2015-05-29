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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbanlist.h"

#define RS_PKT_SUBTYPE_BANLIST_ITEM_deprecated  0x01
#define RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM   	0x02
#define RS_PKT_SUBTYPE_BANLIST_ITEM		0x03

/**************************************************************************/

class RsBanListItem: public RsItem
{
	public:
	RsBanListItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST, 
		RS_PKT_SUBTYPE_BANLIST_ITEM)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_BANLIST_ITEM);
		return; 
	}

    virtual ~RsBanListItem();
    virtual void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvBanList	peerList;
};

class RsBanListConfigItem: public RsItem
{
public:
    RsBanListConfigItem()
            :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST, RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM) {}

    virtual ~RsBanListConfigItem(){}
    virtual void clear();

    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    RsPeerId  	peerId ;
    time_t		update_time ;
    RsTlvBanList	banned_peers;
};

class RsBanListSerialiser: public RsSerialType
{
public:
    RsBanListSerialiser()
            :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_BANLIST)
    { return; }
    virtual     ~RsBanListSerialiser()
    { return; }

    virtual	uint32_t    size(RsItem *);
    virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
    virtual	RsItem *    deserialise(void *data, uint32_t *size);

private:

    virtual	uint32_t    sizeList(RsBanListItem *);
    virtual	bool        serialiseList  (RsBanListItem *item, void *data, uint32_t *size);
    virtual	RsBanListItem *deserialiseList(void *data, uint32_t *size);

    virtual	uint32_t    sizeListConfig(RsBanListConfigItem *);
    virtual	bool        serialiseListConfig  (RsBanListConfigItem *item, void *data, uint32_t *size);
    virtual	RsBanListConfigItem *deserialiseListConfig(void *data, uint32_t *size);


};

/**************************************************************************/

#endif /* RS_BANLIST_ITEMS_H */


