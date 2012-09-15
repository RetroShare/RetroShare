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
#include "retroshare/rsidentityVEG.h"

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
	RsTokReqOptionsVEG Options;
	
	uint32_t status;

	std::list<std::string> inList;
	std::list<std::string> outList;
	//std::map<std::string, void *> readyData;
};


class p3GxsServiceVEG: public p3Service
{
	protected:

	p3GxsServiceVEG(uint16_t type);

	public:

//virtual ~p3Service() { p3Service::~p3Service(); return; }

bool	generateToken(uint32_t &token);
bool	storeRequest(const uint32_t &token, const uint32_t &ansType, const RsTokReqOptionsVEG &opts, const uint32_t &type, const std::list<std::string> &ids);
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


class GxsDataProxyVEG
{
	public:

	GxsDataProxyVEG();

virtual bool getGroupList(     uint32_t &token, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds, std::list<std::string> &outGroupIds);
virtual bool getMsgList(       uint32_t &token, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds, std::list<std::string> &outMsgIds);
virtual bool getMsgRelatedList(uint32_t &token, const RsTokReqOptionsVEG &opts, const std::list<std::string> &msgIds, std::list<std::string> &outMsgIds);


	/* This functions return a token - which can be used to retrieve the RsGroupMetaData, later
	 * This is required, as signatures and keys might have to be generated in the background
	 * Though at the moment: for this test system it won't change anything? FIXME.
	 */
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


	/* Handle Status & Subscribe Modes */
// This is removed as redundant - use getGroupList - with OptFlags to find these.
//virtual bool requestGroupsChanged(uint32_t &token); //std::list<std::string> &groupIds);

        // Get Message Status - is retrived via MessageSummary.
	// These operations could have a token, but for the moment we are going to assume
	// they are async and always succeed - (or fail silently).
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

virtual bool setMessageServiceString(const std::string &msgId, const std::string &str);
virtual bool setGroupServiceString(const std::string &grpId, const std::string &str);

	protected:

	bool filterGroupList(const RsTokReqOptionsVEG &opts, std::list<std::string> &groupIds);
	bool filterMsgList(const RsTokReqOptionsVEG &opts, std::list<std::string> &msgIds);


	RsMutex mDataMtx;
	
	std::map<std::string, void *> mGroupData;
	std::map<std::string, void *> mMsgData;

	std::map<std::string, RsGroupMetaData> mGroupMetaData;
	std::map<std::string, RsMsgMetaData>   mMsgMetaData;

};


class p3GxsDataServiceVEG: public p3GxsServiceVEG
{
	public:

	p3GxsDataServiceVEG(uint16_t type, GxsDataProxyVEG *proxy);
virtual bool    fakeprocessrequests();


	protected:

	GxsDataProxyVEG *mProxy;
};



#endif // P3_GXS_SERVICE_HEADER

