
/*
 * libretroshare/src/serialiser: rsbaseitems.cc
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
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvbase.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

RsChatItem::~RsChatItem()
{
	return;
}

void 	RsChatItem::clear()
{
	chatFlags = 0;
	sendTime = 0;
	message.clear();
}

std::ostream &RsChatItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsChatItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "chatFlags: " << chatFlags << std::endl;

        printIndent(out, int_Indent);
        out << "sendTime:  " << sendTime  << std::endl;

        printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
        out << "msg:  " << cnv_message  << std::endl;

        printRsItemEnd(out, "RsChatItem", indent);
        return out;
}


uint32_t    RsChatSerialiser::sizeItem(RsChatItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* chatFlags */
	s += 4; /* sendTime  */
	s += GetTlvWideStringSize(item->message);

	return s;
}

/* serialise the data to the buffer */
bool     RsChatSerialiser::serialiseItem(RsChatItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->chatFlags);
	std::cerr << "RsChatSerialiser::serialiseItem() chatFlags: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	std::cerr << "RsChatSerialiser::serialiseItem() sendTime: " << ok << std::endl;
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	std::cerr << "RsChatSerialiser::serialiseItem() Message: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

RsChatItem *RsChatSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_CHAT != getRsItemService(rstype)) ||
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
	RsChatItem *item = new RsChatItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->chatFlags));
	ok &= getRawUInt32(data, rssize, &offset, &(item->sendTime));
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);

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


uint32_t    RsChatSerialiser::size(RsItem *item)
{
	return sizeItem((RsChatItem *) item);
}

bool     RsChatSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseItem((RsChatItem *) item, data, pktsize);
}

RsItem *RsChatSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseItem(data, pktsize);
}



/*************************************************************************/


RsMsgItem::~RsMsgItem()
{
	return;
}

void 	RsMsgItem::clear()
{
	msgId    = 0;
	msgFlags = 0;
	sendTime = 0;
	recvTime = 0;
	subject.clear();
	message.clear();

	msgto.TlvClear();
	msgcc.TlvClear();
	msgbcc.TlvClear();

	attachment.TlvClear();
}

std::ostream &RsMsgItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsMsgItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "msgId (not serialised): " << msgId << std::endl;
        printIndent(out, int_Indent);
        out << "msgFlags: " << msgFlags << std::endl;

        printIndent(out, int_Indent);
        out << "sendTime:  " << sendTime  << std::endl;
        printIndent(out, int_Indent);
        out << "recvTime:  " << recvTime  << std::endl;

        printIndent(out, int_Indent);
        out << "Message To: " << std::endl;
	msgto.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message CC: " << std::endl;
	msgcc.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message BCC: " << std::endl;
	msgbcc.print(out, int_Indent);

        printIndent(out, int_Indent);
	std::string cnv_subject(subject.begin(), subject.end());
        out << "subject:  " << cnv_subject  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_message(message.begin(), message.end());
        out << "msg:  " << cnv_message  << std::endl;

        printIndent(out, int_Indent);
        out << "Attachment: " << std::endl;
	attachment.print(out, int_Indent);

        printRsItemEnd(out, "RsMsgItem", indent);
        return out;
}


uint32_t    RsMsgSerialiser::sizeItem(RsMsgItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* msgFlags */
	s += 4; /* sendTime  */
	s += 4; /* recvTime  */

	s += GetTlvWideStringSize(item->subject);
	s += GetTlvWideStringSize(item->message);

	s += item->msgto.TlvSize();
	s += item->msgcc.TlvSize();
	s += item->msgbcc.TlvSize();
	s += item->attachment.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsMsgSerialiser::serialiseItem(RsMsgItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsMsgSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseItem() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->msgFlags);
	std::cerr << "RsMsgSerialiser::serialiseItem() msgFlags: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	std::cerr << "RsMsgSerialiser::serialiseItem() sendTime: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->recvTime);
	std::cerr << "RsMsgSerialiser::serialiseItem() recvTime: " << ok << std::endl;

	ok &= SetTlvWideString(data,tlvsize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	std::cerr << "RsMsgSerialiser::serialiseItem() Subject: " << ok << std::endl;
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	std::cerr << "RsMsgSerialiser::serialiseItem() Message: " << ok << std::endl;

	ok &= item->msgto.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsMsgSerialiser::serialiseItem() MsgTo: " << ok << std::endl;
	ok &= item->msgcc.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsMsgSerialiser::serialiseItem() MsgCC: " << ok << std::endl;
	ok &= item->msgbcc.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsMsgSerialiser::serialiseItem() MsgBCC: " << ok << std::endl;

	ok &= item->attachment.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsMsgSerialiser::serialiseItem() Attachment: " << ok << std::endl;
	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMsgItem *RsMsgSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
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
	RsMsgItem *item = new RsMsgItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgFlags));
	ok &= getRawUInt32(data, rssize, &offset, &(item->sendTime));
	ok &= getRawUInt32(data, rssize, &offset, &(item->recvTime));

	ok &= GetTlvWideString(data,rssize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	ok &= item->msgto.GetTlv(data, rssize, &offset);
	ok &= item->msgcc.GetTlv(data, rssize, &offset);
	ok &= item->msgbcc.GetTlv(data, rssize, &offset);
	ok &= item->attachment.GetTlv(data, rssize, &offset);

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

uint32_t    RsMsgSerialiser::size(RsItem *item)
{
	return sizeItem((RsMsgItem *) item);
}

bool     RsMsgSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseItem((RsMsgItem *) item, data, pktsize);
}

RsItem *RsMsgSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseItem(data, pktsize);
}


/*************************************************************************/

