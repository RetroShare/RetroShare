/*******************************************************************************
 * libretroshare/src/gxs: rsgxsdata.h                                          *
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
#ifndef RSGXSMETA_H
#define RSGXSMETA_H

#include <string>

#include "retroshare/rstypes.h"
#include "serialiser/rstlvkeys.h"
#include "util/rstime.h"
#include "rsitems/rsgxsitems.h"

struct RsGroupMetaData;
struct RsMsgMetaData;

static const uint32_t RS_GXS_GRP_META_DATA_VERSION_ID_0001 = 0x0000 ; // change this, and keep old values if the content changes
static const uint32_t RS_GXS_GRP_META_DATA_VERSION_ID_0002 = 0xaf01 ; // current API

static const uint32_t RS_GXS_MSG_META_DATA_VERSION_ID_0002 = 0x0000 ; // current API

static const uint32_t RS_GXS_GRP_META_DATA_CURRENT_API_VERSION = RS_GXS_GRP_META_DATA_VERSION_ID_0002;

class RsGxsGrpMetaData
{
public:
    static const int MAX_ALLOWED_STRING_SIZE = 50 ;

    RsGxsGrpMetaData();
    bool deserialise(void *data, uint32_t &pktsize);
    bool serialise(void* data, uint32_t &pktsize, uint32_t api_version);
    uint32_t serial_size(uint32_t api_version) const;
    void clear();
    void operator =(const RsGroupMetaData& rMeta);

    //Sort data in same order than serialiser and deserializer
    RsGxsGroupId mGroupId;
    RsGxsGroupId mOrigGrpId;
    RsGxsGroupId mParentGrpId;
    std::string mGroupName;
    uint32_t    mGroupFlags;	// GXS_SERV::FLAG_PRIVACY_RESTRICTED | GXS_SERV::FLAG_PRIVACY_PRIVATE | GXS_SERV::FLAG_PRIVACY_PUBLIC
    uint32_t    mPublishTs;
    uint32_t mCircleType;
    uint32_t mAuthenFlags;
    RsGxsId mAuthorId;
    std::string mServiceString;
    RsGxsCircleId mCircleId;
    RsTlvKeySignatureSet signSet;
    RsTlvSecurityKeySet keys;

    uint32_t    mSignFlags;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; 			// Number of friends who subscribed
    uint32_t    mVisibleMsgCount; 	// Max number of messages reported by a single friend (used for unsubscribed groups)
    uint32_t    mLastPost; 		// Time stamp of last post (not yet filled)
    uint32_t    mReputationCutOff;
    uint32_t    mGrpSize;

    uint32_t    mGroupStatus;
    uint32_t    mRecvTS;
    RsPeerId    mOriginator;
    RsGxsCircleId mInternalCircle;
    RsFileHash mHash;
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

    static int refcount;

    //Sort data in same order than serialiser and deserializer
    RsGxsGroupId mGroupId;
    RsGxsMessageId mMsgId;
    RsGxsMessageId mThreadId;
    RsGxsMessageId mParentId;
    RsGxsMessageId mOrigMsgId;
    RsGxsId mAuthorId;

    RsTlvKeySignatureSet signSet;
    std::string mMsgName;
    rstime_t      mPublishTs;
    uint32_t    mMsgFlags; // used by some services (e.g. by forums to store message moderation flags)

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
    // normally READ / UNREAD flags. LOCAL Data.

    std::string mServiceString;
    uint32_t    mMsgStatus;
    uint32_t    mMsgSize;
    rstime_t      mChildTs;
    uint32_t recvTS;
    RsFileHash mHash;
    bool validated;
};




#endif // RSGXSMETA_H
