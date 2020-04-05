/*******************************************************************************
 * libretroshare/src/gxs: rsgxsdataaccess.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2013 by Christopher Evi-Parker, Robert Fernie                *
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

#include "util/rstime.h"

#include "rsgxsutil.h"
#include "rsgxsdataaccess.h"
#include "retroshare/rsgxsflags.h"

/***********
 * #define DATA_DEBUG	1
 **********/

#define DATA_DEBUG	1

bool operator<(const std::pair<uint32_t,GxsRequest*>& p1,const std::pair<uint32_t,GxsRequest*>& p2)
{
    return p1.second->priority <= p2.second->priority ;	// <= so that new elements with same priority are inserted before
}


RsGxsDataAccess::RsGxsDataAccess(RsGeneralDataService* ds) :
    mDataStore(ds), mDataMutex("RsGxsDataAccess"), mNextToken(0) {}


RsGxsDataAccess::~RsGxsDataAccess()
{
    for(auto& it:mRequestQueue)
		delete it.second;
}
bool RsGxsDataAccess::requestGroupInfo(
        uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
        const std::list<RsGxsGroupId> &groupIds )
{
	if(groupIds.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << " (WW) Group Id list is empty!"
		          << std::endl;
		return false;
	}

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
    else if(reqType & GXS_REQUEST_TYPE_GROUP_SERIALIZED_DATA)
    {
            GroupSerializedDataReq* gir = new GroupSerializedDataReq();
            gir->mGroupIds = groupIds;
            req = gir;
    }

	if(!req)
	{
		std::cerr << __PRETTY_FUNCTION__ << " request type not recognised, "
		          << "reqType: " << reqType << std::endl;
		return false;
	}

	generateToken(token);

#ifdef DATA_DEBUG
	std::cerr << "RsGxsDataAccess::requestGroupInfo() gets token: " << token
	          << std::endl;
#endif

    setReq(req, token, ansType, opts);
    storeRequest(req);

    return true;
}

bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts)
{

    GxsRequest* req = NULL;
    uint32_t reqType = opts.mReqType;

    if(reqType & GXS_REQUEST_TYPE_GROUP_META)
            req = new GroupMetaReq();
    else if(reqType & GXS_REQUEST_TYPE_GROUP_DATA)
            req = new GroupDataReq();
    else if(reqType & GXS_REQUEST_TYPE_GROUP_IDS)
            req = new GroupIdReq();
    else if(reqType & GXS_REQUEST_TYPE_GROUP_SERIALIZED_DATA)
            req = new GroupSerializedDataReq();
    else
    {
            std::cerr << "RsGxsDataAccess::requestGroupInfo() request type not recognised, type "
                              << reqType << std::endl;
            return false;
    }

	generateToken(token);
#ifdef DATA_DEBUG
	std::cerr << "RsGxsDataAccess::requestGroupInfo() gets Token: " << token << std::endl;
#endif

    setReq(req, token, ansType, opts);
    storeRequest(req);

    return true;
}

void RsGxsDataAccess::generateToken(uint32_t &token)
{
	RS_STACK_MUTEX(mDataMutex);
	token = mNextToken++;
}


bool RsGxsDataAccess::requestMsgInfo(uint32_t &token, uint32_t ansType,
		const RsTokReqOptions &opts, const GxsMsgReq &msgIds)
{

	GxsRequest* req = NULL;
	uint32_t reqType = opts.mReqType;

        // remove all empty grpId entries
        GxsMsgReq::const_iterator mit = msgIds.begin();
        std::vector<RsGxsGroupId> toRemove;

        for(; mit != msgIds.end(); ++mit)
        {
            if(mit->second.empty())
                toRemove.push_back(mit->first);
        }

        std::vector<RsGxsGroupId>::const_iterator vit = toRemove.begin();

        GxsMsgReq filteredMsgIds = msgIds;

        for(; vit != toRemove.end(); ++vit)
            filteredMsgIds.erase(*vit);

        if(reqType & GXS_REQUEST_TYPE_MSG_META)
	{
		MsgMetaReq* mmr = new MsgMetaReq();
		mmr->mMsgIds = filteredMsgIds;
		req = mmr;

        }else if(reqType & GXS_REQUEST_TYPE_MSG_DATA)
	{
		MsgDataReq* mdr = new MsgDataReq();
		mdr->mMsgIds = filteredMsgIds;
		req = mdr;

        }else if(reqType & GXS_REQUEST_TYPE_MSG_IDS)
	{
		MsgIdReq* mir = new MsgIdReq();
		mir->mMsgIds = filteredMsgIds;
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
#ifdef DATA_DEBUG
		std::cerr << "RsGxsDataAccess::requestMsgInfo() gets Token: " << token << std::endl;
#endif
	}

	setReq(req, token, ansType, opts);
	storeRequest(req);
	return true;
}

bool RsGxsDataAccess::requestMsgInfo(uint32_t &token, uint32_t ansType,
                   const RsTokReqOptions &opts, const std::list<RsGxsGroupId>& grpIds)
{
        GxsRequest* req = NULL;
        uint32_t reqType = opts.mReqType;

        std::list<RsGxsGroupId>::const_iterator lit = grpIds.begin();

        if(reqType & GXS_REQUEST_TYPE_MSG_META)
        {
                MsgMetaReq* mmr = new MsgMetaReq();

                for(; lit != grpIds.end(); ++lit)
                    mmr->mMsgIds[*lit] = std::set<RsGxsMessageId>();

                req = mmr;
        }
        else if(reqType & GXS_REQUEST_TYPE_MSG_DATA)
        {
                MsgDataReq* mdr = new MsgDataReq();

                for(; lit != grpIds.end(); ++lit)
                    mdr->mMsgIds[*lit] = std::set<RsGxsMessageId>();

                req = mdr;
        }
        else if(reqType & GXS_REQUEST_TYPE_MSG_IDS)
        {
                MsgIdReq* mir = new MsgIdReq();

                for(; lit != grpIds.end(); ++lit)
                    mir->mMsgIds[*lit] = std::set<RsGxsMessageId>();

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
#ifdef DATA_DEBUG
                std::cerr << "RsGxsDataAccess::requestMsgInfo() gets Token: " << token << std::endl;
#endif
        }

        setReq(req, token, ansType, opts);
        storeRequest(req);
        return true;
}


void RsGxsDataAccess::requestServiceStatistic(uint32_t& token)
{
    ServiceStatisticRequest* req = new ServiceStatisticRequest();

    generateToken(token);

    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_SERVICE_STATS;
    setReq(req, token, 0, opts);
    storeRequest(req);
}

