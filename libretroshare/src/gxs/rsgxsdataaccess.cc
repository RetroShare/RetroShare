#include "rsgxsdataaccess.h"

/*
 * libretroshare/src/retroshare: rsgxsdataaccess.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie, Christopher Evi-Parker
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

// This bit will be filled out over time.
#define RS_TOKREQOPT_MSG_VERSIONS	0x0001		// MSGRELATED: Returns All MsgIds with OrigMsgId = MsgId.
#define RS_TOKREQOPT_MSG_ORIGMSG	0x0002		// MSGLIST: All Unique OrigMsgIds in a Group.
#define RS_TOKREQOPT_MSG_LATEST		0x0004		// MSGLIST: All Latest MsgIds in Group. MSGRELATED: Latest MsgIds for Input Msgs.

#define RS_TOKREQOPT_MSG_THREAD		0x0010		// MSGRELATED: All Msgs in Thread. MSGLIST: All Unique Thread Ids in Group.
#define RS_TOKREQOPT_MSG_PARENT		0x0020		// MSGRELATED: All Children Msgs.

#define RS_TOKREQOPT_MSG_AUTHOR		0x0040		// MSGLIST: Messages from this AuthorId


// Status Filtering... should it be a different Option Field.
#define RS_TOKREQOPT_GROUP_UPDATED	0x0100		// GROUPLIST: Groups that have been updated.
#define RS_TOKREQOPT_MSG_UPDATED	0x0200		// MSGLIST: Msg that have been updated from specified groups.
#define RS_TOKREQOPT_MSG_UPDATED	0x0200		// MSGLIST: Msg that have been updated from specified groups.



// Read Status.
#define RS_TOKREQOPT_READ		0x0001
#define RS_TOKREQOPT_UNREAD		0x0002

#define RS_TOKREQ_ANSTYPE_LIST		0x0001
#define RS_TOKREQ_ANSTYPE_SUMMARY	0x0002
#define RS_TOKREQ_ANSTYPE_DATA		0x0003

RsGxsDataAccess::RsGxsDataAccess(RsGeneralDataService* ds)
 : mDataStore(ds)
{
}


bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
		const std::list<std::string> &groupIds)
{
	GxsRequest* req = NULL;
	uint32_t reqType = opts.mReqType;

	if(reqType & GXS_REQUEST_TYPE_GROUP_META)
	{
		GroupMetaReq* gmr = new GroupMetaReq();
		gmr->mGroupIds = groupIds;
		req = gmr;
	}
	else if(reqType & GXS_REQUEST_TYPE_GROUP_DATA)
	{
		GroupDataReq* gdr = new GroupDataReq();
		gdr->mGroupIds = groupIds;
		req = gdr;
	}
	else if(reqType & GXS_REQUEST_TYPE_GROUP_IDS)
	{
		GroupIdReq* gir = new GroupIdReq();
		gir->mGroupIds = groupIds;
		req = gir;
	}

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::requestMsgInfo() request type not recognised, type "
				  << reqType << std::endl;
		return false;
	}else
	{
		generateToken(token);
		std::cerr << "RsGxsDataAccess::requestMsgInfo() gets Token: " << token << std::endl;
	}

	setReq(req, token, ansType, opts);
	storeRequest(req);

	return true;
}

void RsGxsDataAccess::generateToken(uint32_t &token)
{
	RsStackMutex stack(mDataMutex); /****** LOCKED *****/

	token = mNextToken++;

	return;
}


bool RsGxsDataAccess::requestMsgInfo(uint32_t &token, uint32_t ansType,
		const RsTokReqOptions &opts, const GxsMsgReq &msgIds)
{

	GxsRequest* req = NULL;
	uint32_t reqType = opts.mReqType;

	if(reqType & GXS_REQUEST_TYPE_MSG_META)
	{
		MsgMetaReq* mmr = new MsgMetaReq();
		mmr->mMsgIds = msgIds;
		req = mmr;
	}else if(reqType & GXS_REQUEST_TYPE_MSG_DATA)
	{
		MsgDataReq* mdr = new MsgDataReq();
		mdr->mMsgIds = msgIds;
		req = mdr;
	}else if(reqType & GXS_REQUEST_TYPE_MSG_IDS)
	{
		MsgIdReq* mir = new MsgIdReq();
		req = mir;
	}

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::requestMsgInfo() request type not recognised, type "
				  << reqType << std::endl;
		return false;
	}else
	{
		generateToken(token);
		std::cerr << "RsGxsDataAccess::requestMsgInfo() gets Token: " << token << std::endl;
	}

	setReq(req, token, ansType, opts);
	storeRequest(req);
	return true;
}

bool RsGxsDataAccess::requestGroupSubscribe(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::string &grpId)
{

	generateToken(token);

	GroupMetaReq* req = new GroupDataReq();
	req->mGroupIds.push_back(grpId);

	std::cerr << "RsGxsDataAccess::requestGroupSubscribe() gets Token: " << token << std::endl;

	setReq(req, token, ansType, opts);
	storeRequest(req);

	return false;
}

