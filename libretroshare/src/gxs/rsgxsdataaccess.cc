#include "rsgxsdataaccess.h"
#include "retroshare/rsidentity.h"

RsGxsDataAccess::RsGxsDataAccess(RsGeneralDataService* ds)
 : mDataStore(ds)
{
}


bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
		const std::list<std::string> &groupIds)
{

	return true;
}


bool RsGxsDataAccess::requestMsgInfo(uint32_t &token, uint32_t ansType,
		const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{

	generateToken(token);
	std::cerr << "RsGxsDataAccess::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);
	return true;
}

bool RsGxsDataAccess::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions& opts,
		const GxsMsgReq &msgIds)
{
	generateToken(token);
	std::cerr << "RsGxsDataAccess::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

bool RsGxsDataAccess::requestGroupSubscribe(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::string &grpId)
{

	generateToken(token);
	std::cerr << "RsGxsDataAccess::requestGroupSubscribe() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, grpId);

	return false;
}

bool    RsGxsDataAccess::storeRequest(const uint32_t &token, const uint32_t &ansType, const RsTokReqOptions &opts, const uint32_t &type, const std::list<std::string> &ids)
{
	RsStackMutex stack(mDataMutex); /****** LOCKED *****/

	GxsRequest* req;
	req.token = token;
	req.reqTime = time(NULL);
	req.reqType = type;
	req.ansType = ansType;
	req.Options = opts;
	req.status = GXS_REQUEST_STATUS_PENDING;
	req.inList = ids;

	mRequests[token] = req;

	return true;
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

	mRequests.erase(it);

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
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;

	if(checkRequestStatus(token, status, reqtype, anstype, ts))
		return NULL;

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "RsGxsDataAccess::retrieveRequest() ERROR AnsType Wrong" << std::endl;
		return false;
	}

	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "RsGxsDataAccess::retrieveRequest() ERROR ReqType Wrong" << std::endl;
		return false;
	}

	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "RsGxsDataAccess::retrieveRequest() ERROR Status Incomplete" << std::endl;
		return false;
	}

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


	std::map<std::string, RsGxsGrpMetaData*> grpMeta;
	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	return true;
}

