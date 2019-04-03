/*******************************************************************************
 * libretroshare/src/rsitems: rsfiletransferitems.h                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <map>

#include "retroshare/rstypes.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvfileitem.h"
#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserializer.h"

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
		RsFileTransferItem(uint8_t ft_subtype)  : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_FILE_TRANSFER,ft_subtype)  {}

		virtual ~RsFileTransferItem() {}

		virtual void clear() = 0 ;
};

class RsFileTransferDataRequestItem: public RsFileTransferItem
{
	public:
	RsFileTransferDataRequestItem() :RsFileTransferItem(RS_PKT_SUBTYPE_FT_DATA_REQUEST)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_FILE_REQUEST) ;
	}
	virtual ~RsFileTransferDataRequestItem() {}
	virtual void clear();

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

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

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	virtual void clear();

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
		virtual void clear() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

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

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
		virtual void clear() {}

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

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
		virtual void clear() {}

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

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
		virtual void clear() {}

		// Private data part.
		//
        RsFileHash hash ; // hash of the file for which we request the chunk map
		uint32_t chunk_number ;
		Sha1CheckSum check_sum ; // CRC32 map of the file.
};

/**************************************************************************/

class RsFileTransferSerialiser: public RsServiceSerializer
{
	public:
		RsFileTransferSerialiser(): RsServiceSerializer(RS_SERVICE_TYPE_FILE_TRANSFER) {}

		virtual ~RsFileTransferSerialiser() {}

		RsItem *create_item(uint16_t service_type,uint8_t item_type) const ;

};

/**************************************************************************/