void RsGxsDataAccess::requestGroupStatistic(uint32_t& token, const RsGxsGroupId& grpId)
{
    GroupStatisticRequest* req = new GroupStatisticRequest();
    req->mGrpId = grpId;

    generateToken(token);

    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_STATS;
    setReq(req, token, 0, opts);
    storeRequest(req);
}

bool RsGxsDataAccess::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
                                            const std::vector<RsGxsGrpMsgIdPair> &msgIds)
{

    MsgRelatedInfoReq* req = new MsgRelatedInfoReq();
    req->mMsgIds = msgIds;

    generateToken(token);

    setReq(req, token, ansType, opts);
    storeRequest(req);

    return true;
}


void RsGxsDataAccess::setReq(GxsRequest* req, uint32_t token, uint32_t ansType, const RsTokReqOptions& opts) const
{
	req->token = token;
	req->ansType = ansType;
	req->Options = opts;
	return;
}
void RsGxsDataAccess::storeRequest(GxsRequest* req)
{
	RS_STACK_MUTEX(mDataMutex);
	req->status = PENDING;
	req->reqTime = time(NULL);

	mRequestQueue.insert(std::make_pair(req->token,req));
}

RsTokenService::GxsRequestStatus RsGxsDataAccess::requestStatus(uint32_t token)
{
	RsTokenService::GxsRequestStatus status;
	uint32_t reqtype;
	uint32_t anstype;
	rstime_t ts;

	{
		RS_STACK_MUTEX(mDataMutex);

		// first check public tokens
		if(mPublicToken.find(token) != mPublicToken.end())
			return mPublicToken[token];
	}

	if (!checkRequestStatus(token, status, reqtype, anstype, ts))
		return RsTokenService::FAILED;

	return status;
}

bool RsGxsDataAccess::cancelRequest(const uint32_t& token)
{
	RsStackMutex stack(mDataMutex); /****** LOCKED *****/

	GxsRequest* req = locked_retrieveRequest(token);
	if (!req)
	{
		return false;
	}

	req->status = CANCELLED;

	return true;
}

bool RsGxsDataAccess::clearRequest(const uint32_t& token)
{
	RS_STACK_MUTEX(mDataMutex);
    return locked_clearRequest(token);
}

bool RsGxsDataAccess::locked_clearRequest(const uint32_t& token)
{
    auto it = mCompletedRequests.find(token);

    if(it == mCompletedRequests.end())
        return false;

    delete it->second;
    mCompletedRequests.erase(it);

    return true;
}

bool RsGxsDataAccess::getGroupSummary(const uint32_t& token, std::list<const RsGxsGrpMetaData*>& groupInfo)
{
	RS_STACK_MUTEX(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::getGroupSummary() Unable to retrieve "
		          << "group summary" << std::endl;
		return false;
	}

	GroupMetaReq* gmreq = dynamic_cast<GroupMetaReq*>(req);

	if(gmreq)
	{
		groupInfo = gmreq->mGroupMetaData;
		gmreq->mGroupMetaData.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getGroupSummary() Req found, failed"
		          << "cast" << std::endl;
		return false;
	}
	locked_clearRequest(token);

	return true;
}

bool RsGxsDataAccess::getGroupData(const uint32_t& token, std::list<RsNxsGrp*>& grpData)
{
	RS_STACK_MUTEX(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::getGroupData() Unable to retrieve group"
		          << "data" << std::endl;
		return false;
	}

	GroupDataReq* gmreq = dynamic_cast<GroupDataReq*>(req);
	GroupSerializedDataReq* gsreq = dynamic_cast<GroupSerializedDataReq*>(req);

	if(gsreq)
	{
		grpData.swap(gsreq->mGroupData);
		gsreq->mGroupData.clear();
	}
	else if(gmreq)
	{
		grpData.swap(gmreq->mGroupData);
		gmreq->mGroupData.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getGroupData() Req found, failed cast"
		          << " req->reqType: " << req->reqType << std::endl;
		return false;
	}
	locked_clearRequest(token);

	return true;
}

bool RsGxsDataAccess::getMsgData(const uint32_t& token, NxsMsgDataResult& msgData)
{

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
    {

		std::cerr << "RsGxsDataAccess::getMsgData() Unable to retrieve group data" << std::endl;
		return false;
	}

	MsgDataReq* mdreq = dynamic_cast<MsgDataReq*>(req);

	if(mdreq)
	{
		msgData.swap(mdreq->mMsgData);
		mdreq->mMsgData.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getMsgData() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);

	return true;
}

bool RsGxsDataAccess::getMsgRelatedData(const uint32_t &token, NxsMsgRelatedDataResult &msgData)
{

        RsStackMutex stack(mDataMutex);

        GxsRequest* req = locked_retrieveRequest(token);

        if(req == NULL)
        {
			std::cerr << "RsGxsDataAccess::getMsgRelatedData() Unable to retrieve group data" << std::endl;
			return false;
		}
		MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

		if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_DATA)
			return false;

		if(mrireq)
		{
			msgData.swap(mrireq->mMsgDataResult);
			mrireq->mMsgDataResult.clear();
		}
		else
		{
			std::cerr << "RsGxsDataAccess::getMsgRelatedData() Req found, failed caste" << std::endl;
			return false;
		}

		locked_clearRequest(token);

        return true;
}

bool RsGxsDataAccess::getMsgSummary(const uint32_t& token, GxsMsgMetaResult& msgInfo)
{

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
    {
		std::cerr << "RsGxsDataAccess::getMsgSummary() Unable to retrieve group data" << std::endl;
		return false;
	}
	MsgMetaReq* mmreq = dynamic_cast<MsgMetaReq*>(req);

	if(mmreq)
	{
		msgInfo.swap(mmreq->mMsgMetaData);
		mmreq->mMsgMetaData.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getMsgSummary() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
	return true;
}

bool RsGxsDataAccess::getMsgRelatedSummary(const uint32_t &token, MsgRelatedMetaResult &msgMeta)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::getMsgRelatedSummary() Unable to retrieve message summary" << std::endl;
		return false;
	}
	if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_META)
		return false;

	MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

	if(mrireq)
	{
		msgMeta.swap(mrireq->mMsgMetaResult);
		mrireq->mMsgMetaResult.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getMsgRelatedSummary() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
	return true;
}


bool RsGxsDataAccess::getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::getMsgRelatedList() Unable to retrieve message data" << std::endl;
		return false;
	}
	if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_IDS)
		return false;

	MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

	if(mrireq)
	{
		msgIds.swap(mrireq->mMsgIdResult);
		mrireq->mMsgIdResult.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getMsgRelatedList() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
	return true;
}

