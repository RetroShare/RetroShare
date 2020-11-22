/*******************************************************************************
 * libretroshare/src/gxs: rsgxsdata.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie                *
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
 *******************************************************************************/

#include "rsgxsdata.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

RsGxsGrpMetaData::RsGxsGrpMetaData()
{
    clear();
}

uint32_t RsGxsGrpMetaData::serial_size(uint32_t api_version) const
{
    uint32_t s = 8; // header size

    s += mGroupId.serial_size();
    s += mOrigGrpId.serial_size();
    s += mParentGrpId.serial_size();
    s += GetTlvStringSize(mGroupName);
    s += 4;          // mGroupFlags
    s += 4;          // mPublishTs
    s += 4;          // mCircleType
    s += 4;          // mAuthenFlag
    s += mAuthorId.serial_size();
    s += GetTlvStringSize(mServiceString);
    s += mCircleId.serial_size();
    s += signSet.TlvSize();
    s += keys.TlvSize();
    
    if(api_version == RS_GXS_GRP_META_DATA_VERSION_ID_0002)
        s += 4;      // mSignFlag
    else if(api_version != RS_GXS_GRP_META_DATA_VERSION_ID_0001)
        std::cerr << "(EE) wrong/unknown API version " << api_version << " requested in RsGxsGrpMetaData::serial_size()" << std::endl;

    return s;
}

void RsGxsGrpMetaData::clear(){

    mGroupId.clear();
    mOrigGrpId.clear();
    mGroupName.clear();
    mGroupFlags = 0;
    mPublishTs = 0;
    mSignFlags = 0;
    mAuthorId.clear();

    mCircleId.clear();
    mCircleType = 0;

    signSet.TlvClear();
    keys.TlvClear();

    mServiceString.clear();
    mAuthenFlags = 0;
    mParentGrpId.clear();

    mSubscribeFlags = 0;

    mPop = 0;
    mVisibleMsgCount = 0;
    mGroupStatus = 0;
    mLastPost = 0;
    mReputationCutOff = 0;
    mGrpSize = 0 ;

    mGroupStatus = 0 ;
    mRecvTS = 0;

    mOriginator.clear();
    mInternalCircle.clear();
    mHash.clear() ;
}

bool RsGxsGrpMetaData::serialise(void *data, uint32_t &pktsize,uint32_t api_version)
{
    uint32_t tlvsize = serial_size(api_version) ;
    uint32_t offset = 0;

    if (pktsize < tlvsize)
            return false; /* not enough space */

    pktsize = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, api_version, tlvsize); 

#ifdef GXS_DEBUG
    std::cerr << "RsGxsGrpMetaData serialise()" << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Header: " << ok << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Size: " << tlvsize << std::endl;
#endif

    /* skip header */
    offset += 8;

    ok &= mGroupId.serialise(data, tlvsize, offset);
    ok &= mOrigGrpId.serialise(data, tlvsize, offset);
    ok &= mParentGrpId.serialise(data, tlvsize, offset);
    ok &= SetTlvString(data, tlvsize, &offset, 0, mGroupName);
    ok &= setRawUInt32(data, tlvsize, &offset, mGroupFlags);
    ok &= setRawUInt32(data, tlvsize, &offset, mPublishTs);
    ok &= setRawUInt32(data, tlvsize, &offset, mCircleType);
    ok &= setRawUInt32(data, tlvsize, &offset, mAuthenFlags);
    ok &= mAuthorId.serialise(data, tlvsize, offset);
    ok &= SetTlvString(data, tlvsize, &offset, 0, mServiceString);
    ok &= mCircleId.serialise(data, tlvsize, offset);

    ok &= signSet.SetTlv(data, tlvsize, &offset);
    ok &= keys.SetTlv(data, tlvsize, &offset);

    if(api_version == RS_GXS_GRP_META_DATA_VERSION_ID_0002)
	    ok &= setRawUInt32(data, tlvsize, &offset, mSignFlags);	// new in API v2. Was previously missing. Kept in the end for backward compatibility

    return ok;
}

