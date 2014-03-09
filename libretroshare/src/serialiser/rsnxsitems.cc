#include "rsnxsitems.h"
#include "rsbaseserial.h"

/***
 * #define RSSERIAL_DEBUG	1
 ***/

const uint8_t RsNxsSyncGrpItem::FLAG_REQUEST = 0x001;
const uint8_t RsNxsSyncGrpItem::FLAG_RESPONSE = 0x002;

const uint8_t RsNxsSyncMsgItem::FLAG_REQUEST = 0x001;
const uint8_t RsNxsSyncMsgItem::FLAG_RESPONSE = 0x002;

const uint8_t RsNxsSyncGrp::FLAG_USE_SYNC_HASH = 0x001;
const uint8_t RsNxsSyncMsg::FLAG_USE_SYNC_HASH = 0x001;

/** transaction state **/
const uint16_t RsNxsTransac::FLAG_BEGIN_P1 = 0x0001;
const uint16_t RsNxsTransac::FLAG_BEGIN_P2 = 0x0002;
const uint16_t RsNxsTransac::FLAG_END_SUCCESS = 0x0004;
const uint16_t RsNxsTransac::FLAG_CANCEL = 0x0008;
const uint16_t RsNxsTransac::FLAG_END_FAIL_NUM  = 0x0010;
const uint16_t RsNxsTransac::FLAG_END_FAIL_TIMEOUT = 0x0020;
const uint16_t RsNxsTransac::FLAG_END_FAIL_FULL = 0x0040;


/** transaction type **/
const uint16_t RsNxsTransac::FLAG_TYPE_GRP_LIST_RESP = 0x0100;
const uint16_t RsNxsTransac::FLAG_TYPE_MSG_LIST_RESP = 0x0200;
const uint16_t RsNxsTransac::FLAG_TYPE_GRP_LIST_REQ = 0x0400;
const uint16_t RsNxsTransac::FLAG_TYPE_MSG_LIST_REQ = 0x0800;
const uint16_t RsNxsTransac::FLAG_TYPE_GRPS = 0x1000;
const uint16_t RsNxsTransac::FLAG_TYPE_MSGS = 0x2000;


uint32_t RsNxsSerialiser::size(RsItem *item) {

    RsNxsGrp* ngp;
    RsNxsMsg* nmg;
    RsNxsSyncGrp* sg;
    RsNxsSyncGrpItem* sgl;
    RsNxsSyncMsg* sgm;
    RsNxsSyncMsgItem* sgml;
    RsNxsTransac* ntx;


    if((sg = dynamic_cast<RsNxsSyncGrp*>(item))  != NULL)
    {
        return sizeNxsSyncGrp(sg);

    }else if(( ntx = dynamic_cast<RsNxsTransac*>(item)) != NULL){
        return sizeNxsTrans(ntx);
    }
    else if ((sgl = dynamic_cast<RsNxsSyncGrpItem*>(item)) != NULL)
    {
        return sizeNxsSyncGrpItem(sgl);

    }else if ((sgm = dynamic_cast<RsNxsSyncMsg*>(item)) != NULL)
    {
        return sizeNxsSyncMsg(sgm);
    }else if ((sgml = dynamic_cast<RsNxsSyncMsgItem*>(item)) != NULL)
    {
        return sizeNxsSyncMsgItem(sgml);
    }else if((ngp = dynamic_cast<RsNxsGrp*>(item)) != NULL)
    {
        return sizeNxsGrp(ngp);
    }else if((nmg = dynamic_cast<RsNxsMsg*>(item)) != NULL)
    {
        return sizeNxsMsg(nmg);
    }else{
#ifdef RSSERIAL_DEBUG
    	std::cerr << "RsNxsSerialiser::size(): Could not find appropriate size function"
    			  << std::endl;
#endif
    	return 0;
    }
}


