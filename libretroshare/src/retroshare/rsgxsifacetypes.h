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

            mPublishTs = 0;
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
    uint32_t mAuthenFlags;

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

class GxsGroupStatistic
{
public:
	/// number of message
	RsGxsGroupId mGrpId;
	uint32_t mNumMsgs;
	uint32_t mTotalSizeOfMsgs;
};

class GxsServiceStatistic
{
public:

	uint32_t mNumMsgs;
	uint32_t mNumGrps;
	uint32_t mSizeOfMsgs;
	uint32_t mSizeOfGrps;
	uint32_t mNumGrpsSubscribed;
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

    RsGxsGroupUpdateMeta(const std::string& groupId) : mGroupId(groupId) {}

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
    const std::string& getGroupId() const { return mGroupId; }

private:

    GxsMetaUpdate mUpdates;
    std::string mGroupId;
};

#endif /* RSGXSIFACETYPES_H_ */