bool RsGxsGrpMetaData::deserialise(void *data, uint32_t &pktsize)
{

    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);

    bool ok = true ;

    ok &= rssize == pktsize;

    if(!ok) return false;

    ok &= mGroupId.deserialise(data, pktsize, offset);
    ok &= mOrigGrpId.deserialise(data, pktsize, offset);
    ok &= mParentGrpId.deserialise(data, pktsize, offset);
    ok &= GetTlvString(data, pktsize, &offset, 0, mGroupName);
    ok &= getRawUInt32(data, pktsize, &offset, &mGroupFlags);
    ok &= getRawUInt32(data, pktsize, &offset, &mPublishTs);
    ok &= getRawUInt32(data, pktsize, &offset, &mCircleType);
    ok &= getRawUInt32(data, pktsize, &offset, &mAuthenFlags);
    
       
    ok &= mAuthorId.deserialise(data, pktsize, offset);
    ok &= GetTlvString(data, pktsize, &offset, 0, mServiceString);
    ok &= mCircleId.deserialise(data, pktsize, offset);
    ok &= signSet.GetTlv(data, pktsize, &offset);
    ok &= keys.GetTlv(data, pktsize, &offset);
    
    switch(getRsItemId(data))
    {
    case RS_GXS_GRP_META_DATA_VERSION_ID_0002:	ok &= getRawUInt32(data, pktsize, &offset, &mSignFlags);	// current API
	    break ;

    case RS_GXS_GRP_META_DATA_VERSION_ID_0001: mSignFlags = 0;						// old API. Do not leave this uninitialised!
	    break ;

    default:
	    std::cerr << "(EE) RsGxsGrpMetaData::deserialise(): ERROR: unknown API version " << std::hex << getRsItemId(data) << std::dec << std::endl;
    }
 
    if(offset != pktsize)
    {
	std::cerr << "(EE) RsGxsGrpMetaData::deserialise(): ERROR: unmatched size " << offset << ", expected: " << pktsize << std::dec << std::endl;
    	return false ;
    }
#ifdef DROP_NON_CANONICAL_ITEMS
    if(mGroupName.length() > RsGxsGrpMetaData::MAX_ALLOWED_STRING_SIZE)
    {
        std::cerr << "WARNING: Deserialised group with mGroupName.length() = " << mGroupName.length() << ". This is not allowed. This item will be dropped." << std::endl;
        return false ;
    }
    if(mServiceString.length() > RsGxsGrpMetaData::MAX_ALLOWED_STRING_SIZE)
    {
        std::cerr << "WARNING: Deserialised group with mServiceString.length() = " << mGroupName.length() << ". This is not allowed. This item will be dropped." << std::endl;
        return false ;
    }
#endif

    return ok;
}

RsGxsMsgMetaData::RsGxsMsgMetaData(){
	clear();
	//std::cout << "\nrefcount++ : " << ++refcount << std::endl;
	return;
}

RsGxsMsgMetaData::~RsGxsMsgMetaData(){
	//std::cout << "\nrefcount-- : " << --refcount << std::endl;
	return;
}

uint32_t RsGxsMsgMetaData::serial_size() const
{

    uint32_t s = 8; // header size

    s += mGroupId.serial_size();
    s += mMsgId.serial_size();
    s += mThreadId.serial_size();
    s += mParentId.serial_size();
    s += mOrigMsgId.serial_size();
    s += mAuthorId.serial_size();

    s += signSet.TlvSize();
    s += GetTlvStringSize(mMsgName);
    s += 4;          // mPublishTS
    s += 4;          // mMsgFlags

    return s;
}

void RsGxsMsgMetaData::clear()
{
    mGroupId.clear();
    mMsgId.clear();
    mThreadId.clear();
    mParentId.clear();
    mOrigMsgId.clear();
    mAuthorId.clear();

    signSet.TlvClear();
    mMsgName.clear();
    mPublishTs = 0;
    mMsgFlags = 0;

    mServiceString.clear();
    mMsgStatus = 0;
    mMsgSize = 0;
    mChildTs = 0;
    recvTS = 0;
    mHash.clear();
    validated = false;
}

