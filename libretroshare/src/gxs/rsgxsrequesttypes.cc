/*******************************************************************************
 * libretroshare/src/gxs: rsgxsrequesttypes.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
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

#include "rsgxsrequesttypes.h"
#include "util/rsstd.h"

std::ostream& operator<<(std::ostream& o,const GxsRequest& g)
{
    return g.print(o);
}


std::ostream& GroupMetaReq::print(std::ostream& o) const
{
	o << "[Request type=GroupMeta groupIds (size=" << mGroupIds.size() << "): " ;

    if(!mGroupIds.empty())
    {
        o << *mGroupIds.begin() ;

		if(mGroupIds.size() > 1)
			o << " ..." ;
    }

	o << "]" ;

	return o;
}
std::ostream& GroupIdReq::print(std::ostream& o) const
{
    return o << "[Request type=GroupIdReq" << "]" ;
}

std::ostream& GroupSerializedDataReq::print(std::ostream& o) const
{
    return o << "[Request type=GroupSerializedData" << "]" ;
}

std::ostream& GroupDataReq::print(std::ostream& o) const
{
	o << "[Request type=GroupDataReq groupIds (size=" << mGroupIds.size() << "): " ;

    if(!mGroupIds.empty())
	{
		o << *mGroupIds.begin() ;

		if(mGroupIds.size() > 1)
			o << " ..." ;
	}

	o << "]" ;

	return o;
}

std::ostream& MsgIdReq::print(std::ostream& o) const
{
    return o << "[Request type=MsgId" << "]" ;
}

std::ostream& MsgMetaReq::print(std::ostream& o) const
{
	o << "[Request type=MsgMetaReq groups (size=" << mMsgIds.size() << "): " ;

    if(!mMsgIds.empty())
    {
        o << mMsgIds.begin()->first << " (" << mMsgIds.begin()->second.size() << " messages)";

		if(mMsgIds.size() > 1)
			o << " ..." ;
    }

	o << "]" ;

	return o;
}

std::ostream& MsgDataReq::print(std::ostream& o) const
{
	o << "[Request type=MsgDataReq groups (size=" << mMsgIds.size() << "): " ;

    if(!mMsgIds.empty())
    {
        o << mMsgIds.begin()->first << " (" << mMsgIds.begin()->second.size() << " messages)";

		if(mMsgIds.size() > 1)
			o << " ..." ;
    }

	o << "]" ;

	return o;
}

std::ostream& MsgRelatedInfoReq::print(std::ostream& o) const
{
	o << "[Request type=MsgRelatedInfo msgIds (size=" << mMsgIds.size() << "): " ;

    if(!mMsgIds.empty())
    {
        o << mMsgIds.begin()->first ;

		if(mMsgIds.size() > 1)
			o << " ..." ;
    }

	o << "]" ;

	return o;
}

std::ostream& GroupSetFlagReq::print(std::ostream& o) const
{
	return o << "[Request type=GroupFlagSet grpId=" <<  grpId << "]" ;
}



std::ostream& ServiceStatisticRequest::print(std::ostream& o) const
{
    return o << "[Request type=ServiceStatistics" << "]" ;
}

std::ostream& GroupStatisticRequest::print(std::ostream& o) const
{
	return o << "[Request type=GroupStatistics grpId=" << mGrpId << "]" ;
}

GroupMetaReq::~GroupMetaReq()
{
	//rsstd::delete_all(mGroupMetaData.begin(), mGroupMetaData.end());	// now memory ownership is kept by the cache.
	mGroupMetaData.clear();
}

GroupDataReq::~GroupDataReq()
{
	rsstd::delete_all(mGroupData.begin(), mGroupData.end());
}

MsgDataReq::~MsgDataReq()
{
	for (NxsMsgDataResult::iterator it = mMsgData.begin(); it != mMsgData.end(); ++it) {
		rsstd::delete_all(it->second.begin(), it->second.end());
	}
}

MsgRelatedInfoReq::~MsgRelatedInfoReq()
{
    for (NxsMsgRelatedDataResult::iterator dataIt = mMsgDataResult.begin(); dataIt != mMsgDataResult.end(); ++dataIt)
		rsstd::delete_all(dataIt->second.begin(), dataIt->second.end());
}
std::ostream& MessageSetFlagReq::print(std::ostream& o) const
{
	return o << "[Request type=MsgFlagSet" << "]" ;
}

