
/*
 * libretroshare/src/serialiser: rsblogitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2010 by Cyril, Chris Parker .
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

#include "serialiser/rsblogitems.h"

#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

void 	RsBlogMsg::clear()
{
	RsDistribMsg::clear();

	subject.clear();
	message.clear();

	std::list<RsTlvImage >::iterator it = graphic_set.begin();

	for(; it != graphic_set.end(); it++)
	{
		it->TlvClear();
	}

	graphic_set.clear();
}

std::ostream &RsBlogMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsBlogMsg", indent);
	uint16_t int_Indent = indent + 2;

	RsDistribMsg::print(out, int_Indent);

        printIndent(out, int_Indent);

	std::string cnv_subject(subject.begin(), subject.end());
        out << "subject:  " << cnv_subject  << std::endl;

        printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
        out << "message:  " << cnv_message  << std::endl;

    	std::list<RsTlvImage >::iterator it = graphic_set.begin();

    	for(; it != graphic_set.end(); it++)
    	{
    		it->print(out, int_Indent);
    	}


        printRsItemEnd(out, "RsBlogMsg", indent);
        return out;
}


/*************************************************************************/
/*************************************************************************/

uint32_t    RsBlogSerialiser::sizeMsg(RsBlogMsg *item)
{
	uint32_t s = 8; /* header */
	/* RsDistribMsg stuff */
	s += GetTlvStringSize(item->grpId);
	s += GetTlvStringSize(item->parentId);
	s += GetTlvStringSize(item->threadId);
	s += 4; /* timestamp */

	/* RsBlogMsg stuff */
	s += GetTlvWideStringSize(item->subject);
	s += GetTlvWideStringSize(item->message);

	std::list<RsTlvImage >::iterator it = item->graphic_set.begin();

	for(; it != item->graphic_set.end(); it++)
	{
		s += it->TlvSize();
	}

	return s;
}

/* serialise the data to the buffer */
bool     RsBlogSerialiser::serialiseMsg(RsBlogMsg *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeMsg(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsBlogSerialiser::serialiseMsg() Header: " << ok << std::endl;
	std::cerr << "RsBlogSerialiser::serialiseMsg() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
	std::cerr << "RsBlogSerialiser::serialiseMsg() grpId: " << ok << std::endl;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PARENTID, item->parentId);
	std::cerr << "RsBlogSerialiser::serialiseMsg() parentId: " << ok << std::endl;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_THREADID, item->threadId);
	std::cerr << "RsBlogSerialiser::serialiseMsg() threadpId: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->timestamp);
	std::cerr << "RsBlogSerialiser::serialiseMsg() timestamp: " << ok << std::endl;

	/* RsBlogMsg */
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_SUBJECT, item->subject);
	std::cerr << "RsBlogSerialiser::serialiseMsg() subject: " << ok << std::endl;
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	std::cerr << "RsBlogSerialiser::serialiseMsg() msg: " << ok << std::endl;

	std::list<RsTlvImage >::iterator it = item->graphic_set.begin();

	for(; it != item->graphic_set.end(); it++)
	{
		ok &= it->SetTlv(data, tlvsize, &offset);
	}

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsBlogSerialiser::serialiseMsg() Size Error! " << std::endl;
	}

	return ok;
}



RsBlogMsg *RsBlogSerialiser::deserialiseMsg(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_QBLOG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_BLOG_MSG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsBlogMsg *item = new RsBlogMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PARENTID, item->parentId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_THREADID, item->threadId);
	ok &= getRawUInt32(data, rssize, &offset, &(item->timestamp));

	/* RsBlogMsg */

	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_SUBJECT, item->subject);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);

	RsTlvImage image;
	image.TlvClear();

	while(offset != rssize)
	{
		image.GetTlv(data, rssize, &offset);
		item->graphic_set.push_back(image);
		image.TlvClear();
	}

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


uint32_t RsBlogSerialiser::size(RsItem *item)
{
	return sizeMsg((RsBlogMsg *) item);
}

bool RsBlogSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseMsg((RsBlogMsg *) item, data, pktsize);
}

RsItem *RsBlogSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseMsg(data, pktsize);
}



/*************************************************************************/

