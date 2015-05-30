/*
 * libretroshare/src/serialiser: rsbanlist.cc
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsbanlistitems.h"
#include "serialiser/rstlvbanlist.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsBanListItem::~RsBanListItem()
{
	return;
}

void 	RsBanListItem::clear()
{
	peerList.TlvClear();
}

std::ostream &RsBanListItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsBanListItem", indent);
	uint16_t int_Indent = indent + 2;
	peerList.print(out, int_Indent);

        printRsItemEnd(out, "RsBanListItem", indent);
        return out;
}


uint32_t    RsBanListSerialiser::sizeList(RsBanListItem *item)
{
	uint32_t s = 8; /* header */
	s += item->peerList.TlvSize();

	return s;
}

uint32_t    RsBanListSerialiser::sizeListConfig(RsBanListConfigItem *item)
{
    uint32_t s = 8; /* header */
    s += 4 ; // type
    s += item->banned_peers.TlvSize();
    s += 8 ;	// update time
    s += item->peerId.serial_size() ;

    return s;
}
/* serialise the data to the buffer */
bool     RsBanListSerialiser::serialiseList(RsBanListItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeList(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDsdvSerialiser::serialiseRoute() Header: " << ok << std::endl;
	std::cerr << "RsDsdvSerialiser::serialiseRoute() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->peerList.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDsdvSerialiser::serialiseRoute() Size Error! " << std::endl;
#endif
	}

	return ok;
}
/* serialise the data to the buffer */
bool     RsBanListSerialiser::serialiseListConfig(RsBanListConfigItem *item, void *data, uint32_t *pktsize)
{
    uint32_t tlvsize = sizeListConfig(item);
    uint32_t offset = 0;

    if (*pktsize < tlvsize)
        return false; /* not enough space */

    *pktsize = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsBanListSerialiser::serialiseRoute() Header: " << ok << std::endl;
    std::cerr << "RsBanListSerialiser::serialiseRoute() Size: " << tlvsize << std::endl;
#endif

    /* skip the header */
    offset += 8;

    ok &= setRawUInt32(data, tlvsize, &offset,item->type);
    ok &= item->peerId.serialise(data, tlvsize, offset);
    ok &= setRawTimeT(data, tlvsize, &offset,item->update_time);

    /* add mandatory parts first */
    ok &= item->banned_peers.SetTlv(data, tlvsize, &offset);

    if (offset != tlvsize)
    {
        ok = false;
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsBanListSerialiser::serialiseRoute() Size Error! " << std::endl;
#endif
    }

    return ok;
}
RsBanListItem *RsBanListSerialiser::deserialiseList(void *data, uint32_t *pktsize)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t tlvsize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
        (RS_SERVICE_TYPE_BANLIST != getRsItemService(rstype)) ||
        (RS_PKT_SUBTYPE_BANLIST_ITEM != getRsItemSubType(rstype)))
    {
        return NULL; /* wrong type */
    }

    if (*pktsize < tlvsize)    /* check size */
        return NULL; /* not enough data */

    /* set the packet length */
    *pktsize = tlvsize;

    bool ok = true;

    /* ready to load */
    RsBanListItem *item = new RsBanListItem();
    item->clear();

    /* skip the header */
    offset += 8;

    /* add mandatory parts first */
    ok &= item->peerList.GetTlv(data, tlvsize, &offset);

    if (offset != tlvsize)
    {
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
        delete item;
        return NULL;
    }

    return item;
}
RsBanListConfigItem *RsBanListSerialiser::deserialiseListConfig(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_BANLIST != getRsItemService(rstype)) ||
        (RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
    RsBanListConfigItem *item = new RsBanListConfigItem();
	item->clear();

	/* skip the header */
	offset += 8;

    ok &= getRawUInt32(data, tlvsize, &offset,&item->type);
    ok &= item->peerId.deserialise(data, tlvsize, offset);
    ok &= getRawTimeT(data, tlvsize, &offset,item->update_time);

    /* add mandatory parts first */
    ok &= item->banned_peers.GetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/

uint32_t    RsBanListSerialiser::size(RsItem *i)
{
	RsBanListItem *dri;
    RsBanListConfigItem *drc;

	if (NULL != (dri = dynamic_cast<RsBanListItem *>(i)))
	{
		return sizeList(dri);
    }

    if (NULL != (drc = dynamic_cast<RsBanListConfigItem *>(i)))
    {
        return sizeListConfig(drc);
    }
    return 0;
}

bool     RsBanListSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsBanListItem *dri;
    RsBanListConfigItem *drc;

    if (NULL != (dri = dynamic_cast<RsBanListItem *>(i)))
	{
		return serialiseList(dri, data, pktsize);
	}
    if (NULL != (drc = dynamic_cast<RsBanListConfigItem *>(i)))
    {
        return serialiseListConfig(drc, data, pktsize);
    }
    return false;
}

RsItem *RsBanListSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_BANLIST != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_BANLIST_ITEM:
			return deserialiseList(data, pktsize);
			break;
        case RS_PKT_SUBTYPE_BANLIST_CONFIG_ITEM:
            return deserialiseListConfig(data, pktsize);
            break;
        default:
			return NULL;
			break;
    }
}

void RsBanListConfigItem::clear()
{
    banned_peers.TlvClear() ;
}

std::ostream &RsBanListConfigItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsBanListConfigItem", indent);
    uint16_t int_Indent = indent + 2;
    banned_peers.print(out, int_Indent);

        printRsItemEnd(out, "RsBanListConfigItem", indent);
        return out;
}

/*************************************************************************/



