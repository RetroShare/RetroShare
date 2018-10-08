/*******************************************************************************
 * libretroshare/src/file_sharing: rsfilelistsitems.cc                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Mr.Alice <mralice@users.sourceforge.net>                  *
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
 ******************************************************************************/
#include "serialiser/rsbaseserial.h"

#include "serialiser/rstypeserializer.h"

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
    RsTypeSerializer::serial_process           (j,ctx,entry_hash,               "entry_hash") ;
    RsTypeSerializer::serial_process           (j,ctx,checksum,                 "checksum") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,flags,                    "flags") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,last_known_recurs_modf_TS,"last_known_recurs_modf_TS") ;
    RsTypeSerializer::serial_process<uint64_t> (j,ctx,request_id,               "request_id") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,directory_content_data,   "directory_content_data") ;
}
void RsFileListsBannedHashesItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,session_id      ,"session_id") ;
    RsTypeSerializer::serial_process(j,ctx,encrypted_hashes,"encrypted_hashes") ;
}
void RsFileListsBannedHashesConfigItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,primary_banned_files_list,"primary_banned_files_list") ;
}

RsItem *RsFileListsSerialiser::create_item(uint16_t service,uint8_t type) const
{
    if(service != RS_SERVICE_TYPE_FILE_DATABASE)
        return NULL ;

    switch(type)
    {
    case RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM:             return new RsFileListsSyncRequestItem();
    case RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM:             return new RsFileListsSyncResponseItem();
    case RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_ITEM:        return new RsFileListsBannedHashesItem();
    case RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_CONFIG_ITEM: return new RsFileListsBannedHashesConfigItem();
    default:
        return NULL ;
    }
}
void RsFileListsSyncResponseItem::clear()
{
    directory_content_data.TlvClear();
}