bool RsGxsDataAccess::getMsgList(const uint32_t& token, GxsMsgIdResult& msgIds)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
    {
		std::cerr << "RsGxsDataAccess::getMsgList() Unable to retrieve msg Ids" << std::endl;
		return false;
	}
	MsgIdReq* mireq = dynamic_cast<MsgIdReq*>(req);

	if(mireq)
	{
		msgIds.swap(mireq->mMsgIdResult);
		mireq->mMsgIdResult.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getMsgList() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
	return true;
}

bool RsGxsDataAccess::getGroupList(const uint32_t& token, std::list<RsGxsGroupId>& groupIds)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL)
    {
		std::cerr << "RsGxsDataAccess::getGroupList() Unable to retrieve group Ids, Request does not exist" << std::endl;
		return false;
	}
	GroupIdReq* gireq = dynamic_cast<GroupIdReq*>(req);

	if(gireq)
	{
		groupIds.swap(gireq->mGroupIdResult);
		gireq->mGroupIdResult.clear();
	}
	else
	{
		std::cerr << "RsGxsDataAccess::getGroupList() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
	return true;
}

bool RsGxsDataAccess::getGroupStatistic(const uint32_t &token, GxsGroupStatistic &grpStatistic)
{
    RsStackMutex stack(mDataMutex);

    GxsRequest* req = locked_retrieveRequest(token);

    if(req == NULL)
	{
		std::cerr << "RsGxsDataAccess::getGroupStatistic() Unable to retrieve grp stats" << std::endl;
		return false;
	}
	GroupStatisticRequest* gsreq = dynamic_cast<GroupStatisticRequest*>(req);

	if(gsreq)
		grpStatistic = gsreq->mGroupStatistic;
	else
	{
		std::cerr << "RsGxsDataAccess::getGroupStatistic() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
    return true;
}

bool RsGxsDataAccess::getServiceStatistic(const uint32_t &token, GxsServiceStatistic &servStatistic)
{
    RsStackMutex stack(mDataMutex);

    GxsRequest* req = locked_retrieveRequest(token);

    if(req == NULL)
    {
		std::cerr << "RsGxsDataAccess::getServiceStatistic() Unable to retrieve service stats" << std::endl;
		return false;
	}
	ServiceStatisticRequest* ssreq = dynamic_cast<ServiceStatisticRequest*>(req);

	if(ssreq)
		servStatistic = ssreq->mServiceStatistic;
	else
	{
		std::cerr << "RsGxsDataAccess::getServiceStatistic() Req found, failed caste" << std::endl;
		return false;
	}
	locked_clearRequest(token);
    return true;
}
GxsRequest* RsGxsDataAccess::locked_retrieveRequest(const uint32_t& token)
{
    auto it = mCompletedRequests.find(token) ;

    if(it == mCompletedRequests.end())
        return NULL;

	return it->second;
}

#define MAX_REQUEST_AGE 120 // 2 minutes

void RsGxsDataAccess::processRequests()
{
	// process requests

	while (!mRequestQueue.empty())
	{
        // Extract the first elements from the request queue. cleanup all other elements marked at terminated.

		GxsRequest* req = nullptr;
		{
			RsStackMutex stack(mDataMutex); /******* LOCKED *******/
            rstime_t now = time(NULL); // this is ok while in the loop below

            while(!mRequestQueue.empty() && req == nullptr)
            {
                if(now > mRequestQueue.begin()->second->reqTime + MAX_REQUEST_AGE)
                {
					mRequestQueue.erase(mRequestQueue.begin());
					continue;
                }

                switch( mRequestQueue.begin()->second->status )
                {
                case PARTIAL:
                case COMPLETE:
                case DONE:
                    RsErr() << "Found partial/done/complete request in mRequestQueue. This is a bug." << std::endl;	// fallthrough
                case FAILED:
                case CANCELLED:
                    		mRequestQueue.erase(mRequestQueue.begin());
                    		continue;
                    break;
                case PENDING:
                    req = mRequestQueue.begin()->second;
					req->status = PARTIAL;
                    break;
                }

            }
        }

		if (!req)
			break;

		GroupMetaReq* gmr;
		GroupDataReq* gdr;
		GroupIdReq* gir;

		MsgMetaReq* mmr;
		MsgDataReq* mdr;
		MsgIdReq* mir;
		MsgRelatedInfoReq* mri;
		GroupStatisticRequest* gsr;
		GroupSerializedDataReq* grr;
		ServiceStatisticRequest* ssr;

#ifdef DATA_DEBUG
		std::cerr << "RsGxsDataAccess::processRequests() Processing Token: " << req->token << " Status: "
		          << req->status << " ReqType: " << req->reqType << " Age: "
		          << time(NULL) - req->reqTime << std::endl;
#endif

		/* PROCESS REQUEST! */
		bool ok = false;

		if((gmr = dynamic_cast<GroupMetaReq*>(req)) != NULL)
		{
			ok = getGroupSummary(gmr);
		}
		else if((gdr = dynamic_cast<GroupDataReq*>(req)) != NULL)
		{
			ok = getGroupData(gdr);
		}
		else if((gir = dynamic_cast<GroupIdReq*>(req)) != NULL)
		{
			ok = getGroupList(gir);
		}
		else if((mmr = dynamic_cast<MsgMetaReq*>(req)) != NULL)
		{
			ok = getMsgSummary(mmr);
		}
		else if((mdr = dynamic_cast<MsgDataReq*>(req)) != NULL)
		{
			ok = getMsgData(mdr);
		}
		else if((mir = dynamic_cast<MsgIdReq*>(req)) != NULL)
		{
			ok = getMsgList(mir);
		}
		else if((mri = dynamic_cast<MsgRelatedInfoReq*>(req)) != NULL)
		{
			ok = getMsgRelatedInfo(mri);
		}
		else if((gsr = dynamic_cast<GroupStatisticRequest*>(req)) != NULL)
		{
			ok = getGroupStatistic(gsr);
		}
		else if((ssr = dynamic_cast<ServiceStatisticRequest*>(req)) != NULL)
		{
			ok = getServiceStatistic(ssr);
		}
		else if((grr = dynamic_cast<GroupSerializedDataReq*>(req)) != NULL)
		{
			ok = getGroupSerializedData(grr);
		}

		else
		{
			std::cerr << "RsGxsDataAccess::processRequests() Failed to process request, token: "
			          << req->token << std::endl;
		}

        // We cannot easily remove the request here because the queue may have more elements now and mRequestQueue.begin() is not necessarily the same element.
        // but we mark it as COMPLETE/FAILED so that it will be removed in the next loop.
		{
			RsStackMutex stack(mDataMutex); /******* LOCKED *******/

            if(ok)
            {
                // When the request is complete, we move it to the complete list, so that the caller can easily retrieve the request data

				req->status = COMPLETE ;
                mCompletedRequests[req->token] = req;
            }
            else
				req->status = FAILED;
        }

	} // END OF MUTEX.
}



bool RsGxsDataAccess::getGroupSerializedData(GroupSerializedDataReq* req)
{
	std::map<RsGxsGroupId, RsNxsGrp*> grpData;
	std::list<RsGxsGroupId> grpIdsOut;

	getGroupList(req->mGroupIds, req->Options, grpIdsOut);

	if(grpIdsOut.empty())
		return true;


	for(std::list<RsGxsGroupId>::iterator lit = grpIdsOut.begin();lit != grpIdsOut.end();++lit)
		grpData[*lit] = NULL;

	bool ok = mDataStore->retrieveNxsGrps(grpData, true, true);
    req->mGroupData.clear();

	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grpData.begin();

	for(; mit != grpData.end(); ++mit)
        req->mGroupData.push_back(mit->second) ;

	return ok;
}
bool RsGxsDataAccess::getGroupData(GroupDataReq* req)
{
	std::map<RsGxsGroupId, RsNxsGrp*> grpData;
        std::list<RsGxsGroupId> grpIdsOut;

        getGroupList(req->mGroupIds, req->Options, grpIdsOut);

        if(grpIdsOut.empty())
            return true;

        std::list<RsGxsGroupId>::iterator lit = grpIdsOut.begin(),
        lit_end = grpIdsOut.end();

        for(; lit != lit_end; ++lit)
        {
            grpData[*lit] = NULL;
        }

        bool ok = mDataStore->retrieveNxsGrps(grpData, true, true);

	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grpData.begin();
	for(; mit != grpData.end(); ++mit)
		req->mGroupData.push_back(mit->second);

        return ok;
}

bool RsGxsDataAccess::getGroupSummary(GroupMetaReq* req)
{

	RsGxsGrpMetaTemporaryMap grpMeta;
	std::list<RsGxsGroupId> grpIdsOut;

	getGroupList(req->mGroupIds, req->Options, grpIdsOut);

	if(grpIdsOut.empty())
		return true;

	std::list<RsGxsGroupId>::const_iterator lit = grpIdsOut.begin();

	for(; lit != grpIdsOut.end(); ++lit)
		grpMeta[*lit] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

	for(; mit != grpMeta.end(); ++mit)
		req->mGroupMetaData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupList(GroupIdReq* req)
{ return getGroupList(req->mGroupIds, req->Options, req->mGroupIdResult); }

bool RsGxsDataAccess::getGroupList(const std::list<RsGxsGroupId>& grpIdsIn, const RsTokReqOptions& opts, std::list<RsGxsGroupId>& grpIdsOut)
{
	RsGxsGrpMetaTemporaryMap grpMeta;

    for(auto lit = grpIdsIn.begin(); lit != grpIdsIn.end(); ++lit)
		grpMeta[*lit] = nullptr;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    for(auto mit = grpMeta.begin() ; mit != grpMeta.end(); ++mit)
		grpIdsOut.push_back(mit->first);

    filterGrpList(grpIdsOut, opts, grpMeta);

    return true;
}

bool RsGxsDataAccess::getMsgData(MsgDataReq* req)
{
	GxsMsgReq msgIdOut;

	const RsTokReqOptions& opts(req->Options);

	// filter based on options
	getMsgList(req->mMsgIds, opts, msgIdOut);

	// If the list is empty because of filtering do not retrieve from DB
	if((opts.mMsgFlagMask || opts.mStatusMask) && msgIdOut.empty())
		return true;

	mDataStore->retrieveNxsMsgs(msgIdOut, req->mMsgData, true, true);
	return true;
}


bool RsGxsDataAccess::getMsgSummary(MsgMetaReq* req)
{
	GxsMsgReq msgIdOut;

	const RsTokReqOptions& opts(req->Options);

	// filter based on options
	getMsgList(req->mMsgIds, opts, msgIdOut);

	// If the list is empty because of filtering do not retrieve from DB
	if((opts.mMsgFlagMask || opts.mStatusMask) && msgIdOut.empty())
		return true;

	mDataStore->retrieveGxsMsgMetaData(msgIdOut, req->mMsgMetaData);

	return true;
}


bool RsGxsDataAccess::getMsgList(
        const GxsMsgReq& msgIds, const RsTokReqOptions& opts,
        GxsMsgReq& msgIdsOut )
{
    GxsMsgMetaResult result;

    mDataStore->retrieveGxsMsgMetaData(msgIds, result);

    /* CASEs this handles.
     * Input is groupList + Flags.
     * 1) No Flags => All Messages in those Groups.
     *
     */
#ifdef DATA_DEBUG
    std::cerr << "RsGxsDataAccess::getMsgList()";
    std::cerr << std::endl;
#endif

    bool onlyOrigMsgs = false;
    bool onlyLatestMsgs = false;
    bool onlyThreadHeadMsgs = false;

    // Can only choose one of these two.
    if (opts.mOptions & RS_TOKREQOPT_MSG_ORIGMSG)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_ORIGMSG";
            std::cerr << std::endl;
#endif
            onlyOrigMsgs = true;
    }
    else if (opts.mOptions & RS_TOKREQOPT_MSG_LATEST)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_LATEST";
            std::cerr << std::endl;
#endif
            onlyLatestMsgs = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_THREAD)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_THREAD";
            std::cerr << std::endl;
#endif
            onlyThreadHeadMsgs = true;
    }

    GxsMsgMetaResult::iterator meta_it;
    MsgMetaFilter metaFilter;

    for(meta_it = result.begin(); meta_it != result.end(); ++meta_it)
    {
            const RsGxsGroupId& grpId = meta_it->first;

            metaFilter[grpId] = std::map<RsGxsMessageId, RsGxsMsgMetaData*>();

            const std::vector<RsGxsMsgMetaData*>& metaV = meta_it->second;
            if (onlyLatestMsgs) // THIS ONE IS HARD -> LOTS OF COMP.
            {
                    std::vector<RsGxsMsgMetaData*>::const_iterator vit = metaV.begin();

                    // RUN THROUGH ALL MSGS... in map origId -> TS.
                    std::map<RsGxsMessageId, std::pair<RsGxsMessageId, rstime_t> > origMsgTs;
                    std::map<RsGxsMessageId, std::pair<RsGxsMessageId, rstime_t> >::iterator oit;

                    for(; vit != metaV.end(); ++vit)
                    {
                        RsGxsMsgMetaData* msgMeta = *vit;

                        /* if we are grabbing thread Head... then parentId == empty. */
                        if (onlyThreadHeadMsgs)
                        {
                            if (!(msgMeta->mParentId.isNull()))
                            {
                                    continue;
                            }
                        }


                        oit = origMsgTs.find(msgMeta->mOrigMsgId);
                        bool addMsg = false;
                        if (oit == origMsgTs.end())
                        {
#ifdef DATA_DEBUG
                            std::cerr << "RsGxsDataAccess::getMsgList() Found New OrigMsgId: ";
                            std::cerr << msgMeta->mOrigMsgId;
                            std::cerr << " MsgId: " << msgMeta->mMsgId;
                            std::cerr << " TS: " << msgMeta->mPublishTs;
                            std::cerr << std::endl;
#endif

                            addMsg = true;
                        }
                        // check timestamps.
                        else if (oit->second.second < msgMeta->mPublishTs)
                        {
#ifdef DATA_DEBUG
                            std::cerr << "RsGxsDataAccess::getMsgList() Found Later Msg. OrigMsgId: ";
                            std::cerr << msgMeta->mOrigMsgId;
                            std::cerr << " MsgId: " << msgMeta->mMsgId;
                            std::cerr << " TS: " << msgMeta->mPublishTs;
#endif

                            addMsg = true;
                        }

                        if (addMsg)
                        {
                            // add as latest. (overwriting if necessary)
                            origMsgTs[msgMeta->mOrigMsgId] = std::make_pair(msgMeta->mMsgId, msgMeta->mPublishTs);
                            metaFilter[grpId].insert(std::make_pair(msgMeta->mMsgId, msgMeta));
                        }
                    }

                    // Add the discovered Latest Msgs.
                    for(oit = origMsgTs.begin(); oit != origMsgTs.end(); ++oit)
                    {
                            msgIdsOut[grpId].insert(oit->second.first);
                    }

            }
            else	// ALL OTHER CASES.
            {
                std::vector<RsGxsMsgMetaData*>::const_iterator vit = metaV.begin();

                for(; vit != metaV.end(); ++vit)
                {
                    RsGxsMsgMetaData* msgMeta = *vit;
                    bool add = false;

                    /* if we are grabbing thread Head... then parentId == empty. */
                    if (onlyThreadHeadMsgs)
                    {
                            if (!(msgMeta->mParentId.isNull()))
                            {
                                    continue;
                            }
                    }


                    if (onlyOrigMsgs)
                    {
                            if (msgMeta->mMsgId == msgMeta->mOrigMsgId)
                            {
                                add = true;
                            }
                    }
                    else
                    {
                        add = true;
                    }

                    if (add)
                    {
                        msgIdsOut[grpId].insert(msgMeta->mMsgId);
                        metaFilter[grpId].insert(std::make_pair(msgMeta->mMsgId, msgMeta));
                    }

                }
            }
    }

    filterMsgList(msgIdsOut, opts, metaFilter);

    metaFilter.clear();

    // delete meta data
    cleanseMsgMetaMap(result);

    return true;
}

