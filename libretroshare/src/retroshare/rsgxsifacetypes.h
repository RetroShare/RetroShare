/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsifacetypes.h                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013  crispy                                                  *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#ifndef RSGXSIFACETYPES_H_
#define RSGXSIFACETYPES_H_

#include <map>
#include <vector>
#include <string>
#include <inttypes.h>

#include "retroshare/rstypes.h"
#include "retroshare/rsids.h"
#include "serialiser/rsserializable.h"
#include "serialiser/rstypeserializer.h"
#include "util/rstime.h"

typedef Sha1CheckSum RsGxsMessageId;

typedef std::map<RsGxsGroupId, std::set<RsGxsMessageId> > GxsMsgIdResult;
typedef std::pair<RsGxsGroupId, RsGxsMessageId> RsGxsGrpMsgIdPair;
typedef std::map<RsGxsGrpMsgIdPair, std::set<RsGxsMessageId> > MsgRelatedIdResult;
typedef std::map<RsGxsGroupId, std::set<RsGxsMessageId> > GxsMsgReq;

struct RsMsgMetaData;

typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > MsgMetaResult;

enum class GxsRequestPriority {
    VERY_HIGH      =  0x00,
    HIGH           =  0x01,
    NORMAL         =  0x02,
    LOW            =  0x03,
    VERY_LOW       =  0x04,
};

class RsGxsGrpMetaData;
class RsGxsMsgMetaData;

struct RsGroupMetaData : RsSerializable
{
	// (csoler) The correct default value to be used in mCircleType is GXS_CIRCLE_TYPE_PUBLIC, which is defined in rsgxscircles.h,
    // but because of a loop in the includes, I cannot include it here. So I replaced with its current value 0x0001.

	RsGroupMetaData() : mGroupFlags(0), mSignFlags(0), mPublishTs(0),
	    mCircleType(0x0001), mAuthenFlags(0), mSubscribeFlags(0), mPop(0),
	    mVisibleMsgCount(0), mLastPost(0), mGroupStatus(0) {}

    virtual ~RsGroupMetaData() {}

    void operator =(const RsGxsGrpMetaData& rGxsMeta);

    RsGxsGroupId mGroupId;
    std::string mGroupName;
	uint32_t    mGroupFlags;  // Combination of FLAG_PRIVACY_PRIVATE | FLAG_PRIVACY_RESTRICTED | FLAG_PRIVACY_PUBLIC: diffusion
    uint32_t    mSignFlags;   // Combination of RSGXS_GROUP_SIGN_PUBLISH_MASK & RSGXS_GROUP_SIGN_AUTHOR_MASK, i.e. what signatures are required for parent and child msgs

    rstime_t      mPublishTs; // Mandatory.
    RsGxsId    mAuthorId;   // Author of the group. Left to "000....0" if anonymous

    // for circles
    RsGxsCircleId mCircleId;	// Id of the circle to which the group is restricted
    uint32_t mCircleType;		// combination of CIRCLE_TYPE_{ PUBLIC,EXTERNAL,YOUR_FRIENDS_ONLY,LOCAL,EXT_SELF,YOUR_EYES_ONLY }

    // other stuff.
    uint32_t mAuthenFlags;		// Actually not used yet.
    RsGxsGroupId mParentGrpId;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; 			// Popularity = number of friend subscribers
    uint32_t    mVisibleMsgCount; 	// Max messages reported by friends
    rstime_t      mLastPost; 		// Timestamp for last message. Not used yet.

    uint32_t    mGroupStatus;

	/// Service Specific Free-Form local (non-synced) extra storage.
	std::string mServiceString;
    RsPeerId mOriginator;
    RsGxsCircleId mInternalCircle;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mGroupId);
		RS_SERIAL_PROCESS(mGroupName);
		RS_SERIAL_PROCESS(mGroupFlags);
		RS_SERIAL_PROCESS(mSignFlags);
		RS_SERIAL_PROCESS(mPublishTs);
		RS_SERIAL_PROCESS(mAuthorId);
		RS_SERIAL_PROCESS(mCircleId);
		RS_SERIAL_PROCESS(mCircleType);
		RS_SERIAL_PROCESS(mAuthenFlags);
		RS_SERIAL_PROCESS(mParentGrpId);
		RS_SERIAL_PROCESS(mSubscribeFlags);
		RS_SERIAL_PROCESS(mPop);
		RS_SERIAL_PROCESS(mVisibleMsgCount);
		RS_SERIAL_PROCESS(mLastPost);
		RS_SERIAL_PROCESS(mGroupStatus);
		RS_SERIAL_PROCESS(mServiceString);
		RS_SERIAL_PROCESS(mOriginator);
		RS_SERIAL_PROCESS(mInternalCircle);
	}
};

