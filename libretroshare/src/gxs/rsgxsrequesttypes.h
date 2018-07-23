/*******************************************************************************
 * libretroshare/src/gxs: rsgxsrequesttypes.h                                  *
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
#ifndef RSGXSREQUESTTYPES_H_
#define RSGXSREQUESTTYPES_H_

#include "retroshare/rstokenservice.h"
#include "gxs/rsgds.h"
#include "util/rsdeprecate.h"

struct GxsRequest
{
	GxsRequest() :
	    token(0), reqTime(0), ansType(0), reqType(0),
	    status(RsTokenService::FAILED) {}
	virtual ~GxsRequest() {}

	uint32_t token;
	uint32_t reqTime;

	RS_DEPRECATED uint32_t ansType; /// G10h4ck: This is of no use
	uint32_t reqType;
	RsTokReqOptions Options;

	RsTokenService::GxsRequestStatus status;
};

class GroupMetaReq : public GxsRequest
{
public:
	virtual ~GroupMetaReq();

public:
	std::list<RsGxsGroupId> mGroupIds;
	std::list<const RsGxsGrpMetaData*> mGroupMetaData;
};

class GroupIdReq : public GxsRequest
{
public:
	std::list<RsGxsGroupId> mGroupIds;
	std::list<RsGxsGroupId> mGroupIdResult;
};
class GroupSerializedDataReq : public GxsRequest
{
public:
	std::list<RsGxsGroupId> mGroupIds;
	std::list<RsNxsGrp*> mGroupData;
};

class GroupDataReq : public GxsRequest
{
public:
	virtual ~GroupDataReq();

public:
	std::list<RsGxsGroupId> mGroupIds;
	std::list<RsNxsGrp*> mGroupData;
};

class MsgIdReq : public GxsRequest
{
public:
	GxsMsgReq mMsgIds;
	GxsMsgIdResult mMsgIdResult;
};

class MsgMetaReq : public GxsRequest
{
public:
	virtual ~MsgMetaReq();

public:
	GxsMsgReq mMsgIds;
	GxsMsgMetaResult mMsgMetaData;
};

class MsgDataReq : public GxsRequest
{
public:
	virtual ~MsgDataReq();

public:
	GxsMsgReq mMsgIds;
	NxsMsgDataResult mMsgData;
};

class ServiceStatisticRequest: public GxsRequest
{
public:
	GxsServiceStatistic mServiceStatistic;
};

struct GroupStatisticRequest: public GxsRequest
{
public:
	RsGxsGroupId mGrpId;
	GxsGroupStatistic mGroupStatistic;
};

class MsgRelatedInfoReq : public GxsRequest
{
public:
	virtual ~MsgRelatedInfoReq();

public:
	std::vector<RsGxsGrpMsgIdPair> mMsgIds;
	MsgRelatedIdResult mMsgIdResult;
	MsgRelatedMetaResult mMsgMetaResult;
	NxsMsgRelatedDataResult mMsgDataResult;
};

class GroupSetFlagReq : public GxsRequest
{
public:
	const static uint32_t FLAG_SUBSCRIBE;
	const static uint32_t FLAG_STATUS;

	uint8_t type;
	uint32_t flag;
	uint32_t flagMask;
	RsGxsGroupId grpId;
};

class MessageSetFlagReq : public GxsRequest
{
public:
	const static uint32_t FLAG_STATUS;

	uint8_t type;
	uint32_t flag;
	uint32_t flagMask;
	RsGxsGrpMsgIdPair msgId;
};

#endif /* RSGXSREQUESTTYPES_H_ */
