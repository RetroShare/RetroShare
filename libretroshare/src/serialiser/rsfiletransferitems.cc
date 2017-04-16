
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

#include "serialization/rstypeserializer.h"

/***
 * #define RSSERIAL_DEBUG 1
 * #define DEBUG_TRANSFERS 1
***/


#ifdef DEBUG_TRANSFERS
	#include "util/rsprint.h"
#endif

#include <iostream>

void RsFileTransferDataRequestItem::clear()
{
	file.TlvClear();
	fileoffset = 0;
	chunksize  = 0;
}
void RsFileTransferDataItem::clear()
{
	fd.TlvClear();
}
 
void RsFileTransferDataRequestItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t> (j,ctx,fileoffset,"fileoffset") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,chunksize, "chunksize") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,file,      "file") ;
}

void RsFileTransferDataItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,fd,"fd") ;
}

void RsFileTransferChunkMapRequestItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<bool>    (j,ctx,is_client,"is_client") ;
    RsTypeSerializer::serial_process          (j,ctx,hash,     "hash") ;
}

void RsFileTransferChunkMapItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<bool>    (j,ctx,is_client,     "is_client") ;
    RsTypeSerializer::serial_process          (j,ctx,hash,          "hash") ;
    RsTypeSerializer::serial_process          (j,ctx,compressed_map,"compressed_map") ;
}

void RsFileTransferSingleChunkCrcRequestItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,hash,        "hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
}

void RsFileTransferSingleChunkCrcItem::serial_process(RsItem::SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,hash,        "hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
    RsTypeSerializer::serial_process          (j,ctx,check_sum,   "check_sum") ;
}

//===================================================================================================//
//                                         CompressedChunkMap                                        //
//===================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const CompressedChunkMap& s)
{
	return 4 + 4*s._map.size() ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,const CompressedChunkMap& s)
{
    bool ok = true ;

	ok &= setRawUInt32(data, size, &offset, s._map.size());

	for(uint32_t i=0;i<s._map.size() && ok;++i)
		ok &= setRawUInt32(data, size, &offset, s._map[i]);

    return ok;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,CompressedChunkMap& s)
{
	uint32_t S =0;
	bool ok = getRawUInt32(data, size, &offset, &S);

	if(ok)
	{
		s._map.resize(S) ;

		for(uint32_t i=0;i<S && ok;++i)
			ok &= getRawUInt32(data, size, &offset, &(s._map[i]));
	}

    return ok;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const CompressedChunkMap& s)
{
    std::cerr << "  [Compressed chunk map] " << n << " : length=" << s._map.size() << std::endl;
}

//===================================================================================================//
//                                            Serializer                                             //
//===================================================================================================//

RsItem *RsFileTransferSerialiser::create_item(uint16_t service_type,uint8_t item_type) const
{
    if(service_type != RS_SERVICE_TYPE_FILE_TRANSFER)
        return NULL ;

    switch(item_type)
    {
	case RS_PKT_SUBTYPE_FT_DATA_REQUEST     	: return new RsFileTransferDataRequestItem();
	case RS_PKT_SUBTYPE_FT_DATA               	: return new RsFileTransferDataItem();
	case RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST  	: return new RsFileTransferChunkMapRequestItem();
	case RS_PKT_SUBTYPE_FT_CHUNK_MAP          	: return new RsFileTransferChunkMapItem();
	case RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST  	: return new RsFileTransferSingleChunkCrcRequestItem();
    case RS_PKT_SUBTYPE_FT_CHUNK_CRC			: return new RsFileTransferSingleChunkCrcItem() ;
    default:
        return NULL ;
    }
}
