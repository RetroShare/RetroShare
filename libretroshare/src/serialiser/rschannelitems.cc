
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

void RsChannelReadStatus::clear()
{

	RsDistribChildConfig::clear();

	channelId.clear();
	msgReadStatus.clear();

	return;

}

std::ostream& RsChannelReadStatus::print(std::ostream &out, uint16_t indent = 0)
{

    printRsItemBase(out, "RsChannelMsg", indent);
    uint16_t int_Indent = indent + 2;

    RsDistribChildConfig::print(out, int_Indent);

    printIndent(out, int_Indent);
    out << "ChannelId: " << channelId << std::endl;

    std::map<std::string, uint32_t>::iterator mit = msgReadStatus.begin();

    for(; mit != msgReadStatus.end(); mit++)
    {

        printIndent(out, int_Indent);
        out << "msgId : " << mit->first << std::endl;

        printIndent(out, int_Indent);
        out << " status : " << mit->second << std::endl;

    }

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


uint32_t    RsChannelSerialiser::sizeReadStatus(RsChannelReadStatus *item)
{
	uint32_t s = 8; /* header */
	/* RsDistribChildConfig stuff */

	s += 4; /* save_type */

	/* RsChannelReadStatus stuff */

	s += GetTlvStringSize(item->channelId);

	std::map<std::string, uint32_t>::iterator mit = item->msgReadStatus.begin();

	for(; mit != item->msgReadStatus.end(); mit++)
	{
		s += GetTlvStringSize(mit->first); /* key */
		s += 4; /* value */
	}

	return s;
}

/* serialise the data to the buffer */
bool     RsChannelSerialiser::serialiseReadStatus(RsChannelReadStatus *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReadStatus(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsChannelSerialiser::serialiseReadStatus() Header: " << ok << std::endl;
	std::cerr << "RsChannelSerialiser::serialiseReadStatus() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */

	ok &= setRawUInt32(data, tlvsize, &offset, item->save_type);
	std::cerr << "RsChannelSerialiser::serialiseReadStatus() save_type: " << ok << std::endl;



	/* RsChannelMsg */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, item->channelId);
	std::cerr << "RsChannelSerialiser::serialiseReadStatus() channelId: " << ok << std::endl;

	std::map<std::string, uint32_t>::iterator mit = item->msgReadStatus.begin();

	for(; mit != item->msgReadStatus.end(); mit++)
	{
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSGID, mit->first); /* key */
		ok &= setRawUInt32(data, tlvsize, &offset, mit->second); /* value */
	}

	std::cerr << "RsChannelSerialiser::serialiseReadStatus() msgReadStatus: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsChannelSerialiser::serialiseReadStatus() Size Error! " << std::endl;
	}

	return ok;
}



RsChannelReadStatus *RsChannelSerialiser::deserialiseReadStatus(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_CHANNEL != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_CHANNEL_READ_STATUS != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsChannelReadStatus *item = new RsChannelReadStatus();
	item->clear();

	/* skip the header */
	offset += 8;

	/* RsDistribMsg first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->save_type));

	/* RschannelMsg */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GROUPID, item->channelId);

	std::string key;
	uint32_t value;

	while(offset != rssize)
	{
		key.clear();
		value = 0;

		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSGID, key); /* key */

		/* incomplete key value pair? then fail*/
		if(offset == rssize)
		{
			delete item;
			return NULL;
		}

		ok &= getRawUInt32(data, rssize, &offset, &value); /* value */

		item->msgReadStatus.insert(std::pair<std::string, uint32_t>(key, value));
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/************************************************************/

uint32_t    RsChannelSerialiser::size(RsItem *item)
{
	RsChannelMsg* dcm;
	RsChannelReadStatus* drs;

	if( NULL != ( dcm = dynamic_cast<RsChannelMsg*>(item)))
	{
		return sizeMsg(dcm);
	}
	else if(NULL != (drs = dynamic_cast<RsChannelReadStatus* >(item)))
	{
		return sizeReadStatus(drs);
	}

	return false;
}

bool     RsChannelSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	RsChannelMsg* dcm;
	RsChannelReadStatus* drs;

	if( NULL != ( dcm = dynamic_cast<RsChannelMsg*>(item)))
	{
		return serialiseMsg(dcm, data, pktsize);
	}
	else if(NULL != (drs = dynamic_cast<RsChannelReadStatus* >(item)))
	{
		return serialiseReadStatus(drs, data, pktsize);
	}

	return false;
}

RsItem *RsChannelSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_CHANNEL != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_CHANNEL_MSG:
			return deserialiseMsg(data, pktsize);
		case RS_PKT_SUBTYPE_CHANNEL_READ_STATUS:
			return deserialiseReadStatus(data, pktsize);
		default:
			return NULL;
	}

	return NULL;
}



/*************************************************************************/

