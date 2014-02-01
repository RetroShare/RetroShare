/*
 * libretroshare/src/serialiser: rshistoryitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Thunder.
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

#include "serialiser/rshistoryitems.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsconfigitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsHistoryMsgItem::RsHistoryMsgItem() : RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_HISTORY_CONFIG, RS_PKT_SUBTYPE_DEFAULT)
{
	incoming = false;
	sendTime = 0;
	recvTime = 0;
	msgId = 0;
	saveToDisc = true;
}

RsHistoryMsgItem::~RsHistoryMsgItem()
{
}

void RsHistoryMsgItem::clear()
{
	incoming = false;
	peerId.clear();
	peerName.clear();
	sendTime = 0;
	recvTime = 0;
	message.clear();
	msgId = 0;
	saveToDisc = true;
}

std::ostream& RsHistoryMsgItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsHistoryMsgItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "chatPeerid: " << chatPeerId << std::endl;

	printIndent(out, int_Indent);
	out << "incoming:   " << (incoming ? "1" : "0") << std::endl;

	printIndent(out, int_Indent);
	out << "peerId:     " << peerId << std::endl;

	printIndent(out, int_Indent);
	out << "peerName:   " << peerName << std::endl;

	printIndent(out, int_Indent);
	out << "sendTime:   " << sendTime << std::endl;

	printIndent(out, int_Indent);
	out << "recvTime:   " << recvTime << std::endl;

	printIndent(out, int_Indent);
	std::string cnv_message(message.begin(), message.end());
	out << "message:    " << cnv_message << std::endl;

	printRsItemEnd(out, "RsHistoryMsgItem", indent);
	return out;
}

RsHistorySerialiser::RsHistorySerialiser() : RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_HISTORY_CONFIG)
{
}

RsHistorySerialiser::~RsHistorySerialiser()
{
}

uint32_t RsHistorySerialiser::sizeHistoryMsgItem(RsHistoryMsgItem* item)
{
	uint32_t s = 8; /* header */
	s += 2; /* version */
	s += GetTlvStringSize(item->chatPeerId);
	s += 1; /* incoming */
	s += GetTlvStringSize(item->peerId);
	s += GetTlvStringSize(item->peerName);
	s += 4; /* sendTime */
	s += 4; /* recvTime */
	s += GetTlvStringSize(item->message);

	return s;
}

/* serialise the data to the buffer */
bool RsHistorySerialiser::serialiseHistoryMsgItem(RsHistoryMsgItem* item, void* data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeHistoryMsgItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsHistorySerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsHistorySerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, 0); // version
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LOCATION, item->chatPeerId);
	uint8_t dummy = item->incoming ? 1 : 0;
	ok &= setRawUInt8(data, tlvsize, &offset, dummy);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->peerId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->peerName);
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	ok &= setRawUInt32(data, tlvsize, &offset, item->recvTime);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->message);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsHistorySerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsHistoryMsgItem *RsHistorySerialiser::deserialiseHistoryMsgItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_HISTORY_CONFIG != getRsItemType(rstype)) ||
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
	RsHistoryMsgItem *item = new RsHistoryMsgItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	uint16_t version = 0;
	ok &= getRawUInt16(data, rssize, &offset, &version);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LOCATION, item->chatPeerId);
	uint8_t dummy;
	ok &= getRawUInt8(data, rssize, &offset, &dummy);
	item->incoming = (dummy == 1);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->peerId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->peerName);
	ok &= getRawUInt32(data, rssize, &offset, &(item->sendTime));
	ok &= getRawUInt32(data, rssize, &offset, &(item->recvTime));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->message);

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

uint32_t RsHistorySerialiser::size(RsItem *item)
{
	RsHistoryMsgItem* hi;

	if (NULL != (hi = dynamic_cast<RsHistoryMsgItem*>(item)))
	{
		return sizeHistoryMsgItem(hi);
	}

	return 0;
}

bool RsHistorySerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	RsHistoryMsgItem* hi;

	if (NULL != (hi = dynamic_cast<RsHistoryMsgItem*>(item)))
	{
		return serialiseHistoryMsgItem(hi, data, pktsize);
	}

	return false;
}

RsItem* RsHistorySerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_HISTORY_CONFIG != getRsItemType(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
	case RS_PKT_SUBTYPE_DEFAULT:
		return deserialiseHistoryMsgItem(data, pktsize);
	}

	return NULL;
}

/*************************************************************************/
