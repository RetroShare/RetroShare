
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

#include <iostream>

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsqblogitems.h"
#include "serialiser/rstlvbase.h"


/************************************************************/

RsQblogMsg::~RsQblogMsg(void)
{
	return;
}

void RsQblogMsg::clear()
{
	timeStamp = 0;
	blogMsg.clear();
}


std::ostream &RsQblogMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsQblogMsg", indent);
		uint16_t int_Indent = indent + 2;
		
		/* print out the content of the item */
        printIndent(out, int_Indent);
        out << "blogMsg(time): " << timeStamp << std::endl;
        printIndent(out, int_Indent);
        std::string cnv_blog(blogMsg.begin(), blogMsg.end());
        out << "blogMsg(message): " << cnv_blog << std::endl;   
        printIndent(out, int_Indent);     
        return out;
}



uint32_t RsQblogMsgSerialiser::sizeItem(RsQblogMsg *item)
{
	uint32_t s = 8; // for header size
   	s += 4; // blog creation time
 	s += GetTlvWideStringSize(item->blogMsg); // size of actual blog
   	  	
   	return s;
}

/*******************************************************************************/

bool RsQblogMsgSerialiser::serialiseItem(RsQblogMsg* item, void* data, uint32_t *size)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsQblogSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsQblogSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif
	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->timeStamp);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->blogMsg);


	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsQblogSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

/**************************************************************************/

RsQblogMsg* RsQblogMsgSerialiser::deserialiseItem(void * data, uint32_t *size)
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
	RsQblogMsg *item = new RsQblogMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->timeStamp));
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->blogMsg);

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

bool RsQblogMsgSerialiser::serialise(RsItem *item, void* data, uint32_t* size)
{
	return serialiseItem((RsQblogMsg *) item, data, size);
}

RsItem* RsQblogMsgSerialiser::deserialise(void* data, uint32_t* size)
{
	return deserialiseItem(data, size);
}

uint32_t RsQblogMsgSerialiser::size(RsItem *item)
{
	return sizeItem((RsQblogMsg *) item);
}

/********************************************* RsQblogProfile section ***********************************/
/********************************************* RsQblogProfile section ***********************************/
/********************************************* RsQblogProfile section ***********************************/

RsQblogProfile::~RsQblogProfile(void)
{
	return;
}

void RsQblogProfile::clear()
{
	timeStamp = 0;
	openProfile.TlvClear();
	favoriteFiles.TlvClear();
}


std::ostream &RsQblogProfile::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsQblogProfile", indent);
        uint16_t int_Indent = indent + 2;
        out << "RsQblogProfile::print() : timeStamp" << timeStamp;
        out << std::endl;
        openProfile.print(out, int_Indent);
        favoriteFiles.print(out, int_Indent);
        favoriteFiles.print(out, int_Indent);
        printRsItemEnd(out, "RsQblogProfile", indent);
        return out;
}



uint32_t RsQblogProfileSerialiser::sizeItem(RsQblogProfile *item)
{
	uint32_t s = 8; // for header size
	s += 4; // time stamp
	s += item->openProfile.TlvSize();
	s += item->favoriteFiles.TlvSize();
   	  	
   	return s;
}

/*******************************************************************************/

bool RsQblogProfileSerialiser::serialiseItem(RsQblogProfile* item, void* data, uint32_t *size)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsQblogSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsQblogSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif
	/* skip the header */
	offset += 8;

	/* add mandatory part */
	ok &= setRawUInt32(data, tlvsize, &offset, item->timeStamp);
	ok &= item->openProfile.SetTlv(data, *size, &offset);
	ok &= item->favoriteFiles.SetTlv(data, *size, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsQblogSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

/**************************************************************************/

RsQblogProfile* RsQblogProfileSerialiser::deserialiseItem(void * data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_QBLOG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_QBLOG_PROFILE != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;
	
	/* ready to load */
	RsQblogProfile *item = new RsQblogProfile();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	RsTlvKeyValueSet* kvSetOpen;
	RsTlvFileSet* fSet;
	
	ok &= getRawUInt32(data, rssize, &offset, &(item->timeStamp));
	ok &= kvSetOpen->GetTlv(data, *size, &offset);
	ok &= fSet->GetTlv(data, *size, &offset);

	/* copy over deserialised files */
	
	item->openProfile = *kvSetOpen;
	item->favoriteFiles = *fSet;
	
	if (offset != rssize)
	{
		/* error, improper item */
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

bool RsQblogProfileSerialiser::serialise(RsItem *item, void* data, uint32_t* size)
{
	return serialiseItem((RsQblogProfile *) item, data, size);
}

RsItem* RsQblogProfileSerialiser::deserialise(void* data, uint32_t* size)
{
	return deserialiseItem(data, size);
}

uint32_t RsQblogProfileSerialiser::size(RsItem *item)
{
	return sizeItem((RsQblogProfile *) item);
}


