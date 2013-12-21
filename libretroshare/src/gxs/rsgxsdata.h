#ifndef RSGXSMETA_H
#define RSGXSMETA_H

/*
 * libretroshare/src/gxs: rsgxsdata.h
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

#include <string>
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsgxsitems.h"

class RsGroupMetaData;
class RsMsgMetaData;

class RsGxsGrpMetaData
{
public:

    RsGxsGrpMetaData();
    bool deserialise(void *data, uint32_t &pktsize);
    bool serialise(void* data, uint32_t &pktsize);
    uint32_t serial_size();
    void clear();
    void operator =(const RsGroupMetaData& rMeta);

    RsGxsGroupId mGroupId;
    RsGxsGroupId mOrigGrpId;
    std::string mGroupName;
    uint32_t    mGroupFlags;
    uint32_t    mPublishTs;
    uint32_t    mSignFlags;
    std::string mAuthorId;

    std::string mCircleId;
    uint32_t mCircleType;


    RsTlvKeySignatureSet signSet;
    RsTlvSecurityKeySet keys;

    std::string mServiceString;
    uint32_t mAuthenFlags;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; // HOW DO WE DO THIS NOW.
    uint32_t    mMsgCount; // ???
    uint32_t      mLastPost; // ???

    uint32_t    mGroupStatus;
    uint32_t    mRecvTS;
    std::string mOriginator;
    std::string mInternalCircle;
    std::string mHash;
};


class RsGxsMsgMetaData
{
public:

    explicit RsGxsMsgMetaData();
    ~RsGxsMsgMetaData();
    bool deserialise(void *data, uint32_t *size);
    bool serialise(void* data, uint32_t *size);
    uint32_t serial_size();
    void clear();
    void operator =(const RsMsgMetaData& rMeta);

    RsGxsGroupId mGroupId;
    RsGxsMessageId mMsgId;
    static int refcount;
    RsGxsMessageId mThreadId;
    RsGxsMessageId mParentId;
    RsGxsMessageId mOrigMsgId;
    std::string mAuthorId;

    RsTlvKeySignatureSet signSet;

    std::string mServiceString;

    std::string mMsgName;
    time_t      mPublishTs;
    uint32_t    mMsgFlags; // Whats this for?

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
    // normally READ / UNREAD flags. LOCAL Data.

    uint32_t    mMsgStatus;
    time_t      mChildTs;
    uint32_t recvTS;
    std::string mHash;
    bool validated;

};




#endif // RSGXSMETA_H
