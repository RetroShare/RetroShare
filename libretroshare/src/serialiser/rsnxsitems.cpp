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
        std::cerr << "RsDistribSerialiser::deserialise()" << std::endl;
#endif
        /* get the type and size */
        uint32_t rstype = getRsItemId(data);

        if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
                (SERVICE_TYPE != getRsItemService(rstype)))
        {
                return NULL; /* wrong type */
        }

        switch(rstype)
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
            return deserialGrpMsgResp(data, size);
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

    // number of msgs
    uint32_t nMsgs = item->msgs.size();

    ok &= setRawUInt32(data, *size, &offset, nMsgs);

    for(; mit != item->msgs.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= setRawUInt32(data, *size, &offset, *lit);
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

    // number of msgs
    uint32_t nMsgs = item->msgs.size();
    ok &= setRawUInt32(data, *size, &offset,nMsgs);

    std::list<RsTlvBinaryData>::iterator lit = item->msgs.begin();

    for(; lit != item->msgs.end(); lit++){
        RsTlvBinaryData& b = *lit;
        b.SetTlv(data, *size, &offset);
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
    std::cerr << "RsNxsSerialiser::serialiseGrpReq()" << std::endl;
#endif

    uint32_t tlvsize = sizeGrpResp(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsNxsSerialiser::serialiseGrpReq()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok = setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    // number of grps
    uint32_t nGrps = item->grps.size();
    ok &= setRawUInt32(data, *size, &offset, nGrps);

    std::list<RsTlvBinaryData>::iterator lit = item->grps.begin();

    for(; lit != item->grps.end(); lit++){
        RsTlvBinaryData& b = *lit;
        b.SetTlv(data, *size, &offset);
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
        std::cerr << "RsNxsSerialiser::serialiseSyncGrpList()" << std::endl;
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

    // number of grps
    uint32_t nGrps = item->grps.size();

    ok &= setRawUInt8(data, *size, &offset, item->flag);
    ok &= setRawUInt32(data, *size, &offset, nGrps);

    for(; mit != item->grps.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= setRawUInt32(data, *size, &offset, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= setRawUInt32(data, *size, &offset, *lit);
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

    RsGrpResp* item = new RsGrpResp(getRsItemService(rstype));
    item->grps;

    while(offset < *size){

        RsTlvBinaryData b(SERVICE_TYPE);
        ok &= b.GetTlv(data, *size, &offset);
        item->grps.push_back(b);
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

    while(offset < *size){

        RsTlvBinaryData b(SERVICE_TYPE);
        ok &= b.GetTlv(data, *size, &offset);
        item->msgs.push_back(b);
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

    RsSyncGrpList* item = new RsSyncGrpList(getRsItemService(rstype));

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    // now get number of grps

    uint32_t nGroups = 0;
    ok &= GetTlvUInt32(data, *size, &offset, TLV_TYPE_UINT32_SIZE, &nGroups);

    for(uint32_t i  =0; i < nGroups; i++){
        uint32_t nVersions;
        ok &= getRawUInt32(data, *size, &offset, &nVersions);

        if(!ok) break;

        std::string grpId;
        ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, grpId);

        if(!ok) break;

        std::list<uint32_t> verL;

        for(uint32_t j  =0; j < nVersions; j++){

            uint32_t version;
            ok &= getRawUInt32(data, *size, &offset, &version);

            if(!ok) break;

            verL.push_back(version);

        }

        if(!ok) break;
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

    ok &= getRawUInt8(data, *size, &offset, &(item->flag));
    ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_GROUPID, item->grpId);

    // now get number of msgs

    uint32_t nMsgs = 0;
    ok &= getRawUInt32(data, *size, &offset, &nMsgs);

    for(uint32_t i  =0; i < nMsgs; i++){
        uint32_t nVersions;
        ok &= getRawUInt32(data, *size, &offset, &nVersions);

        if(!ok) break;

        std::string msgId;
        ok &= GetTlvString(data, *size, &offset, TLV_TYPE_STR_MSGID,msgId);

        if(!ok) break;

        std::list<uint32_t> verL;

        for(uint32_t j  =0; j < nVersions; j++){

            uint32_t version;
            ok &= getRawUInt32(data, *size, &offset, &version);

            if(!ok) break;

            verL.push_back(version);

        }

        if(!ok) break;
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

    std::list<RsTlvBinaryData>::iterator it =
            item->msgs.begin();

    for(; it != item->msgs.end(); it++)
        s += (*it).TlvSize();

    return s;
}

uint32_t RsNxsSerialiser::sizeGrpResp(RsGrpResp *item)
{

    uint32_t s = 8; // header size

    std::list<RsTlvBinaryData>::iterator it =
            item->grps.begin();

    for(; it != item->grps.end(); it++)
        s += (*it).TlvSize();


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
    s += 4; // number of grps

    std::map<std::string, std::list<uint32_t> >::iterator mit
            = item->grps.begin();

    for(; mit != item->grps.end(); mit++){

        s += 4; // number of versions
        s += GetTlvStringSize(mit->first);

        const std::list<uint32_t>& verL = mit->second;
        std::list<uint32_t>::const_iterator lit =
                verL.begin();

        for(; lit != verL.end(); lit++){
            s += 4; // version
        }
    }
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
    s += 4; // number of msgs
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
            s += 4; // version
        }
    }
}


uint32_t RsNxsSerialiser::sizeNxsSearchReq(RsNxsSearchReq *item)
{
    return 0;
}

uint32_t RsNxsSerialiser::sizeNxsSearchResp(RsNxsSearchResp *item)
{
    return 0;
}
