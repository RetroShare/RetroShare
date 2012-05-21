#include "rsnxsitems.h"
#include "rsbaseserial.h"


const uint8_t RsSyncGrpList::FLAG_REQUEST = 0x001;
const uint8_t RsSyncGrpList::FLAG_RESPONSE = 0x002;

const uint8_t RsSyncGrpMsgList::FLAG_REQUEST = 0x001;
const uint8_t RsSyncGrpMsgList::FLAG_RESPONSE = 0x002;

const uint8_t RsSyncGrp::FLAG_USE_SYNC_HASH = 0x001;

const uint8_t RsSyncGrpMsg::FLAG_USE_SYNC_HASH = 0x001;


uint32_t RsNxsSerialiser::size(RsItem *item) {

    RsNxsGrp* ngp;
    RsNxsMsg* nmg;
    RsSyncGrp* sg;
    RsSyncGrpList* sgl;
    RsSyncGrpMsg* sgm;
    RsSyncGrpMsgList* sgml;


    if((sg = dynamic_cast<RsSyncGrp*>(item))  != NULL)
    {
        sizeSyncGrp(sg);

    }else if ((sgl = dynamic_cast<RsSyncGrpList*>(item)) != NULL)
    {
        sizeSyncGrpList(sgl);

    }else if ((sgm = dynamic_cast<RsSyncGrpMsg*>(item)) != NULL)
    {
        sizeSyncGrpMsg(sgm);
    }else if ((sgml = dynamic_cast<RsSyncGrpMsgList*>(item)) != NULL)
    {
        sizeSyncGrpMsgList(sgml);
    }else if((ngp = dynamic_cast<RsNxsGrp*>(item)) != NULL)
    {
        sizeNxsGrp(ngp);
    }else if((nmg = dynamic_cast<RsNxsMsg*>(item)) != NULL)
    {
        sizeNxsMsg(nmg);
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

        case RS_PKT_SUBTYPE_SYNC_GRP:
            return deserialSyncGrp(data, size);
        case RS_PKT_SUBTYPE_SYNC_GRP_LIST:
            return deserialSyncGrpList(data, size);
        case RS_PKT_SUBTYPE_SYNC_MSG:
            return deserialSyncGrpMsg(data, size);
        case RS_PKT_SUBTYPE_SYNC_MSG_LIST:
            return deserialSyncGrpMsgList(data, size);
        case RS_PKT_SUBTYPE_NXS_GRP:
            return deserialNxsGrp(data, size);
        case RS_PKT_SUBTYPE_NXS_MSG:
            return deserialNxsMsg(data, size);
        case RS_PKT_SUBTYPE_NXS_EXTENDED:
            return deserialNxsExtended(data, size);
        default:
            {
#ifdef NXS_DEBUG
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
    RsSyncGrp* sg;
    RsSyncGrpList* sgl;
    RsSyncGrpMsg* sgm;
    RsSyncGrpMsgList* sgml;
    RsNxsExtended* nxt;

    if((sg = dynamic_cast<RsSyncGrp*>(item))  != NULL)
    {
        return serialiseSyncGrp(sg, data, size);

    }else if ((sgl = dynamic_cast<RsSyncGrpList*>(item)) != NULL)
    {
        return serialiseSyncGrpList(sgl, data, size);

    }else if ((sgm = dynamic_cast<RsSyncGrpMsg*>(item)) != NULL)
    {
        return serialiseSyncGrpMsg(sgm, data, size);
    }else if ((sgml = dynamic_cast<RsSyncGrpMsgList*>(item)) != NULL)
    {
        return serialiseSynGrpMsgList(sgml, data, size);
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


bool RsNxsSerialiser::serialiseSynGrpMsgList(RsSyncGrpMsgList *item, void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseSynGrpMsgList()" << std::endl;
#endif

    uint32_t tlvsize = sizeSyncGrpMsgList(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSynGrpMsgList()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsSyncGrpMsgList */

    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);

    SyncList::iterator mit =
            item->msgs.begin();

    for(; mit != item->msgs.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<RsTlvKeySignature>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            RsTlvKeySignature& b = *lit;
            b.SetTlv(data, *size, &offset);
        }
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSynGrpMsgList() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseSynGrpMsgList() NOK" << std::endl;
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

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;


    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSGID, item->msgId);
    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->identity);
    ok &= setRawUInt32(data, tlvsize, &offset, item->timeStamp);
    ok &= setRawUInt32(data, tlvsize, &offset, item->msgFlag);
    ok &= item->idSign.SetTlv(data, tlvsize, &offset);
    ok &= item->publishSign.SetTlv(data, tlvsize, &offset);
    ok &= item->msg.SetTlv(data, tlvsize, &offset);


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

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    // grp id

    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->identity);
    ok &= setRawUInt32(data, tlvsize, &offset, item->grpFlag);
    ok &= setRawUInt32(data, tlvsize, &offset, item->timeStamp);
    ok &= item->idSign.SetTlv(data, tlvsize, &offset);
    ok &= item->adminSign.SetTlv(data, tlvsize, &offset);
    ok &= item->keys.SetTlv(data, tlvsize, &offset);

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

bool RsNxsSerialiser::serialiseSyncGrp(RsSyncGrp *item, void *data, uint32_t *size)
{

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseSyncGrp()" << std::endl;
#endif

    uint32_t tlvsize = sizeSyncGrp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrp()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= setRawUInt32(data, *size, &offset, item->syncAge);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);

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


bool RsNxsSerialiser::serialiseSyncGrpList(RsSyncGrpList *item, void *data, uint32_t *size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseSyncGrpList()" << std::endl;
#endif

    uint32_t tlvsize = sizeSyncGrpList(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpList() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsSyncGrpList */

    SyncList::iterator mit =
            item->grps.begin();


    ok &= setRawUInt8(data, *size, &offset, item->flag);

    for(; mit != item->grps.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<RsTlvKeySignature>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            RsTlvKeySignature& b = *lit;
            b.SetTlv(data, *size, &offset);
        }
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpList( FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpList() NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsSerialiser::serialiseSyncGrpMsg(RsSyncGrpMsg *item, void *data, uint32_t *size){
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseSyncGrpMsg()" << std::endl;
#endif

    uint32_t tlvsize = sizeSyncGrpMsg(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpMsg()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= setRawUInt32(data, *size, &offset, item->syncAge);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpMsg() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpMsg( NOK" << std::endl;
    }
#endif

    return ok;
}

// TODO: need to finalise search term members
bool RsNxsSerialiser::serialiseNxsSearchReq(RsNxsSearchReq *item, void *data, uint32_t *size){
    return false;
}


bool RsNxsSerialiser::serialiseNxsExtended(RsNxsExtended *item, void *data, uint32_t *size){

    return false;
}

/*** deserialisation ***/


RsNxsGrp* RsNxsSerialiser::deserialNxsGrp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialGrpResp()" << std::endl;
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

    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_NAME, item->identity);
    ok &= getRawUInt32(data, *size, &offset, &(item->grpFlag));
    ok &= getRawUInt32(data, *size, &offset, &(item->timeStamp));
    ok &= item->idSign.GetTlv(data, *size, &offset);
    ok &= item->adminSign.GetTlv(data, *size, &offset);
    ok &= item->keys.GetTlv(data, *size, &offset);

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

    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, item->msgId);
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_NAME, item->identity);
    ok &= getRawUInt32(data, *size, &offset, &(item->timeStamp));
    ok &= getRawUInt32(data, *size, &offset, &(item->msgFlag));
    ok &= item->idSign.GetTlv(data, *size, &offset);
    ok &= item->publishSign.GetTlv(data, *size, &offset);
    ok &= item->msg.GetTlv(data, *size, &offset);;

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


RsSyncGrp* RsNxsSerialiser::deserialSyncGrp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialSyncGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_SYNC_GRP != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrp() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrp() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsSyncGrp* item = new RsSyncGrp(getRsItemService(rstype));
    /* skip the header */
    offset += 8;

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= getRawUInt32(data, *size, &offset, &(item->syncAge));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsSyncGrpList* RsNxsSerialiser::deserialSyncGrpList(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialSyncGrpList()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_SYNC_GRP_LIST != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpList() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpList() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsSyncGrpList* item = new RsSyncGrpList(SERVICE_TYPE);
    /* skip the header */
    offset += 8;

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));

    while((offset < rssize) && ok)
    {
        std::string grpId;
        ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, grpId);

        uint32_t nVersions;
        ok &= getRawUInt32(data, *size, &offset, &nVersions);

        uint32_t verCount  = 0;
        while((offset < rssize) && ok && (verCount < nVersions))
        {
            RsTlvKeySignature sign;
            ok &= sign.GetTlv(data, *size, &offset);
            item->grps[grpId].push_back(sign);
            verCount++;
        }
    }

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpList() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpList() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsSyncGrpMsgList* RsNxsSerialiser::deserialSyncGrpMsgList(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialSyncGrpMsgList()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_SYNC_MSG_LIST != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsgList() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsgList( FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsSyncGrpMsgList* item = new RsSyncGrpMsgList(getRsItemService(rstype));
    /* skip the header */
    offset += 8;

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);

    // now get number of msgs
    while((offset < rssize) && ok)
    {

        std::string msgId;
        ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, msgId);

        uint32_t nVersions;
        ok &= getRawUInt32(data, *size, &offset, &nVersions);

        uint32_t verCount  = 0;
        while((offset < rssize) && ok && (verCount < nVersions))
        {
            RsTlvKeySignature sign;
            ok &= sign.GetTlv(data, *size, &offset);
            item->msgs[msgId].push_back(sign);
            verCount++;
        }

    }

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsgList() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsgList() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsSyncGrpMsg* RsNxsSerialiser::deserialSyncGrpMsg(void *data, uint32_t *size)
{


#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialSyncGrp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_SYNC_MSG != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsg() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsg() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsSyncGrpMsg* item = new RsSyncGrpMsg(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= getRawUInt32(data, *size, &offset, &(item->syncAge));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsg() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialSyncGrpMsg() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsNxsSearchReq* RsNxsSerialiser::deserialNxsSearchReq(void *data, uint32_t *size)
{
    return NULL;
}

RsNxsExtended* RsNxsSerialiser::deserialNxsExtended(void *data, uint32_t *size){
    return NULL;
}



/*** size functions ***/


uint32_t RsNxsSerialiser::sizeNxsMsg(RsNxsMsg *item)
{

    uint32_t s = 8; //header size

    s += GetTlvStringSize(item->grpId);
    s += GetTlvStringSize(item->msgId);
    s += GetTlvStringSize(item->identity);
    s += 4; // msgFlag
    s += 4; // timeStamp
    s += item->publishSign.TlvSize();
    s += item->idSign.TlvSize();
    s += item->msg.TlvSize();

    return s;
}

uint32_t RsNxsSerialiser::sizeNxsGrp(RsNxsGrp *item)
{
    uint32_t s = 8; // header size

    s += GetTlvStringSize(item->grpId);
    s += GetTlvStringSize(item->identity);
    s += 4; // grpFlag
    s += 4; // timestamp
    s += item->adminSign.TlvSize();
    s += item->idSign.TlvSize();
    s += item->keys.TlvSize();
    s += item->grp.TlvSize();

    return s;
}


uint32_t RsNxsSerialiser::sizeSyncGrp(RsSyncGrp *item)
{
    uint32_t s = 8; // header size

    s += 1; // flag
    s += 4; // sync age
    s += GetTlvStringSize(item->syncHash);

    return s;
}


uint32_t RsNxsSerialiser::sizeSyncGrpList(RsSyncGrpList *item)
{
    uint32_t s = 8; // header size

    s += 1; // flag

    std::map<std::string, std::list<RsTlvKeySignature> >::iterator mit
            = item->grps.begin();

    for(; mit != item->grps.end(); mit++){

        s += 4; // number of versions
        s += GetTlvStringSize(mit->first);

        std::list<RsTlvKeySignature>& verL = mit->second;
        std::list<RsTlvKeySignature>::iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            RsTlvKeySignature& sign = *lit; // version
            s += sign.TlvSize();
        }
    }
    return s;
}


uint32_t RsNxsSerialiser::sizeSyncGrpMsg(RsSyncGrpMsg *item)
{

    uint32_t s = 8;

    s += 1; // flag
    s += 4; // age
    s += GetTlvStringSize(item->grpId);
    s += GetTlvStringSize(item->syncHash);

    return s;
}


uint32_t RsNxsSerialiser::sizeSyncGrpMsgList(RsSyncGrpMsgList *item)
{
    uint32_t s = 8; // header size

    s += 1; // flag
    s += GetTlvStringSize(item->grpId);
    std::map<std::string, std::list<RsTlvKeySignature> >::iterator mit
            = item->msgs.begin();

    for(; mit != item->msgs.end(); mit++){

        s += 4; // number of versions
        s += GetTlvStringSize(mit->first);

        std::list<RsTlvKeySignature>& verL = mit->second;
        std::list<RsTlvKeySignature>::iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            RsTlvKeySignature& sign = *lit; // version
            s += sign.TlvSize();
        }
    }
    return s;
}