// This is the parent class of all interface-level GXS group data. Derived classes
// will include service-specific information, such as icon, description, etc

struct RsGxsGenericGroupData
{
    virtual ~RsGxsGenericGroupData() = default; // making the type polymorphic

	RsGroupMetaData mMeta;
};

struct RsMsgMetaData : RsSerializable
{
	RsMsgMetaData() : mPublishTs(0), mMsgFlags(0), mMsgStatus(0), mChildTs(0) {}

    virtual ~RsMsgMetaData() {}
    void operator =(const RsGxsMsgMetaData& rGxsMeta);

    RsGxsGroupId mGroupId;
    RsGxsMessageId mMsgId;

    RsGxsMessageId mThreadId;
    RsGxsMessageId mParentId;
    RsGxsMessageId mOrigMsgId;

    RsGxsId    mAuthorId;

    std::string mMsgName;
    rstime_t      mPublishTs;

	/** the lower 16 bits for service, upper 16 bits for GXS
	 * @todo mixing service level flags and GXS level flag into the same member
	 * is prone to confusion, use separated members for those things, this could
	 * be done without breaking network retro-compatibility */
	uint32_t mMsgFlags;

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
    // normally READ / UNREAD flags. LOCAL Data.

	/** the first 16 bits for service, last 16 for GXS
	 * @todo mixing service level flags and GXS level flag into the same member
	 * is prone to confusion, use separated members for those things, this could
	 * be done without breaking network retro-compatibility */
	uint32_t mMsgStatus;

    rstime_t      mChildTs;
    std::string mServiceString; // Service Specific Free-Form extra storage.

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mGroupId);
		RS_SERIAL_PROCESS(mMsgId);
		RS_SERIAL_PROCESS(mThreadId);
		RS_SERIAL_PROCESS(mParentId);
		RS_SERIAL_PROCESS(mOrigMsgId);
		RS_SERIAL_PROCESS(mAuthorId);
		RS_SERIAL_PROCESS(mMsgName);
		RS_SERIAL_PROCESS(mPublishTs);
		RS_SERIAL_PROCESS(mMsgFlags);
		RS_SERIAL_PROCESS(mMsgStatus);
		RS_SERIAL_PROCESS(mChildTs);
		RS_SERIAL_PROCESS(mServiceString);
	}
};

struct RsGxsGenericMsgData
{
    virtual ~RsGxsGenericMsgData() = default; // making the type polymorphic

	RsMsgMetaData mMeta;
};


class GxsGroupStatistic
{
public:
	GxsGroupStatistic()
	{
		mNumMsgs = 0;
		mTotalSizeOfMsgs = 0;
		mNumThreadMsgsNew = 0;
		mNumThreadMsgsUnread = 0;
		mNumChildMsgsNew = 0;
        mNumChildMsgsUnread = 0;
	}

public:
	/// number of message
    RsGxsGroupId mGrpId;

    uint32_t mNumMsgs;			// from the database
	uint32_t mTotalSizeOfMsgs;
	uint32_t mNumThreadMsgsNew;
	uint32_t mNumThreadMsgsUnread;
	uint32_t mNumChildMsgsNew;
    uint32_t mNumChildMsgsUnread;
};

class GxsServiceStatistic
{
public:
	GxsServiceStatistic()
	{
		mNumMsgs = 0;
		mNumGrps = 0;
		mSizeOfMsgs = 0;
		mSizeOfGrps = 0;
		mNumGrpsSubscribed = 0;
		mNumThreadMsgsNew = 0;
		mNumThreadMsgsUnread = 0;
		mNumChildMsgsNew = 0;
		mNumChildMsgsUnread = 0;
		mSizeStore = 0;
	}

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
