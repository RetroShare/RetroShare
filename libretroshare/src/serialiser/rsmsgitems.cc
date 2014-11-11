
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

#include <stdexcept>
#include <time.h>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

void 	RsMsgItem::clear()
{
	msgId    = 0;
	msgFlags = 0;
	sendTime = 0;
	recvTime = 0;
	subject.clear();
	message.clear();

	rspeerid_msgto.TlvClear();
	rspeerid_msgcc.TlvClear();
	rspeerid_msgbcc.TlvClear();

	rsgxsid_msgto.TlvClear();
	rsgxsid_msgcc.TlvClear();
	rsgxsid_msgbcc.TlvClear();

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
		  rspeerid_msgto.print(out, int_Indent);
		  rsgxsid_msgto.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message CC: " << std::endl;
		  rspeerid_msgcc.print(out, int_Indent);
		  rsgxsid_msgcc.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message BCC: " << std::endl;
		  rspeerid_msgbcc.print(out, int_Indent);
		  rsgxsid_msgbcc.print(out, int_Indent);

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

void RsMsgTagType::clear()
{
	text.clear();
	tagId = 0;
	rgb_color = 0;
}

void RsPublicMsgInviteConfigItem::clear()
{
	hash.clear() ;
	time_stamp = 0 ;
}
std::ostream& RsPublicMsgInviteConfigItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPublicMsgInviteConfigItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "hash : " << hash  << std::endl;

	printIndent(out, int_Indent);
	out << "timt : " << time_stamp << std::endl;

	printRsItemEnd(out, "RsPublicMsgInviteConfigItem", indent);

	return out;
}
void RsMsgTags::clear()
{
	msgId = 0;
	tagIds.clear();
}

std::ostream& RsMsgTagType::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgTagType", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "rgb_color : " << rgb_color  << std::endl;


	printIndent(out, int_Indent);
	out << "text: " << text << std::endl;

	printIndent(out, int_Indent);
	out << "tagId: " << tagId << std::endl;

	printRsItemEnd(out, "RsMsgTagTypeItem", indent);

	return out;
}

std::ostream& RsMsgTags::print(std::ostream &out, uint16_t indent)
{

	printRsItemBase(out, "RsMsgTagsItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	std::list<uint32_t>::iterator it;

	for(it=tagIds.begin(); it != tagIds.end(); ++it)
	{
		printIndent(out, int_Indent);
		out << "tagId : " << *it << std::endl;
	}

	printRsItemEnd(out, "RsMsgTags", indent);

	return out;
}

uint32_t    RsMsgItem::serial_size(bool m_bConfiguration)
{
	uint32_t s = 8; /* header */
	s += 4; /* msgFlags */
	s += 4; /* sendTime  */
	s += 4; /* recvTime  */

	s += GetTlvStringSize(subject);
	s += GetTlvStringSize(message);

	s += rspeerid_msgto.TlvSize();
	s += rspeerid_msgcc.TlvSize();
	s += rspeerid_msgbcc.TlvSize();

	s += rsgxsid_msgto.TlvSize();
	s += rsgxsid_msgcc.TlvSize();
	s += rsgxsid_msgbcc.TlvSize();

	s += attachment.TlvSize();

	if (m_bConfiguration) {
		// serialise msgId too
		s += 4;
	}
	
	return s;
}

/* serialise the data to the buffer */
bool     RsMsgItem::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size( config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, msgFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, sendTime);
	ok &= setRawUInt32(data, tlvsize, &offset, recvTime);

	ok &= SetTlvString(data,tlvsize,&offset,TLV_TYPE_STR_SUBJECT,subject);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, message);

	ok &= rspeerid_msgto.SetTlv(data, tlvsize, &offset);
	ok &= rspeerid_msgcc.SetTlv(data, tlvsize, &offset);
	ok &= rspeerid_msgbcc.SetTlv(data, tlvsize, &offset);

	ok &= rsgxsid_msgto.SetTlv(data, tlvsize, &offset);
	ok &= rsgxsid_msgcc.SetTlv(data, tlvsize, &offset);
	ok &= rsgxsid_msgbcc.SetTlv(data, tlvsize, &offset);

	ok &= attachment.SetTlv(data, tlvsize, &offset);

	if (config) // serialise msgId too
		ok &= setRawUInt32(data, tlvsize, &offset, msgId);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMsgItem *RsMsgSerialiser::deserialiseMsgItem(void *data, uint32_t *pktsize)
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

	ok &= GetTlvString(data,rssize,&offset,TLV_TYPE_STR_SUBJECT,item->subject);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->message);

	ok &= item->rspeerid_msgto.GetTlv(data, rssize, &offset);
	ok &= item->rspeerid_msgcc.GetTlv(data, rssize, &offset);
	ok &= item->rspeerid_msgbcc.GetTlv(data, rssize, &offset);
	ok &= item->rsgxsid_msgto.GetTlv(data, rssize, &offset);
	ok &= item->rsgxsid_msgcc.GetTlv(data, rssize, &offset);
	ok &= item->rsgxsid_msgbcc.GetTlv(data, rssize, &offset);

	ok &= item->attachment.GetTlv(data, rssize, &offset);

	if (m_bConfiguration) {
		// deserialise msgId too
		// ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
		getRawUInt32(data, rssize, &offset, &(item->msgId)); //use this line for backward compatibility 
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
uint32_t RsPublicMsgInviteConfigItem::serial_size(bool)
{
	uint32_t s = 8; /* header */

	s += GetTlvStringSize(hash);
	s += 4; /* time_stamp */

	return s;
}

uint32_t RsMsgTagType::serial_size(bool)
{
	uint32_t s = 8; /* header */

	s += GetTlvStringSize(text);
	s +=  4; /* color */
	s += 4; /* tag id */

	return s;
}

bool RsPublicMsgInviteConfigItem::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size(config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data,tlvsize,&offset, TLV_TYPE_STR_HASH_SHA1, hash);
	ok &= setRawUInt32(data,tlvsize,&offset, time_stamp);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
	}

	return ok;
}


