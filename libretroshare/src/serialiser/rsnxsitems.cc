#include "rsnxsitems.h"
#include "rsbaseserial.h"

/***
 * #define RSSERIAL_DEBUG	1
 ***/

const uint8_t RsNxsSyncGrpItem::FLAG_REQUEST   = 0x001;
const uint8_t RsNxsSyncGrpItem::FLAG_RESPONSE  = 0x002;

const uint8_t RsNxsSyncMsgItem::FLAG_REQUEST   = 0x001;
const uint8_t RsNxsSyncMsgItem::FLAG_RESPONSE  = 0x002;

#ifdef UNUSED_CODE
const uint8_t RsNxsSyncGrpItem::FLAG_USE_SYNC_HASH = 0x001;
const uint8_t RsNxsSyncMsgItem::FLAG_USE_SYNC_HASH = 0x001;
#endif

/** transaction state **/
const uint16_t RsNxsTransacItem::FLAG_BEGIN_P1         = 0x0001;
const uint16_t RsNxsTransacItem::FLAG_BEGIN_P2         = 0x0002;
const uint16_t RsNxsTransacItem::FLAG_END_SUCCESS      = 0x0004;
const uint16_t RsNxsTransacItem::FLAG_CANCEL           = 0x0008;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_NUM     = 0x0010;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_TIMEOUT = 0x0020;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_FULL    = 0x0040;


/** transaction type **/
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRP_LIST_RESP  = 0x0100;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSG_LIST_RESP  = 0x0200;
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRP_LIST_REQ   = 0x0400;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSG_LIST_REQ   = 0x0800;
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRPS           = 0x1000;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSGS           = 0x2000;
const uint16_t RsNxsTransacItem::FLAG_TYPE_ENCRYPTED_DATA = 0x4000;

uint32_t RsNxsSerialiser::size(RsItem *item) 
{
    RsNxsItem *nxs_item = dynamic_cast<RsNxsItem*>(item) ;
    
    if(nxs_item != NULL)
        return nxs_item->serial_size() ;
    else
    {
#ifdef RSSERIAL_DEBUG
    	std::cerr << "RsNxsSerialiser::size(): Could not find appropriate size function"
    			  << std::endl;
#endif
    	return 0;
    }
}