bool RsGxsDataAccess::getMsgRelatedInfo(MsgRelatedInfoReq *req)
{
    /* CASEs this handles.
     * Input is msgList + Flags.
     * 1) No Flags => return nothing
     */
#ifdef DATA_DEBUG
    std::cerr << "RsGxsDataAccess::getMsgRelatedList()";
    std::cerr << std::endl;
#endif

    const RsTokReqOptions& opts = req->Options;

    bool onlyLatestMsgs = false;
    bool onlyAllVersions = false;
    bool onlyChildMsgs = false;
    bool onlyThreadMsgs = false;

    if (opts.mOptions & RS_TOKREQOPT_MSG_LATEST)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_LATEST";
            std::cerr << std::endl;
#endif
            onlyLatestMsgs = true;
    }
    else if (opts.mOptions & RS_TOKREQOPT_MSG_VERSIONS)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_VERSIONS";
            std::cerr << std::endl;
#endif
            onlyAllVersions = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_PARENT)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_PARENTS";
            std::cerr << std::endl;
#endif
            onlyChildMsgs = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_THREAD)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_THREAD";
            std::cerr << std::endl;
#endif
            onlyThreadMsgs = true;
    }

    if (onlyAllVersions && onlyChildMsgs)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (VERSIONS & PARENT)";
            std::cerr << std::endl;
