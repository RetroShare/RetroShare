
/*
 * libretroshare/src/serialiser: rsmailtransportitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie.
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

#include <stdexcept>
#include <time.h>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsmailtransportitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/
// RsMailMimeItem

bool  	RsMailChunkItem::isPartial()
{
	return (mPartCount != 1);
}


void 	RsMailChunkItem::clear()
{
	mMailId.TlvClear();
	mPartCount = 0;
	mMailIndex = 0;
	mWholeMailId.clear();
	mMessage.clear();
}

std::ostream &RsMailChunkItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsMailChunkItem", indent);
	uint16_t int_Indent = indent + 2;

	mMailId.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "PartCount: " << mPartCount << std::endl;
        printIndent(out, int_Indent);
        out << "MailIndex: " << mMailIndex << std::endl;

        printIndent(out, int_Indent);
        out << "WholeMailId:  " << mWholeMailId.toStdString()  << std::endl;

        printIndent(out, int_Indent);
        out << "mMessage:  " << mMessage  << std::endl;

        printRsItemEnd(out, "RsMailChunkItem", indent);
        return out;
}

uint32_t    RsMailChunkItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += mMailId.TlvSize(); /* mMailId */
	s += 2; /* mPartCount  */
	s += 2; /* mMailIndex   */
	s += mWholeMailId.serial_size(); /* mRecvTime  */
	s += GetTlvStringSize(mMessage);

	return s;
}

/* serialise the data to the buffer */
bool     RsMailChunkItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMailChunkItem::serialise() Header: " << ok << std::endl;
	std::cerr << "RsMailChunkItem::serialise() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= mMailId.SetTlv(data, tlvsize, &offset);
	ok &= setRawUInt16(data, tlvsize, &offset, mPartCount);
	ok &= setRawUInt16(data, tlvsize, &offset, mMailIndex);
	ok &= mWholeMailId.serialise(data, tlvsize, offset);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, mMessage);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMailMimeSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMailChunkItem *RsMailTransportSerialiser::deserialiseChunkItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(mServiceType != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MAIL_TRANSPORT_CHUNK != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMailChunkItem *item = new RsMailChunkItem(mServiceType);
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->mMailId.GetTlv(data, rssize, &offset);
	ok &= getRawUInt16(data, rssize, &offset, &(item->mPartCount));
	ok &= getRawUInt16(data, rssize, &offset, &(item->mMailIndex));
	ok &= item->mWholeMailId.deserialise(data, rssize, offset);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->mMessage);

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


void RsMailAckItem::clear()
{
	mMailId.TlvClear();
}

std::ostream& RsMailAckItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsMailAckItem", indent);
	uint16_t int_Indent = indent + 2;

	mMailId.print(out, int_Indent);

	printRsItemEnd(out, "RsMailAckItem", indent);

	return out;
}

uint32_t RsMailAckItem::serial_size()
{
	uint32_t s = 8; /* header */

	s += mMailId.TlvSize();
	return s;
}


bool RsMailAckItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMailAckItem::serialise() Header: " << ok << std::endl;
	std::cerr << "RsMailAckItem::serialise() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= mMailId.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMailAckItem::serialise() Size Error! " << std::endl;
	}

	return ok;
}


RsMailAckItem* RsMailTransportSerialiser::deserialiseAckItem(void *data,uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(mServiceType != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MAIL_TRANSPORT_ACK != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMailAckItem *item = new RsMailAckItem(mServiceType);
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= item->mMailId.GetTlv(data, rssize, &offset);

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

/**************************/

RsItem* RsMailTransportSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMailTransportSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(mServiceType != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_MAIL_TRANSPORT_CHUNK:
			return deserialiseChunkItem(data, pktsize);
			break;

		case RS_PKT_SUBTYPE_MAIL_TRANSPORT_ACK:
			return deserialiseAckItem(data, pktsize);
			break;

		default:
			return NULL;
			break;
	}

	return NULL;
}


bool RsMailTransportSerialiser::serialise(RsItem *item, void *data, uint32_t *size){
	if (item->PacketService() != mServiceType)
	{
		return false;
	}
	return dynamic_cast<RsMailTransportItem*>(item)->serialise(data,*size);
}



/*************************************************************************/

