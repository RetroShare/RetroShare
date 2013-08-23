
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
#include "serialiser/rstlvbase.h"

/***
 * #define RSSERIAL_DEBUG 1
 * #define DEBUG_TRANSFERS 1
***/


#ifdef DEBUG_TRANSFERS
	#include "util/rsprint.h"
#endif

#include <iostream>

/*************************************************************************/

uint32_t    RsFileItemSerialiser::size(RsItem *i)
{
	RsFileRequest *rfr;
	RsFileData    *rfd;
	RsFileChunkMapRequest    *rfcmr;
	RsFileChunkMap *rfcm;
	RsFileCRC32MapRequest    *rfcrcr;
	RsFileCRC32Map *rfcrc;
	RsFileSingleChunkCrcRequest    *rfscrcr;
	RsFileSingleChunkCrc *rfscrc;

	if (NULL != (rfr = dynamic_cast<RsFileRequest *>(i)))
	{
		return sizeReq(rfr);
	}
	else if (NULL != (rfd = dynamic_cast<RsFileData *>(i)))
	{
		return sizeData(rfd);
	}
	else if (NULL != (rfcmr = dynamic_cast<RsFileChunkMapRequest *>(i)))
	{
		return sizeChunkMapReq(rfcmr);
	}
	else if (NULL != (rfcm = dynamic_cast<RsFileChunkMap *>(i)))
	{
		return sizeChunkMap(rfcm);
	}
	else if (NULL != (rfcrcr = dynamic_cast<RsFileCRC32MapRequest *>(i)))
	{
		return sizeCRC32MapReq(rfcrcr);
	}
	else if (NULL != (rfcrc = dynamic_cast<RsFileCRC32Map *>(i)))
	{
		return sizeCRC32Map(rfcrc);
	}
	else if (NULL != (rfscrcr = dynamic_cast<RsFileSingleChunkCrcRequest *>(i)))
	{
		return sizeChunkCrcReq(rfscrcr);
	}
	else if (NULL != (rfscrc = dynamic_cast<RsFileSingleChunkCrc *>(i)))
	{
		return sizeChunkCrc(rfscrc);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsFileItemSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsFileRequest *rfr;
	RsFileData    *rfd;
	RsFileChunkMapRequest    *rfcmr;
	RsFileChunkMap *rfcm;
	RsFileCRC32MapRequest    *rfcrcr;
	RsFileCRC32Map *rfcrc;
	RsFileSingleChunkCrcRequest    *rfscrcr;
	RsFileSingleChunkCrc *rfscrc;

	if (NULL != (rfr = dynamic_cast<RsFileRequest *>(i)))
	{
		return serialiseReq(rfr, data, pktsize);
	}
	else if (NULL != (rfd = dynamic_cast<RsFileData *>(i)))
	{
		return serialiseData(rfd, data, pktsize);
	}
	else if (NULL != (rfcmr = dynamic_cast<RsFileChunkMapRequest *>(i)))
	{
		return serialiseChunkMapReq(rfcmr,data,pktsize);
	}
	else if (NULL != (rfcm = dynamic_cast<RsFileChunkMap *>(i)))
	{
		return serialiseChunkMap(rfcm,data,pktsize);
	}
	else if (NULL != (rfcrcr = dynamic_cast<RsFileCRC32MapRequest *>(i)))
	{
		return serialiseCRC32MapReq(rfcrcr,data,pktsize);
	}
	else if (NULL != (rfcrc = dynamic_cast<RsFileCRC32Map *>(i)))
	{
		return serialiseCRC32Map(rfcrc,data,pktsize);
	}
	else if (NULL != (rfscrcr = dynamic_cast<RsFileSingleChunkCrcRequest *>(i)))
	{
		return serialiseChunkCrcReq(rfscrcr,data,pktsize);
	}
	else if (NULL != (rfscrc = dynamic_cast<RsFileSingleChunkCrc *>(i)))
	{
		return serialiseChunkCrc(rfscrc,data,pktsize);
	}

	std::cerr << "RsFileItemSerialiser::serialize(): unhandled packet type !!" << std::endl;
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
		case RS_PKT_SUBTYPE_FI_CHUNK_MAP_REQUEST:
			return deserialiseChunkMapReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_CHUNK_MAP:
			return deserialiseChunkMap(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_CRC32_MAP_REQUEST:
			return deserialiseCRC32MapReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_CRC32_MAP:
			return deserialiseCRC32Map(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_CHUNK_CRC_REQUEST:
			return deserialiseChunkCrcReq(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FI_CHUNK_CRC:
			return deserialiseChunkCrc(data, pktsize);
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
        out << "FileOffset: " << fileoffset;
        out << " ChunkSize:  " << chunksize  << std::endl;
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseReq() Header: " << ok << std::endl;
	std::cerr << "RsFileItemSerialiser::serialiseReq() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, item->fileoffset);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunksize);
	ok &= item->file.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseReq() Size Error! " << std::endl;
#endif
	}

	/*** Debugging Transfer rates. 
	 * print timestamp, and file details so we can workout packet lags.
	 ***/

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::serialiseReq() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif

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


	/*** Debugging Transfer rates. 
	 * print timestamp, and file details so we can workout packet lags.
	 ***/

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::deserialiseReq() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif

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
std::ostream &RsFileChunkMap::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileChunkMap", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printIndent(out, int_Indent); out << "chunks: " << std::hex << compressed_map._map[0] << std::dec << "..." << std::endl ;
        printRsItemEnd(out, "RsFileChunkMap", indent);
        return out;
}
std::ostream &RsFileChunkMapRequest::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileChunkMapRequest", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printRsItemEnd(out, "RsFileChunkMapRequest", indent);
        return out;
}
std::ostream& RsFileCRC32Map::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileCRC32Map", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printIndent(out, int_Indent); out << "chunks: " << std::hex << crc_map._ccmap._map[0]  << std::dec<< "..." << std::endl ;
        printRsItemEnd(out, "RsFileCRC32Map", indent);
        return out;
}
std::ostream& RsFileSingleChunkCrcRequest::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileSingleChunkCrcRequest", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printIndent(out, int_Indent); out << " chunk: " << chunk_number << "..." << std::endl ;
        printRsItemEnd(out, "RsFileSingleChunkCrcRequest", indent);
        return out;
}
std::ostream& RsFileSingleChunkCrc::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileSingleChunkCrc", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printIndent(out, int_Indent); out << " chunk: " << chunk_number << "..." << std::endl ;
        printIndent(out, int_Indent); out << "  sha1: " << check_sum.toStdString() << "..." << std::endl ;
        printRsItemEnd(out, "RsFileSingleChunkCrc", indent);
        return out;
}
std::ostream& RsFileCRC32MapRequest::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileCRC32MapRequest", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent); out << "PeerId: " << PeerId() << std::endl ;
        printIndent(out, int_Indent); out << "  hash: " << hash << std::endl ;
        printRsItemEnd(out, "RsFileCRC32MapRequest", indent);
        return out;
}
std::ostream &RsFileData::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileData", indent);
	uint16_t int_Indent = indent + 2;
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->fd.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::serialiseData() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif


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

