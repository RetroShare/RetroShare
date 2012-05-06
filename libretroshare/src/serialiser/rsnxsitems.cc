#include "rsnxsitems.h"
#include "rsbaseserial.h"


const uint8_t RsSyncGrpList::FLAG_REQUEST = 0x001;
const uint8_t RsSyncGrpList::FLAG_RESPONSE = 0x002;

const uint8_t RsSyncGrpMsgList::FLAG_REQUEST = 0x001;
const uint8_t RsSyncGrpMsgList::FLAG_RESPONSE = 0x002;

const uint8_t RsSyncGrp::FLAG_USE_SYNC_HASH = 0x001;

const uint8_t RsSyncGrpMsg::FLAG_USE_SYNC_HASH = 0x001;


uint32_t RsNxsSerialiser::size(RsItem *item) {

    RsGrpResp* grp;
    RsGrpMsgResp* gmp;
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
    }else if((grp = dynamic_cast<RsGrpResp*>(item)) != NULL)
    {
        sizeGrpResp(grp);
    }else if((gmp = dynamic_cast<RsGrpMsgResp*>(item)) != NULL)
    {
        sizeGrpMsgResp(gmp);
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
        case RS_PKT_SUBTYPE_GRPS_RESP:
            return deserialGrpResp(data, size);
        case RS_PKT_SUBTYPE_MSG_RESP:
            return deserialGrpMsgResp(data, size);
        case RS_PKT_SUBTYPE_SEARCH_REQ:
            return deserialNxsSearchReq(data, size);
        case RS_PKT_SUBTYPE_SEARCH_RESP:
            return deserialNxsSearchResp(data, size);
        default:
            {
#ifdef NXS_DEBUG
                std::cerr << "RsNxsSerialiser::deserialise() : data has not type"
                          << std::endl;
#endif
                return NULL;

            }
        }
}



