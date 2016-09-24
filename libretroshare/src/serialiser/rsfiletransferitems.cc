
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
#include "serialiser/rstlvbase.h"
#include "serialiser/rsfiletransferitems.h"

/***
 * #define RSSERIAL_DEBUG 1
 * #define DEBUG_TRANSFERS 1
***/


#ifdef DEBUG_TRANSFERS
	#include "util/rsprint.h"
#endif

#include <iostream>

/**********************************************************************************************/
/*                                          SERIALISER STUFF                                  */
/**********************************************************************************************/

RsFileTransferItem *RsFileTransferSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER != getRsItemService(rstype)) 
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_FT_CACHE_ITEM:        return deserialise_RsFileTransferCacheItem(data, *pktsize);
		case RS_PKT_SUBTYPE_FT_DATA_REQUEST:      return deserialise_RsFileTransferDataRequestItem(data, *pktsize);
		case RS_PKT_SUBTYPE_FT_DATA:              return deserialise_RsFileTransferDataItem(data, *pktsize);
		case RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST: return deserialise_RsFileTransferChunkMapRequestItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_FT_CHUNK_MAP:         return deserialise_RsFileTransferChunkMapItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST: return deserialise_RsFileTransferSingleChunkCrcRequestItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_FT_CHUNK_CRC:         return deserialise_RsFileTransferSingleChunkCrcItem(data,*pktsize) ;
		default:
																std::cerr << "RsFileTransferSerialiser::deserialise(): Could not de-serialise item. SubPacket id = " << std::hex << getRsItemSubType(rstype) << " id = " << rstype << std::dec << std::endl;
			return NULL;
	}
	return NULL;
}

/**********************************************************************************************/
/*                                          OUTPUTS                                           */
/**********************************************************************************************/

std::ostream& RsFileTransferCacheItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferCacheItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "CacheId: " << cacheType << "/" << cacheSubId << std::endl;
	file.print(out, int_Indent);
	printRsItemEnd(out, "RsFileTransferCacheItem", indent);
	return out;
}
std::ostream& RsFileTransferDataRequestItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferDataRequestItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "FileOffset: " << fileoffset;
	out << " ChunkSize:  " << chunksize  << std::endl;
	file.print(out, int_Indent);
	printRsItemEnd(out, "RsFileTransferDataRequestItem", indent);
	return out;
}
std::ostream& RsFileTransferChunkMapItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferChunkMapItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
	printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
	printIndent(out, int_Indent); out << "chunks: " << std::hex << compressed_map._map[0] << std::dec << "..." << std::endl ;
	printRsItemEnd(out, "RsFileTransferChunkMapItem", indent);
	return out;
}
std::ostream& RsFileTransferChunkMapRequestItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferChunkMapRequestItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
	printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
	printRsItemEnd(out, "RsFileTransferChunkMapRequestItem", indent);
	return out;
}
std::ostream& RsFileTransferSingleChunkCrcRequestItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferSingleChunkCrcRequestItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
	printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
	printIndent(out, int_Indent); out << " chunk: " << chunk_number << "..." << std::endl ;
	printRsItemEnd(out, "RsFileTransferSingleChunkCrcRequestItem", indent);
	return out;
}
std::ostream& RsFileTransferSingleChunkCrcItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferSingleChunkCrcItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
	printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
	printIndent(out, int_Indent); out << " chunk: " << chunk_number << "..." << std::endl ;
	printIndent(out, int_Indent); out << "  sha1: " << check_sum.toStdString() << "..." << std::endl ;
	printRsItemEnd(out, "RsFileTransferSingleChunkCrcItem", indent);
	return out;
}
std::ostream& RsFileTransferDataItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileTransferDataItem", indent);
	uint16_t int_Indent = indent + 2;
	fd.print(out, int_Indent);
	printRsItemEnd(out, "RsFileTransferDataItem", indent);
	return out;
}

