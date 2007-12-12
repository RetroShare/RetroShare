
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
#include "serialiser/rsbaseitems.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

uint32_t    RsFileItemSerialiser::size(RsItem *i)
{
	RsFileRequest *rfr;
	RsFileData    *rfd;

	if (NULL != (rfr = dynamic_cast<RsFileRequest *>(i)))
	{
		return sizeReq(rfr);
	}
	else if (NULL != (rfd = dynamic_cast<RsFileData *>(i)))
	{
		return sizeData(rfd);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsFileItemSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsFileRequest *rfr;
	RsFileData    *rfd;

	if (NULL != (rfr = dynamic_cast<RsFileRequest *>(i)))
	{
		return serialiseReq(rfr, data, pktsize);
	}
	else if (NULL != (rfd = dynamic_cast<RsFileData *>(i)))
	{
		return serialiseData(rfd, data, pktsize);
	}

	return false;
}

RsItem *RsFileItemSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE != getRsItemType(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_FI_REQUEST:
			return deserialiseReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_DATA:
			return deserialiseData(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsFileRequest::~RsFileRequest()
{
	return;
}

void 	RsFileRequest::clear()
{
	file.TlvClear();
	fileoffset = 0;
	chunksize  = 0;
}

std::ostream &RsFileRequest::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileRequest", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "FileOffset: " << fileoffset << std::endl;
        out << "ChunkSize:  " << chunksize  << std::endl;
	file.print(out, int_Indent);
        printRsItemEnd(out, "RsFileRequest", indent);
        return out;
}


uint32_t    RsFileItemSerialiser::sizeReq(RsFileRequest *item)
{
	uint32_t s = 8; /* header */
	s += 8; /* offset */
	s += 4; /* chunksize */
	s += item->file.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsFileItemSerialiser::serialiseReq(RsFileRequest *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsFileItemSerialiser::serialiseReq() Header: " << ok << std::endl;
	std::cerr << "RsFileItemSerialiser::serialiseReq() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, item->fileoffset);
	std::cerr << "RsFileItemSerialiser::serialiseReq() Fileoffset: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunksize);
	std::cerr << "RsFileItemSerialiser::serialiseReq() Chunksize: " << ok << std::endl;
	ok &= item->file.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsFileItemSerialiser::serialiseReq() FileItem: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseReq() Size Error! " << std::endl;
	}

	return ok;
}

RsFileRequest *RsFileItemSerialiser::deserialiseReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_REQUEST != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileRequest *item = new RsFileRequest();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &(item->fileoffset));
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunksize));
	ok &= item->file.GetTlv(data, rssize, &offset);

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


/*************************************************************************/

RsFileData::~RsFileData()
{
	return;
}

void 	RsFileData::clear()
{
	fd.TlvClear();
}

std::ostream &RsFileData::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileData", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
	fd.print(out, int_Indent);
        printRsItemEnd(out, "RsFileData", indent);
        return out;
}