bool RsNxsSerialiser::serialise(RsItem *item, void *data, uint32_t *size){

    RsGrpResp* grp;
    RsGrpMsgResp* gmp;
    RsSyncGrp* sg;
    RsSyncGrpList* sgl;
    RsSyncGrpMsg* sgm;
    RsSyncGrpMsgList* sgml;

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
    }else if((grp = dynamic_cast<RsGrpResp*>(item)) != NULL)
    {
        return serialiseGrpResp(grp, data, size);
    }else if((gmp = dynamic_cast<RsGrpMsgResp*>(item)) != NULL)
    {
        return serialiseGrpMsgResp(gmp, data, size);
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

    std::map<std::string, std::list<uint32_t> >::iterator mit =
            item->msgs.begin();

    for(; mit != item->msgs.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= SetTlvUInt32(data, *size, &offset, TLV_TYPE_UINT32_AGE, *lit);
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


bool RsNxsSerialiser::serialiseGrpMsgResp(RsGrpMsgResp *item, void *data, uint32_t *size)
{

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseGrpMsgResp()" << std::endl;
#endif

    uint32_t tlvsize = sizeGrpMsgResp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseGrpMsgResp()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsSyncGrpMsgList */

    std::list<RsTlvBinaryData*>::iterator lit = item->msgs.begin();

    for(; lit != item->msgs.end(); lit++){
        RsTlvBinaryData*& b = *lit;
        b->SetTlv(data, *size, &offset);
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseGrpMsgResp() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseGrpMsgResp() NOK" << std::endl;
    }
#endif

    return ok;
}


bool RsNxsSerialiser::serialiseGrpResp(RsGrpResp *item, void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::serialiseGrpResp()" << std::endl;
#endif

    uint32_t tlvsize = sizeGrpResp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseGrpResp()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    std::list<RsTlvBinaryData*>::iterator lit = item->grps.begin();

    for(; lit != item->grps.end(); lit++){
        RsTlvBinaryData*& b = *lit;
        b->SetTlv(data, *size, &offset);
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseGrpResp() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsNxsSerialiser::serialiseGrpResp() NOK" << std::endl;
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

    std::map<std::string, std::list<uint32_t> >::iterator mit =
            item->grps.begin();


    ok &= setRawUInt8(data, *size, &offset, item->flag);

    for(; mit != item->grps.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= SetTlvUInt32(data, *size, &offset, TLV_TYPE_UINT32_AGE, *lit);
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

bool RsNxsSerialiser::serialiseNxsSearchResp(RsNxsSearchResp *item, void *data, uint32_t *size){
    return false;
}


/*** deserialisation ***/


RsGrpResp* RsNxsSerialiser::deserialGrpResp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialGrpResp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_GRPS_RESP != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpResp() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpResp() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGrpResp* item = new RsGrpResp(SERVICE_TYPE);

    /* skip the header */
    offset += 8;

    while(offset < *size && ok){

        RsTlvBinaryData *b = new RsTlvBinaryData(SERVICE_TYPE);
        ok &= b->GetTlv(data, *size, &offset);

        if(ok)
            item->grps.push_back(b);
        else{
            delete b;

#ifdef RSSERIAL_DEBUG
            std::cerr <<  "RsNxsSerialiser::deserialGrpResp(): Failure to deserialise binary group!" << std::endl;
#endif
        }
    }


    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpResp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpResp() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}


RsGrpMsgResp* RsNxsSerialiser::deserialGrpMsgResp(void *data, uint32_t *size){

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsNxsSerialiser::deserialGrpResp()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_MSG_RESP != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpMsgResp() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpMsgResp() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGrpMsgResp* item = new RsGrpMsgResp(getRsItemService(rstype));
    /* skip the header */
    offset += 8;

    while(offset < *size){

        RsTlvBinaryData *b = new RsTlvBinaryData(SERVICE_TYPE);
        ok &= b->GetTlv(data, *size, &offset);

        if(ok)
            item->msgs.push_back(b);
        else
        {
            delete b;
#ifdef RSSERIAL_DEBUG
            std::cerr <<  "RsNxsSerialiser::deserialGrpMsgResp(): Failure to deserialise binary message!" << std::endl;
#endif
        }
    }

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpMsgResp() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsNxsSerialiser::deserialGrpMsgResp() NOK" << std::endl;
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

        std::list<uint32_t> verL;
        uint32_t verCount  = 0;
        while((offset < rssize) && ok && (verCount < nVersions))
        {
            uint32_t version;
            ok &= GetTlvUInt32(data, *size, &offset, TLV_TYPE_UINT32_AGE, &version);
            verL.push_back(version);
            verCount++;
        }

        item->grps[grpId] = verL;
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

        std::list<uint32_t> verL;
        uint32_t verCount  = 0;
        while((offset < rssize) && ok && (verCount < nVersions))
        {
            uint32_t version;
            ok &= GetTlvUInt32(data, *size, &offset, TLV_TYPE_UINT32_AGE, &version);
            verL.push_back(version);
            verCount++;
        }

        item->msgs[msgId] = verL;
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

RsNxsSearchResp* RsNxsSerialiser::deserialNxsSearchResp(void *data, uint32_t *size)
{

    return NULL;
}



/*** size functions ***/


uint32_t RsNxsSerialiser::sizeGrpMsgResp(RsGrpMsgResp *item)
{

    uint32_t s = 8; //header size

    std::list<RsTlvBinaryData*>::iterator it =
            item->msgs.begin();

    for(; it != item->msgs.end(); it++)
        s += (*it)->TlvSize();

    return s;
}

uint32_t RsNxsSerialiser::sizeGrpResp(RsGrpResp *item)
{

    uint32_t s = 8; // header size

    std::list<RsTlvBinaryData*>::iterator it =
            item->grps.begin();

    for(; it != item->grps.end(); it++)
        s += (*it)->TlvSize();


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

    std::map<std::string, std::list<uint32_t> >::iterator mit
            = item->grps.begin();

    for(; mit != item->grps.end(); mit++){

        s += 4; // number of versions
        s += GetTlvStringSize(mit->first);

        const std::list<uint32_t>& verL = mit->second;
        std::list<uint32_t>::const_iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            s += GetTlvUInt32Size(); // version
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
    std::map<std::string, std::list<uint32_t> >::iterator mit
            = item->msgs.begin();

    for(; mit != item->msgs.end(); mit++){

        s += 4; // number of versions
        s += GetTlvStringSize(mit->first);

        const std::list<uint32_t>& verL = mit->second;
        std::list<uint32_t>::const_iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            s += GetTlvUInt32Size(); // version
        }
    }
    return s;
}


uint32_t RsNxsSerialiser::sizeNxsSearchReq(RsNxsSearchReq *item)
{
    return 0;
}

uint32_t RsNxsSerialiser::sizeNxsSearchResp(RsNxsSearchResp *item)
{
    return 0;
}


/** print and clear functions **/

void RsGrpMsgResp::clear()
{

    std::list<RsTlvBinaryData*>::iterator it =
            msgs.begin();

    for(; it != msgs.end(); it++)
        (*it)->TlvClear();

    msgs.clear();
}

void RsGrpResp::clear()
{
    std::list<RsTlvBinaryData*>::iterator it =
            grps.begin();

    for(; it != grps.end(); it++)
        (*it)->TlvClear();

    grps.clear();
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

    std::map<std::string, std::list<uint32_t> >::iterator mit
            = grps.begin();

    for(; mit != grps.end(); mit++){

        out << "grpId: " <<  mit->first;
        printIndent(out , int_Indent);

        const std::list<uint32_t>& verL = mit->second;
        std::list<uint32_t>::const_iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            out << "version: " << *lit << std::endl;
            printIndent(out , int_Indent);
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

    std::map<std::string, std::list<uint32_t> >::iterator mit
            = msgs.begin();

    for(; mit != msgs.end(); mit++){

        out << "msgId: " <<  mit->first;
        printIndent(out , int_Indent);

        const std::list<uint32_t>& verL = mit->second;
        std::list<uint32_t>::const_iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            out << "version: " << *lit << std::endl;
            printIndent(out , int_Indent);
        }
    }

    printRsItemEnd(out ,"RsSyncGrpMsgList", indent);
    return out;
}

std::ostream& RsGrpMsgResp::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsGrpMsgResp", indent);

    std::list<RsTlvBinaryData*>::iterator it =
            msgs.begin();

    for(; it != msgs.end(); it++)
        (*it)->print(out, indent);


    printRsItemEnd(out, "RsGrpMsgResp", indent);
    return out;
}

std::ostream& RsGrpResp::print(std::ostream &out, uint16_t indent){

    printRsItemBase(out, "RsGrpResp", indent);

    std::list<RsTlvBinaryData*>::iterator it =
            grps.begin();

    for(; it != grps.end(); it++)
        (*it)->print(out, indent);


    printRsItemEnd(out ,"RsGrpMsgResp", indent);
    return out;
}