uint32_t RsNxsSerialiser::sizeNxsSearchReq(RsNxsSearchReq *item){
    return 0;
}


/** print and clear functions **/

void RsNxsMsg::clear()
{
    msg.TlvClear();
    grpId.clear();
    msgId.clear();
    msgFlag = 0;
    timeStamp = 0;
    publishSign.TlvClear();
    idSign.TlvClear();
    identity.clear();
}

void RsNxsGrp::clear()
{
    grpId.clear();
    timeStamp = 0;
    grp.TlvClear();
    adminSign.TlvClear();
    keys.TlvClear();
    identity.clear();
    grpFlag = 0;
    idSign.TlvClear();
}

void RsSyncGrp::clear()
{
    flag = 0;
    syncAge = 0;
    syncHash.clear();
}

void RsSyncGrpMsg::clear()
{
    grpId.clear();
    flag = 0;
    syncAge = 0;
    syncHash.clear();
}

void RsSyncGrpList::clear()
{
    flag = 0;
    grps.clear();
}

void RsSyncGrpMsgList::clear()
{
    flag = 0;
    msgs.clear();
}

std::ostream& RsSyncGrp::print(std::ostream &out, uint16_t indent)
{

    printRsItemBase(out, "RsSyncGrp", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "Hash: " << syncHash << std::endl;
    printIndent(out , int_Indent);
    out << "Sync Age: " << syncAge << std::endl;
    printIndent(out , int_Indent);
    out << "flag" << flag << std::endl;


    printRsItemEnd(out ,"RsSyncGrp", indent);

    return out;
}

std::ostream& RsNxsExtended::print(std::ostream &out, uint16_t indent){
    printRsItemBase(out, "RsNxsExtended", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "type: " << type << std::endl;
    printIndent(out , int_Indent);
    extData.print(out, indent);

    printRsItemEnd(out ,"RsNxsExtended", indent);

    return out;
}

std::ostream& RsSyncGrpMsg::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsSyncGrpMsg", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "GrpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "syncAge: " << syncAge << std::endl;
    printIndent(out , int_Indent);
    out << "syncHash: " << syncHash << std::endl;
    printIndent(out , int_Indent);
    out << "flag: " << flag << std::endl;

    printRsItemEnd(out, "RsSyncGrpMsg", indent);
    return out;
}

std::ostream& RsSyncGrpList::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsSyncGrpList", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "flag: " << flag << std::endl;
    printIndent(out , int_Indent);

    SyncList::iterator mit
            = grps.begin();

    for(; mit != grps.end(); mit++){

        out << "grpId: " <<  mit->first;
        printIndent(out , int_Indent);

        std::list<RsTlvKeySignature>& verL = mit->second;
        std::list<RsTlvKeySignature>::iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            RsTlvKeySignature& sign = *lit;
            sign.print(out, indent);
        }
    }

    printRsItemEnd(out , "RsSyncGrpList", indent);
    return out;
}



