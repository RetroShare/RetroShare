
#include "rsgxsdata.h"
#include "serialiser/rsbaseserial.h"

RsGxsGrpMetaData::RsGxsGrpMetaData()
{

}

uint32_t RsGxsGrpMetaData::serial_size()
{
    uint32_t s = 8; // header size

    s += GetTlvStringSize(mGroupId);
    s += GetTlvStringSize(mOrigGrpId);
    s += GetTlvStringSize(mGroupName);
    s += 4;
    s += 4;
    s += GetTlvStringSize(mAuthorId);
    s += adminSign.TlvSize();
    s += keys.TlvSize();
    s += idSign.TlvSize();

    return s;
}

bool RsGxsGrpMetaData::serialise(void *data, uint32_t &pktsize)
{

    uint32_t tlvsize = serial_size() ;
    uint32_t offset = 0;

    if (pktsize < tlvsize)
            return false; /* not enough space */

    pktsize = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, 0, tlvsize);

#ifdef GXS_DEBUG
    std::cerr << "RsGxsGrpMetaData serialise()" << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Header: " << ok << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Size: " << tlvsize << std::endl;
#endif

    /* skip header */
    offset += 8;

    ok &= SetTlvString(data, tlvsize, &offset, 0, mGroupId);
    ok &= SetTlvString(data, tlvsize, &offset, 0, mOrigGrpId);
    ok &= SetTlvString(data, tlvsize, &offset, 0, mGroupName);
    ok &= setRawUInt32(data, tlvsize, &offset, mGroupFlags);
    ok &= setRawUInt32(data, tlvsize, &offset, mPublishTs);
    ok &= SetTlvString(data, tlvsize, &offset, 0, mAuthorId);

    ok &= adminSign.SetTlv(data, tlvsize, &offset);
    ok &= keys.SetTlv(data, tlvsize, &offset);
    ok &= idSign.SetTlv(data, tlvsize, &offset);

    return ok;
}

bool RsGxsGrpMetaData::deserialise(void *data, uint32_t &pktsize)
{

    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);

    bool ok = true ;

    ok &= rssize != pktsize;

    if(!ok) return false;

    ok &= GetTlvString(data, pktsize, &offset, 0, mGroupId);
    ok &= GetTlvString(data, pktsize, &offset, 0, mOrigGrpId);
    ok &= GetTlvString(data, pktsize, &offset, 0, mOrigGrpId);
    ok &= getRawUInt32(data, pktsize, &offset, &mGroupFlags);
    ok &= getRawUInt32(data, pktsize, &offset, &mPublishTs);
    ok &= GetTlvString(data, pktsize, &offset, 0, mAuthorId);

    ok &= adminSign.GetTlv(data, pktsize, &offset);
    ok &= keys.SetTlv(data, pktsize, &offset);
    ok &= idSign.SetTlv(data, pktsize, &offset);

    return ok;
}

uint32_t RsGxsMsgMetaData::serial_size()
{

    uint32_t s = 8; // header size

    s += GetTlvStringSize(mGroupId);
    s += GetTlvStringSize(mMsgId);
    s += GetTlvStringSize(mThreadId);
    s += GetTlvStringSize(mParentId);
    s += GetTlvStringSize(mOrigMsgId);

    s += pubSign.TlvSize();
    s += idSign.TlvSize();
    s += GetTlvStringSize(mMsgName);
    s += 4;
    s += 4;

    return s;
}

bool RsGxsMsgMetaData::serialise(void *data, uint32_t *size)
{
    uint32_t tlvsize = serial_size() ;
    uint32_t offset = 0;

    if (*size < tlvsize)
            return false; /* not enough space */

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, 0, tlvsize);

#ifdef GXS_DEBUG
    std::cerr << "RsGxsGrpMetaData serialise()" << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Header: " << ok << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Size: " << tlvsize << std::endl;
#endif

    /* skip header */
    offset += 8;

    ok &= SetTlvString(data, *size, &offset, 0, mGroupId);
    ok &= SetTlvString(data, *size, &offset, 0, mMsgId);
    ok &= SetTlvString(data, *size, &offset, 0, mThreadId);
    ok &= SetTlvString(data, *size, &offset, 0, mParentId);
    ok &= SetTlvString(data, *size, &offset, 0, mOrigMsgId);

    ok &= pubSign.SetTlv(data, *size, &offset);
    ok &= idSign.SetTlv(data, *size, &offset);
    ok &= SetTlvString(data, *size, &offset, 0, mMsgName);
    ok &= setRawUInt32(data, *size, &offset, mPublishTs);
    ok &= setRawUInt32(data, *size, &offset, mMsgFlags);

    return ok;
}


bool RsGxsMsgMetaData::deserialise(void *data, uint32_t *size)
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);

    bool ok = true ;

    ok &= rssize != *size;

    if(!ok) return false;

    ok &= GetTlvString(data, *size, &offset, 0, mGroupId);
    ok &= GetTlvString(data, *size, &offset, 0, mMsgId);
    ok &= GetTlvString(data, *size, &offset, 0, mThreadId);
    ok &= GetTlvString(data, *size, &offset, 0, mParentId);
    ok &= GetTlvString(data, *size, &offset, 0, mOrigMsgId);

    ok &= pubSign.GetTlv(data, *size, &offset);
    ok &= idSign.GetTlv(data, *size, &offset);
    ok &= GetTlvString(data, *size, &offset, 0, mMsgName);
    uint32_t t;
    ok &= getRawUInt32(data, *size, &offset, &t);
    mPublishTs = t;
    ok &= getRawUInt32(data, *size, &offset, &mMsgFlags);

    return ok;
}
