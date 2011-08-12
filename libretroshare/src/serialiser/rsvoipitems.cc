
/*
 * libretroshare/src/serialiser: rsvoipitems.cc
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
#include "serialiser/rsvoipitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/


RsVoipPingItem::~RsVoipPingItem()
{
	return;
}

void 	RsVoipPingItem::clear()
{
	mSeqNo = 0;
	mPingTS = 0;
}

std::ostream& RsVoipPingItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipPingItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipPingItem", indent);
	return out;
}





RsVoipPongItem::~RsVoipPongItem()
{
	return;
}

void 	RsVoipPongItem::clear()
{
	mSeqNo = 0;
	mPingTS = 0;
	mPongTS = 0;
}


std::ostream& RsVoipPongItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipPongItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "PongTS: " << std::hex << mPongTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipPongItem", indent);
	return out;
}


/*************************************************************************/


uint32_t    RsVoipSerialiser::sizeVoipPingItem(RsVoipPingItem */*item*/)
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */

	return s;
}

/* serialise the data to the buffer */
bool     RsVoipSerialiser::serialiseVoipPingItem(RsVoipPingItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeVoipPingItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, item->mPingTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size Error! " << std::endl;
	}

	return ok;
}

RsVoipPingItem *RsVoipSerialiser::deserialiseVoipPingItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_VOIP != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_VOIP_PING != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsVoipPingItem *item = new RsVoipPingItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->mSeqNo));
	ok &= getRawUInt64(data, rssize, &offset, &(item->mPingTS));

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

/*************************************************************************/
/*************************************************************************/


uint32_t    RsVoipSerialiser::sizeVoipPongItem(RsVoipPongItem */*item*/)
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */
	s += 8; /* pongTS */

	return s;
}

/* serialise the data to the buffer */
bool     RsVoipSerialiser::serialiseVoipPongItem(RsVoipPongItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeVoipPongItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, item->mPingTS);
	ok &= setRawUInt64(data, tlvsize, &offset, item->mPongTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Size Error! " << std::endl;
	}

	return ok;
}

RsVoipPongItem *RsVoipSerialiser::deserialiseVoipPongItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_VOIP != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_VOIP_PONG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsVoipPongItem *item = new RsVoipPongItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->mSeqNo));
	ok &= getRawUInt64(data, rssize, &offset, &(item->mPingTS));
	ok &= getRawUInt64(data, rssize, &offset, &(item->mPongTS));

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

/*************************************************************************/

uint32_t    RsVoipSerialiser::size(RsItem *i)
{
	RsVoipPingItem *ping;
	RsVoipPongItem *pong;

	if (NULL != (ping = dynamic_cast<RsVoipPingItem *>(i)))
	{
		return sizeVoipPingItem(ping);
	}
	else if (NULL != (pong = dynamic_cast<RsVoipPongItem *>(i)))
	{
		return sizeVoipPongItem(pong);
	}
	return 0;
}

bool     RsVoipSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialise()" << std::endl;
#endif

	RsVoipPingItem *ping;
	RsVoipPongItem *pong;

	if (NULL != (ping = dynamic_cast<RsVoipPingItem *>(i)))
	{
		return serialiseVoipPingItem(ping, data, pktsize);
	}
	else if (NULL != (pong = dynamic_cast<RsVoipPongItem *>(i)))
	{
		return serialiseVoipPongItem(pong, data, pktsize);
	}
	return false;
}

RsItem* RsVoipSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_VOIP != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_VOIP_PING:
			return deserialiseVoipPingItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_VOIP_PONG:
			return deserialiseVoipPongItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}

	return NULL;
}


/*************************************************************************/

