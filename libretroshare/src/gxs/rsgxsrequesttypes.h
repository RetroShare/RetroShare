/*
 * rsgxsrequesttypes.h
 *
 *  Created on: 21 Jul 2012
 *      Author: crispy
 */

#ifndef RSGXSREQUESTTYPES_H_
#define RSGXSREQUESTTYPES_H_


class GxsRequest
{

public:


	virtual ~GxsRequest() { return; }
	uint32_t token;
	uint32_t reqTime;

	uint32_t ansType;
	uint32_t reqType;
	RsTokReqOptions Options;

	uint32_t status;
};

class GroupMetaReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
	std::list<RsGxsGrpMetaData*> mGroupMetaData;
};

class GroupIdReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
	std::list<std::string> mGroupIdResult;
};

class GroupDataReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
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
	GxsMsgReq mMsgIds;
	GxsMsgMetaResult   mMsgMetaData;
};

class MsgDataReq : public GxsRequest
{

public:
	GxsMsgReq mMsgIds;
	NxsMsgDataResult mMsgData;
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