#endif

            return false;
    }

    if (onlyAllVersions && onlyThreadMsgs)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (VERSIONS & THREAD)";
            std::cerr << std::endl;
#endif

            return false;
    }

    if ((!onlyLatestMsgs) && onlyChildMsgs)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (!LATEST & PARENT)";
            std::cerr << std::endl;
#endif

            return false;
    }

    if ((!onlyLatestMsgs) && onlyThreadMsgs)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (!LATEST & THREAD)";
            std::cerr << std::endl;
#endif

            return false;
    }

    if (onlyChildMsgs && onlyThreadMsgs)
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (PARENT & THREAD)";
            std::cerr << std::endl;
#endif

            return false;
    }


    /* FALL BACK OPTION */
    if ((!onlyLatestMsgs) && (!onlyAllVersions) && (!onlyChildMsgs) && (!onlyThreadMsgs))
    {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() FALLBACK -> NO FLAGS -> SIMPLY RETURN nothing";
            std::cerr << std::endl;
#endif

            return true;
    }

    std::vector<RsGxsGrpMsgIdPair>::iterator vit_msgIds = req->mMsgIds.begin();

    for(; vit_msgIds != req->mMsgIds.end(); ++vit_msgIds)
    {
        MsgMetaFilter filterMap;


        const RsGxsGrpMsgIdPair& grpMsgIdPair = *vit_msgIds;

        // get meta data for all in group
        GxsMsgMetaResult result;
        GxsMsgReq msgIds;
        msgIds.insert(std::make_pair(grpMsgIdPair.first, std::set<RsGxsMessageId>()));
        mDataStore->retrieveGxsMsgMetaData(msgIds, result);
        std::vector<RsGxsMsgMetaData*>& metaV = result[grpMsgIdPair.first];
        std::vector<RsGxsMsgMetaData*>::iterator vit_meta;

        // msg id to relate to
        const RsGxsMessageId& msgId = grpMsgIdPair.second;
        const RsGxsGroupId& grpId = grpMsgIdPair.first;

        std::set<RsGxsMessageId> outMsgIds;

        RsGxsMsgMetaData* origMeta = NULL;
        for(vit_meta = metaV.begin(); vit_meta != metaV.end(); ++vit_meta)
        {
            RsGxsMsgMetaData* meta = *vit_meta;

            if(msgId == meta->mMsgId)
            {
                origMeta = meta;
                break;
            }
        }

        if(!origMeta)
        {
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::getMsgRelatedInfo(): Cannot find meta of msgId (to relate to)!"
                      << std::endl;
#endif
            cleanseMsgMetaMap(result);
            return false;
        }

        const RsGxsMessageId& origMsgId = origMeta->mOrigMsgId;
        std::map<RsGxsMessageId, RsGxsMsgMetaData*>& metaMap = filterMap[grpId];

        if (onlyLatestMsgs)
        {
            if (onlyChildMsgs || onlyThreadMsgs)
            {
                // RUN THROUGH ALL MSGS... in map origId -> TS.
                std::map<RsGxsMessageId, std::pair<RsGxsMessageId, rstime_t> > origMsgTs;
                std::map<RsGxsMessageId, std::pair<RsGxsMessageId, rstime_t> >::iterator oit;
                for(vit_meta = metaV.begin(); vit_meta != metaV.end(); ++vit_meta)
                {

                    RsGxsMsgMetaData* meta = *vit_meta;

                    // skip msgs that aren't children.
                    if (onlyChildMsgs)
                    {
                        if (meta->mParentId != origMsgId)
                        {
                            continue;
                        }
                    }
                    else /* onlyThreadMsgs */
                    {
                        if (meta->mThreadId != msgId)
                        {
                            continue;
                        }
                    }


                    oit = origMsgTs.find(meta->mOrigMsgId);

                    bool addMsg = false;
                    if (oit == origMsgTs.end())
                    {
#ifdef DATA_DEBUG
                            std::cerr << "RsGxsDataAccess::getMsgRelatedList() Found New OrigMsgId: ";
                            std::cerr << meta->mOrigMsgId;
                            std::cerr << " MsgId: " << meta->mMsgId;
                            std::cerr << " TS: " << meta->mPublishTs;
                            std::cerr << std::endl;
#endif

                            addMsg = true;
                    }
                    // check timestamps.
                    else if (oit->second.second < meta->mPublishTs)
                    {
#ifdef DATA_DEBUG
                            std::cerr << "RsGxsDataAccess::getMsgRelatedList() Found Later Msg. OrigMsgId: ";
                            std::cerr << meta->mOrigMsgId;
                            std::cerr << " MsgId: " << meta->mMsgId;
                            std::cerr << " TS: " << meta->mPublishTs;
#endif

                            addMsg = true;
                    }

                    if (addMsg)
                    {
                        // add as latest. (overwriting if necessary)
                        origMsgTs[meta->mOrigMsgId] = std::make_pair(meta->mMsgId, meta->mPublishTs);
                        metaMap.insert(std::make_pair(meta->mMsgId, meta));
                    }
                }

                // Add the discovered Latest Msgs.
                for(oit = origMsgTs.begin(); oit != origMsgTs.end(); ++oit)
                {
                    outMsgIds.insert(oit->second.first);
                }
            }
            else
            {

                /* first guess is potentially better than Orig (can't be worse!) */
                rstime_t latestTs = 0;
                RsGxsMessageId latestMsgId;
                RsGxsMsgMetaData* latestMeta=NULL;

                for(vit_meta = metaV.begin(); vit_meta != metaV.end(); ++vit_meta)
                {
                    RsGxsMsgMetaData* meta = *vit_meta;

                    if (meta->mOrigMsgId == origMsgId)
                    {
                        if (meta->mPublishTs > latestTs)
                        {
                            latestTs = meta->mPublishTs;
                            latestMsgId = meta->mMsgId;
                            latestMeta = meta;
                        }
                    }
                }
                outMsgIds.insert(latestMsgId);
                metaMap.insert(std::make_pair(latestMsgId, latestMeta));
            }
        }
        else if (onlyAllVersions)
        {
            for(vit_meta = metaV.begin(); vit_meta != metaV.end(); ++vit_meta)
            {
                RsGxsMsgMetaData* meta = *vit_meta;

                if (meta->mOrigMsgId == origMsgId)
                {
                    outMsgIds.insert(meta->mMsgId);
                    metaMap.insert(std::make_pair(meta->mMsgId, meta));
                }
            }
        }

        GxsMsgIdResult filteredOutMsgIds;
        filteredOutMsgIds[grpId] = outMsgIds;
        filterMsgList(filteredOutMsgIds, opts, filterMap);

        if(!filteredOutMsgIds[grpId].empty())
        {
            if(req->Options.mReqType == GXS_REQUEST_TYPE_MSG_RELATED_IDS)
            {
                req->mMsgIdResult[grpMsgIdPair] = filteredOutMsgIds[grpId];
            }
            else if(req->Options.mReqType == GXS_REQUEST_TYPE_MSG_RELATED_META)
            {
                GxsMsgMetaResult metaResult;
                mDataStore->retrieveGxsMsgMetaData(filteredOutMsgIds, metaResult);
                req->mMsgMetaResult[grpMsgIdPair] = metaResult[grpId];
            }
            else if(req->Options.mReqType == GXS_REQUEST_TYPE_MSG_RELATED_DATA)
            {
                GxsMsgResult msgResult;
                mDataStore->retrieveNxsMsgs(filteredOutMsgIds, msgResult, false, true);
                req->mMsgDataResult[grpMsgIdPair] = msgResult[grpId];
            }
        }

        outMsgIds.clear();
        filteredOutMsgIds.clear();

        cleanseMsgMetaMap(result);
    }
    return true;
}