uint32_t    RsFileItemSerialiser::sizeData(RsFileData *item)
{
	uint32_t s = 8; /* header  */
	s += item->fd.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsFileItemSerialiser::serialiseData(RsFileData *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeData(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->fd.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsFileItemSerialiser::serialiseData() TlvFileData: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}

RsFileData *RsFileItemSerialiser::deserialiseData(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_DATA != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileData *item = new RsFileData();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->fd.GetTlv(data, rssize, &offset);

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


/*************************************************************************/
/*************************************************************************/

uint32_t    RsCacheItemSerialiser::size(RsItem *i)
{
	RsCacheRequest *rfr;
	RsCacheItem    *rfi;

	if (NULL != (rfr = dynamic_cast<RsCacheRequest *>(i)))
	{
		return sizeReq(rfr);
	}
	else if (NULL != (rfi = dynamic_cast<RsCacheItem *>(i)))
	{
		return sizeItem(rfi);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsCacheItemSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsCacheRequest *rfr;
	RsCacheItem    *rfi;

	if (NULL != (rfr = dynamic_cast<RsCacheRequest *>(i)))
	{
		return serialiseReq(rfr, data, pktsize);
	}
	else if (NULL != (rfi = dynamic_cast<RsCacheItem *>(i)))
	{
		return serialiseItem(rfi, data, pktsize);
	}

	return false;
}

RsItem *RsCacheItemSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_CACHE != getRsItemType(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_CACHE_REQUEST:
			return deserialiseReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_CACHE_ITEM:
			return deserialiseItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsCacheRequest::~RsCacheRequest()
{
	return;
}

void 	RsCacheRequest::clear()
{
	cacheType  = 0;
	cacheSubId = 0;
	file.TlvClear();
}

std::ostream &RsCacheRequest::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsCacheRequest", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "CacheId: " << cacheType << "/" << cacheSubId << std::endl;
	file.print(out, int_Indent);
        printRsItemEnd(out, "RsCacheRequest", indent);
        return out;
}


uint32_t    RsCacheItemSerialiser::sizeReq(RsCacheRequest *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* type/subid */
	s += item->file.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsCacheItemSerialiser::serialiseReq(RsCacheRequest *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsCacheItemSerialiser::serialiseReq() Header: " << ok << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheType);
	std::cerr << "RsCacheItemSerialiser::serialiseReq() cacheType: " << ok << std::endl;
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheSubId);
	std::cerr << "RsCacheItemSerialiser::serialiseReq() cacheSubId: " << ok << std::endl;
	ok &= item->file.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsCacheItemSerialiser::serialiseReq() FileItem: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseReq() Size Error! " << std::endl;
	}

	return ok;
}

RsCacheRequest *RsCacheItemSerialiser::deserialiseReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_CACHE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_CACHE_REQUEST != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsCacheRequest *item = new RsCacheRequest();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt16(data, rssize, &offset, &(item->cacheType));
	ok &= getRawUInt16(data, rssize, &offset, &(item->cacheSubId));
	ok &= item->file.GetTlv(data, rssize, &offset);

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


/*************************************************************************/


RsCacheItem::~RsCacheItem()
{
	return;
}

void 	RsCacheItem::clear()
{
	cacheType  = 0;
	cacheSubId = 0;
	file.TlvClear();
}

std::ostream &RsCacheItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsCacheItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "CacheId: " << cacheType << "/" << cacheSubId << std::endl;
	file.print(out, int_Indent);
        printRsItemEnd(out, "RsCacheItem", indent);
        return out;
}


uint32_t    RsCacheItemSerialiser::sizeItem(RsCacheItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* type/subid */
	s += item->file.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsCacheItemSerialiser::serialiseItem(RsCacheItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsCacheItemSerialiser::serialiseItem() Header: " << ok << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheType);
	std::cerr << "RsCacheItemSerialiser::serialiseItem() cacheType: " << ok << std::endl;
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheSubId);
	std::cerr << "RsCacheItemSerialiser::serialiseItem() cacheSubId: " << ok << std::endl;
	ok &= item->file.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsCacheItemSerialiser::serialiseItem() FileItem: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

RsCacheItem *RsCacheItemSerialiser::deserialiseItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_CACHE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_CACHE_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsCacheItem *item = new RsCacheItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt16(data, rssize, &offset, &(item->cacheType));
	ok &= getRawUInt16(data, rssize, &offset, &(item->cacheSubId));
	ok &= item->file.GetTlv(data, rssize, &offset);

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


/*************************************************************************/
/*************************************************************************/

uint32_t    RsServiceSerialiser::size(RsItem *i)
{
	RsRawItem *item = dynamic_cast<RsRawItem *>(i);

	if (item)
	{
		return item->getRawLength();
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsServiceSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsRawItem *item = dynamic_cast<RsRawItem *>(i);
	if (!item)
	{
		return false;
	}

	uint32_t tlvsize = item->getRawLength();
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	if (tlvsize > getRsPktMaxSize())
		return false; /* packet too big */

	*pktsize = tlvsize;

	/* its serialised already!!! */
	memcpy(data, item->getRawData(), tlvsize);

	return true;
}

RsItem *RsServiceSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	if (rssize > getRsPktMaxSize())
		return NULL; /* packet too big */

	/* set the packet length */
	*pktsize = rssize;

	RsRawItem *item = new RsRawItem(rstype, rssize);
	void *item_data = item->getRawData();

	memcpy(item_data, data, rssize);

	return item;
}


