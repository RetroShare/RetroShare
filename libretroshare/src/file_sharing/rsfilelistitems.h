/*******************************************************************************
 * libretroshare/src/file_sharing: rsfilelistsitems.h                          *
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
#pragma once

#include <map>
#include <openssl/ssl.h>

#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvkeys.h"
#include "gxs/rsgxsdata.h"

#include "serialiser/rsserializer.h"
#include "retroshare/rsfiles.h"

const uint8_t RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM             = 0x01;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM             = 0x02;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_CONFIG_ITEM               = 0x03;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_ITEM        = 0x04;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_CONFIG_ITEM = 0x05;

/*!
 * Base class for filelist sync items
 */
class RsFileListsItem : public RsItem
{
public:
    explicit RsFileListsItem(uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_FILE_DATABASE, subtype)
    {
        setPriorityLevel(QOS_PRIORITY_RS_SLOW_SYNC_REQUEST);	// this is the default. Someitems may be faster, on demand.
        return;
    }
    virtual ~RsFileListsItem(){}
   
    virtual void clear() = 0;

    static const uint32_t FLAGS_SYNC_REQUEST      = 0x0001 ;
    static const uint32_t FLAGS_SYNC_RESPONSE     = 0x0002 ;
    static const uint32_t FLAGS_SYNC_DIR_CONTENT  = 0x0004 ;
    static const uint32_t FLAGS_ENTRY_UP_TO_DATE  = 0x0008 ;
    static const uint32_t FLAGS_ENTRY_WAS_REMOVED = 0x0010 ;
    static const uint32_t FLAGS_SYNC_PARTIAL      = 0x0020 ;
    static const uint32_t FLAGS_SYNC_PARTIAL_END  = 0x0040 ;
};

/*!
 * Use to request synchronization on a specific directory. Also used to respond that the directory is up to date.
 */
class RsFileListsSyncRequestItem : public RsFileListsItem
{
public:

    RsFileListsSyncRequestItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM), flags(0), last_known_recurs_modf_TS(0), request_id(0) {}

    virtual void clear(){}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsFileHash entry_hash ;               // hash of the directory to sync
    uint32_t   flags;                     // used to say that it's a request or a response, say that the directory has been removed, ask for further update, etc.
    uint32_t   last_known_recurs_modf_TS; // time of last modification, computed over all files+directories below.
    uint64_t   request_id;                // use to determine if changes that have occured since last hash
};

class RsFileListsSyncResponseItem : public RsFileListsItem
{
public:

    RsFileListsSyncResponseItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM), flags(0), last_known_recurs_modf_TS(0), request_id(0) {}

    virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsFileHash entry_hash ;               // hash of the directory to sync
    RsFileHash checksum   ;               // checksum of the bindary data, for checking
    uint32_t   flags;                     // is it a partial/final item (used for large items only)
    uint32_t   last_known_recurs_modf_TS; // time of last modification, computed over all files+directories below.
    uint64_t   request_id;                // use to determine if changes that have occured since last hash

    RsTlvBinaryData directory_content_data ;	// encoded binary data. This allows to vary the encoding format, in a way that is transparent to the serialiser.
};

class RsFileListsBannedHashesItem: public RsFileListsItem
{
public:
	RsFileListsBannedHashesItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_ITEM){}

    virtual void clear() { encrypted_hashes.clear(); }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    uint32_t session_id ;                  // used to allow to send in multiple parts.
    std::set<RsFileHash> encrypted_hashes ;// hash of hash for each banned file.
};

class RsFileListsBannedHashesConfigItem: public RsFileListsItem
{
public:
	RsFileListsBannedHashesConfigItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_CONFIG_ITEM){}

    virtual void clear() { primary_banned_files_list.clear(); }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    std::map<RsFileHash,BannedFileEntry> primary_banned_files_list ;
};

class RsFileListsSerialiser : public RsServiceSerializer
{
public:

    RsFileListsSerialiser() : RsServiceSerializer(RS_SERVICE_TYPE_FILE_DATABASE) {}

    virtual ~RsFileListsSerialiser() {}

    virtual RsItem *create_item(uint16_t service,uint8_t type) const ;
};


