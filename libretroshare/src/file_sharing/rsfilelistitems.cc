/*
 * RetroShare File lists service items
 *
 *      file_sharing/rsfilelistsitems.cc
 *
 * Copyright 2016 Mr.Alice
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
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */
#include "serialiser/rsbaseserial.h"

#include "serialization/rstypeserializer.h"

#include "file_sharing/rsfilelistitems.h"

void RsFileListsSyncRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,entry_hash,"entry_hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,flags     ,"flags") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,last_known_recurs_modf_TS,"last_known_recurs_modf_TS") ;
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,request_id,"request_id") ;
}
void RsFileListsSyncResponseItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process           (j,ctx,entry_hash,"entry_hash") ;
    RsTypeSerializer::serial_process           (j,ctx,checksum,"checksum") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,flags     ,"flags") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,last_known_recurs_modf_TS,"last_known_recurs_modf_TS") ;
    RsTypeSerializer::serial_process<uint64_t> (j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,directory_content_data,"directory_content_data") ;
}

RsItem *RsFileListsSerialiser::create_item(uint16_t service,uint8_t type) const
{
    if(service != RS_SERVICE_TYPE_FILE_DATABASE)
        return NULL ;

    switch(type)
    {
    case RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM: return new RsFileListsSyncRequestItem();
    case RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM: return new RsFileListsSyncResponseItem();
    default:
        return NULL ;
    }
}
void RsFileListsSyncResponseItem::clear()
{
    directory_content_data.TlvClear();
}
