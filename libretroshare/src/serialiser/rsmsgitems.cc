
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
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

std::ostream& RsChatMsgItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatMsgItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "QblogMs " << chatFlags << std::endl;

	printIndent(out, int_Indent);
	out << "sendTime:  " << sendTime  << std::endl;

	printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
	out << "msg:  " << cnv_message  << std::endl;

	printRsItemEnd(out, "RsChatMsgItem", indent);
	return out;
}

std::ostream& RsChatStatusItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatStatusItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "Status string: " << status_string << std::endl;
	out << "Flags : " << (void*)flags << std::endl;

	printRsItemEnd(out, "RsChatStatusItem", indent);
	return out;
}

std::ostream& RsChatAvatarItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatAvatarItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "Image size: " << image_size << std::endl;
	printRsItemEnd(out, "RsChatStatusItem", indent);

	return out;
}
RsItem *RsChatSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

#ifdef CHAT_DEBUG
	std::cerr << "deserializing packet..."<< std::endl ;
#endif
	// look what we have...
	if (*pktsize < rssize)    /* check size */
	{
#ifdef CHAT_DEBUG
		std::cerr << "chat deserialisation: not enough size: pktsize=" << *pktsize << ", rssize=" << rssize << std::endl ;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	/* ready to load */

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_CHAT != getRsItemService(rstype))) 
	{
#ifdef CHAT_DEBUG
		std::cerr << "chat deserialisation: wrong type !" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DEFAULT:		return new RsChatMsgItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_STATUS:	return new RsChatStatusItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_AVATAR:	return new RsChatAvatarItem(data,*pktsize) ;
		default:
			std::cerr << "Unknown packet type in chat!" << std::endl ;
			return NULL ;
	}
}

uint32_t RsChatMsgItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4; /* chatFlags */
	s += 4; /* sendTime  */
	s += GetTlvWideStringSize(message);

	return s;
}

uint32_t RsChatStatusItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4 ; // flags
	s += GetTlvStringSize(status_string); 			 /* status */

	return s;
}

uint32_t RsChatAvatarItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4 ;						// size
	s += image_size ;			// data

	return s;
}

RsChatAvatarItem::~RsChatAvatarItem()
{
	if(image_data != NULL)
	{
		delete[] image_data ;
		image_data = NULL ;
	}
}

/* serialise the data to the buffer */
bool RsChatMsgItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, chatFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, sendTime);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, message);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}

bool RsChatStatusItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser serialising chat status item." << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= SetTlvString(data, tlvsize, &offset,TLV_TYPE_STR_MSG, status_string);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}

bool RsChatAvatarItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser serialising chat avatar item." << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset,image_size);

	memcpy((void*)( (unsigned char *)data + offset),image_data,image_size) ;
	offset += image_size ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}
RsChatMsgItem::RsChatMsgItem(void *data,uint32_t size)
	: RsChatItem(RS_PKT_SUBTYPE_DEFAULT)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &chatFlags);
	ok &= getRawUInt32(data, rssize, &offset, &sendTime);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, message);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat msg item." << std::endl ;
#endif
	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsChatStatusItem::RsChatStatusItem(void *data,uint32_t size)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_STATUS)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat status item." << std::endl ;
#endif
	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &flags);
	ok &= GetTlvString(data, rssize, &offset,TLV_TYPE_STR_MSG, status_string);

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsChatAvatarItem::RsChatAvatarItem(void *data,uint32_t size)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_STATUS)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat status item." << std::endl ;
#endif
	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset,&image_size);

	image_data = new unsigned char[image_size] ;
	memcpy(image_data,(void*)((unsigned char*)data+offset),image_size) ;
	offset += image_size ;

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
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

void RsMsgTagType::clear()
{
	text.clear();
	tagId = 0;
	rgb_color = 0;
}


void RsMsgTags::clear()
{
	msgId = 0;
	tagIds.clear();
}

std::ostream& RsMsgTagType::print(std::ostream &out, uint16_t indent)
{

	return out;
}

std::ostream& RsMsgTags::print(std::ostream &out, uint16_t indent)
{

	return out;
}

