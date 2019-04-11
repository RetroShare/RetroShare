/*******************************************************************************
 * libretroshare/src/rsitems: rsfiletransferitems.cc                           *
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
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "rsitems/rsfiletransferitems.h"

#include "serialiser/rstypeserializer.h"

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
 
void RsFileTransferDataRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t> (j,ctx,fileoffset,"fileoffset") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,chunksize, "chunksize") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,file,      "file") ;
}

void RsFileTransferDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,fd,"fd") ;
}

void RsFileTransferChunkMapRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<bool>    (j,ctx,is_client,"is_client") ;
    RsTypeSerializer::serial_process          (j,ctx,hash,     "hash") ;
}

void RsFileTransferChunkMapItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<bool>    (j,ctx,is_client,     "is_client") ;
    RsTypeSerializer::serial_process          (j,ctx,hash,          "hash") ;
    RsTypeSerializer::serial_process          (j,ctx,compressed_map,"compressed_map") ;
}

void RsFileTransferSingleChunkCrcRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,hash,        "hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
}

void RsFileTransferSingleChunkCrcItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,hash,        "hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
    RsTypeSerializer::serial_process          (j,ctx,check_sum,   "check_sum") ;
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