RsItem* RsNxsSerialiser::deserialise(void *data, uint32_t *size) {


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

        case RS_PKT_SUBTYPE_NXS_SYNC_GRP:
            return deserialNxsSyncGrp(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM:
            return deserialNxsSyncGrpItem(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG:
            return deserialNxsSyncMsg(data, size);
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM:
            return deserialNxsSyncMsgItem(data, size);
        case RS_PKT_SUBTYPE_NXS_GRP:
            return deserialNxsGrp(data, size);
        case RS_PKT_SUBTYPE_NXS_MSG:
            return deserialNxsMsg(data, size);
        case RS_PKT_SUBTYPE_NXS_TRANS:
            return deserialNxsTrans(data, size);
        case RS_PKT_SUBTYPE_NXS_EXTENDED:
            return deserialNxsExtended(data, size);
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



bool RsNxsSerialiser::serialise(RsItem *item, void *data, uint32_t *size){

    RsNxsGrp* ngp;
    RsNxsMsg* nmg;
    RsNxsSyncGrp* sg;
    RsNxsSyncGrpItem* sgl;
    RsNxsSyncMsg* sgm;
    RsNxsSyncMsgItem* sgml;
    RsNxsExtended* nxt;
    RsNxsTransac* ntx;

    if((sg = dynamic_cast<RsNxsSyncGrp*>(item))  != NULL)
    {
        return serialiseNxsSyncGrp(sg, data, size);

    }else if ((ntx = dynamic_cast<RsNxsTransac*>(item)) != NULL)
    {
        return serialiseNxsTrans(ntx, data, size);

    }else if ((sgl = dynamic_cast<RsNxsSyncGrpItem*>(item)) != NULL)
    {
        return serialiseNxsSyncGrpItem(sgl, data, size);

    }else if ((sgm = dynamic_cast<RsNxsSyncMsg*>(item)) != NULL)
    {
        return serialiseNxsSyncMsg(sgm, data, size);
    }else if ((sgml = dynamic_cast<RsNxsSyncMsgItem*>(item)) != NULL)
    {
        return serialiseNxsSynMsgItem(sgml, data, size);
    }else if((ngp = dynamic_cast<RsNxsGrp*>(item)) != NULL)
    {
        return serialiseNxsGrp(ngp, data, size);
    }else if((nmg = dynamic_cast<RsNxsMsg*>(item)) != NULL)
    {
        return serialiseNxsMsg(nmg, data, size);
    }else if((nxt = dynamic_cast<RsNxsExtended*>(item)) != NULL){

        return serialiseNxsExtended(nxt, data, size);
    }

#ifdef NXS_DEBUG
    std::cerr << "RsNxsSerialiser::serialise() item does not caste to know type"
              << std::endl;
#endif

    return NULL;
}


bool RsNxsSerialiser::serialiseNxsSynMsgItem(RsNxsSyncMsgItem *item, void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsSynMsgItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsSyncMsgItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSynMsgItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsNxsSyncMsgItem */

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= item->grpId.serialise(data, *size, offset);
    ok &= item->msgId.serialise(data, *size, offset);
    ok &= item->authorId.serialise(data, *size, offset);

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


bool RsNxsSerialiser::serialiseNxsMsg(RsNxsMsg *item, void *data, uint32_t *size)
{

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsMsg()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsMsg(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsMsg()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->pos);
    ok &= item->msgId.serialise(data, *size, offset);
    ok &= item->grpId.serialise(data, *size, offset);
    ok &= item->msg.SetTlv(data, tlvsize, &offset);
    ok &= item->meta.SetTlv(data, *size, &offset);


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


bool RsNxsSerialiser::serialiseNxsGrp(RsNxsGrp *item, void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsGrp()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsGrp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsGrp()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    // grp id
    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->pos);
    ok &= item->grpId.serialise(data, *size, offset);
    ok &= item->grp.SetTlv(data, tlvsize, &offset);
    ok &= item->meta.SetTlv(data, *size, &offset);

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

bool RsNxsSerialiser::serialiseNxsSyncGrp(RsNxsSyncGrp *item, void *data, uint32_t *size)
{

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsSyncGrp()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsSyncGrp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncGrp()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= setRawUInt32(data, *size, &offset, item->createdSince);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= setRawUInt32(data, *size, &offset, item->updateTS);

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


bool RsNxsSerialiser::serialiseNxsTrans(RsNxsTransac *item, void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsTrans()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsTrans(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsTrans() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt16(data, *size, &offset, item->transactFlag);
    ok &= setRawUInt32(data, *size, &offset, item->nItems);
    ok &= setRawUInt32(data, *size, &offset, item->updateTS);



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

bool RsNxsSerialiser::serialiseNxsSyncGrpItem(RsNxsSyncGrpItem *item, void *data, uint32_t *size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsSyncGrpItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsSyncGrpItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncm() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsNxsSyncm */

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= item->grpId.serialise(data, *size, offset);
    ok &= setRawUInt32(data, *size, &offset, item->publishTs);
    ok &= item->authorId.serialise(data, *size, offset);

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


bool RsNxsSerialiser::serialiseNxsSyncMsg(RsNxsSyncMsg *item, void *data, uint32_t *size){
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseNxsSyncMsg()" << std::endl;
#endif

    uint32_t tlvsize = sizeNxsSyncMsg(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseNxsSyncMsg()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt32(data, *size, &offset, item->transactionNumber);
    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= setRawUInt32(data, *size, &offset, item->createdSince);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= item->grpId.serialise(data, *size, offset);
    ok &= setRawUInt32(data, *size, &offset, item->updateTS);

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


bool RsNxsSerialiser::serialiseNxsExtended(RsNxsExtended *item, void *data, uint32_t *size){

    return false;
}

/*** deserialisation ***/


RsNxsGrp* RsNxsSerialiser::deserialNxsGrp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_GRP != getRsItemSubType(rstype)))
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


RsNxsMsg* RsNxsSerialiser::deserialNxsMsg(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsMsg()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_MSG != getRsItemSubType(rstype)))
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


RsNxsSyncGrp* RsNxsSerialiser::deserialNxsSyncGrp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsSyncGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_SYNC_GRP != getRsItemSubType(rstype)))
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

    RsNxsSyncGrp* item = new RsNxsSyncGrp(getRsItemService(rstype));
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

RsNxsTransac* RsNxsSerialiser::deserialNxsTrans(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialNxsTrans()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_NXS_TRANS != getRsItemSubType(rstype)))
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

    RsNxsTransac* item = new RsNxsTransac(SERVICE_TYPE);

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


RsNxsSyncMsg* RsNxsSerialiser::deserialNxsSyncMsg(void *data, uint32_t *size)
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
            (RS_PKT_SUBTYPE_NXS_SYNC_MSG != getRsItemSubType(rstype)))
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

    RsNxsSyncMsg* item = new RsNxsSyncMsg(getRsItemService(rstype));

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


RsNxsExtended* RsNxsSerialiser::deserialNxsExtended(void *data, uint32_t *size){
    return NULL;
}



/*** size functions ***/


uint32_t RsNxsSerialiser::sizeNxsMsg(RsNxsMsg *item)
{

    uint32_t s = 8; //header size

    s += 4; // transaction number
    s += 1; // pos
    s += item->grpId.serial_size();
    s += item->msgId.serial_size();
    s += item->msg.TlvSize();
    s += item->meta.TlvSize();

    return s;
}

uint32_t RsNxsSerialiser::sizeNxsGrp(RsNxsGrp *item)
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // pos
    s += item->grpId.serial_size();
    s += item->grp.TlvSize();
    s += item->meta.TlvSize();

    return s;
}


uint32_t RsNxsSerialiser::sizeNxsSyncGrp(RsNxsSyncGrp *item)
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // flag
    s += 4; // sync age
    s += GetTlvStringSize(item->syncHash);
    s += 4; // updateTS

    return s;
}


uint32_t RsNxsSerialiser::sizeNxsSyncGrpItem(RsNxsSyncGrpItem *item)
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 4; // publishTs
    s += 1; // flag
    s += item->grpId.serial_size();
    s += item->authorId.serial_size();

    return s;
}


uint32_t RsNxsSerialiser::sizeNxsSyncMsg(RsNxsSyncMsg *item)
{

    uint32_t s = 8;

    s += 4; // transaction number
    s += 1; // flag
    s += 4; // age
    s += item->grpId.serial_size();
    s += GetTlvStringSize(item->syncHash);
    s += 4; // updateTS

    return s;
}


uint32_t RsNxsSerialiser::sizeNxsSyncMsgItem(RsNxsSyncMsgItem *item)
{
    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 1; // flag
    s += item->grpId.serial_size();
    s += item->msgId.serial_size();
    s += item->authorId.serial_size();

    return s;
}

uint32_t RsNxsSerialiser::sizeNxsTrans(RsNxsTransac *item){

    uint32_t s = 8; // header size

    s += 4; // transaction number
    s += 2; // flag
    s += 4; // nMsgs
    s += 4; // updateTS

    return s;
}

uint32_t RsNxsSerialiser::sizeNxsExtended(RsNxsExtended *item){

    return 0;
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

void RsNxsSyncGrp::clear()
{
    flag = 0;
    createdSince = 0;
    syncHash.clear();
    updateTS = 0;
}

void RsNxsSyncMsg::clear()
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

void RsNxsTransac::clear(){
    transactFlag = 0;
    nItems = 0;
    updateTS = 0;
    timestamp = 0;
    transactionNumber = 0;
}

std::ostream& RsNxsSyncGrp::print(std::ostream &out, uint16_t indent)
{

    printRsItemBase(out, "RsNxsSyncGrp", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "Hash: " << syncHash << std::endl;
    printIndent(out , int_Indent);
    out << "Sync Age: " << createdSince << std::endl;
    printIndent(out , int_Indent);
    out << "flag" << flag << std::endl;
    printIndent(out , int_Indent);
    out << "updateTS" << updateTS << std::endl;

    printRsItemEnd(out ,"RsNxsSyncGrp", indent);

    return out;
}

std::ostream& RsNxsExtended::print(std::ostream &out, uint16_t indent){
    printRsItemBase(out, "RsNxsExtended", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "type: " << type << std::endl;
    printIndent(out , int_Indent);
    extData.print(out, int_Indent);

    printRsItemEnd(out ,"RsNxsExtended", indent);

    return out;
}

std::ostream& RsNxsSyncMsg::print(std::ostream &out, uint16_t indent)
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
    out << "flag: " << flag << std::endl;
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
    out << "flag: " << flag << std::endl;
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
    out << "flag: " << flag << std::endl;
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


std::ostream& RsNxsTransac::print(std::ostream &out, uint16_t indent){

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
