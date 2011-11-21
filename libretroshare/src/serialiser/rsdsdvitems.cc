/*
 * libretroshare/src/serialiser: rsgameitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
#include "serialiser/rsdsdvitems.h"
#include "serialiser/rstlvdsdv.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsDsdvRouteItem::~RsDsdvRouteItem()
{
	return;
}

void 	RsDsdvRouteItem::clear()
{
	routes.TlvClear();
}

std::ostream &RsDsdvRouteItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDsdvRouteItem", indent);
	uint16_t int_Indent = indent + 2;
	routes.print(out, int_Indent);

        printRsItemEnd(out, "RsDsdvRouteItem", indent);
        return out;
}


uint32_t    RsDsdvSerialiser::sizeItem(RsDsdvRouteItem *item)
{
	uint32_t s = 8; /* header */
	s += item->routes.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsDsdvSerialiser::serialiseItem(RsDsdvRouteItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDsdvSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsDsdvSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->routes.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDsdvSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDsdvRouteItem *RsDsdvSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DSDV != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DSDV_ROUTE != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsDsdvRouteItem *item = new RsDsdvRouteItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->routes.GetTlv(data, tlvsize, &offset);

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


uint32_t    RsDsdvSerialiser::size(RsItem *item)
{
	return sizeItem((RsDsdvRouteItem *) item);
}

bool     RsDsdvSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseItem((RsDsdvRouteItem *) item, data, pktsize);
}

RsItem *RsDsdvSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseItem(data, pktsize);
}

/*************************************************************************/



