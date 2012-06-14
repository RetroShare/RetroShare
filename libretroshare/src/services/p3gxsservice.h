/*
 * libretroshare/src/services p3gxsservice.h
 *
 * Generic Service Support Class for RetroShare.
 *
 * Copyright 2012 by Robert Fernie.
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

#ifndef P3_GXS_SERVICE_HEADER
#define P3_GXS_SERVICE_HEADER

#include "services/p3service.h"
#include "retroshare/rsidentity.h"

/* 
 * This class provides useful generic support for GXS style services.
 * I expect much of this will be incorporated into the base GXS.
 *
 */

#define GXS_REQUEST_STATUS_FAILED		0
#define GXS_REQUEST_STATUS_PENDING		1
#define GXS_REQUEST_STATUS_PARTIAL		2
#define GXS_REQUEST_STATUS_FINISHED_INCOMPLETE	3
#define GXS_REQUEST_STATUS_COMPLETE		4
#define GXS_REQUEST_STATUS_DONE			5 // ONCE ALL DATA RETRIEVED.

#define GXS_REQUEST_TYPE_GROUPS			0x00010000
#define GXS_REQUEST_TYPE_MSGS			0x00020000
#define GXS_REQUEST_TYPE_MSGRELATED		0x00040000

class gxsRequest
{
	public:
	uint32_t token;
	uint32_t reqTime;

	uint32_t ansType; 
	uint32_t reqType; 
	RsTokReqOptions Options;
	
	uint32_t status;

	std::list<std::string> inList;
	std::list<std::string> outList;
	//std::map<std::string, void *> readyData;
};


class p3GxsService: public p3Service
{
	protected:

	p3GxsService(uint16_t type);

	public:

//virtual ~p3Service() { p3Service::~p3Service(); return; }

bool	generateToken(uint32_t &token);
bool	storeRequest(const uint32_t &token, const uint32_t &ansType, const RsTokReqOptions &opts, const uint32_t &type, const std::list<std::string> &ids);
bool	clearRequest(const uint32_t &token);

bool	updateRequestStatus(const uint32_t &token, const uint32_t &status);
bool	updateRequestInList(const uint32_t &token, std::list<std::string> ids);
bool	updateRequestOutList(const uint32_t &token, std::list<std::string> ids);
//bool	updateRequestData(const uint32_t &token, std::map<std::string, void *> data);
bool    checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, uint32_t &anstype, time_t &ts);

	// special ones for testing (not in final design)
bool    tokenList(std::list<uint32_t> &tokens);
bool    popRequestInList(const uint32_t &token, std::string &id);
bool    popRequestOutList(const uint32_t &token, std::string &id);
bool    loadRequestOutList(const uint32_t &token, std::list<std::string> &ids);

virtual bool    fakeprocessrequests();

	protected:

	RsMutex mReqMtx;

	uint32_t mNextToken;

	std::map<uint32_t, gxsRequest> mRequests;

};


class GxsDataProxy
{
	public:

	GxsDataProxy();

virtual bool getGroupList(     uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds, std::list<std::string> &outGroupIds);
virtual bool getMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds, std::list<std::string> &outMsgIds);
virtual bool getMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds, std::list<std::string> &outMsgIds);


virtual bool createGroup(void *groupData);
virtual bool createMsg(void *msgData);

	/* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *groupData, RsMsgMetaData &meta);

	/* extract Data */
	bool getGroupSummary(const std::list<std::string> &groupIds, std::list<RsGroupMetaData> &groupSummary);
	bool getMsgSummary(const std::list<std::string> &msgIds, std::list<RsMsgMetaData> &msgSummary);

	bool getGroupSummary(const std::string &groupId, RsGroupMetaData &groupSummary);
	bool getMsgSummary(const std::string &msgId, RsMsgMetaData &msgSummary);

	//bool getGroupData(const std::list<std::string> &groupIds, std::list<void *> &groupData);
	//bool getMsgData(const std::list<std::string> &msgIds, std::list<void *> &msgData);
	bool getGroupData(const std::string &groupId, void * &groupData);
	bool getMsgData(const std::string &msgId, void * &msgData);

	bool isUniqueGroup(const std::string &groupId);
	bool isUniqueMsg(const std::string &msgId);


	RsMutex mDataMtx;
	
	std::map<std::string, void *> mGroupData;
	std::map<std::string, void *> mMsgData;

	std::map<std::string, RsGroupMetaData> mGroupMetaData;
	std::map<std::string, RsMsgMetaData>   mMsgMetaData;

};


class p3GxsDataService: public p3GxsService
{
	public:

	p3GxsDataService(uint16_t type, GxsDataProxy *proxy);
virtual bool    fakeprocessrequests();


	protected:

	GxsDataProxy *mProxy;
};



#endif // P3_GXS_SERVICE_HEADER