bool RsGxsMsgMetaData::serialise(void *data, uint32_t *size)
{
    uint32_t tlvsize = serial_size() ;
    uint32_t offset = 0;

    if (*size < tlvsize)
            return false; /* not enough space */

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, RS_GXS_MSG_META_DATA_VERSION_ID_0002, tlvsize);

#ifdef GXS_DEBUG
    std::cerr << "RsGxsGrpMetaData serialise()" << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Header: " << ok << std::endl;
    std::cerr << "RsGxsGrpMetaData serialise(): Size: " << tlvsize << std::endl;
#endif

    /* skip header */
    offset += 8;

    ok &= mGroupId.serialise(data, *size, offset);
    ok &= mMsgId.serialise(data, *size, offset);
    ok &= mThreadId.serialise(data, *size, offset);
    ok &= mParentId.serialise(data, *size, offset);
    ok &= mOrigMsgId.serialise(data, *size, offset);
    ok &= mAuthorId.serialise(data, *size, offset);

    ok &= signSet.SetTlv(data, *size, &offset);
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

    ok &= rssize == *size;

    if(!ok) return false;

    ok &= mGroupId.deserialise(data, *size, offset);
    ok &= mMsgId.deserialise(data, *size, offset);
    ok &= mThreadId.deserialise(data, *size, offset);
    ok &= mParentId.deserialise(data, *size, offset);
    ok &= mOrigMsgId.deserialise(data, *size, offset);
    ok &= mAuthorId.deserialise(data, *size, offset);

    ok &= signSet.GetTlv(data, *size, &offset);
    ok &= GetTlvString(data, *size, &offset, 0, mMsgName);
    uint32_t t=0;
    ok &= getRawUInt32(data, *size, &offset, &t);
    mPublishTs = t;
    ok &= getRawUInt32(data, *size, &offset, &mMsgFlags);

    return ok;
}

void RsGxsGrpMetaData::operator =(const RsGroupMetaData& rMeta)
{
    this->mAuthorId = rMeta.mAuthorId;
    this->mGroupFlags = rMeta.mGroupFlags;
    this->mGroupId = rMeta.mGroupId;
    this->mGroupStatus = rMeta.mGroupStatus ;
    this->mLastPost = rMeta.mLastPost;
    this->mVisibleMsgCount = rMeta.mVisibleMsgCount ;
    this->mPop = rMeta.mPop;
    this->mPublishTs = rMeta.mPublishTs;
    this->mSubscribeFlags = rMeta.mSubscribeFlags;
    this->mGroupName = rMeta.mGroupName;
    this->mServiceString = rMeta.mServiceString;
        this->mSignFlags = rMeta.mSignFlags;
        this->mCircleId = rMeta.mCircleId;
        this->mCircleType = rMeta.mCircleType;
        this->mInternalCircle = rMeta.mInternalCircle;
        this->mOriginator = rMeta.mOriginator;
        this->mAuthenFlags = rMeta.mAuthenFlags;
    //std::cout << "rMeta.mParentGrpId= " <<rMeta.mParentGrpId<<"\n";
    this->mParentGrpId = rMeta.mParentGrpId;
}

void RsGxsMsgMetaData::operator =(const RsMsgMetaData& rMeta)
{
    this->mAuthorId = rMeta.mAuthorId;
    this->mChildTs = rMeta.mChildTs ;
    this->mGroupId = rMeta.mGroupId;
    this->mMsgFlags = rMeta.mMsgFlags ;
    this->mMsgId = rMeta.mMsgId ;
    this->mMsgName = rMeta.mMsgName;
    this->mMsgStatus = rMeta.mMsgStatus;
    this->mOrigMsgId = rMeta.mOrigMsgId;
    this->mParentId = rMeta.mParentId ;
    this->mPublishTs = rMeta.mPublishTs ;
    this->mThreadId = rMeta.mThreadId;
    this->mServiceString = rMeta.mServiceString;
}


