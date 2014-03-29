#pragma once

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

#include "retroshare/rstypes.h"
#include "serialiser/rsserial.h"
//#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rsserviceids.h"


const uint8_t RS_PKT_SUBTYPE_FT_DATA_REQUEST       = 0x01;
const uint8_t RS_PKT_SUBTYPE_FT_DATA               = 0x02;
const uint8_t RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST  = 0x04;
const uint8_t RS_PKT_SUBTYPE_FT_CHUNK_MAP          = 0x05;
const uint8_t RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST  = 0x08;
const uint8_t RS_PKT_SUBTYPE_FT_CHUNK_CRC          = 0x09;

const uint8_t RS_PKT_SUBTYPE_FT_CACHE_ITEM    = 0x0A;
const uint8_t RS_PKT_SUBTYPE_FT_CACHE_REQUEST = 0x0B;

//const uint8_t RS_PKT_SUBTYPE_FT_TRANSFER           = 0x03;
//const uint8_t RS_PKT_SUBTYPE_FT_CRC32_MAP_REQUEST  = 0x06;
//const uint8_t RS_PKT_SUBTYPE_FT_CRC32_MAP          = 0x07;

/**************************************************************************/

class RsFileTransferItem: public RsItem
{
	public:
		RsFileTransferItem(uint8_t ft_subtype) 
			: RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_FILE_TRANSFER,ft_subtype) 
		{}

		virtual ~RsFileTransferItem() {}

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	
		virtual uint32_t serial_size() = 0 ; 						

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0) = 0;
		virtual void clear() = 0 ;

	protected:
		bool serialise_header(void *data, uint32_t& pktsize, uint32_t& tlvsize, uint32_t& offset) ;
};

class RsFileTransferDataRequestItem: public RsFileTransferItem
{
	public:
	RsFileTransferDataRequestItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_DATA_REQUEST)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_FILE_REQUEST) ;
	}
	virtual ~RsFileTransferDataRequestItem() {}

	virtual bool serialise(void *data,uint32_t& size) ;	
	virtual uint32_t serial_size() ; 						

	virtual void clear();

	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	// Private data part.
	//
	uint64_t fileoffset;  /* start of data requested */
	uint32_t chunksize;   /* size of data requested */
	RsTlvFileItem file;   /* file information */
};

/**************************************************************************/

class RsFileTransferDataItem: public RsFileTransferItem
{
	public:
	RsFileTransferDataItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_DATA)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_FILE_DATA) ;	
	}
	virtual ~RsFileTransferDataItem() { clear() ; }

	virtual bool serialise(void *data,uint32_t& size) ;	
	virtual uint32_t serial_size() ; 						
	virtual void clear();

	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	// Private data part.
	//
	RsTlvFileData fd;
};

class RsFileTransferChunkMapRequestItem: public RsFileTransferItem
{
	public:
		RsFileTransferChunkMapRequestItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST)
		{
			setPriorityLevel(QOS_PRIORITY_RS_FILE_MAP_REQUEST) ;
		}
		virtual ~RsFileTransferChunkMapRequestItem() {}
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						
		virtual void clear() {}
		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// Private data part.
		//
		bool is_client ; 		// is the request for a client, or a server ? 
        RsFileHash hash ;	// hash of the file for which we request the chunk map
};

class RsFileTransferChunkMapItem: public RsFileTransferItem
{
	public:
		RsFileTransferChunkMapItem() 
			:RsFileTransferItem(RS_PKT_SUBTYPE_FT_CHUNK_MAP)
		{
			setPriorityLevel(QOS_PRIORITY_RS_FILE_MAP) ;
		}
		virtual ~RsFileTransferChunkMapItem() {}
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						
		virtual void clear() {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// Private data part.
		//
		bool is_client ; 		// is the request for a client, or a server ? 
        RsFileHash hash ;	// hash of the file for which we request the chunk map
		CompressedChunkMap compressed_map ; // Chunk map of the file.
};

class RsFileTransferSingleChunkCrcRequestItem: public RsFileTransferItem
{
	public:
		RsFileTransferSingleChunkCrcRequestItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST)
		{
			setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC_REQUEST) ;
		}
		virtual ~RsFileTransferSingleChunkCrcRequestItem() {}
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						
		virtual void clear() {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// Private data part.
		//
        RsFileHash hash ;		// hash of the file for which we request the crc
		uint32_t chunk_number ;	// chunk number
};

class RsFileTransferSingleChunkCrcItem: public RsFileTransferItem
{
	public:
		RsFileTransferSingleChunkCrcItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_CHUNK_CRC)
		{
			setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC) ;
		}
		virtual ~RsFileTransferSingleChunkCrcItem() {}
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						
		virtual void clear() {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// Private data part.
		//
        RsFileHash hash ; // hash of the file for which we request the chunk map
		uint32_t chunk_number ;
		Sha1CheckSum check_sum ; // CRC32 map of the file.
};

class RsFileTransferCacheItem: public RsFileTransferItem
{
	public:
		RsFileTransferCacheItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_CACHE_ITEM)
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_CACHE_ITEM); 
		}

		virtual ~RsFileTransferCacheItem(){ clear() ; }
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						
		virtual void clear();
		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// private part.
		//
		uint16_t cacheType;
		uint16_t cacheSubId;
		RsTlvFileItem file;   /* file information */
};


/**************************************************************************/

class RsFileTransferSerialiser: public RsSerialType
{
	public:
		RsFileTransferSerialiser(): RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_FILE_TRANSFER) {}

		virtual ~RsFileTransferSerialiser() {}

		virtual uint32_t size(RsItem *item)
		{
			return dynamic_cast<RsFileTransferItem*>(item)->serial_size() ;
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size)
		{
			return dynamic_cast<RsFileTransferItem*>(item)->serialise(data,*size) ;
		}
		virtual RsFileTransferItem *deserialise(void *data, uint32_t *size);

	private:
		RsFileTransferItem *deserialise_RsFileTransferCacheItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferChunkMapRequestItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferChunkMapItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferDataRequestItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferDataItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferSingleChunkCrcItem(void *data, uint32_t pktsize);
		RsFileTransferItem *deserialise_RsFileTransferSingleChunkCrcRequestItem(void *data, uint32_t pktsize);

};

/**************************************************************************/