std::ostream& RsSyncGrpMsgList::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsSyncGrpMsgList", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out , int_Indent);
    out << "flag: " << flag << std::endl;
    printIndent(out , int_Indent);
    out << "grpId: " << grpId << std::endl;

    SyncList::iterator mit
            = msgs.begin();

    for(; mit != msgs.end(); mit++){

        out << "msgId: " <<  mit->first;
        printIndent(out , int_Indent);

        std::list<RsTlvKeySignature>& verL = mit->second;
        std::list<RsTlvKeySignature>::iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
           RsTlvKeySignature& sign = *lit;
           sign.print(out, indent);
        }
    }

    printRsItemEnd(out ,"RsSyncGrpMsgList", indent);
    return out;
}

std::ostream& RsNxsGrp::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsNxsGrp", indent);
    uint16_t int_Indent = indent + 2;

    out << "grpId: " << grpId << std::endl;
    printIndent(out , int_Indent);
    out << "timeStamp: " << timeStamp << std::endl;
    printIndent(out , int_Indent);
    out << "identity: " << identity << std::endl;
    printIndent(out , int_Indent);
    out << "grpFlag: " << grpFlag << std::endl;
    out << "adminSign: " << std::endl;
    adminSign.print(out, indent);
    out << "idSign: " << std::endl;
    idSign.print(out, indent);
    out << "keys: " << std::endl;
    keys.print(out, indent);

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
    out << "msgFlag: " << msgFlag << std::endl;
    printIndent(out , int_Indent);
    out << "identity: " << identity << std::endl;
    printIndent(out , int_Indent);
    out << "timeStamp: " << timeStamp << std::endl;
    printIndent(out , int_Indent);
    out << "pub sign: " << std::endl;
    publishSign.print(out, indent);
    out << "id sign: " << std::endl;
    idSign.print(out, indent);

    printRsItemEnd(out ,"RsNxsMsg", indent);
    return out;
}
