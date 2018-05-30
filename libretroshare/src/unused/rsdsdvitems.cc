/*******************************************************************************
 * libretroshare/src/unused: rsdsdvitems.cc                                    *
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


uint32_t    RsDsdvSerialiser::sizeRoute(RsDsdvRouteItem *item)
{
	uint32_t s = 8; /* header */
	s += item->routes.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsDsdvSerialiser::serialiseRoute(RsDsdvRouteItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeRoute(item);
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
	ok &= item->routes.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDsdvSerialiser::serialiseRoute() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDsdvRouteItem *RsDsdvSerialiser::deserialiseRoute(void *data, uint32_t *pktsize)
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

/*************************************************************************/

RsDsdvDataItem::~RsDsdvDataItem()
{
	return;
}

void 	RsDsdvDataItem::clear()
{
	src.TlvClear();
	dest.TlvClear();
	ttl = 0;
	data.TlvClear();
}

std::ostream &RsDsdvDataItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDsdvDataItem", indent);
	uint16_t int_Indent = indent + 2;
	src.print(out, int_Indent);
	dest.print(out, int_Indent);

	printIndent(out, int_Indent);
        out << "TTL: " << ttl << std::endl;

	data.print(out, int_Indent);

        printRsItemEnd(out, "RsDsdvDataItem", indent);
        return out;
}


uint32_t    RsDsdvSerialiser::sizeData(RsDsdvDataItem *item)
{
	uint32_t s = 8; /* header */
	s += item->src.TlvSize();
	s += item->dest.TlvSize();
	s += 4;
	s += item->data.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsDsdvSerialiser::serialiseData(RsDsdvDataItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeData(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDsdvSerialiser::serialiseData() Header: " << ok << std::endl;
	std::cerr << "RsDsdvSerialiser::serialiseData() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->src.SetTlv(data, tlvsize, &offset);
	ok &= item->dest.SetTlv(data, tlvsize, &offset);
	ok &= setRawUInt32(data, tlvsize, &offset, item->ttl);
	ok &= item->data.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDsdvSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDsdvDataItem *RsDsdvSerialiser::deserialiseData(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DSDV != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DSDV_DATA != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsDsdvDataItem *item = new RsDsdvDataItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->src.GetTlv(data, tlvsize, &offset);
	ok &= item->dest.GetTlv(data, tlvsize, &offset);
	ok &= getRawUInt32(data, tlvsize, &offset, &(item->ttl));
	ok &= item->data.GetTlv(data, tlvsize, &offset);

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

uint32_t    RsDsdvSerialiser::size(RsItem *i)
{
	RsDsdvRouteItem *dri;
	RsDsdvDataItem *ddi;

	if (NULL != (dri = dynamic_cast<RsDsdvRouteItem *>(i)))
	{
		return sizeRoute(dri);
	}
	if (NULL != (ddi = dynamic_cast<RsDsdvDataItem *>(i)))
	{
		return sizeData(ddi);
	}
	return 0;
}

bool     RsDsdvSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsDsdvRouteItem *dri;
	RsDsdvDataItem *ddi;

	if (NULL != (dri = dynamic_cast<RsDsdvRouteItem *>(i)))
	{
		return serialiseRoute(dri, data, pktsize);
	}
	if (NULL != (ddi = dynamic_cast<RsDsdvDataItem *>(i)))
	{
		return serialiseData(ddi, data, pktsize);
	}
	return false;
}

RsItem *RsDsdvSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DSDV != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DSDV_ROUTE:
			return deserialiseRoute(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DSDV_DATA:
			return deserialiseData(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
}

/*************************************************************************/



