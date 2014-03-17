/*
 * libretroshare/src/serialiser: rsgxsrecogitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2013-2013 by Robert Fernie.
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsgxsrecognitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsGxsRecognReqItem::~RsGxsRecognReqItem()
{
	return;
}

void 	RsGxsRecognReqItem::clear()
{
	issued_at = 0;
	period = 0;
	tag_class = 0;
	tag_type = 0;

	identity.clear();
	nickname.clear();
	comment.clear();

	sign.TlvClear();

}

std::ostream &RsGxsRecognReqItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsRecognReqItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "issued_at: " << issued_at << std::endl;

	printIndent(out, int_Indent);
	out << "period: " << period << std::endl;

	printIndent(out, int_Indent);
	out << "tag_class: " << tag_class << std::endl;

	printIndent(out, int_Indent);
	out << "tag_type: " << tag_type << std::endl;

	printIndent(out, int_Indent);
	out << "identity: " << identity << std::endl;

	printIndent(out, int_Indent);
	out << "nickname: " << nickname << std::endl;

	printIndent(out, int_Indent);
	out << "comment: " << comment << std::endl;

	printIndent(out, int_Indent);
	out << "signature: " << std::endl;
	sign.print(out, int_Indent + 2);

	printRsItemEnd(out, "RsGxsRecognReqItem", indent);
	return out;
}


uint32_t    RsGxsRecognSerialiser::sizeReq(RsGxsRecognReqItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; // issued_at;
	s += 4; // period;
	s += 2; // tag_class;
	s += 2; // tag_type;
	s += item->identity.serial_size();
	s += GetTlvStringSize(item->nickname);
	s += GetTlvStringSize(item->comment);
	s += item->sign.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsGxsRecognSerialiser::serialiseReq(RsGxsRecognReqItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGxsRecognSerialiser::serialiseReq() Header: " << ok << std::endl;
	std::cerr << "RsGxsRecognSerialiser::serialiseReq() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= setRawUInt32(data, tlvsize, &offset, item->issued_at);
        ok &= setRawUInt32(data, tlvsize, &offset, item->period);
        ok &= setRawUInt16(data, tlvsize, &offset, item->tag_class);
        ok &= setRawUInt16(data, tlvsize, &offset, item->tag_type);


        ok &= item->identity.serialise(data, tlvsize, offset);
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->nickname);
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->comment);

	ok &= item->sign.SetTlv(data, tlvsize, &offset);


	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGxsRecognSerialiser::serialiseReq() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsGxsRecognReqItem *RsGxsRecognSerialiser::deserialiseReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_GXS_RECOGN != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RECOGN_REQ != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsGxsRecognReqItem *item = new RsGxsRecognReqItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= getRawUInt32(data, tlvsize, &offset, &(item->issued_at));
        ok &= getRawUInt32(data, tlvsize, &offset, &(item->period));
        ok &= getRawUInt16(data, tlvsize, &offset, &(item->tag_class));
        ok &= getRawUInt16(data, tlvsize, &offset, &(item->tag_type));


        ok &= item->identity.serialise(data, tlvsize, offset);
	ok &= GetTlvString(data, tlvsize, &offset, 1, item->nickname);
	ok &= GetTlvString(data, tlvsize, &offset, 1, item->comment);
	ok &= item->sign.GetTlv(data, tlvsize, &offset);


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

RsGxsRecognTagItem::~RsGxsRecognTagItem()
{
	return;
}

void 	RsGxsRecognTagItem::clear()
{
	valid_from = 0;
	valid_to = 0;

	tag_class = 0;
	tag_type = 0;

	identity.clear();
	nickname.clear();

	sign.TlvClear();
}

std::ostream &RsGxsRecognTagItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsRecognTagItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "valid_from: " << valid_from << std::endl;

	printIndent(out, int_Indent);
	out << "valid_to: " << valid_to << std::endl;

	printIndent(out, int_Indent);
	out << "tag_class: " << tag_class << std::endl;

	printIndent(out, int_Indent);
	out << "tag_type: " << tag_type << std::endl;

	printIndent(out, int_Indent);
	out << "identity: " << identity << std::endl;

	printIndent(out, int_Indent);
	out << "nickname: " << nickname << std::endl;

	printIndent(out, int_Indent);
	out << "signature: " << std::endl;
	sign.print(out, int_Indent + 2);

	printRsItemEnd(out, "RsGxsRecognTagItem", indent);
	return out;
}


uint32_t    RsGxsRecognSerialiser::sizeTag(RsGxsRecognTagItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; // valid_from;
	s += 4; // valid_to;
	s += 2; // tag_class;
	s += 2; // tag_type;

	s += item->identity.serial_size();
	s += GetTlvStringSize(item->nickname);

	s += item->sign.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsGxsRecognSerialiser::serialiseTag(RsGxsRecognTagItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeTag(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGxsRecognSerialiser::serialiseTag() Header: " << ok << std::endl;
	std::cerr << "RsGxsRecognSerialiser::serialiseTag() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= setRawUInt32(data, tlvsize, &offset, item->valid_from);
        ok &= setRawUInt32(data, tlvsize, &offset, item->valid_to);

        ok &= setRawUInt16(data, tlvsize, &offset, item->tag_class);
        ok &= setRawUInt16(data, tlvsize, &offset, item->tag_type);


	ok &= item->identity.serialise(data, tlvsize, offset);
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->nickname);

	ok &= item->sign.SetTlv(data, tlvsize, &offset);


	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGxsRecognSerialiser::serialiseTag() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsGxsRecognTagItem *RsGxsRecognSerialiser::deserialiseTag(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_GXS_RECOGN != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RECOGN_TAG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsGxsRecognTagItem *item = new RsGxsRecognTagItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= getRawUInt32(data, tlvsize, &offset, &(item->valid_from));
        ok &= getRawUInt32(data, tlvsize, &offset, &(item->valid_to));

        ok &= getRawUInt16(data, tlvsize, &offset, &(item->tag_class));
        ok &= getRawUInt16(data, tlvsize, &offset, &(item->tag_type));


	ok &= item->identity.deserialise(data, tlvsize, offset);
	ok &= GetTlvString(data, tlvsize, &offset, 1, item->nickname);
	ok &= item->sign.GetTlv(data, tlvsize, &offset);


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

RsGxsRecognSignerItem::~RsGxsRecognSignerItem()
{
	return;
}

void 	RsGxsRecognSignerItem::clear()
{
	signing_classes.TlvClear();
	key.TlvClear();
	sign.TlvClear();
}

std::ostream &RsGxsRecognSignerItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsRecognSignerItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "signing_classes: " << std::endl;
	signing_classes.print(out, int_Indent + 2);

	printIndent(out, int_Indent);
	out << "key: " << std::endl;
	key.print(out, int_Indent + 2);

	printIndent(out, int_Indent);
	out << "signature: " << std::endl;
	sign.print(out, int_Indent + 2);


	printRsItemEnd(out, "RsGxsRecognSignerItem", indent);
	return out;
}




uint32_t    RsGxsRecognSerialiser::sizeSigner(RsGxsRecognSignerItem *item)
{
	uint32_t s = 8; /* header */
	s += item->signing_classes.TlvSize();
	s += item->key.TlvSize();
	s += item->sign.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsGxsRecognSerialiser::serialiseSigner(RsGxsRecognSignerItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeSigner(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGxsRecognSerialiser::serialiseSigner() Header: " << ok << std::endl;
	std::cerr << "RsGxsRecognSerialiser::serialiseSigner() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->signing_classes.SetTlv(data, tlvsize, &offset);
	ok &= item->key.SetTlv(data, tlvsize, &offset);
	ok &= item->sign.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGxsRecognSerialiser::serialiseSigner() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsGxsRecognSignerItem *RsGxsRecognSerialiser::deserialiseSigner(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_GXS_RECOGN != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RECOGN_SIGNER != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsGxsRecognSignerItem *item = new RsGxsRecognSignerItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->signing_classes.GetTlv(data, tlvsize, &offset);
	ok &= item->key.GetTlv(data, tlvsize, &offset);
	ok &= item->sign.GetTlv(data, tlvsize, &offset);

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

uint32_t    RsGxsRecognSerialiser::size(RsItem *i)
{
	RsGxsRecognReqItem *rqi;
	RsGxsRecognTagItem *rti;
	RsGxsRecognSignerItem *rsi;

	if (NULL != (rqi = dynamic_cast<RsGxsRecognReqItem *>(i)))
	{
		return sizeReq(rqi);
	}
	if (NULL != (rti = dynamic_cast<RsGxsRecognTagItem *>(i)))
	{
		return sizeTag(rti);
	}
	if (NULL != (rsi = dynamic_cast<RsGxsRecognSignerItem *>(i)))
	{
		return sizeSigner(rsi);
	}
	return 0;
}

bool     RsGxsRecognSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsGxsRecognReqItem *rri;
	RsGxsRecognTagItem *rti;
	RsGxsRecognSignerItem *rsi;

	if (NULL != (rri = dynamic_cast<RsGxsRecognReqItem *>(i)))
	{
		return serialiseReq(rri, data, pktsize);
	}
	if (NULL != (rti = dynamic_cast<RsGxsRecognTagItem *>(i)))
	{
		return serialiseTag(rti, data, pktsize);
	}
	if (NULL != (rsi = dynamic_cast<RsGxsRecognSignerItem *>(i)))
	{
		return serialiseSigner(rsi, data, pktsize);
	}
	return false;
}

RsItem *RsGxsRecognSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_GXS_RECOGN != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_RECOGN_REQ:
			return deserialiseReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_RECOGN_TAG:
			return deserialiseTag(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_RECOGN_SIGNER:
			return deserialiseSigner(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
}

/*************************************************************************/



