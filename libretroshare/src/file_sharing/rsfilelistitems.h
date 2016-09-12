/*
 * RetroShare File lists service items.
 *
 *      file_sharing/rsfilelistitems.h
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
#pragma once

#include <map>
#include <openssl/ssl.h>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvkeys.h"
#include "gxs/rsgxsdata.h"

// These items have "flag type" numbers, but this is not used.

const uint8_t RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM    = 0x01;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM    = 0x02;
const uint8_t RS_PKT_SUBTYPE_FILELISTS_CONFIG_ITEM      = 0x03;

/*!
 * Base class for filelist sync items
 */
class RsFileListsItem : public RsItem
{
public:
    RsFileListsItem(uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_FILE_DATABASE, subtype)
    {
        setPriorityLevel(QOS_PRIORITY_RS_SLOW_SYNC_REQUEST);	// this is the default. Someitems may be faster, on demand.
        return;
    }
    virtual ~RsFileListsItem(){}
   
    virtual bool serialise(void *data,uint32_t& size) const = 0 ;	
    virtual uint32_t serial_size() const = 0 ; 			
    virtual void clear() = 0;
    virtual std::ostream &print(std::ostream &out, uint16_t indent = 0) = 0;

    bool serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const;

    static const uint32_t FLAGS_SYNC_REQUEST      = 0x0001 ;
    static const uint32_t FLAGS_SYNC_RESPONSE     = 0x0002 ;
    static const uint32_t FLAGS_SYNC_DIR_CONTENT  = 0x0004 ;
    static const uint32_t FLAGS_ENTRY_UP_TO_DATE  = 0x0008 ;
    static const uint32_t FLAGS_ENTRY_WAS_REMOVED = 0x0010 ;
};

/*!
 * Use to request synchronization on a specific directory. Also used to respond that the directory is up to date.
 */
class RsFileListsSyncRequestItem : public RsFileListsItem
{
public:

    RsFileListsSyncRequestItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM) {}

	virtual void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent);

	virtual bool serialise(void *data,uint32_t& size) const;	
	virtual uint32_t serial_size() const ; 			
        
    RsFileHash entry_hash ;               // hash of the directory to sync
    uint32_t   flags;                     // used to say that it's a request or a response, say that the directory has been removed, ask for further update, etc.
    uint32_t   last_known_recurs_modf_TS; // time of last modification, computed over all files+directories below.
    uint64_t   request_id;                // use to determine if changes that have occured since last hash
};

class RsFileListsSyncResponseItem : public RsFileListsItem
{
public:

    RsFileListsSyncResponseItem() : RsFileListsItem(RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM) {}

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    virtual bool serialise(void *data,uint32_t& size) const;
    virtual uint32_t serial_size() const ;

    RsFileHash entry_hash ;               // hash of the directory to sync
    uint32_t   flags;                     // is it a partial/final item (used for large items only)
    uint32_t   last_known_recurs_modf_TS; // time of last modification, computed over all files+directories below.
    uint64_t   request_id;                // use to determine if changes that have occured since last hash

    RsTlvBinaryData directory_content_data ;	// encoded binary data. This allows to vary the encoding format, in a way that is transparent to the serialiser.
};

class RsFileListsSerialiser : public RsSerialType
{
public:

    RsFileListsSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_FILE_DATABASE) {}

    virtual ~RsFileListsSerialiser() {}

    virtual uint32_t size(RsItem *item);
    virtual bool serialise(RsItem *item, void *data, uint32_t *size);
    virtual RsItem* deserialise(void *data, uint32_t *size);

private:
    RsFileListsSyncRequestItem  *deserialFileListsSyncRequestItem(void *data, uint32_t *size); /* RS_PKT_SUBTYPE_SYNC_GRP */
    RsFileListsSyncResponseItem  *deserialFileListsSyncResponseItem(void *data, uint32_t *size); /* RS_PKT_SUBTYPE_SYNC_GRP */
//    RsFileListsSyncResponseItem  *deserialFileListsConfigItem (void *data, uint32_t *size); /* RS_PKT_SUBTYPE_SYNC_GRP */

    bool checkItemHeader(void *data, uint32_t *size, uint8_t subservice_type);
};


