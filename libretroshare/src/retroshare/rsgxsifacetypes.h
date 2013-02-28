/*
 * rsgxsifacetypes.h
 *
 *  Created on: 28 Feb 2013
 *      Author: crispy
 */

#ifndef RSGXSIFACETYPES_H_
#define RSGXSIFACETYPES_H_

#include <map>
#include <vector>

typedef std::string RsGxsGroupId;
typedef std::string RsGxsMessageId;

typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgIdResult;
typedef std::pair<RsGxsGroupId, RsGxsMessageId> RsGxsGrpMsgIdPair;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsGxsMessageId> > MsgRelatedIdResult;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgReq;

class RsMsgMetaData;

typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > MsgMetaResult;


class RsGxsGrpMetaData;
class RsGxsMsgMetaData;


class RsGroupMetaData
{
public:

    RsGroupMetaData()
    {
            mGroupFlags = 0;
            mSubscribeFlags = 0;

            mPop = 0;
            mMsgCount = 0;
            mLastPost = 0;

            mGroupStatus = 0;
            mCircleType = 0;

            //mPublishTs = 0;
    }

    void operator =(const RsGxsGrpMetaData& rGxsMeta);

    std::string mGroupId;
    std::string mGroupName;
    uint32_t    mGroupFlags;
    uint32_t    mSignFlags;   // Combination of RSGXS_GROUP_SIGN_PUBLISH_MASK & RSGXS_GROUP_SIGN_AUTHOR_MASK.

    time_t      mPublishTs; // Mandatory.
    std::string mAuthorId;   // Optional.

    // for circles
    std::string mCircleId;
    uint32_t mCircleType;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; // HOW DO WE DO THIS NOW.
    uint32_t    mMsgCount; // ???
    time_t      mLastPost; // ???

    uint32_t    mGroupStatus;
    std::string mServiceString; // Service Specific Free-Form extra storage.
    std::string mOriginator;
    std::string mInternalCircle;
};




class RsMsgMetaData
{

public:

    RsMsgMetaData()
    {
            mPublishTs = 0;
            mMsgFlags = 0;

            mMsgStatus = 0;
            mChildTs = 0;
    }

    void operator =(const RsGxsMsgMetaData& rGxsMeta);


    std::string mGroupId;
    std::string mMsgId;

    std::string mThreadId;
    std::string mParentId;
    std::string mOrigMsgId;

    std::string mAuthorId;

    std::string mMsgName;
    time_t      mPublishTs;

    /// the first 16 bits for service, last 16 for GXS
    uint32_t    mMsgFlags;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
    // normally READ / UNREAD flags. LOCAL Data.

    /// the first 16 bits for service, last 16 for GXS
    uint32_t    mMsgStatus;

    time_t      mChildTs;
    std::string mServiceString; // Service Specific Free-Form extra storage.

};


#endif /* RSGXSIFACETYPES_H_ */
