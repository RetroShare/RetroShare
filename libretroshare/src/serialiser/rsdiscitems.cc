
/*
 * libretroshare/src/serialiser: rsdiscitems.cc
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
#include "serialiser/rsbaseserial.h"

#include "serialiser/rsserviceids.h"
#include "serialiser/rsdiscitems.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

uint32_t    RsDiscSerialiser::size(RsItem *i)
{
	RsDiscItem  *rdi;
	RsDiscReply *rdr;

	/* do reply first - as it is derived from Item */
	if (NULL != (rdr = dynamic_cast<RsDiscReply *>(i)))
	{
		return sizeReply(rdr);
	}
	else if (NULL != (rdi = dynamic_cast<RsDiscItem *>(i)))
	{
		return sizeItem(rdi);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsDiscSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsDiscItem  *rdi;
	RsDiscReply *rdr;

	/* do reply first - as it is derived from Item */
	if (NULL != (rdr = dynamic_cast<RsDiscReply *>(i)))
	{
		return serialiseReply(rdr, data, pktsize);
	}
	else if (NULL != (rdi = dynamic_cast<RsDiscItem *>(i)))
	{
		return serialiseItem(rdi, data, pktsize);
	}

	return false;
}

RsItem *RsDiscSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)))
	{
		std::cerr << "RsDiscSerialiser::deserialise() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DISC_REPLY:
			return deserialiseReply(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_ITEM:
			return deserialiseItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsDiscItem::~RsDiscItem()
{
	return;
}

void 	RsDiscItem::clear()
{
	memset(&laddr, 0, sizeof(laddr));
	memset(&saddr, 0, sizeof(laddr));
	contact_tf = 0;
	discFlags = 0;
}

std::ostream &RsDiscItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDiscItem", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Local Address: " << inet_ntoa(laddr.sin_addr);
	out << " Port: " << ntohs(laddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Server Address: " << inet_ntoa(saddr.sin_addr);
	out << " Port: " << ntohs(saddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Contact TimeFrame: " << contact_tf;
        out << std::endl;

        printIndent(out, int_Indent);
        out << "DiscFlags:  " << discFlags  << std::endl;

        printRsItemEnd(out, "RsDiscItem", indent);
        return out;
}


uint32_t    RsDiscSerialiser::sizeItem(RsDiscItem *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvIpAddrPortV4Size(); /* laddr */
	s += GetTlvIpAddrPortV4Size(); /* saddr */
	s += 2; /* contact_tf */
	s += 4; /* discFlags  */

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseItem(RsDiscItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG 
	std::cerr << "RsDiscSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, 
					TLV_TYPE_IPV4_LOCAL, &(item->laddr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, 
					TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= setRawUInt16(data, tlvsize, &offset, item->contact_tf);
	ok &= setRawUInt32(data, tlvsize, &offset, item->discFlags);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscItem *RsDiscSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_ITEM != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseItem() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseItem() Not Enough Space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscItem *item = new RsDiscItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, 
					TLV_TYPE_IPV4_LOCAL, &(item->laddr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, 
					TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= getRawUInt16(data, rssize, &offset, &(item->contact_tf));
	ok &= getRawUInt32(data, rssize, &offset, &(item->discFlags));

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseItem() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseItem() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/


RsDiscReply::~RsDiscReply()
{
	return;
}

void 	RsDiscReply::clear()
{
	memset(&laddr, 0, sizeof(laddr));
	memset(&saddr, 0, sizeof(laddr));
	contact_tf = 0;
	discFlags = 0;
	aboutId.clear();
	certDER.TlvClear();
}

std::ostream &RsDiscReply::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDiscReply", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Local Address: " << inet_ntoa(laddr.sin_addr);
	out << " Port: " << ntohs(laddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Server Address: " << inet_ntoa(saddr.sin_addr);
	out << " Port: " << ntohs(saddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Contact TimeFrame: " << contact_tf;
        out << std::endl;

        printIndent(out, int_Indent);
        out << "DiscFlags:  " << discFlags  << std::endl;

        printIndent(out, int_Indent);
        out << "AboutId:  " << aboutId  << std::endl;
	certDER.print(out, int_Indent);

        printRsItemEnd(out, "RsDiscReply", indent);
        return out;
}


uint32_t    RsDiscSerialiser::sizeReply(RsDiscReply *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvIpAddrPortV4Size(); /* laddr */
	s += GetTlvIpAddrPortV4Size(); /* saddr */
	s += 2; /* connect_tr */
	s += 4; /* discFlags  */
	s += GetTlvStringSize(item->aboutId);
	s += item->certDER.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseReply(RsDiscReply *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReply(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG 
	std::cerr << "RsDiscSerialiser::serialiseReply() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseReply() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->laddr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= setRawUInt16(data, tlvsize, &offset, item->contact_tf);
	ok &= setRawUInt32(data, tlvsize, &offset, item->discFlags);

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->aboutId);

	ok &= item->certDER.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::serialiseReply() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscReply *RsDiscSerialiser::deserialiseReply(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_REPLY != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseReply() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseReply() pktsize != rssize" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscReply *item = new RsDiscReply();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, 
					TLV_TYPE_IPV4_LOCAL, &(item->laddr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, 
					TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= getRawUInt16(data, rssize, &offset, &(item->contact_tf));
	ok &= getRawUInt32(data, rssize, &offset, &(item->discFlags));

	ok &= GetTlvString(data, rssize, &offset, 
					TLV_TYPE_STR_PEERID, item->aboutId);
	ok &= item->certDER.GetTlv(data, rssize, &offset);


	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseReply() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG 
		std::cerr << "RsDiscSerialiser::deserialiseReply() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}



/*************************************************************************/