bool RsGxsDataAccess::getGroupStatistic(GroupStatisticRequest *req)
{
    // filter based on options
    GxsMsgIdResult metaReq;
    metaReq[req->mGrpId] = std::set<RsGxsMessageId>();
    GxsMsgMetaResult metaResult;
    mDataStore->retrieveGxsMsgMetaData(metaReq, metaResult);

    std::vector<RsGxsMsgMetaData*>& msgMetaV = metaResult[req->mGrpId];

    req->mGroupStatistic.mGrpId = req->mGrpId;
    req->mGroupStatistic.mNumMsgs = msgMetaV.size();
    req->mGroupStatistic.mTotalSizeOfMsgs = 0;
    req->mGroupStatistic.mNumThreadMsgsNew = 0;
    req->mGroupStatistic.mNumThreadMsgsUnread = 0;
    req->mGroupStatistic.mNumChildMsgsNew = 0;
    req->mGroupStatistic.mNumChildMsgsUnread = 0;

    std::set<RsGxsMessageId> obsolete_msgs ;	// stored message ids that are referred to as older versions of an existing message

    for(uint32_t i = 0; i < msgMetaV.size(); ++i)
        if(!msgMetaV[i]->mOrigMsgId.isNull() && msgMetaV[i]->mOrigMsgId!=msgMetaV[i]->mMsgId)
            obsolete_msgs.insert(msgMetaV[i]->mOrigMsgId);

    for(uint32_t i = 0; i < msgMetaV.size(); ++i)
    {
        RsGxsMsgMetaData* m = msgMetaV[i];
        req->mGroupStatistic.mTotalSizeOfMsgs += m->mMsgSize + m->serial_size();

        if(obsolete_msgs.find(m->mMsgId) != obsolete_msgs.end()) 	// skip obsolete messages.
            continue;

        if (IS_MSG_NEW(m->mMsgStatus))
        {
            if (m->mParentId.isNull())
            {
                ++req->mGroupStatistic.mNumThreadMsgsNew;
            } else {
                ++req->mGroupStatistic.mNumChildMsgsNew;
            }
        }
        if (IS_MSG_UNREAD(m->mMsgStatus))
        {
            if (m->mParentId.isNull())
            {
                ++req->mGroupStatistic.mNumThreadMsgsUnread;
            } else {
                ++req->mGroupStatistic.mNumChildMsgsUnread;
            }
        }
    }

    cleanseMsgMetaMap(metaResult);
    return true;
}