void RsGxsDataAccess::setReq(GxsRequest* req, const uint32_t& token, const uint32_t& ansType, const RsTokReqOptions& opts) const
{
	req->token = token;
	req->ansType = ansType;
	req->Options = opts;
	return;
}
void    RsGxsDataAccess::storeRequest(GxsRequest* req)
{
	RsStackMutex stack(mDataMutex); /****** LOCKED *****/

	mRequests[req->token] = req;

	return;
}

uint32_t RsGxsDataAccess::requestStatus(uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}



bool RsGxsDataAccess::cancelRequest(const uint32_t& token)
{
	return clearRequest(token);
}

bool RsGxsDataAccess::clearRequest(const uint32_t& token)
{
	RsStackMutex stack(mDataMutex); /****** LOCKED *****/

	std::map<uint32_t, GxsRequest*>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	delete it->second;
	mRequests.erase(it->first);

	return true;
}

bool RsGxsDataAccess::getGroupSummary(const uint32_t& token, std::list<RsGxsGrpMetaData*>& groupInfo)
{

	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupSummary() Unable to retrieve group summary" << std::endl;
		return false;
	}else{

		GroupMetaReq* gmreq = dynamic_cast<GroupMetaReq*>(req);

		if(gmreq)
		{
			groupInfo = gmreq->mGroupMetaData;
		}else{
			std::cerr << "RsGxsDataAccess::getGroupSummary() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}

bool RsGxsDataAccess::getGroupData(const uint32_t& token, std::list<RsNxsGrp*>& grpData)
{

	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupData() Unable to retrieve group data" << std::endl;
		return false;
	}else{

		GroupDataReq* gmreq = dynamic_cast<GroupMetaReq*>(req);

		if(gmreq)
		{
			grpData = gmreq->mGroupData;
		}else{
			std::cerr << "RsGxsDataAccess::getGroupData() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}

bool RsGxsDataAccess::getMsgData(const uint32_t& token, GxsMsgDataResult& msgData)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgData() Unable to retrieve group data" << std::endl;
		return false;
	}else{

		MsgDataReq* mdreq = dynamic_cast<GroupMetaReq*>(req);

		if(mdreq)
		{
		 msgData = mdreq->mMsgData;
		}else{
			std::cerr << "RsGxsDataAccess::getMsgData() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}

bool RsGxsDataAccess::getMsgSummary(const uint32_t& token, GxsMsgMetaResult& msgInfo)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgSummary() Unable to retrieve group data" << std::endl;
		return false;
	}else{

		MsgMetaReq* mmreq = dynamic_cast<GroupMetaReq*>(req);

		if(mmreq)
		{
		 msgInfo = mmreq->mMsgMetaData;

		}else{
			std::cerr << "RsGxsDataAccess::getMsgSummary() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}

bool RsGxsDataAccess::getMsgList(const uint32_t& token, GxsMsgIdResult& msgIds)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgList() Unable to retrieve group data" << std::endl;
		return false;
	}else{

		MsgIdReq* mireq = dynamic_cast<GroupMetaReq*>(req);

		if(mireq)
		{
		 msgIds = mireq->mMsgIdResult;

		}else{
			std::cerr << "RsGxsDataAccess::getMsgList() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}

bool RsGxsDataAccess::getGroupList(const uint32_t& token, std::list<std::string>& groupIds)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupList() Unable to retrieve group data" << std::endl;
		return false;
	}else{

		GroupIdReq* gireq = dynamic_cast<GroupMetaReq*>(req);

		if(gireq)
		{
		 groupIds = gireq->mGroupIdResult;

		}else{
			std::cerr << "RsGxsDataAccess::getGroupList() Req found, failed caste" << std::endl;
			return false;
		}
	}

	return true;
}


GxsRequest* RsGxsDataAccess::retrieveRequest(const uint32_t& token)
{

	RsStackMutex stack(mDataMutex);

	if(mRequests.find(token) == mRequests.end()) return NULL;

	GxsRequest* req = mRequests;

	return req;
}

#define MAX_REQUEST_AGE 10

void RsGxsDataAccess::processRequests()
{

	std::list<uint32_t> toClear;
	std::list<uint32_t>::iterator cit;
	time_t now = time(NULL);

	{
		RsStackMutex stack(mDataMutex); /******* LOCKED *******/

		std::map<uint32_t, GxsRequest*>::iterator it;

		GroupMetaReq* gmr;
		GroupDataReq* gdr;
		GroupIdReq* gir;

		MsgMetaReq* mmr;
		MsgDataReq* mdr;
		MsgIdReq* mir;

		for(it = mRequests.begin(); it != mRequests.end(); it++)
		{

			GxsRequest* req = it->second;
			if (req->status == GXS_REQUEST_STATUS_PENDING)
			{
				std::cerr << "p3GxsDataService::fakeprocessrequests() Processing Token: " << req->token << " Status: "
						<< req->status << " ReqType: " << req->reqType << " Age: "
						<< now - req->reqTime << std::endl;

				req->status = GXS_REQUEST_STATUS_PARTIAL;

				/* PROCESS REQUEST! */

				if((gmr = dynamic_cast<GroupMetaReq*>(req)) != NULL)
				{
					getGroupSummary(gmr);
				}
				else if((gdr = dynamic_cast<GroupDataReq*>(req)) != NULL)
				{
					getGroupData(gdr);
				}
				else if((gir = dynamic_cast<GroupIdReq*>(req)) != NULL)
				{
					getGroupList(gir);
				}
				else if((mmr = dynamic_cast<MsgMetaReq*>(req)) != NULL)
				{
					getMsgSummary(mmr);
				}
				else if((mdr = dynamic_cast<MsgDataReq*>(req)) != NULL)
				{
					getMsgData(mdr);
				}
				else if((mir = dynamic_cast<MsgIdReq*>(req)) != NULL)
				{
					getMsgList(mir);
				}
				else
				{
	#ifdef GXSDATA_SERVE_DEBUG
					std::cerr << "RsGxsDataAccess::processRequests() Failed to process request, token: "
							  << req->token << std::endl;
	#endif

					req->status = GXS_REQUEST_STATUS_FAILED;
				}
			}
			else if (req->status == GXS_REQUEST_STATUS_PARTIAL)
			{
					req->status = GXS_REQUEST_STATUS_COMPLETE;
			}
			else if (req->status == GXS_REQUEST_STATUS_DONE)
			{
				std::cerr << "RsGxsDataAccess::processrequests() Clearing Done Request Token: "
						  << req->token;
				std::cerr << std::endl;
				toClear.push_back(req->token);
			}
			else if (now - req->reqTime > MAX_REQUEST_AGE)
			{
				std::cerr << "RsGxsDataAccess::processrequests() Clearing Old Request Token: " << req->token;
				std::cerr << std::endl;
				toClear.push_back(req->token);
			}
		}

	} // END OF MUTEX.

	for(cit = toClear.begin(); cit != toClear.end(); cit++)
	{
		clearRequest(*cit);
	}

	return;
}


bool RsGxsDataAccess::getGroupData(GroupDataReq* req)
{

	std::map<std::string, RsNxsGrp*> grpData;
	mDataStore->retrieveNxsGrps(grpData, true);

	std::map<std::string, RsNxsGrp*>::iterator mit = grpData.begin();
	for(; mit != grpData.end(); mit++)
		req->mGroupData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupSummary(GroupMetaReq* req)
{

	std::map<std::string, RsGxsGrpMetaData*> grpMeta;

	std::list<std::string>::const_iterator lit = req->mGroupIds.begin();

	for(; lit != req->mGroupIds.end(); lit++)
		grpMeta[*lit] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	std::map<std::string, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

	for(; mit != grpMeta.end(); mit++)
		req->mGroupMetaData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupList(GroupIdReq* req)
{
	std::map<std::string, RsGxsGrpMetaData*> grpMeta;

	std::list<std::string>::const_iterator lit = req->mGroupIds.begin();

	for(; lit != req->mGroupIds.end(); lit++)
		grpMeta[*lit] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	std::map<std::string, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

	for(; mit != grpMeta.end(); mit++)
	{
		req->mGroupIdResult.push_back(mit->first);
		delete mit->second; // so wasteful!!
	}

	return true;
}

bool RsGxsDataAccess::getMsgData(MsgDataReq* req)
{


	GxsMsgResult result;
	mDataStore->retrieveNxsMsgs(req->mMsgIds, result, true);

	req->mMsgData = result;

	return true;
}


bool RsGxsDataAccess::getMsgSummary(MsgMetaReq* req)
{
	GxsMsgMetaResult result;
	std::vector<std::string> groupIds;
	GxsMsgReq::iterator mit = req->mMsgIds.begin();
	for(; mit != req->mMsgIds.end(); mit++)
		groupIds.push_back(mit->first);

	mDataStore->retrieveGxsMsgMetaData(groupIds, result);

	req->mMsgMetaData = result;

	return true;
}

bool RsGxsDataAccess::getMsgList(MsgIdReq* req)
{
	GxsMsgMetaResult result;
	std::vector<std::string> groupIds;
	GxsMsgReq::iterator mit = req->mMsgIds.begin();

	for(; mit != req->mMsgIds.end(); mit++)
		groupIds.push_back(mit->first);

	mDataStore->retrieveGxsMsgMetaData(groupIds, result);

	GxsMsgMetaResult::iterator mit2 = result.begin();

	for(; mit2 != result.end(); mit2++)
	{
		std::vector<RsGxsMsgMetaData*>& msgIdV = mit2->second;
		std::vector<RsGxsMsgMetaData*>::iterator vit = mit2->second.begin();
		std::vector<std::string> msgIds;
		for(; vit != mit2->second.end(); vit++)
		{
			msgIds.push_back((*vit)->mMsgId);
			delete *vit;
		}

		req->mMsgIdResult.insert(std::pair<std::string,
				std::vector<std::string> >(mit2->first, msgIds));

	}

	return true;
}
