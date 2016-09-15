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

#include "file_sharing/rsfilelistitems.h"

RsItem* RsFileListsSerialiser::deserialise(void *data, uint32_t *size)
{
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsFileListsSerialiser::deserialise()" << std::endl;
#endif
        /* get the type and size */
        uint32_t rstype = getRsItemId(data);

        if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (getRsItemService(rstype) != RS_SERVICE_TYPE_FILE_DATABASE))
            return NULL; /* wrong type */

        switch(getRsItemSubType(rstype))
        {
        case RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM:   return deserialFileListsSyncRequestItem(data, size);
        case RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM:   return deserialFileListsSyncResponseItem(data, size);
//        case RS_PKT_SUBTYPE_FILELISTS_CONFIG_ITEM:     return deserialFileListsConfigItem (data, size);

        default:
            {
                std::cerr << "(WW) RsFileListsSerialiser::deserialise() : unhandled item type " << getRsItemSubType(rstype) << std::endl;
                return NULL;

            }
        }
}

uint32_t RsFileListsSerialiser::size(RsItem *item)
{
    RsFileListsItem *flst_item = dynamic_cast<RsFileListsItem*>(item) ;

    if(flst_item != NULL)
        return flst_item->serial_size() ;
	else
	{
        std::cerr << "RsFileListsSerialiser::serialise(): Not an RsFileListsItem!"  << std::endl;
		return 0;
	}
}

bool RsFileListsSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
    RsFileListsItem *flst_item = dynamic_cast<RsFileListsItem*>(item) ;

    if(flst_item != NULL)
        return flst_item->serialise(data,*size) ;
	else
	{
        std::cerr << "RsFileListsSerialiser::serialise(): Not an RsFileListsItem!"  << std::endl;
		return 0;
	}
}

bool RsFileListsItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}

bool RsFileListsSyncRequestItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsFileListsSerialiser::serialiseFileListsSyncReqItem()" << std::endl;
#endif

    /* RsFileListsSyncMsgItem */

    ok &= entry_hash.serialise(data, size, offset);
    ok &= setRawUInt32(data, size, &offset, flags      );
    ok &= setRawUInt32(data, size, &offset, last_known_recurs_modf_TS);
    ok &= setRawUInt64(data, size, &offset, request_id);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsFileListsSerialiser::serialiseNxsSynMsgItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsFileListsSerialiser::serialiseNxsSynMsgItem() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsFileListsSyncResponseItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsFileListsSerialiser::serialiseFileListsSyncReqItem()" << std::endl;
#endif

    /* RsFileListsSyncMsgItem */

    ok &= entry_hash.serialise(data, size, offset);
    ok &= checksum.serialise(data, size, offset);
    ok &= setRawUInt32(data, size, &offset, flags      );
    ok &= setRawUInt32(data, size, &offset, last_known_recurs_modf_TS);
    ok &= setRawUInt64(data, size, &offset, request_id);
    ok &= directory_content_data.SetTlv(data,size,&offset) ;

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsFileListsSerialiser::serialiseNxsSynMsgItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsFileListsSerialiser::serialiseNxsSynMsgItem() NOK" << std::endl;
    }
#endif

    return ok;
}

//============================================================================================================================//
//                                                     Deserialisation                                                        //
//============================================================================================================================//

//RsFileListsConfigItem* RsFileListsSerialiser::deserialFileListsConfigItem(void *data, uint32_t *size)
//{
//    NOT_IMPLEMENTED();
//
//    return NULL ;
//}

RsFileListsSyncRequestItem* RsFileListsSerialiser::deserialFileListsSyncRequestItem(void *data, uint32_t *size)
{
    bool ok = checkItemHeader(data,size,RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM);
    uint32_t offset = 8;

    RsFileListsSyncRequestItem* item = new RsFileListsSyncRequestItem();

    ok &= item->entry_hash.deserialise(data, *size, offset);
    ok &= getRawUInt32(data, *size, &offset, &item->flags);
    ok &= getRawUInt32(data, *size, &offset, &item->last_known_recurs_modf_TS);
    ok &= getRawUInt64(data, *size, &offset, &item->request_id);

    if (offset != *size)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::deserialNxsGrp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::deserialNxsGrp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}