// potentially very expensive!
bool RsGxsDataAccess::getServiceStatistic(ServiceStatisticRequest *req)
{
	RsGxsGrpMetaTemporaryMap grpMeta ;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    req->mServiceStatistic.mNumGrps = grpMeta.size();
    req->mServiceStatistic.mNumMsgs = 0;
    req->mServiceStatistic.mSizeOfGrps = 0;
    req->mServiceStatistic.mSizeOfMsgs = 0;
    req->mServiceStatistic.mNumGrpsSubscribed = 0;
    req->mServiceStatistic.mNumThreadMsgsNew = 0;
    req->mServiceStatistic.mNumThreadMsgsUnread = 0;
    req->mServiceStatistic.mNumChildMsgsNew = 0;
    req->mServiceStatistic.mNumChildMsgsUnread = 0;

    for(auto mit = grpMeta.begin(); mit != grpMeta.end(); ++mit)
    {
        const RsGxsGrpMetaData* m = mit->second;
        req->mServiceStatistic.mSizeOfGrps += m->mGrpSize + m->serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);

        if (IS_GROUP_SUBSCRIBED(m->mSubscribeFlags))
        {
            ++req->mServiceStatistic.mNumGrpsSubscribed;

            GroupStatisticRequest gr;
            gr.mGrpId = m->mGroupId;
            getGroupStatistic(&gr);

            req->mServiceStatistic.mNumMsgs += gr.mGroupStatistic.mNumMsgs;
            req->mServiceStatistic.mSizeOfMsgs += gr.mGroupStatistic.mTotalSizeOfMsgs;
            req->mServiceStatistic.mNumThreadMsgsNew += gr.mGroupStatistic.mNumThreadMsgsNew;
            req->mServiceStatistic.mNumThreadMsgsUnread += gr.mGroupStatistic.mNumThreadMsgsUnread;
            req->mServiceStatistic.mNumChildMsgsNew += gr.mGroupStatistic.mNumChildMsgsNew;
            req->mServiceStatistic.mNumChildMsgsUnread += gr.mGroupStatistic.mNumChildMsgsUnread;
        }
    }

    req->mServiceStatistic.mSizeStore = req->mServiceStatistic.mSizeOfGrps + req->mServiceStatistic.mSizeOfMsgs;

    return true;
}

bool RsGxsDataAccess::getMsgList(MsgIdReq* req)
{

    GxsMsgMetaResult result;

    mDataStore->retrieveGxsMsgMetaData(req->mMsgIds, result);


    GxsMsgMetaResult::iterator mit = result.begin(), mit_end = result.end();

    for(; mit != mit_end; ++mit)
    {
        const RsGxsGroupId grpId = mit->first;
        std::vector<RsGxsMsgMetaData*>& metaV = mit->second;
        std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin(),
        vit_end = metaV.end();

        for(; vit != vit_end; ++vit)
        {
            RsGxsMsgMetaData* meta = *vit;
            req->mMsgIdResult[grpId].insert(meta->mMsgId);
            delete meta; // discard meta data mem
        }
    }

    GxsMsgReq msgIdOut;

    // filter based on options
    getMsgList(req->mMsgIdResult, req->Options, msgIdOut);
    req->mMsgIdResult = msgIdOut;

    return true;
}

void RsGxsDataAccess::cleanseMsgMetaMap(GxsMsgMetaResult& result)
{
    GxsMsgMetaResult::iterator mit = result.begin();

        for(; mit !=result.end(); ++mit)
	{

            std::vector<RsGxsMsgMetaData*>& msgMetaV = mit->second;
            std::vector<RsGxsMsgMetaData*>::iterator vit = msgMetaV.begin();
                for(; vit != msgMetaV.end(); ++vit)
		{
                        delete *vit;
		}
	}

        result.clear();
	return;
}

void RsGxsDataAccess::filterMsgList(
        GxsMsgIdResult& resultsMap, const RsTokReqOptions& opts,
        const MsgMetaFilter& msgMetas ) const
{
	for( GxsMsgIdResult::iterator grpIt = resultsMap.begin();
	     grpIt != resultsMap.end(); ++grpIt )
	{
		const RsGxsGroupId& groupId(grpIt->first);
		std::set<RsGxsMessageId>& msgsIdSet(grpIt->second);

		MsgMetaFilter::const_iterator cit = msgMetas.find(groupId);
		if(cit == msgMetas.end()) continue;
#ifdef DATA_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " " << msgsIdSet.size()
		          << " for group: " << groupId << " before filtering"
		          << std::endl;
#endif

		for( std::set<RsGxsMessageId>::iterator msgIdIt = msgsIdSet.begin();
		     msgIdIt != msgsIdSet.end(); )
		{
			const RsGxsMessageId& msgId(*msgIdIt);
			const std::map<RsGxsMessageId, RsGxsMsgMetaData*>& msgsMetaMap =
			        cit->second;

			bool keep = false;
			std::map<RsGxsMessageId, RsGxsMsgMetaData*>::const_iterator msgsMetaMapIt;

			if( (msgsMetaMapIt = msgsMetaMap.find(msgId)) != msgsMetaMap.end() )
			{
				keep = checkMsgFilter(opts, msgsMetaMapIt->second);
			}

			if(keep) ++msgIdIt;
			else msgIdIt = msgsIdSet.erase(msgIdIt);
		}

#ifdef DATA_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " " << msgsIdSet.size()
		          << " for group: " << groupId << " after filtering"
		          << std::endl;
#endif
	}
}

void RsGxsDataAccess::filterGrpList(std::list<RsGxsGroupId> &grpIds, const RsTokReqOptions &opts, const GrpMetaFilter &meta) const
{
    std::list<RsGxsGroupId>::iterator lit = grpIds.begin();

    for(; lit != grpIds.end(); )
    {
        GrpMetaFilter::const_iterator cit = meta.find(*lit);

        bool keep = false;

        if(cit != meta.end())
        {
           keep = checkGrpFilter(opts, cit->second);
        }

        if(keep)
        {
            ++lit;
        }else
        {
            lit = grpIds.erase(lit);
        }

    }
}


