
/*
 * libretroshare/src/serialiser: rschannelitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2008 by Robert Fernie.
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

#include "serialiser/rschannelitems.h"

#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

void 	RsChannelMsg::clear()
{
	RsDistribMsg::clear();

	subject.clear();
	message.clear();

	attachment.TlvClear();
	thumbnail.TlvClear();
}

std::ostream &RsChannelMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsChannelMsg", indent);
	uint16_t int_Indent = indent + 2;

	RsDistribMsg::print(out, int_Indent);

        printIndent(out, int_Indent);

	std::string cnv_subject(subject.begin(), subject.end());
        out << "subject:  " << cnv_subject  << std::endl;

        printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
        out << "message:  " << cnv_message  << std::endl;

	printIndent(out, int_Indent);
	out << "Attachment: " << std::endl;
	attachment.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "Thumbnail: " << std::endl;
	thumbnail.print(out, int_Indent);

        printRsItemEnd(out, "RsChannelMsg", indent);
        return out;
}


/*************************************************************************/
/*************************************************************************/

uint32_t    RsChannelSerialiser::sizeMsg(RsChannelMsg *item)
{
	uint32_t s = 8; /* header */
	/* RsDistribMsg stuff */
	s += GetTlvStringSize(item->grpId);
	s += 4; /* timestamp */

	/* RsChannelMsg stuff */
	s += GetTlvWideStringSize(item->subject);
	s += GetTlvWideStringSize(item->message);
	s += item->attachment.TlvSize();
	s += item->thumbnail.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsChannelSerialiser::serialiseMsg(RsChannelMsg *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeMsg(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsChannelSerialiser::serialiseMsg() Header: " << ok << std::endl;
	std::cerr << "RsChannelSerialiser::serialiseMsg() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
	std::cerr << "RsChannelSerialiser::serialiseMsg() grpId: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->timestamp);
	std::cerr << "RsChannelSerialiser::serialiseMsg() timestamp: " << ok << std::endl;

	/* RsChannelMsg */
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_SUBJECT, item->subject);
	std::cerr << "RsChannelSerialiser::serialiseMsg() Title: " << ok << std::endl;
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	std::cerr << "RsChannelSerialiser::serialiseMsg() Msg: " << ok << std::endl;

	ok &= item->attachment.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsChannelSerialiser::serialiseMsg() Attachment: " << ok << std::endl;

	ok &= item->thumbnail.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsChannelSerialiser::serialiseMsg() thumbnail: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsChannelSerialiser::serialiseMsg() Size Error! " << std::endl;
	}

	return ok;
}



RsChannelMsg *RsChannelSerialiser::deserialiseMsg(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_CHANNEL != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_CHANNEL_MSG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsChannelMsg *item = new RsChannelMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
	ok &= getRawUInt32(data, rssize, &offset, &(item->timestamp));

	/* RsChannelMsg */
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_SUBJECT, item->subject);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);
    ok &= item->attachment.GetTlv(data, rssize, &offset);
    ok &= item->thumbnail.GetTlv(data, rssize, &offset);


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


uint32_t    RsChannelSerialiser::size(RsItem *item)
{
	return sizeMsg((RsChannelMsg *) item);
}

bool     RsChannelSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseMsg((RsChannelMsg *) item, data, pktsize);
}

RsItem *RsChannelSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseMsg(data, pktsize);
}



/*************************************************************************/