/**********************************************************************************************/
/*                                          SERIAL SIZE                                       */
/**********************************************************************************************/

uint32_t RsFileTransferDataRequestItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 8; /* offset */
	s += 4; /* chunksize */
	s += file.TlvSize();

	return s;
}
uint32_t RsFileTransferCacheItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4; /* type/subid */
	s += file.TlvSize();

	return s;
}
uint32_t    RsFileTransferDataItem::serial_size()
{
	uint32_t s = 8; /* header  */
	s += fd.TlvSize();

	return s;
}
uint32_t RsFileTransferChunkMapRequestItem::serial_size()
{
	uint32_t s = 8; /* header  */
	s += 1 ; 							// is_client
    s += hash.serial_size() ; // hash

	return s;
}
uint32_t RsFileTransferChunkMapItem::serial_size()
{
	uint32_t s = 8; /* header  */
	s += 1 ; 								// is_client
    s += hash.serial_size() ; 	// hash
	s += 4 ;									// compressed map size
	s += 4 * compressed_map._map.size() ; // compressed chunk map

	return s;
}
uint32_t RsFileTransferSingleChunkCrcItem::serial_size()
{
	uint32_t s = 8; /* header  */
	s += hash.serial_size() ; 	// hash
	s += 4 ; // chunk number
	s += check_sum.serial_size() ; // sha1

	return s;
}
uint32_t RsFileTransferSingleChunkCrcRequestItem::serial_size()
{
	uint32_t s = 8; /* header  */
    s += hash.serial_size() ; 	// hash
    s += 4 ; // chunk number

	return s;
}

/*************************************************************************/

void RsFileTransferDataRequestItem::clear()
{
	file.TlvClear();
	fileoffset = 0;
	chunksize  = 0;
}
void RsFileTransferCacheItem::clear()
{
	cacheType  = 0;
	cacheSubId = 0;
	file.TlvClear();
}
void RsFileTransferDataItem::clear()
{
	fd.TlvClear();
}
 
/**********************************************************************************************/
/*                                         SERIALISATION                                      */
/**********************************************************************************************/

bool RsFileTransferItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset)
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}

/* serialise the data to the buffer */
bool RsFileTransferChunkMapRequestItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt8(data, tlvsize, &offset, is_client);
    ok &= hash.serialise(data, tlvsize, offset) ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}
bool RsFileTransferSingleChunkCrcRequestItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
    ok &= hash.serialise(data, tlvsize, offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_number) ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}
bool RsFileTransferSingleChunkCrcItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
    ok &= hash.serialise(data, tlvsize, offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_number) ;

	ok &= check_sum.serialise(data,tlvsize,offset) ;

	//ok &= setRawUInt32(data, tlvsize, &offset, check_sum.fourbytes[0]) ;
	//ok &= setRawUInt32(data, tlvsize, &offset, check_sum.fourbytes[1]) ;
	//ok &= setRawUInt32(data, tlvsize, &offset, check_sum.fourbytes[2]) ;
	//ok &= setRawUInt32(data, tlvsize, &offset, check_sum.fourbytes[3]) ;
	//ok &= setRawUInt32(data, tlvsize, &offset, check_sum.fourbytes[4]) ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}
bool RsFileTransferChunkMapItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt8(data, tlvsize, &offset, is_client);
    ok &= hash.serialise(data, tlvsize, offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, compressed_map._map.size());

	for(uint32_t i=0;i<compressed_map._map.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, compressed_map._map[i]);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}

/* serialise the data to the buffer */
bool RsFileTransferDataItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= fd.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::serialiseData() at: " << RsUtil::AccurateTimeString() << std::endl;
	print(std::cerr, 10);
#endif

	return ok;
}

/* serialise the data to the buffer */
bool RsFileTransferDataRequestItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, fileoffset);
	ok &= setRawUInt32(data, tlvsize, &offset, chunksize);
	ok &= file.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileTransferDataRequestItem::serialise() Size Error! " << std::endl;
	}

	/*** Debugging Transfer rates. 
	 * print timestamp, and file details so we can workout packet lags.
	 ***/

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileTransferDataRequestItem::serialise() at: " << RsUtil::AccurateTimeString() << std::endl;
	print(std::cerr, 10);
