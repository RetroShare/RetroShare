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
#include <string>
#include <inttypes.h>

#include <retroshare/rstypes.h>

typedef GXSGroupId   RsGxsGroupId;
typedef Sha1CheckSum RsGxsMessageId;
typedef GXSId        RsGxsId;
typedef GXSCircleId  RsGxsCircleId;

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
            mAuthenFlags = 0;

            mPublishTs = 0;
    }

    void operator =(const RsGxsGrpMetaData& rGxsMeta);

    RsGxsGroupId mGroupId;
    std::string mGroupName;
    uint32_t    mGroupFlags;
    uint32_t    mSignFlags;   // Combination of RSGXS_GROUP_SIGN_PUBLISH_MASK & RSGXS_GROUP_SIGN_AUTHOR_MASK.

    time_t      mPublishTs; // Mandatory.
    RsGxsId    mAuthorId;   // Optional.

    // for circles
    RsGxsCircleId mCircleId;
    uint32_t mCircleType;

    // other stuff.
    uint32_t mAuthenFlags;
    RsGxsGroupId mParentGrpId;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; // HOW DO WE DO THIS NOW.
    uint32_t    mMsgCount; // ???
    time_t      mLastPost; // ???

    uint32_t    mGroupStatus;
    std::string mServiceString; // Service Specific Free-Form extra storage.
    RsPeerId mOriginator;
    RsGxsCircleId mInternalCircle;
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


    RsGxsGroupId mGroupId;
    RsGxsMessageId mMsgId;

    RsGxsMessageId mThreadId;
    RsGxsMessageId mParentId;
    RsGxsMessageId mOrigMsgId;

    RsGxsId    mAuthorId;

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

class GxsGroupStatistic
{
public:
	/// number of message
	RsGxsGroupId mGrpId;
	uint32_t mNumMsgs;
	uint32_t mTotalSizeOfMsgs;
	uint32_t mNumThreadMsgsNew;
	uint32_t mNumThreadMsgsUnread;
	uint32_t mNumChildMsgsNew;
	uint32_t mNumChildMsgsUnread;
};

class GxsServiceStatistic
{
public:

	uint32_t mNumMsgs;
	uint32_t mNumGrps;
	uint32_t mSizeOfMsgs;
	uint32_t mSizeOfGrps;
	uint32_t mNumGrpsSubscribed;
	uint32_t mNumThreadMsgsNew;
	uint32_t mNumThreadMsgsUnread;
	uint32_t mNumChildMsgsNew;
	uint32_t mNumChildMsgsUnread;
	uint32_t mSizeStore;
};

class UpdateItem
{
public:
    virtual ~UpdateItem() { }
};

class StringUpdateItem : public UpdateItem
{
public:
    StringUpdateItem(const std::string update) : mUpdate(update) {}
    const std::string& getUpdate() const { return mUpdate; }

private:
    std::string mUpdate;
};

class RsGxsGroupUpdateMeta
{
public:

    // expand as support is added for other utypes
    enum UpdateType { DESCRIPTION, NAME };

    RsGxsGroupUpdateMeta(const RsGxsGroupId& groupId) : mGroupId(groupId) {}

    typedef std::map<UpdateType, std::string> GxsMetaUpdate;

    /*!
     * Only one item of a utype can exist
     * @param utype the type of meta update
     * @param item update item containing the change value
     */
    void setMetaUpdate(UpdateType utype, const std::string& update)
    {
        mUpdates[utype] = update;
    }

    /*!
     * @param utype update type to remove
     * @return false if update did not exist, true if update successfully removed
     */
    bool removeUpdateType(UpdateType utype){ return mUpdates.erase(utype) == 1; }

    const GxsMetaUpdate* getUpdates() const { return &mUpdates; }
    const RsGxsGroupId& getGroupId() const { return mGroupId; }

private:

    GxsMetaUpdate mUpdates;
    RsGxsGroupId mGroupId;
};

#endif /* RSGXSIFACETYPES_H_ */