bool RsMsgTagType::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size( config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data,tlvsize,&offset, TLV_TYPE_STR_NAME, text);
	ok &= setRawUInt32(data, tlvsize, &offset, rgb_color);
	ok &= setRawUInt32(data, tlvsize, &offset, tagId);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
	}

	return ok;
}
RsPublicMsgInviteConfigItem* RsMsgSerialiser::deserialisePublicMsgInviteConfigItem(void *data,uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_INVITE != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsPublicMsgInviteConfigItem *item = new RsPublicMsgInviteConfigItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvString(data,rssize,&offset,TLV_TYPE_STR_HASH_SHA1,item->hash);

	uint32_t ts ;
	ok &= getRawUInt32(data, rssize, &offset, &ts) ;
	item->time_stamp = ts ;

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


RsMsgTagType* RsMsgSerialiser::deserialiseTagItem(void *data,uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_TAG_TYPE != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgTagType *item = new RsMsgTagType();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= GetTlvString(data,rssize,&offset,TLV_TYPE_STR_NAME,item->text);
	ok &= getRawUInt32(data, rssize, &offset, &(item->rgb_color));
	ok &= getRawUInt32(data, rssize, &offset, &(item->tagId));

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

uint32_t RsMsgTags::serial_size(bool)
{
	uint32_t s = 8; /* header */

	s += 4; /* msgId */
	s += tagIds.size() * 4; /* tagIds */

	return s;
}

bool RsMsgTags::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size( config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data,tlvsize,&offset, msgId);

	std::list<uint32_t>::iterator mit = tagIds.begin();
	for(;mit != tagIds.end(); ++mit)
		ok &= setRawUInt32(data, tlvsize, &offset, *mit);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMsgTags* RsMsgSerialiser::deserialiseMsgTagItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_TAGS != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgTags *item = new RsMsgTags();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &item->msgId);

	uint32_t tagId;
	while (offset != rssize)
	{
		tagId = 0;

		ok &= getRawUInt32(data, rssize, &offset, &tagId);

		item->tagIds.push_back(tagId);
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


/************************************** Message SrcId **********************/

std::ostream& RsMsgSrcId::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgSrcIdItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	printIndent(out, int_Indent);
	out << "SrcId: " << srcId << std::endl;


	printRsItemEnd(out, "RsMsgItem", indent);

	return out;
}

void RsMsgSrcId::clear()
{
	msgId = 0;
	srcId.clear();

	return;
}

uint32_t RsMsgSrcId::serial_size(bool)
{
	uint32_t s = 8; /* header */

	s += 4;
	s += srcId.serial_size() ;

	return s;
}


bool RsMsgSrcId::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size(config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data, tlvsize, &offset, msgId);
	ok &= srcId.serialise(data, tlvsize, offset) ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMsgSrcId* RsMsgSerialiser::deserialiseMsgSrcIdItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_SRC_TAG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgSrcId *item = new RsMsgSrcId();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
	ok &= item->srcId.deserialise(data, rssize, offset);

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

/************************* end of definition of msgSrcId serialisation functions ************************/

/************************************** Message ParentId **********************/

std::ostream& RsMsgParentId::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgParentIdItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	printIndent(out, int_Indent);
	out << "msgParentId: " << msgParentId << std::endl;


	printRsItemEnd(out, "RsMsgParentId", indent);

	return out;
}

void RsMsgParentId::clear()
{
	msgId = 0;
	msgParentId = 0;

	return;
}

uint32_t RsMsgParentId::serial_size(bool)
{
	uint32_t s = 8; /* header */

	s += 4; // srcId
	s += 4; // msgParentId

	return s;
}

bool RsMsgParentId::serialise(void *data, uint32_t& pktsize,bool config)
{
	uint32_t tlvsize = serial_size( config) ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data, tlvsize, &offset, msgId);
	ok &= setRawUInt32(data, tlvsize, &offset, msgParentId);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Size Error! " << std::endl;
	}

	return ok;
}

RsMsgParentId* RsMsgSerialiser::deserialiseMsgParentIdItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_PARENT_TAG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgParentId *item = new RsMsgParentId();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgParentId));

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

/************************* end of definition of msgParentId serialisation functions ************************/

RsItem* RsMsgSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DEFAULT:
			return deserialiseMsgItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_SRC_TAG:
			return deserialiseMsgSrcIdItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_PARENT_TAG:
			return deserialiseMsgParentIdItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_TAG_TYPE:
			return deserialiseTagItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_INVITE:
			return deserialisePublicMsgInviteConfigItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_TAGS:
			return deserialiseMsgTagItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}

	return NULL;
}


/*************************************************************************/

