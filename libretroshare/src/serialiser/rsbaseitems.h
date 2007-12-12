#ifndef RS_BASE_ITEMS_H
#define RS_BASE_ITEMS_H

/*
 * libretroshare/src/serialiser: rsbaseitems.h
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

#include <map>

#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

const uint8_t RS_PKT_TYPE_FILE          = 0x01;
const uint8_t RS_PKT_TYPE_CACHE         = 0x02;

const uint8_t RS_PKT_SUBTYPE_FI_REQUEST  = 0x01;
const uint8_t RS_PKT_SUBTYPE_FI_DATA     = 0x02;
const uint8_t RS_PKT_SUBTYPE_FI_TRANSFER = 0x03;

const uint8_t RS_PKT_SUBTYPE_CACHE_ITEM    = 0x01;
const uint8_t RS_PKT_SUBTYPE_CACHE_REQUEST = 0x02;

/**************************************************************************/

class RsFileRequest: public RsItem
{
	public:
	RsFileRequest() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE,
		RS_PKT_SUBTYPE_FI_REQUEST)
	{ return; }
virtual ~RsFileRequest();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint64_t fileoffset;  /* start of data requested */
	uint32_t chunksize;   /* size of data requested */
	RsTlvFileItem file;   /* file information */
};

/**************************************************************************/

class RsFileData: public RsItem
{
	public:
	RsFileData() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE, 
		RS_PKT_SUBTYPE_FI_DATA)
	{ return; }
virtual ~RsFileData();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvFileData fd;
};

/**************************************************************************/

class RsFileItemSerialiser: public RsSerialType
{
	public:
	RsFileItemSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE)
	{ return; }
virtual     ~RsFileItemSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* sub types */
virtual	uint32_t    sizeReq(RsFileRequest *);
virtual	bool        serialiseReq (RsFileRequest *item, void *data, uint32_t *size);
virtual	RsFileRequest *  deserialiseReq(void *data, uint32_t *size);

virtual	uint32_t    sizeData(RsFileData *);
virtual	bool        serialiseData (RsFileData *item, void *data, uint32_t *size);
virtual	RsFileData *  deserialiseData(void *data, uint32_t *size);

};

/**************************************************************************/

class RsCacheRequest: public RsItem
{
	public:
	RsCacheRequest() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_CACHE,
		RS_PKT_SUBTYPE_CACHE_REQUEST)
	{ return; }
virtual ~RsCacheRequest();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint16_t cacheType;
	uint16_t cacheSubId;
	RsTlvFileItem file;   /* file information */
};

/**************************************************************************/

class RsCacheItem: public RsItem
{
	public:
	RsCacheItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_CACHE,
		RS_PKT_SUBTYPE_CACHE_ITEM)
	{ return; }
virtual ~RsCacheItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint16_t cacheType;
	uint16_t cacheSubId;
	RsTlvFileItem file;   /* file information */
};

/**************************************************************************/

class RsCacheItemSerialiser: public RsSerialType
{
	public:
	RsCacheItemSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_CACHE)
	{ return; }
virtual     ~RsCacheItemSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* sub types */
virtual	uint32_t    sizeReq(RsCacheRequest *);
virtual	bool        serialiseReq (RsCacheRequest *item, void *data, uint32_t *size);
virtual	RsCacheRequest *  deserialiseReq(void *data, uint32_t *size);

virtual	uint32_t    sizeItem(RsCacheItem *);
virtual	bool        serialiseItem (RsCacheItem *item, void *data, uint32_t *size);
virtual	RsCacheItem *  deserialiseItem(void *data, uint32_t *size);

};

/**************************************************************************/

class RsServiceSerialiser: public RsSerialType
{
	public:
	RsServiceSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, 0, 0)
	{ return; }
virtual     ~RsServiceSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

#endif

