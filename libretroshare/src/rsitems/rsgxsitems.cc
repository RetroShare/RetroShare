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
#include "serialiser/rstypeserializer.h"
#include "serialiser/rsbaseserial.h"
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
        this->mParentGrpId = rGxsMeta.mParentGrpId;
    }

template<> uint32_t RsTypeSerializer::serial_size(const TurtleGxsInfo& i)
{
    uint32_t s = 0 ;

    s += 2 ; // service_id
    s += i.group_id.SIZE_IN_BYTES ;
    s += GetTlvStringSize(i.name) ;

    return s;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[],uint32_t size,uint32_t& offset,TurtleGxsInfo& i)
{
    uint32_t saved_offset = offset ;
    bool ok = true ;

	ok &= getRawUInt16(data, size, &offset, &i.service_id); 			 // service_id
	ok &= i.group_id.deserialise(data, size, offset);                    // group_id
	ok &= GetTlvString(data, size, &offset, TLV_TYPE_STR_NAME, i.name);  // group name

    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[],uint32_t size,uint32_t& offset,const TurtleGxsInfo& i)
{
	uint32_t saved_offset = offset ;
    bool ok = true ;

	ok &= setRawUInt16(data, size, &offset, i.service_id); 					 // service_id
	ok &= i.group_id.serialise(data, size, offset);					         // group_id
	ok &= SetTlvString(data, size, &offset, TLV_TYPE_STR_NAME, i.name);  // group name

	if(!ok)
		offset = saved_offset ;

	return ok;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const TurtleGxsInfo& i)
{
    std::cerr << "  [GXS Info  ] " << n << " group_id=" << i.group_id << " service=" << std::hex << i.service_id << std::dec << ", name=" << i.name << std::endl;
}

void RsTurtleGxsSearchResultGroupSummaryItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,result,"result") ;
}
void RsTurtleGxsSearchResultGroupDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::TlvMemBlock_proxy prox(encrypted_nxs_group_data,encrypted_nxs_group_data_len) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"encrypted_nxs_data") ;
}

RS_TYPE_SERIALIZER_FROM_JSON_NOT_IMPLEMENTED_DEF(TurtleGxsInfo)
RS_TYPE_SERIALIZER_TO_JSON_NOT_IMPLEMENTED_DEF(TurtleGxsInfo)