RsMsgTagType::~RsMsgTagType()
{
	return;
}

RsMsgTags::~RsMsgTags()
{
	return;
}

uint32_t    RsMsgSerialiser::sizeMsgItem(RsMsgItem *item)
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

	if (m_bConfiguration) {
		// serialise msgId too
		s += 4;
	}
	
	return s;
}

/* serialise the data to the buffer */
bool     RsMsgSerialiser::serialiseMsgItem(RsMsgItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeMsgItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->msgFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	ok &= setRawUInt32(data, tlvsize, &offset, item->recvTime);

	ok &= SetTlvWideString(data,tlvsize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);

	ok &= item->msgto.SetTlv(data, tlvsize, &offset);
	ok &= item->msgcc.SetTlv(data, tlvsize, &offset);
	ok &= item->msgbcc.SetTlv(data, tlvsize, &offset);

	ok &= item->attachment.SetTlv(data, tlvsize, &offset);

	if (m_bConfiguration) {
		// serialise msgId too
		ok &= setRawUInt32(data, tlvsize, &offset, item->msgId);
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
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

	ok &= GetTlvWideString(data,rssize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	ok &= item->msgto.GetTlv(data, rssize, &offset);
	ok &= item->msgcc.GetTlv(data, rssize, &offset);
	ok &= item->msgbcc.GetTlv(data, rssize, &offset);
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


uint32_t RsMsgSerialiser::sizeTagItem(RsMsgTagType* item)
{
	uint32_t s = 8; /* header */

	s += GetTlvStringSize(item->text);
	s +=  4; /* color */
	s += 4; /* tag id */

	return s;
}


bool RsMsgSerialiser::serialiseTagItem(RsMsgTagType *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeTagItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data,tlvsize,&offset, TLV_TYPE_STR_NAME, item->text);
	ok &= setRawUInt32(data, tlvsize, &offset, item->rgb_color);
	ok &= setRawUInt32(data, tlvsize, &offset, item->tagId);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
#endif
	}

	return ok;
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

uint32_t RsMsgSerialiser::sizeMsgTagItem(RsMsgTags* item)
{
	uint32_t s = 8; /* header */

	s += 4; /* msgId */
	s += item->tagIds.size() * 4; /* tagIds */

	return s;
}

bool RsMsgSerialiser::serialiseMsgTagItem(RsMsgTags *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeMsgTagItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data,tlvsize,&offset, item->msgId);

	std::list<uint32_t>::iterator mit = item->tagIds.begin();
	for(;mit != item->tagIds.end(); mit++)
	{
		ok &= setRawUInt32(data, tlvsize, &offset, *mit);
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
#endif
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

uint32_t    RsMsgSerialiser::size(RsItem *i)
{
	RsMsgItem *mi;
	RsMsgTagType *mtt;
	RsMsgTags *mts;

	/* in order of frequency */
	if (NULL != (mi = dynamic_cast<RsMsgItem *>(i)))
	{
		return sizeMsgItem(mi);
	}
	else if (NULL != (mtt = dynamic_cast<RsMsgTagType *>(i)))
	{
		return sizeTagItem(mtt);
	}
	else if (NULL != (mts = dynamic_cast<RsMsgTags *>(i)))
	{
		return sizeMsgTagItem(mts);
	}

	return 0;
}

bool     RsMsgSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialise()" << std::endl;
#endif

	RsMsgItem *mi;
	RsMsgTagType *mtt;
	RsMsgTags *mts;

	if (NULL != (mi = dynamic_cast<RsMsgItem *>(i)))
	{
		return serialiseMsgItem(mi, data, pktsize);
	}
	else if (NULL != (mtt = dynamic_cast<RsMsgTagType *>(i)))
	{
		return serialiseTagItem(mtt, data, pktsize);
	}
	else if (NULL != (mts = dynamic_cast<RsMsgTags *>(i)))
	{
		return serialiseMsgTagItem(mts, data, pktsize);
	}
	return false;
}

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
		case RS_PKT_SUBTYPE_MSG_TAG_TYPE:
			return deserialiseTagItem(data, pktsize);
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

