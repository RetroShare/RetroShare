
/*
 * libretroshare/src/serialiser: rsrttitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "rsitems/rsrttitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

#include "serialiser/rstypeserializer.h"

/*************************************************************************/

RsItem *RsRttSerialiser::create_item(uint16_t service,uint8_t type) const
{
	if(service != RS_SERVICE_TYPE_RTT)
		return NULL ;

	switch(type)
	{
	case RS_PKT_SUBTYPE_RTT_PING: return new RsRttPingItem() ; //= 0x01;
	case RS_PKT_SUBTYPE_RTT_PONG: return new RsRttPongItem() ; // = 0x02;
	default:
		return NULL ;
	}
}

void RsRttPingItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
}

void RsRttPongItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mSeqNo,"mSeqNo") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPingTS,"mPingTS") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,mPongTS,"mPongTS") ;
}

#ifdef TO_REMOVE
RsRttPingItem::~RsRttPingItem()
{
	return;
}

void 	RsRttPingItem::clear()
{
	mSeqNo = 0;
	mPingTS = 0;
}

std::ostream& RsRttPingItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsRttPingItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printRsItemEnd(out, "RsRttPingItem", indent);
	return out;
}





RsRttPongItem::~RsRttPongItem()
{
	return;
}

void 	RsRttPongItem::clear()
{
	mSeqNo = 0;
	mPingTS = 0;
	mPongTS = 0;
}


std::ostream& RsRttPongItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsRttPongItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "PongTS: " << std::hex << mPongTS << std::dec << std::endl;

	printRsItemEnd(out, "RsRttPongItem", indent);
	return out;
}


/*************************************************************************/


uint32_t    RsRttSerialiser::sizeRttPingItem(RsRttPingItem */*item*/)
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */

	return s;
}

/* serialise the data to the buffer */
bool     RsRttSerialiser::serialiseRttPingItem(RsRttPingItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeRttPingItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsRttSerialiser::serialiseRttPingItem() Header: " << ok << std::endl;
	std::cerr << "RsRttSerialiser::serialiseRttPingItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, item->mPingTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsRttSerialiser::serialiseRttPingItem() Size Error! " << std::endl;
	}

	return ok;
}

RsRttPingItem *RsRttSerialiser::deserialiseRttPingItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_RTT != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RTT_PING != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsRttPingItem *item = new RsRttPingItem();
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


uint32_t    RsRttSerialiser::sizeRttPongItem(RsRttPongItem */*item*/)
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */
	s += 8; /* pongTS */

	return s;
}

/* serialise the data to the buffer */
bool     RsRttSerialiser::serialiseRttPongItem(RsRttPongItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeRttPongItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsRttSerialiser::serialiseRttPongItem() Header: " << ok << std::endl;
	std::cerr << "RsRttSerialiser::serialiseRttPongItem() Size: " << tlvsize << std::endl;
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
		std::cerr << "RsRttSerialiser::serialiseRttPongItem() Size Error! " << std::endl;
	}

	return ok;
}

RsRttPongItem *RsRttSerialiser::deserialiseRttPongItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_RTT != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RTT_PONG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsRttPongItem *item = new RsRttPongItem();
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

uint32_t    RsRttSerialiser::size(RsItem *i)
{
	RsRttPingItem *ping;
	RsRttPongItem *pong;

	if (NULL != (ping = dynamic_cast<RsRttPingItem *>(i)))
	{
		return sizeRttPingItem(ping);
	}
	else if (NULL != (pong = dynamic_cast<RsRttPongItem *>(i)))
	{
		return sizeRttPongItem(pong);
	}
	return 0;
}

bool     RsRttSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialise()" << std::endl;
#endif

	RsRttPingItem *ping;
	RsRttPongItem *pong;

	if (NULL != (ping = dynamic_cast<RsRttPingItem *>(i)))
	{
		return serialiseRttPingItem(ping, data, pktsize);
	}
	else if (NULL != (pong = dynamic_cast<RsRttPongItem *>(i)))
	{
		return serialiseRttPongItem(pong, data, pktsize);
	}
	return false;
}

RsItem* RsRttSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsRttSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_RTT != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_RTT_PING:
			return deserialiseRttPingItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_RTT_PONG:
			return deserialiseRttPongItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}

	return NULL;
}


/*************************************************************************/
#endif

