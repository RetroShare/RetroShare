#ifndef RSGXSITEMS_H
#define RSGXSITEMS_H

/*
 * libretroshare/src/serialiser: rsgxsitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012   Christopher Evi-Parker, Robert Fernie.
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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"

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

                //mPublishTs = 0;
        }

    	void operator =(const RsGxsGrpMetaData& rGxsMeta);

        std::string mGroupId;
        std::string mGroupName;
        uint32_t    mGroupFlags;
    	uint32_t    mSignFlags;   // Combination of RSGXS_GROUP_SIGN_PUBLISH_MASK & RSGXS_GROUP_SIGN_AUTHOR_MASK.

        time_t      mPublishTs; // Mandatory.
        std::string mAuthorId;   // Optional.

        // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

        uint32_t    mSubscribeFlags;

        uint32_t    mPop; // HOW DO WE DO THIS NOW.
        uint32_t    mMsgCount; // ???
        time_t      mLastPost; // ???

        uint32_t    mGroupStatus;
    	std::string mServiceString; // Service Specific Free-Form extra storage.

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

        uint32_t    mMsgFlags; // Whats this for?

        // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
        // normally READ / UNREAD flags. LOCAL Data.
        uint32_t    mMsgStatus;
        time_t      mChildTs;
    	std::string mServiceString; // Service Specific Free-Form extra storage.

};


class RsGxsGrpItem : public RsItem
{

public:

    RsGxsGrpItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsGrpItem(){}

    RsGroupMetaData meta;
};

class RsGxsMsgItem : public RsItem
{

public:
    RsGxsMsgItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsMsgItem(){}

    RsMsgMetaData meta;
};





#endif // RSGXSITEMS_H
