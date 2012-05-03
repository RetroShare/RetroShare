#include "rsnxsitems.h"





RsNxsSerialiser::size(RsItem *item) {

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
        sizeSynGrpMsgList(sgml);
    }else if((grp = dynamic_cast<RsGrpResp*>(item)) != NULL)
    {
        sizeGrpMsgResp(grp);
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
            return deserialGrpReq(data, size);
        case RS_PKT_SUBTYPE_SYNC_MSG:
            return deserialSynGrpMsg(data, size);
        case RS_PKT_SUBTYPE_SYNC_MSG_LIST:
            return deserialSynGrpMsgList(data, size);
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
        return serialiseGrpMsgResp(grp, data, size);
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

    uint32_t tlvsize = sizeSynGrpMsgList(item);
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

    ok &= SetTlvUInt8(data, size, offset, TLV_TYPE_UINT8_SERID, item->flag);
    ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_GROUPID, item->grpId);

    std::map<std::string, std::list<uint32_t> >::iterator mit =
            item->msgs.begin();

    // number of msgs
    uint32_t nMsgs = item->msgs.size();

    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nMsgs);

    for(; mit != item->msgs.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_AGE, *lit);
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
    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nMsgs);

    std::list<RsTlvBinaryData>::iterator lit = item->msgs.begin();

    for(; lit != item->msgs.end(); lit++){
        RsTlvBinaryData& b = *lit;
        b.SetTlv(data, size, offset);
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
    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nGrps);

    std::list<RsTlvBinaryData>::iterator lit = item->grps.begin();

    for(; lit != item->grps.end(); lit++){
        RsTlvBinaryData& b = *lit;
        b.SetTlv(data, size, offset);
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

    ok &= SetTlvUInt8(data, size, offset, TLV_TYPE_UINT8_SERID, item->flag);
    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_AGE, item->syncAge);
    ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);

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
    uint32_t nGrps = item->msgs.size();

    ok &= SetTlvUInt8(data, size, offset, TLV_TYPE_UINT8_SERID, item->flag);
    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nGrps);

    for(; mit != item->grps.end(); mit++){

        // if not version contains then this
        // entry is invalid
        ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_MSGID, mit->first);

        uint32_t nVersions = mit->second.size();

        ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_SIZE, nVersions);

        std::list<uint32_t>::iterator lit = mit->second.begin();
        for(; lit != mit->second.end(); lit++)
        {
            ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_AGE, *lit);
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

    ok &= SetTlvUInt8(data, size, offset, TLV_TYPE_UINT8_SERID, item->flag);
    ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_UINT32_AGE, item->syncAge);
    ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_HASH_SHA1, item->syncHash);
    ok &= SetTlvString(data, size, offset, TLV_TYPE_STR_GROUPID, item->grpId);

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
