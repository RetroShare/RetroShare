/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsitems.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
 * Copyright 2012-2012 by Christopher Evi-Parker                               *
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
#include "rsgxsitems.h"
#include "gxs/rsgxsdata.h"
#include <iostream>

    void RsMsgMetaData::operator =(const RsGxsMsgMetaData& rGxsMeta)
    {
        this->mAuthorId = rGxsMeta.mAuthorId;
        this->mChildTs = rGxsMeta.mChildTs;
        this->mGroupId = rGxsMeta.mGroupId;
        this->mMsgFlags = rGxsMeta.mMsgFlags;
        this->mMsgId = rGxsMeta.mMsgId;
        this->mMsgName = rGxsMeta.mMsgName;
        this->mMsgStatus = rGxsMeta.mMsgStatus;
        this->mOrigMsgId = rGxsMeta.mOrigMsgId;
        this->mParentId = rGxsMeta.mParentId;
        this->mPublishTs = rGxsMeta.mPublishTs;
        this->mThreadId = rGxsMeta.mThreadId;
        this->mServiceString = rGxsMeta.mServiceString;
    }


    void RsGroupMetaData::operator =(const RsGxsGrpMetaData& rGxsMeta)
    {
        this->mAuthorId = rGxsMeta.mAuthorId;
        this->mGroupFlags = rGxsMeta.mGroupFlags;
        this->mGroupId = rGxsMeta.mGroupId;
        this->mGroupStatus = rGxsMeta.mGroupStatus;
        this->mLastPost = rGxsMeta.mLastPost;
        this->mVisibleMsgCount = rGxsMeta.mVisibleMsgCount;
        this->mPop = rGxsMeta.mPop;
        this->mPublishTs = rGxsMeta.mPublishTs;
        this->mSubscribeFlags = rGxsMeta.mSubscribeFlags;
        this->mGroupName = rGxsMeta.mGroupName;
        this->mServiceString = rGxsMeta.mServiceString;
        this->mSignFlags = rGxsMeta.mSignFlags;
        this->mCircleId = rGxsMeta.mCircleId;
        this->mCircleType = rGxsMeta.mCircleType;
        this->mInternalCircle = rGxsMeta.mInternalCircle;
        this->mOriginator = rGxsMeta.mOriginator;
        this->mAuthenFlags = rGxsMeta.mAuthenFlags;
 //       std::cout << "rGxsMeta.mParentGrpId= " <<rGxsMeta.mParentGrpId<<"\n";
 //       std::cout << "rGxsMeta.mParentGrpId.length()= " <<rGxsMeta.mParentGrpId.length()<<"\n";
        //std::cout << "this->mParentGrpId= " <<this->mParentGrpId<<"\n";
        this->mParentGrpId = rGxsMeta.mParentGrpId;
    }

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta)
{
        out << "[ GroupId: " << meta.mGroupId << " Name: " << meta.mGroupName << " ]";
        return out;
}

std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta)
{
        out << "[ GroupId: " << meta.mGroupId << " MsgId: " << meta.mMsgId;
        out << " Name: " << meta.mMsgName;
        out << " OrigMsgId: " << meta.mOrigMsgId;
        out << " ThreadId: " << meta.mThreadId;
        out << " ParentId: " << meta.mParentId;
        out << " AuthorId: " << meta.mAuthorId;
        out << " Name: " << meta.mMsgName << " ]";
        return out;
}


