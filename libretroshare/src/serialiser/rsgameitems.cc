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
#include "serialiser/rsgameitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsGameItem::~RsGameItem()
{
	return;
}

void 	RsGameItem::clear()
{
	serviceId = 0;
	numPlayers = 0;
	msg = 0;

	gameId.clear();
	gameComment.clear();
	players.TlvClear();
}

std::ostream &RsGameItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsGameItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "serviceId: " << serviceId << std::endl;
        printIndent(out, int_Indent);
        out << "numPlayers:  " << numPlayers << std::endl;
        printIndent(out, int_Indent);
        out << "msg:  " << msg << std::endl;

        printIndent(out, int_Indent);
        out << "gameId:  " << gameId << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_comment(gameComment.begin(), gameComment.end());
        out << "msg:  " << cnv_comment  << std::endl;

        printIndent(out, int_Indent);
        out << "Players Ids: " << std::endl;
	players.print(out, int_Indent);

        printRsItemEnd(out, "RsGameItem", indent);
        return out;
}


uint32_t    RsGameSerialiser::sizeItem(RsGameItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* serviceId */
	s += 4; /* numPlayers  */
	s += 4; /* msg */
	s += GetTlvStringSize(item->gameId);
	s += GetTlvWideStringSize(item->gameComment);
	s += item->players.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsGameSerialiser::serialiseItem(RsGameItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGameSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsGameSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->serviceId);
	ok &= setRawUInt32(data, tlvsize, &offset, item->numPlayers);
	ok &= setRawUInt32(data, tlvsize, &offset, item->msg);

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->gameId);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_COMMENT, item->gameComment);
	ok &= item->players.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGameSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsGameItem *RsGameSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_GAME_LAUNCHER != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsGameItem *item = new RsGameItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= getRawUInt32(data, tlvsize, &offset, &(item->serviceId));
	ok &= getRawUInt32(data, tlvsize, &offset, &(item->numPlayers));
	ok &= getRawUInt32(data, tlvsize, &offset, &(item->msg));

	ok &= GetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->gameId);
	ok &= GetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_COMMENT, item->gameComment);
	ok &= item->players.GetTlv(data, tlvsize, &offset);

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


uint32_t    RsGameSerialiser::size(RsItem *item)
{
	return sizeItem((RsGameItem *) item);
}

bool     RsGameSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseItem((RsGameItem *) item, data, pktsize);
}

RsItem *RsGameSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseItem(data, pktsize);
}

/*************************************************************************/