bool RsGxsDataAccess::checkRequestStatus(
        uint32_t token, GxsRequestStatus& status, uint32_t& reqtype,
        uint32_t& anstype, rstime_t& ts )
{
	RS_STACK_MUTEX(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if (req == NULL || req->status == CANCELLED)
		return false;

	anstype = req->ansType;
	reqtype = req->reqType;
	status = req->status;
	ts = req->reqTime;

	return true;
}

bool RsGxsDataAccess::addGroupData(RsNxsGrp* grp) {

	RsStackMutex stack(mDataMutex);

	std::list<RsNxsGrp*> grpM;
	grpM.push_back(grp);
	return mDataStore->storeGroup(grpM);
}

bool RsGxsDataAccess::updateGroupData(RsNxsGrp* grp) {

	RsStackMutex stack(mDataMutex);

	std::list<RsNxsGrp*> grpM;
	grpM.push_back(grp);
	return mDataStore->updateGroup(grpM);
}

bool RsGxsDataAccess::getGroupData(const RsGxsGroupId& grpId, RsNxsGrp *& grp_data)
{
	RsStackMutex stack(mDataMutex);

    std::map<RsGxsGroupId, RsNxsGrp*> grps ;

    grps[grpId] = NULL ;

    if(mDataStore->retrieveNxsGrps(grps, false, true))	// the false here is very important: it removes the private key parts.
    {
        grp_data = grps.begin()->second;
        return true;
    }
    else
        return false ;
}

bool RsGxsDataAccess::addMsgData(RsNxsMsg* msg) {

	RsStackMutex stack(mDataMutex);

	std::list<RsNxsMsg*> msgM;
	msgM.push_back(msg);
	return mDataStore->storeMessage(msgM);
}



void RsGxsDataAccess::tokenList(std::list<uint32_t>& tokens)
{

	RsStackMutex stack(mDataMutex);

	for(auto& it:mRequestQueue)
		tokens.push_back(it.second->token);

	for(auto& it:mCompletedRequests)
        tokens.push_back(it.first);
}

bool RsGxsDataAccess::locked_updateRequestStatus( uint32_t token, RsTokenService::GxsRequestStatus status )
{
	GxsRequest* req = locked_retrieveRequest(token);

	if(req) req->status = status;
	else return false;

	return true;
}

uint32_t RsGxsDataAccess::generatePublicToken()
{
	uint32_t token;
	generateToken(token);

	{
		RS_STACK_MUTEX(mDataMutex);
		mPublicToken[token] = RsTokenService::PENDING;
	}

	return token;
}



bool RsGxsDataAccess::updatePublicRequestStatus(
        uint32_t token, RsTokenService::GxsRequestStatus status )
{
	RS_STACK_MUTEX(mDataMutex);
	std::map<uint32_t, RsTokenService::GxsRequestStatus>::iterator mit =
	        mPublicToken.find(token);
	if(mit != mPublicToken.end()) mit->second = status;
	else return false;
	return true;
}



bool RsGxsDataAccess::disposeOfPublicToken(uint32_t token)
{
	RS_STACK_MUTEX(mDataMutex);
	std::map<uint32_t, RsTokenService::GxsRequestStatus>::iterator mit =
	        mPublicToken.find(token);
	if(mit != mPublicToken.end()) mPublicToken.erase(mit);
	else return false;
	return true;
}

bool RsGxsDataAccess::checkGrpFilter(const RsTokReqOptions &opts, const RsGxsGrpMetaData *meta) const
{

    bool subscribeMatch = false;

    if(opts.mSubscribeMask)
    {
        // Exact Flags match required.
        if ((opts.mSubscribeMask & opts.mSubscribeFilter) == (opts.mSubscribeMask & meta->mSubscribeFlags))
        {
                subscribeMatch = true;
        }
    }
    else
    {
        subscribeMatch = true;
    }

    return subscribeMatch;
}
bool RsGxsDataAccess::checkMsgFilter(
        const RsTokReqOptions& opts, const RsGxsMsgMetaData* meta ) const
{
	if (opts.mStatusMask)
	{
		// Exact Flags match required.
		if ( (opts.mStatusMask & opts.mStatusFilter) ==
		     (opts.mStatusMask & meta->mMsgStatus) )
		{
#ifdef DATA_DEBUG
			std::cerr << __PRETTY_FUNCTION__
			          << " Continue checking Msg as StatusMatches: "
			          << " Mask: " << opts.mStatusMask
			          << " StatusFilter: " << opts.mStatusFilter
			          << " MsgStatus: " << meta->mMsgStatus
			          << " MsgId: " << meta->mMsgId << std::endl;
#endif
		}
		else
		{
#ifdef DATA_DEBUG
			std::cerr << __PRETTY_FUNCTION__
			          << " Dropping Msg due to !StatusMatch "
			          << " Mask: " << opts.mStatusMask
			          << " StatusFilter: " << opts.mStatusFilter
			          << " MsgStatus: " << meta->mMsgStatus
			          << " MsgId: " << meta->mMsgId << std::endl;
#endif

			return false;
		}
	}
	else
	{
#ifdef DATA_DEBUG
		std::cerr << __PRETTY_FUNCTION__
		          << " Status check not requested"
		          << " mStatusMask: " << opts.mStatusMask
		          << " MsgId: " << meta->mMsgId << std::endl;
#endif
	}

	if(opts.mMsgFlagMask)
	{
		// Exact Flags match required.
		if ( (opts.mMsgFlagMask & opts.mMsgFlagFilter) ==
		     (opts.mMsgFlagMask & meta->mMsgFlags) )
		{
#ifdef DATA_DEBUG
			std::cerr << __PRETTY_FUNCTION__
			          << " Accepting Msg as FlagMatches: "
			          << " Mask: " << opts.mMsgFlagMask
			          << " FlagFilter: " << opts.mMsgFlagFilter
			          << " MsgFlag: " << meta->mMsgFlags
			          << " MsgId: " << meta->mMsgId << std::endl;
#endif
		}
		else
		{
#ifdef DATA_DEBUG
			std::cerr << __PRETTY_FUNCTION__
			          << " Dropping Msg due to !FlagMatch "
			          << " Mask: " << opts.mMsgFlagMask
			          << " FlagFilter: " << opts.mMsgFlagFilter
			          << " MsgFlag: " << meta->mMsgFlags
			          << " MsgId: " << meta->mMsgId << std::endl;
#endif

			return false;
		}
	}
	else
	{
#ifdef DATA_DEBUG
		std::cerr << __PRETTY_FUNCTION__
		          << " Flags check not requested"
		          << " mMsgFlagMask: " << opts.mMsgFlagMask
		          << " MsgId: " << meta->mMsgId << std::endl;
#endif
	}

	return true;
}