RsItem* RsNxsSerialiser::deserialise(void *data, uint32_t *size) 
{
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::deserialise()" << std::endl;
#endif
        /* get the type and size */
        uint32_t rstype = getRsItemId(data);

        if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
                (SERVICE_TYPE != getRsItemService(rstype)))
        {
                return NULL; /* wrong type */
        }

        switch(getRsItemSubType(rstype))
        {

        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM:   return deserialNxsSyncGrpReqItem(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM:       return deserialNxsSyncGrpItem(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM:   return deserialNxsSyncMsgReqItem(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM:       return deserialNxsSyncMsgItem(data, size);
        case RS_PKT_SUBTYPE_NXS_GRP_ITEM:            return deserialNxsGrpItem(data, size);
        case RS_PKT_SUBTYPE_NXS_MSG_ITEM:            return deserialNxsMsgItem(data, size);
        case RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM:        return deserialNxsTransacItem(data, size);
        case RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM:return deserialNxsGroupPublishKeyItem(data, size);
        case RS_PKT_SUBTYPE_NXS_SESSION_KEY_ITEM:    return deserialNxsSessionKeyItem(data, size);
        case RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM: return deserialNxsEncryptedDataItem(data, size);
        default:
            {
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsNxsSerialiser::deserialise() : data has no type"
                          << std::endl;
#endif
                return NULL;

            }
        }
}



bool RsNxsSerialiser::serialise(RsItem *item, void *data, uint32_t *size) 
{
	RsNxsItem *nxs_item = dynamic_cast<RsNxsItem*>(item) ;

	if(nxs_item != NULL)
		return nxs_item->serialise(data,*size) ;
	else
	{
		std::cerr << "RsNxsSerialiser::serialise(): Not an RsNxsItem!"  << std::endl;
		return 0;
	}
}

bool RsNxsItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
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

bool RsNxsSyncMsgItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsSynMsgItem()" << std::endl;
#endif

    /* RsNxsSyncMsgItem */

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, flag);
    ok &= grpId.serialise(data, size, offset);
    ok &= msgId.serialise(data, size, offset);
    ok &= authorId.serialise(data, size, offset);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSynMsgItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsSynMsgItem() NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsMsg::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, pos);
    ok &= msgId.serialise(data, size, offset);
    ok &= grpId.serialise(data, size, offset);
    ok &= msg.SetTlv(data, tlvsize, &offset);
    ok &= meta.SetTlv(data, size, &offset);


    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsMsg() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsMsg() NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsGrp::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    // grp id
    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, pos);
    ok &= grpId.serialise(data, size, offset);
    ok &= grp.SetTlv(data, tlvsize, &offset);
    ok &= meta.SetTlv(data, size, &offset);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsGrp() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsGrp() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsNxsSyncGrpReqItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, flag);
    ok &= setRawUInt32(data, size, &offset, createdSince);
    ok &= SetTlvString(data, size, &offset, TLV_TYPE_STR_HASH_SHA1, syncHash);
    ok &= setRawUInt32(data, size, &offset, updateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrp() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseSyncGrp() NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsTransacItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt16(data, size, &offset, transactFlag);
    ok &= setRawUInt32(data, size, &offset, nItems);
    ok &= setRawUInt32(data, size, &offset, updateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsTrans() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsTrans() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsNxsSyncGrpItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* RsNxsSyncm */

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, flag);
    ok &= grpId.serialise(data, size, offset);
    ok &= setRawUInt32(data, size, &offset, publishTs);
    ok &= authorId.serialise(data, size, offset);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncm( FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncm() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsNxsSyncMsgReqItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= setRawUInt32(data, size, &offset, transactionNumber);
    ok &= setRawUInt8(data, size, &offset, flag);
    ok &= setRawUInt32(data, size, &offset, createdSince);
    ok &= SetTlvString(data, size, &offset, TLV_TYPE_STR_HASH_SHA1, syncHash);
    ok &= grpId.serialise(data, size, offset);
    ok &= setRawUInt32(data, size, &offset, updateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncMsg() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncMsg( NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsGroupPublishKeyItem::serialise(void *data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= grpId.serialise(data, size, offset) ;
    ok &= key.SetTlv(data, size, &offset) ;

    if(offset != tlvsize)
    {
        std::cerr << "RsNxsSerialiser::serialiseGroupPublishKeyItem() FAIL Size Error! " << std::endl;
        ok = false;
    }

    if (!ok)
        std::cerr << "RsNxsSerialiser::serialiseGroupPublishKeyItem( NOK" << std::endl;

    return ok;
}

/*** deserialisation ***/

RsNxsGrp* RsNxsSerialiser::deserialNxsGrpItem(void *data, uint32_t *size)
{

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_GRP_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGrp() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGrp() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsGrp* item = new RsNxsGrp(SERVICE_TYPE);

    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->pos));
    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= item->grp.GetTlv(data, *size, &offset);
    ok &= item->meta.GetTlv(data, *size, &offset);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGrp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGrp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsNxsMsg* RsNxsSerialiser::deserialNxsMsgItem(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsMsg()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_MSG_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsMsg() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsMsg() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsMsg* item = new RsNxsMsg(getRsItemService(rstype));
    /* skip the header */

    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->pos));
    ok &= item->msgId.deserialise(data, *size, offset);
    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= item->msg.GetTlv(data, *size, &offset);
    ok &= item->meta.GetTlv(data, *size, &offset);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsMsg() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsMsg() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsNxsSyncGrpReqItem* RsNxsSerialiser::deserialNxsSyncGrpReqItem(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsSyncGrpReqItem* item = new RsNxsSyncGrpReqItem(getRsItemService(rstype));
    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= getRawUInt32(data, *size, &offset, &(item->createdSince));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= getRawUInt32(data, *size, &offset, &(item->updateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsNxsSyncGrpItem* RsNxsSerialiser::deserialNxsSyncGrpItem(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsSyncGrpItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrpItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrpItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsSyncGrpItem* item = new RsNxsSyncGrpItem(SERVICE_TYPE);
    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= getRawUInt32(data, *size, &offset, &(item->publishTs));
    ok &= item->authorId.deserialise(data, *size, offset);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrpItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncGrpItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

RsNxsTransacItem* RsNxsSerialiser::deserialNxsTransacItem(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsTrans()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsTrans() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem( FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    /* skip the header */
    offset += 8;

    bool ok = true;

    RsNxsTransacItem* item = new RsNxsTransacItem(SERVICE_TYPE);

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt16(data, *size, &offset, &(item->transactFlag));
    ok &= getRawUInt32(data, *size, &offset, &(item->nItems));
    ok &= getRawUInt32(data, *size, &offset, &(item->updateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsTrans() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsTrans() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;


}

RsNxsSyncMsgItem* RsNxsSerialiser::deserialNxsSyncMsgItem(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem( FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsSyncMsgItem* item = new RsNxsSyncMsgItem(getRsItemService(rstype));
    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= item->msgId.deserialise(data, *size, offset);
    ok &= item->authorId.deserialise(data, *size, offset);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsgItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsNxsSyncMsgReqItem* RsNxsSerialiser::deserialNxsSyncMsgReqItem(void *data, uint32_t *size)
{


#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsg() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsg() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsSyncMsgReqItem* item = new RsNxsSyncMsgReqItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->transactionNumber));
    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= getRawUInt32(data, *size, &offset, &(item->createdSince));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= getRawUInt32(data, *size, &offset, &(item->updateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsg() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsSyncMsg() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}
RsNxsGroupPublishKeyItem* RsNxsSerialiser::deserialNxsGroupPublishKeyItem(void *data, uint32_t *size)
{


#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsGroupPublishKeyItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM != getRsItemSubType(rstype)))
    {
            std::cerr << "RsNxsSerialiser::deserialNxsGroupPublishKeyItem() FAIL wrong type" << std::endl;
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
            std::cerr << "RsNxsSerialiser::deserialNxsGroupPublishKeyItem() FAIL wrong size" << std::endl;
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsNxsGroupPublishKeyItem* item = new RsNxsGroupPublishKeyItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= item->grpId.deserialise(data, *size, offset);
    ok &= item->key.GetTlv(data, *size, &offset) ;

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGroupPublishKeyItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialNxsGroupPublishKeyItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

RsNxsSessionKeyItem      *RsNxsSerialiser::deserialNxsSessionKeyItem(void* data, uint32_t *size)
{
    std::cerr << __PRETTY_FUNCTION__ << ": Not implemented!" << std::endl;
    return NULL ;
}
RsNxsEncryptedDataItem   *RsNxsSerialiser::deserialNxsEncryptedDataItem(void* data, uint32_t *size)
{
    std::cerr << __PRETTY_FUNCTION__ << ": Not implemented!" << std::endl;
    return NULL ;
}

/*** size functions ***/


uint32_t RsNxsMsg::serial_size()const
{

    uint32_t s = 8; //header size

    s += 4; // transaction number
    s += 1; // pos
    s += grpId.serial_size();
    s += msgId.serial_size();
    s += msg.TlvSize();
    s += meta.TlvSize();

    return s;
}

uint32_t RsNxsGrp::serial_size() const
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // pos
    s += grpId.serial_size();
    s += grp.TlvSize();
    s += meta.TlvSize();

    return s;
}

uint32_t RsNxsGroupPublishKeyItem::serial_size() const
{
    uint32_t s = 8; // header size

    s += grpId.serial_size() ;
    s += key.TlvSize();

    return s;
}
uint32_t RsNxsSyncGrpReqItem::serial_size() const
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // flag
    s += 4; // sync age
    s += GetTlvStringSize(syncHash);
    s += 4; // updateTS

    return s;
}


uint32_t RsNxsSyncGrpItem::serial_size() const
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 4; // publishTs
    s += 1; // flag
    s += grpId.serial_size();
    s += authorId.serial_size();

    return s;
}


uint32_t RsNxsSyncMsgReqItem::serial_size() const
{

    uint32_t s = 8;

    s += 4; // transaction number
    s += 1; // flag
    s += 4; // age
    s += grpId.serial_size();
    s += GetTlvStringSize(syncHash);
    s += 4; // updateTS

    return s;
}


uint32_t RsNxsSyncMsgItem::serial_size() const
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // flag
    s += grpId.serial_size();
    s += msgId.serial_size();
    s += authorId.serial_size();

    return s;
}

uint32_t RsNxsTransacItem::serial_size() const
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 2; // flag
    s += 4; // nMsgs
    s += 4; // updateTS

    return s;
}


int RsNxsGrp::refcount = 0;
/** print and clear functions **/
int RsNxsMsg::refcount = 0;
void RsNxsMsg::clear()
{

    msg.TlvClear();
    meta.TlvClear();
}

void RsNxsGrp::clear()
{
    grpId.clear();
    grp.TlvClear();
    meta.TlvClear();
}

void RsNxsSyncGrpReqItem::clear()
{
    flag = 0;
    createdSince = 0;
    syncHash.clear();
    updateTS = 0;
}
void RsNxsGroupPublishKeyItem::clear()
{
    key.TlvClear();
}
void RsNxsSyncMsgReqItem::clear()
{
    grpId.clear();
    flag = 0;
    createdSince = 0;
    syncHash.clear();
    updateTS = 0;
}

void RsNxsSyncGrpItem::clear()
{
    flag = 0;
    publishTs = 0;
    grpId.clear();
    authorId.clear();
}

void RsNxsSyncMsgItem::clear()
{
    flag = 0;
    msgId.clear();
    grpId.clear();
    authorId.clear();
}

void RsNxsTransacItem::clear(){
    transactFlag = 0;
    nItems = 0;
    updateTS = 0;
    timestamp = 0;
    transactionNumber = 0;
}
std::ostream& RsNxsSyncGrpReqItem::print(std::ostream &out, uint16_t indent)
{

    printRsItemBase(out, "RsNxsSyncGrp", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "Hash: " << syncHash << std::endl;
    printIndent(out , int_Indent);
    out << "Sync Age: " << createdSince << std::endl;
    printIndent(out , int_Indent);
    out << "flag: " << (uint32_t) flag << std::endl;
    printIndent(out , int_Indent);
    out << "updateTS: " << updateTS << std::endl;

    printRsItemEnd(out ,"RsNxsSyncGrp", indent);

    return out;
}
std::ostream& RsNxsGroupPublishKeyItem::print(std::ostream &out, uint16_t indent)
{

    printRsItemBase(out, "RsNxsGroupPublishKeyItem", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "GroupId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "keyId: " << key.keyId << std::endl;

    printRsItemEnd(out ,"RsNxsGroupPublishKeyItem", indent);

    return out;
}


std::ostream& RsNxsSyncMsgReqItem::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsNxsSyncMsg", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "GrpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "createdSince: " << createdSince << std::endl;
    printIndent(out , int_Indent);
    out << "syncHash: " << syncHash << std::endl;
    printIndent(out , int_Indent);
    out << "flag: " << (uint32_t) flag << std::endl;
    printIndent(out , int_Indent);
    out << "updateTS: " << updateTS << std::endl;

    printRsItemEnd(out, "RsNxsSyncMsg", indent);
    return out;
}

std::ostream& RsNxsSyncGrpItem::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsNxsSyncGrpItem", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "flag: " << (uint32_t) flag << std::endl;
    printIndent(out , int_Indent);
    out << "grpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "publishTs: " << publishTs << std::endl;
    printIndent(out , int_Indent);
	out << "authorId: " << authorId << std::endl;

    printRsItemEnd(out , "RsNxsSyncGrpItem", indent);
    return out;
}



std::ostream& RsNxsSyncMsgItem::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsNxsSyncMsgItem", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "flag: " << (uint32_t) flag << std::endl;
    printIndent(out , int_Indent);
    out << "grpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "msgId: " << msgId << std::endl;
    printIndent(out , int_Indent);
    out << "authorId: " << authorId << std::endl;
	printIndent(out , int_Indent);

    printRsItemEnd(out ,"RsNxsSyncMsgItem", indent);
    return out;
}

RsNxsGrp* RsNxsGrp::clone() const {
	RsNxsGrp* grp = new RsNxsGrp(PacketService());
	*grp = *this;

	if(this->metaData)
	{
		grp->metaData = new RsGxsGrpMetaData();
		*(grp->metaData) = *(this->metaData);
	}

	return grp;
}

std::ostream& RsNxsGrp::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsNxsGrp", indent);
    uint16_t int_Indent = indent + 2;

    out << "grpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "grp: " << std::endl;
    printIndent(out , int_Indent);
	out << "pos: " << pos << std::endl;
    grp.print(out, int_Indent);
    out << "meta: " << std::endl;
    meta.print(out, int_Indent);

    printRsItemEnd(out, "RsNxsGrp", indent);
    return out;
}

std::ostream& RsNxsMsg::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsNxsMsg", indent);
    uint16_t int_Indent = indent + 2;

    out << "msgId: " << msgId << std::endl;
    printIndent(out , int_Indent);
    out << "grpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
	out << "pos: " << pos << std::endl;
    printIndent(out , int_Indent);
    out << "msg: " << std::endl;
    msg.print(out, indent);
    out << "meta: " << std::endl;
    meta.print(out, int_Indent);

    printRsItemEnd(out ,"RsNxsMsg", indent);
    return out;
}


std::ostream& RsNxsTransacItem::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsNxsTransac", indent);
    uint16_t int_Indent = indent + 2;

    out << "transactFlag: " << transactFlag << std::endl;
    printIndent(out , int_Indent);
    out << "nItems: " << nItems << std::endl;
    printIndent(out , int_Indent);
    out << "timeout: " << timestamp << std::endl;
    printIndent(out , int_Indent);
    out << "updateTS: " << updateTS << std::endl;
    printIndent(out , int_Indent);
    out << "transactionNumber: " << transactionNumber << std::endl;
    printIndent(out , int_Indent);

    printRsItemEnd(out ,"RsNxsTransac", indent);
    return out;
}
