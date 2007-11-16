
/*
 * libretroshare/src/serialiser: rsbaseitems.cc
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
#include "serialiser/rsbaseitems.h"

RsFileItem::~RsFileItem()
{
	return;
}

void 	RsFileItem::clear()
{
	file.TlvClear();
	reqType = 0;
}

uint32_t    RsFileItemSerialiser::size(RsItem *i)
{
	RsFileItem *item = (RsFileItem *) i;
	uint32_t s = 12; /* header + 4 for reqType */
	s += item->file.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsFileItemSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsFileItem *item = (RsFileItem *) i;

	uint32_t tlvsize = size(item);
	uint32_t offset = 8;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->reqType);
	ok &= item->file.SetTlv(data, tlvsize, &offset);

	return ok;
}

RsItem *RsFileItemSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE_ITEM != getRsItemType(rstype)) ||
		(0 != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileItem *item = new RsFileItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->reqType));
	ok &= item->file.GetTlv(data, rssize, &offset);

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}



