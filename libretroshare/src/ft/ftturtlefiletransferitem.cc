/*******************************************************************************
 * libretroshare/src/ft: ftturtlefiletransferitem.cc                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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
#include <iostream>
#include <stdexcept>

#include <util/rsmemory.h>
#include <rsitems/itempriorities.h>
#include <ft/ftturtlefiletransferitem.h>

#include <serialiser/rstypeserializer.h>

void RsTurtleFileMapRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,direction,"direction") ;
}

void RsTurtleFileMapItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,direction,"direction") ;
    RsTypeSerializer::serial_process          (j,ctx,compressed_map._map,"map") ;
}
void RsTurtleChunkCrcRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
}
void RsTurtleChunkCrcItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_number,"chunk_number") ;
    RsTypeSerializer::serial_process          (j,ctx,check_sum,"check_sum") ;
}

void RsTurtleFileRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,chunk_offset,"chunk_offset") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_size,"chunk_size") ;
}
void RsTurtleFileDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,chunk_offset,"chunk_offset") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(chunk_data,chunk_size) ;

    RsTypeSerializer::serial_process(j,ctx,prox,"chunk_data") ;
}

