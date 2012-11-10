
/*
 * libretroshare/src/gxs: rsgxsdata.cc
 *
 * Gxs Data types used to specific services
 *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rsgxsdata.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

RsGxsGrpMetaData::RsGxsGrpMetaData()
{
    clear();
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
    s += GetTlvStringSize(mServiceString);
    s += signSet.TlvSize();
    s += keys.TlvSize();


    return s;
}

void RsGxsGrpMetaData::clear(){

    mGroupId.clear();
    mOrigGrpId.clear();
    mAuthorId.clear();
    mGroupName.clear();
    mServiceString.clear();
    mPublishTs = 0;
    mGroupFlags = 0;
    mPop = 0;
    mMsgCount = 0;
    mGroupStatus = 0;
    mLastPost = 0;
    mSubscribeFlags = 0;
    signSet.TlvClear();
    keys.TlvClear();

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
    ok &= SetTlvString(data, tlvsize, &offset, 0, mServiceString);
    ok &= signSet.SetTlv(data, tlvsize, &offset);
    ok &= keys.SetTlv(data, tlvsize, &offset);


    return ok;
}

bool RsGxsGrpMetaData::deserialise(void *data, uint32_t &pktsize)
{

    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);

    bool ok = true ;

    ok &= rssize == pktsize;

    if(!ok) return false;

    ok &= GetTlvString(data, pktsize, &offset, 0, mGroupId);
    ok &= GetTlvString(data, pktsize, &offset, 0, mOrigGrpId);
    ok &= GetTlvString(data, pktsize, &offset, 0, mGroupName);
    ok &= getRawUInt32(data, pktsize, &offset, &mGroupFlags);
    ok &= getRawUInt32(data, pktsize, &offset, &mPublishTs);
    ok &= GetTlvString(data, pktsize, &offset, 0, mAuthorId);
    ok &= GetTlvString(data, pktsize, &offset, 0, mServiceString);
    ok &= signSet.GetTlv(data, pktsize, &offset);
    ok &= keys.GetTlv(data, pktsize, &offset);


    return ok;
}

RsGxsMsgMetaData::RsGxsMsgMetaData(){

}

uint32_t RsGxsMsgMetaData::serial_size()
{

    uint32_t s = 8; // header size

    s += GetTlvStringSize(mGroupId);
    s += GetTlvStringSize(mMsgId);
    s += GetTlvStringSize(mThreadId);
    s += GetTlvStringSize(mParentId);
    s += GetTlvStringSize(mOrigMsgId);
    s += GetTlvStringSize(mAuthorId);

    s += signSet.TlvSize();
    s += GetTlvStringSize(mMsgName);
    s += 4;
    s += 4;

    return s;
}

void RsGxsMsgMetaData::clear()
{
    mGroupId.clear();
    mMsgId.clear();
    mThreadId.clear();
    mParentId.clear();
    mAuthorId.clear();
    mOrigMsgId.clear();
    mMsgName.clear();
    mServiceString.clear();

    signSet.TlvClear();
    mPublishTs = 0;
    mMsgFlags = 0;
    mMsgStatus = 0;
    mChildTs = 0;
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
    ok &= SetTlvString(data, *size, &offset, 0, mAuthorId);

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

    ok &= GetTlvString(data, *size, &offset, 0, mGroupId);
    ok &= GetTlvString(data, *size, &offset, 0, mMsgId);
    ok &= GetTlvString(data, *size, &offset, 0, mThreadId);
    ok &= GetTlvString(data, *size, &offset, 0, mParentId);
    ok &= GetTlvString(data, *size, &offset, 0, mOrigMsgId);
    ok &= GetTlvString(data, *size, &offset, 0, mAuthorId);

    ok &= signSet.GetTlv(data, *size, &offset);
    ok &= GetTlvString(data, *size, &offset, 0, mMsgName);
    uint32_t t;
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
	this->mMsgCount = rMeta.mMsgCount ;
	this->mPop = rMeta.mPop;
	this->mPublishTs = rMeta.mPublishTs;
	this->mSubscribeFlags = rMeta.mSubscribeFlags;
	this->mGroupName = rMeta.mGroupName;
	this->mServiceString = rMeta.mServiceString;
        this->mSignFlags = rMeta.mSignFlags;
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