#ifdef DEBUG_TRANSFERS
	std::cerr << "RsFileItemSerialiser::deserialiseData() at: " << RsUtil::AccurateTimeString() << std::endl;
	item->print(std::cerr, 10);
#endif


	return item;
}

uint32_t    RsFileItemSerialiser::sizeChunkMapReq(RsFileChunkMapRequest *item)
{
	uint32_t s = 8; /* header  */
	s += 1 ; 							// is_client
	s += GetTlvStringSize(item->hash) ; // hash

	return s;
}
uint32_t    RsFileItemSerialiser::sizeChunkMap(RsFileChunkMap *item)
{
	uint32_t s = 8; /* header  */
	s += 1 ; 								// is_client
	s += GetTlvStringSize(item->hash) ; 	// hash
	s += 4 ;									// compressed map size
	s += 4 * item->compressed_map._map.size() ; // compressed chunk map

	return s;
}
uint32_t    RsFileItemSerialiser::sizeChunkCrc(RsFileSingleChunkCrc *item)
{
	uint32_t s = 8; /* header  */
	s += GetTlvStringSize(item->hash) ; // hash
	s += 4 ; // chunk number
	s += 20 ; // sha1

	return s;
}
uint32_t    RsFileItemSerialiser::sizeChunkCrcReq(RsFileSingleChunkCrcRequest *item)
{
	uint32_t s = 8; /* header  */
	s += GetTlvStringSize(item->hash) ; // hash
	s += 4 ; // chunk number

	return s;
}
uint32_t    RsFileItemSerialiser::sizeCRC32MapReq(RsFileCRC32MapRequest *item)
{
	uint32_t s = 8; /* header  */
	s += GetTlvStringSize(item->hash) ; // hash

	return s;
}
uint32_t    RsFileItemSerialiser::sizeCRC32Map(RsFileCRC32Map *item)
{
	uint32_t s = 8; /* header  */
	s += GetTlvStringSize(item->hash) ; 		// hash
	s += 4 ;												// crc32 map size
	s += 4 * item->crc_map._ccmap._map.size() ;	// compressed chunk map
	s += 4 ;												// crc32 map size
	s += 4 * item->crc_map._crcs.size() ; 		// compressed chunk map

	return s;
}
/* serialise the data to the buffer */
bool     RsFileItemSerialiser::serialiseCRC32MapReq(RsFileCRC32MapRequest *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeCRC32MapReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
bool     RsFileItemSerialiser::serialiseCRC32Map(RsFileCRC32Map *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeCRC32Map(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);
	ok &= setRawUInt32(data, tlvsize, &offset, item->crc_map._ccmap._map.size());

	for(uint32_t i=0;i<item->crc_map._ccmap._map.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, item->crc_map._ccmap._map[i]);

	ok &= setRawUInt32(data, tlvsize, &offset, item->crc_map._crcs.size());

	for(uint32_t i=0;i<item->crc_map._crcs.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, item->crc_map._crcs[i]);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
RsFileCRC32MapRequest *RsFileItemSerialiser::deserialiseCRC32MapReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CRC32_MAP_REQUEST != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileCRC32MapRequest *item = new RsFileCRC32MapRequest();
	item->clear();

	/* skip the header */
	offset += 8;
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash

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
RsFileCRC32Map *RsFileItemSerialiser::deserialiseCRC32Map(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CRC32_MAP != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileCRC32Map *item = new RsFileCRC32Map();
	item->clear();

	/* skip the header */
	offset += 8;
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash
	uint32_t size =0;
	ok &= getRawUInt32(data, *pktsize, &offset, &size); 

	if(ok)
	{
		item->crc_map._ccmap._map.resize(size) ;

		for(uint32_t i=0;i<size && ok;++i)
			ok &= getRawUInt32(data, *pktsize, &offset, &(item->crc_map._ccmap._map[i])); 
	}

	uint32_t size2 =0;
	ok &= getRawUInt32(data, *pktsize, &offset, &size2); 

	if(ok)
	{
		item->crc_map._crcs.resize(size2) ;

		for(uint32_t i=0;i<size2 && ok;++i)
			ok &= getRawUInt32(data, *pktsize, &offset, &(item->crc_map._crcs[i])); 
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

/* serialise the data to the buffer */
bool     RsFileItemSerialiser::serialiseChunkMapReq(RsFileChunkMapRequest *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeChunkMapReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt8(data, tlvsize, &offset, item->is_client);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
bool     RsFileItemSerialiser::serialiseChunkCrcReq(RsFileSingleChunkCrcRequest *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeChunkCrcReq(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_number) ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
bool     RsFileItemSerialiser::serialiseChunkCrc(RsFileSingleChunkCrc *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeChunkCrc(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_number) ;
	ok &= setRawUInt32(data, tlvsize, &offset, item->check_sum.fourbytes[0]) ;
	ok &= setRawUInt32(data, tlvsize, &offset, item->check_sum.fourbytes[1]) ;
	ok &= setRawUInt32(data, tlvsize, &offset, item->check_sum.fourbytes[2]) ;
	ok &= setRawUInt32(data, tlvsize, &offset, item->check_sum.fourbytes[3]) ;
	ok &= setRawUInt32(data, tlvsize, &offset, item->check_sum.fourbytes[4]) ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
bool     RsFileItemSerialiser::serialiseChunkMap(RsFileChunkMap *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeChunkMap(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt8(data, tlvsize, &offset, item->is_client);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->hash);
	ok &= setRawUInt32(data, tlvsize, &offset, item->compressed_map._map.size());

	for(uint32_t i=0;i<item->compressed_map._map.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, item->compressed_map._map[i]);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
#endif
	}

	return ok;
}
RsFileChunkMapRequest *RsFileItemSerialiser::deserialiseChunkMapReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CHUNK_MAP_REQUEST != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileChunkMapRequest *item = new RsFileChunkMapRequest();
	item->clear();

	/* skip the header */
	offset += 8;
	uint8_t tmp ;
	ok &= getRawUInt8(data, *pktsize, &offset, &tmp); item->is_client = tmp; 
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash

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
RsFileChunkMap *RsFileItemSerialiser::deserialiseChunkMap(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CHUNK_MAP != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileChunkMap *item = new RsFileChunkMap();
	item->clear();

	/* skip the header */
	offset += 8;
	uint8_t tmp ;
	ok &= getRawUInt8(data, *pktsize, &offset, &tmp); item->is_client = tmp; 
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash
	uint32_t size =0;
	ok &= getRawUInt32(data, *pktsize, &offset, &size); 

	if(ok)
	{
		item->compressed_map._map.resize(size) ;

		for(uint32_t i=0;i<size && ok;++i)
			ok &= getRawUInt32(data, *pktsize, &offset, &(item->compressed_map._map[i])); 
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsCacheItemSerialiser::serialiseReq() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheType);
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheSubId);
	ok &= item->file.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseReq() Size Error! " << std::endl;
#endif
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsCacheItemSerialiser::serialiseItem() Header: " << ok << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheType);
	ok &= setRawUInt16(data, tlvsize, &offset, item->cacheSubId);
	ok &= item->file.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileItemSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
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

RsFileSingleChunkCrcRequest *RsFileItemSerialiser::deserialiseChunkCrcReq(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CHUNK_CRC_REQUEST != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileSingleChunkCrcRequest *item = new RsFileSingleChunkCrcRequest();
	item->clear();

	/* skip the header */
	offset += 8;
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash
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
RsFileSingleChunkCrc *RsFileItemSerialiser::deserialiseChunkCrc(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_BASE != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FI_CHUNK_CRC != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileSingleChunkCrc *item = new RsFileSingleChunkCrc();
	item->clear();

	/* skip the header */
	offset += 8;
	ok &= GetTlvString(data, *pktsize, &offset, TLV_TYPE_STR_VALUE, item->hash); 	// file hash
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_number));
	ok &= getRawUInt32(data, rssize, &offset, &(item->check_sum.fourbytes[0]));
	ok &= getRawUInt32(data, rssize, &offset, &(item->check_sum.fourbytes[1]));
	ok &= getRawUInt32(data, rssize, &offset, &(item->check_sum.fourbytes[2]));
	ok &= getRawUInt32(data, rssize, &offset, &(item->check_sum.fourbytes[3]));
	ok &= getRawUInt32(data, rssize, &offset, &(item->check_sum.fourbytes[4]));

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

        #ifdef RSSERIAL_DEBUG
                std::cerr << "RsServiceSerialiser::serialise() serializing raw item. pktsize : " << *pktsize;
        #endif

	uint32_t tlvsize = item->getRawLength();
        #ifdef RSSERIAL_DEBUG
                std::cerr << "tlvsize : " << tlvsize << std::endl;
        #endif

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


