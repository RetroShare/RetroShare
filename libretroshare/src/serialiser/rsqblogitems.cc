
/*
 * libretroshare/src/serialiser: rsqblogitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker.
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
#include "serialiser/rsqblogitems.h"
#include "serialiser/rstlvbase.h"

#include <iostream>

/************************************************************/

RsQblogItem::~RsQblogItem(void)
{
	return;
}

void RsQblogItem::clear()
{
	blogMsg.first = 0;
	blogMsg.second = "";
	status = "";
}


std::ostream &RsQblogItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsQblogItem", indent);
		uint16_t int_Indent = indent + 2;
		
		/* print out the content of the item */
        printIndent(out, int_Indent);
        out << "blogMsg(time): " << blogMsg.first << std::endl;
        printIndent(out, int_Indent);
        out << "blogMsg(message): " << blogMsg.second << std::endl;   
        printIndent(out, int_Indent);     
        out << "status  " << status  << std::endl;
        
        printRsItemEnd(out, "RsQblogItem", indent);
        return out;
}



uint32_t RsQblogSerialiser::sizeItem(RsQblogItem *item)
{
	uint32_t s = 8; // for header size
   	s += 4; // blog creation time
   	s += GetTlvStringSize(item->blogMsg.second); // string part of blog
   	s += GetTlvStringSize(item->status);
   	s += GetTlvStringSize(item->favSong);
   	
   	return s;
}

/*******************************************************************************/

bool RsQblogSerialiser::serialiseItem(RsQblogItem* item, void* data, uint32_t *size)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif
	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->blogMsg.first);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->blogMsg.second);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->status);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->favSong);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

/**************************************************************************/

RsQblogItem* RsQblogSerialiser::deserialiseItem(void * data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_QBLOG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;
	
	/* ready to load */
	RsQblogItem *item = new RsQblogItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->blogMsg.first));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->blogMsg.second);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->status);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->favSong);

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

/*********************************************************************/

bool RsQblogSerialiser::serialise(RsItem *item, void* data, uint32_t* size)
{
	return serialiseItem((RsQblogItem *) item, data, size);
}

RsItem* RsQblogSerialiser::deserialise(void* data, uint32_t* size)
{
	return deserialiseItem(data, size);
}

uint32_t RsQblogSerialiser::size(RsItem *item)
{
	return sizeItem((RsQblogItem *) item);
}