#endif

	return ok;
}

/* serialise the data to the buffer */
bool RsFileTransferCacheItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, cacheType);
	ok &= setRawUInt16(data, tlvsize, &offset, cacheSubId);
	ok &= file.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

/**********************************************************************************************/
/*                                       DESERIALISATION                                      */
/**********************************************************************************************/

RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferCacheItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_CACHE_ITEM != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferCacheItem(): wong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferCacheItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferCacheItem *item = new RsFileTransferCacheItem();
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

RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapRequestItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapRequestItem(): wong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapRequestItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferChunkMapRequestItem *item = new RsFileTransferChunkMapRequestItem();
	item->clear();

	/* skip the header */
	offset += 8;
	uint8_t tmp ;
	ok &= getRawUInt8(data, rssize, &offset, &tmp); item->is_client = tmp; 
    ok &= item->hash.deserialise(data, rssize, offset) ; // File hash

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

RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferDataItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_DATA != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferDataItem(): wong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferDataItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferDataItem *item = new RsFileTransferDataItem();
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

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferDataItem() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif

	return item;
}


RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferDataRequestItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_DATA_REQUEST != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferDataRequestItem(): wrong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferDataRequestItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	RsFileTransferDataRequestItem *item = new RsFileTransferDataRequestItem() ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &item->fileoffset);
	ok &= getRawUInt32(data, rssize, &offset, &item->chunksize);
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

	/*** Debugging Transfer rates. 
	 * print timestamp, and file details so we can workout packet lags.
	 ***/

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::deserialise_RsFileTransferDataRequestItem() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif

	return item;
}
RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_CHUNK_MAP != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapItem(): wrong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferChunkMapItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferChunkMapItem *item = new RsFileTransferChunkMapItem();
	item->clear();

	/* skip the header */
	offset += 8;
	uint8_t tmp ;
	ok &= getRawUInt8(data, rssize, &offset, &tmp); item->is_client = tmp; 
    ok &= item->hash.deserialise(data, rssize, offset) ; // File hash
    uint32_t size =0;
	ok &= getRawUInt32(data, rssize, &offset, &size); 

	if(ok)
	{
		item->compressed_map._map.resize(size) ;

		for(uint32_t i=0;i<size && ok;++i)
			ok &= getRawUInt32(data, rssize, &offset, &(item->compressed_map._map[i])); 
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

RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcRequestItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcRequestItem(): wrong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcRequestItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferSingleChunkCrcRequestItem *item = new RsFileTransferSingleChunkCrcRequestItem();
	item->clear();

	/* skip the header */
	offset += 8;
    ok &= item->hash.deserialise(data, rssize, offset) ;
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_number));

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
RsFileTransferItem *RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcItem(void *data, uint32_t pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_FILE_TRANSFER  != getRsItemType(rstype) || RS_PKT_SUBTYPE_FT_CHUNK_CRC != getRsItemSubType(rstype))
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcItem(): wrong subtype!" << std::endl;
		return NULL; /* wrong type */
	}

	if (pktsize < rssize)    /* check size */
	{
		std::cerr << "RsFileTransferSerialiser::deserialise_RsFileTransferSingleChunkCrcItem(): size inconsistency!" << std::endl;
		return NULL; /* not enough data */
	}

	bool ok = true;

	/* ready to load */
	RsFileTransferSingleChunkCrcItem *item = new RsFileTransferSingleChunkCrcItem();
	item->clear();

	/* skip the header */
	offset += 8;
    ok &= item->hash.deserialise(data, rssize, offset) ;
    ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_number));
	ok &= item->check_sum.deserialise(data,rssize,offset) ;

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
