/*
 * libretroshare/src/services: ftturtlefiletransferitem.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

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