RsFileListsSyncResponseItem* RsFileListsSerialiser::deserialFileListsSyncResponseItem(void *data, uint32_t *size)
{
    bool ok = checkItemHeader(data,size,RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM);
    uint32_t offset = 8;

    RsFileListsSyncResponseItem* item = new RsFileListsSyncResponseItem();

    uint32_t entry_index ;              // index of the directory to sync
    uint32_t flags;                     // used to say that it's a request or a response, say that the directory has been removed, ask for further update, etc.
    uint32_t last_known_recurs_modf_TS; // time of last modification, computed over all files+directories below.
    uint64_t request_id;                // use to determine if changes that have occured since last hash

    ok &= item->entry_hash.deserialise(data, *size, offset);
    ok &= item->checksum.deserialise(data, *size, offset);
    ok &= getRawUInt32(data, *size, &offset, &item->flags);
    ok &= getRawUInt32(data, *size, &offset, &item->last_known_recurs_modf_TS);
    ok &= getRawUInt64(data, *size, &offset, &item->request_id);

    ok &= item->directory_content_data.GetTlv(data,*size,&offset) ;

    if (offset != *size)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::deserialNxsGrp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::deserialNxsGrp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}
bool RsFileListsSerialiser::checkItemHeader(void *data,uint32_t *size,uint8_t subservice_type)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsFileListsSerialiser::checkItemHeader()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_FILE_DATABASE != getRsItemService(rstype)) || (subservice_type != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::checkItemHeader() FAIL wrong type" << std::endl;
#endif
            return false; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileListsSerialiser::checkItemHeader() FAIL wrong size" << std::endl;
#endif
            return false; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;
    
    return true ;
}

//============================================================================================================================//
//                                                         Sizes                                                              //
//============================================================================================================================//

uint32_t RsFileListsSyncRequestItem::serial_size()const
{

    uint32_t s = 8; //header size

    s += RsFileHash::serial_size(); // entry hash
    s += 4; // flags
    s += 4; // last_known_recurs_modf_TS
    s += 8; // request_id

    return s;
}

uint32_t RsFileListsSyncResponseItem::serial_size()const
{

    uint32_t s = 8; //header size

    s += RsFileHash::serial_size(); // entry hash
    s += RsFileHash::serial_size(); // checksum
    s += 4; // flags
    s += 4; // last_known_recurs_modf_TS
    s += 8; // request_id
    s += directory_content_data.TlvSize();

    return s;
}

void RsFileListsSyncRequestItem::clear()
{
}
void RsFileListsSyncResponseItem::clear()
{
    directory_content_data.TlvClear();
}
std::ostream& RsFileListsSyncRequestItem::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsFileListsSyncReqItem", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent); out << "Entry hash:   " << entry_hash << std::endl;
    printIndent(out , int_Indent); out << "Flags:        " << (uint32_t) flags << std::endl;
    printIndent(out , int_Indent); out << "Last modf TS: " << last_known_recurs_modf_TS << std::endl;
    printIndent(out , int_Indent); out << "request id:   " << std::hex << request_id  << std::dec << std::endl;

    printRsItemEnd(out ,"RsFileListsSyncReqItem", indent);

    return out;
}

std::ostream& RsFileListsSyncResponseItem::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsFileListsSyncDirItem", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent); out << "Entry hash:   " << entry_hash << std::endl;
    printIndent(out , int_Indent); out << "Checksum  :   " << checksum << std::endl;
    printIndent(out , int_Indent); out << "Flags:        " << (uint32_t) flags << std::endl;
    printIndent(out , int_Indent); out << "Last modf TS: " << last_known_recurs_modf_TS << std::endl;
    printIndent(out , int_Indent); out << "request id:   " << std::hex << request_id  << std::dec << std::endl;
    printIndent(out , int_Indent); out << "Data size:    " << directory_content_data.bin_len << std::endl;

    printRsItemEnd(out ,"RsFileListsSyncDirItem", indent);

    return out;
}
