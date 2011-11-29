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

/*************************************************************************/

uint32_t    RsBanListSerialiser::size(RsItem *i)
{
	RsBanListItem *dri;

	if (NULL != (dri = dynamic_cast<RsBanListItem *>(i)))
	{
		return sizeList(dri);
	}
	return 0;
}

bool     RsBanListSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsBanListItem *dri;

	if (NULL != (dri = dynamic_cast<RsBanListItem *>(i)))
	{
		return serialiseList(dri, data, pktsize);
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
		default:
			return NULL;
			break;
	}
}

/*************************************************************************/



