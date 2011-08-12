/*
 * libretroshare/src/serialiser: rsstatusitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Vinny Do.
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

#include "serialiser/rsstatusitems.h"
#include "serialiser/rsbaseserial.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsStatusItem::~RsStatusItem()
{
	return;
}

void 	RsStatusItem::clear()
{
	sendTime = 0;
	status = 0;
}

std::ostream &RsStatusItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsStatusItem", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "sendTime: " << sendTime  << std::endl;

        printIndent(out, int_Indent);
	out << "status: " << status << std::endl;

        printRsItemEnd(out, "RsStatusItem", indent);
        return out;
}

uint32_t    RsStatusSerialiser::sizeItem(RsStatusItem */*item*/)
{
	uint32_t s = 8; /* header */
	s += 4; /* sendTime  */
	s += 4; /* status */

	return s;
}

/* serialise the data to the buffer */
bool     RsStatusSerialiser::serialiseItem(RsStatusItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsStatusSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsStatusSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	ok &= setRawUInt32(data, tlvsize, &offset, item->status);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsStatusSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsStatusItem *RsStatusSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_STATUS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsStatusItem *item = new RsStatusItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->sendTime));
	ok &= getRawUInt32(data, rssize, &offset, &(item->status));

	if (offset != rssize)
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

uint32_t    RsStatusSerialiser::size(RsItem *item)
{
	return sizeItem((RsStatusItem *) item);
}

bool     RsStatusSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseItem((RsStatusItem *) item, data, pktsize);
}

RsItem *RsStatusSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseItem(data, pktsize);
}

/*************************************************************************/
