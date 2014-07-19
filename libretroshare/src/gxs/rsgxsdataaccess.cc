/*
 * libretroshare/src/retroshare: rsgxsdataaccess.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2013 by Robert Fernie, Christopher Evi-Parker
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

#include <time.h>

#include "rsgxsdataaccess.h"

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


        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_FAILED = 0;
        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_PENDING = 1;
        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_PARTIAL = 2;
        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_FINISHED_INCOMPLETE = 3;
        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE = 4;
        const uint8_t RsTokenService::GXS_REQUEST_V2_STATUS_DONE = 5;			 // ONCE ALL DATA RETRIEVED.


/***********
 * #define DATA_DEBUG	1
 **********/

RsGxsDataAccess::RsGxsDataAccess(RsGeneralDataService* ds)
 : mDataStore(ds), mDataMutex("RsGxsDataAccess"), mNextToken(0)
{
}


bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
		const std::list<RsGxsGroupId> &groupIds)
{
    if(groupIds.empty())
    {
    	std::cerr << "Group Id list is empty" << std::endl;
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

    if(req == NULL)
    {
            std::cerr << "RsGxsDataAccess::requestGroupInfo() request type not recognised, type "
                              << reqType << std::endl;
            return false;
    }else
    {
            generateToken(token);

#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::requestGroupInfo() gets Token: " << token << std::endl;
#endif
    }

    setReq(req, token, ansType, opts);
    storeRequest(req);

    return true;
}

bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts)
{

    GxsRequest* req = NULL;
    uint32_t reqType = opts.mReqType;

    if(reqType & GXS_REQUEST_TYPE_GROUP_META)
    {
            GroupMetaReq* gmr = new GroupMetaReq();
            req = gmr;
    }
    else if(reqType & GXS_REQUEST_TYPE_GROUP_DATA)
    {
            GroupDataReq* gdr = new GroupDataReq();
            req = gdr;
    }
    else if(reqType & GXS_REQUEST_TYPE_GROUP_IDS)
    {
            GroupIdReq* gir = new GroupIdReq();
            req = gir;
    }

    if(req == NULL)
    {
            std::cerr << "RsGxsDataAccess::requestGroupInfo() request type not recognised, type "
                              << reqType << std::endl;
            return false;
    }else
    {
            generateToken(token);
#ifdef DATA_DEBUG
            std::cerr << "RsGxsDataAccess::requestGroupInfo() gets Token: " << token << std::endl;
#endif
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

        // remove all empty grpId entries
        GxsMsgReq::const_iterator mit = msgIds.begin();
        std::vector<RsGxsGroupId> toRemove;

        for(; mit != msgIds.end(); mit++)
        {
            if(mit->second.empty())
                toRemove.push_back(mit->first);
        }

        std::vector<RsGxsGroupId>::const_iterator vit = toRemove.begin();

        GxsMsgReq filteredMsgIds = msgIds;

        for(; vit != toRemove.end(); vit++)
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

                for(; lit != grpIds.end(); lit++)
                    mmr->mMsgIds[*lit] = std::vector<RsGxsMessageId>();

                req = mmr;
        }else if(reqType & GXS_REQUEST_TYPE_MSG_DATA)
        {
                MsgDataReq* mdr = new MsgDataReq();

                for(; lit != grpIds.end(); lit++)
                    mdr->mMsgIds[*lit] = std::vector<RsGxsMessageId>();

                req = mdr;
        }else if(reqType & GXS_REQUEST_TYPE_MSG_IDS)
        {
                MsgIdReq* mir = new MsgIdReq();

                for(; lit != grpIds.end(); lit++)
                    mir->mMsgIds[*lit] = std::vector<RsGxsMessageId>();

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

    setReq(req, token, 0, RsTokReqOptions());
    storeRequest(req);
}

void RsGxsDataAccess::requestGroupStatistic(uint32_t& token, const RsGxsGroupId& grpId)
{
    GroupStatisticRequest* req = new GroupStatisticRequest();
    req->mGrpId = grpId;

    generateToken(token);

    setReq(req, token, 0, RsTokReqOptions());
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

        req->status = GXS_REQUEST_V2_STATUS_PENDING;
        req->reqTime = time(NULL);
	mRequests[req->token] = req;

	return;
}

uint32_t RsGxsDataAccess::requestStatus(uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;

	{
		RsStackMutex stack(mDataMutex);

		// first check public tokens
		if(mPublicToken.find(token) != mPublicToken.end())
			return mPublicToken[token];
	}
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

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupSummary() Unable to retrieve group summary" << std::endl;
		return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

		GroupMetaReq* gmreq = dynamic_cast<GroupMetaReq*>(req);

		if(gmreq)
		{
			groupInfo = gmreq->mGroupMetaData;
                        locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
		}else{
			std::cerr << "RsGxsDataAccess::getGroupSummary() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getGroupSummary() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getGroupData(const uint32_t& token, std::list<RsNxsGrp*>& grpData)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupData() Unable to retrieve group data" << std::endl;
		return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

		GroupDataReq* gmreq = dynamic_cast<GroupDataReq*>(req);

		if(gmreq)
		{
			grpData = gmreq->mGroupData;
                        locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
		}else{
			std::cerr << "RsGxsDataAccess::getGroupData() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getGroupData() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getMsgData(const uint32_t& token, NxsMsgDataResult& msgData)
{

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgData() Unable to retrieve group data" << std::endl;
		return false;
        }else if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                MsgDataReq* mdreq = dynamic_cast<MsgDataReq*>(req);

		if(mdreq)
		{
                     msgData = mdreq->mMsgData;
                     locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
                }
                else
                {
			std::cerr << "RsGxsDataAccess::getMsgData() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getMsgData() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getMsgRelatedData(const uint32_t &token, NxsMsgRelatedDataResult &msgData)
{

        RsStackMutex stack(mDataMutex);

        GxsRequest* req = locked_retrieveRequest(token);

        if(req == NULL){

                std::cerr << "RsGxsDataAccess::getMsgRelatedData() Unable to retrieve group data" << std::endl;
                return false;
        }else if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

                if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_DATA)
                    return false;

                if(mrireq)
                {
                    msgData = mrireq->mMsgDataResult;
                    locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
                }
                else
                {
                        std::cerr << "RsGxsDataAccess::getMsgRelatedData() Req found, failed caste" << std::endl;
                        return false;
                }
        }else{
                std::cerr << "RsGxsDataAccess::getMsgRelatedData() Req not ready" << std::endl;
                return false;
        }

        return true;
}

bool RsGxsDataAccess::getMsgSummary(const uint32_t& token, GxsMsgMetaResult& msgInfo)
{

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgSummary() Unable to retrieve group data" << std::endl;
		return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                MsgMetaReq* mmreq = dynamic_cast<MsgMetaReq*>(req);

		if(mmreq)
		{
                     msgInfo = mmreq->mMsgMetaData;
                     locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);

                }
                else
                {
			std::cerr << "RsGxsDataAccess::getMsgSummary() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getMsgSummary() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getMsgRelatedSummary(const uint32_t &token, MsgRelatedMetaResult &msgMeta)
{

        RsStackMutex stack(mDataMutex);

        GxsRequest* req = locked_retrieveRequest(token);



        if(req == NULL){

                std::cerr << "RsGxsDataAccess::getMsgRelatedSummary() Unable to retrieve message summary" << std::endl;
                return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

            if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_META)
                return false;

            MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

            if(mrireq)
            {
                msgMeta = mrireq->mMsgMetaResult;
                locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
            }
            else
            {
                    std::cerr << "RsGxsDataAccess::getMsgRelatedSummary() Req found, failed caste" << std::endl;
                    return false;
            }
        }
        else
        {
                std::cerr << "RsGxsDataAccess::getMsgRelatedSummary() Req not ready" << std::endl;
                return false;
        }

        return true;
}


bool RsGxsDataAccess::getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
        RsStackMutex stack(mDataMutex);

        GxsRequest* req = locked_retrieveRequest(token);

        if(req == NULL){

                std::cerr << "RsGxsDataAccess::getMsgRelatedList() Unable to retrieve message data" << std::endl;
                return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

            if(req->Options.mReqType != GXS_REQUEST_TYPE_MSG_RELATED_IDS)
                return false;

            MsgRelatedInfoReq* mrireq = dynamic_cast<MsgRelatedInfoReq*>(req);

            if(mrireq)
            {
                msgIds = mrireq->mMsgIdResult;
                locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
            }
            else{
                    std::cerr << "RsGxsDataAccess::getMsgRelatedList() Req found, failed caste" << std::endl;
                    return false;
            }
        }
        else
        {
                std::cerr << "RsGxsDataAccess::getMsgRelatedList() Req not ready" << std::endl;
                return false;
        }

        return true;
}

bool RsGxsDataAccess::getMsgList(const uint32_t& token, GxsMsgIdResult& msgIds)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

                std::cerr << "RsGxsDataAccess::getMsgList() Unable to retrieve msg Ids" << std::endl;
		return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                MsgIdReq* mireq = dynamic_cast<MsgIdReq*>(req);

		if(mireq)
                {
                    msgIds = mireq->mMsgIdResult;
                    locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
                }
                else{
			std::cerr << "RsGxsDataAccess::getMsgList() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getMsgList() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getGroupList(const uint32_t& token, std::list<RsGxsGroupId>& groupIds)
{
	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

	if(req == NULL){

                std::cerr << "RsGxsDataAccess::getGroupList() Unable to retrieve group Ids,"
				"\nRequest does not exist" << std::endl;
		return false;
        }else if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

		GroupIdReq* gireq = dynamic_cast<GroupIdReq*>(req);

		if(gireq)
		{
		 groupIds = gireq->mGroupIdResult;
                 locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);

		}else{
			std::cerr << "RsGxsDataAccess::getGroupList() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getGroupList() Req not ready" << std::endl;
		return false;
	}

	return true;
}


GxsRequest* RsGxsDataAccess::locked_retrieveRequest(const uint32_t& token)
{

	if(mRequests.find(token) == mRequests.end()) return NULL;

	GxsRequest* req = mRequests[token];

	return req;
}

#define MAX_REQUEST_AGE 30

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
        MsgRelatedInfoReq* mri;
        GroupStatisticRequest* gsr;
        ServiceStatisticRequest* ssr;

		for(it = mRequests.begin(); it != mRequests.end(); it++)
		{

			GxsRequest* req = it->second;
                        if (req->status == GXS_REQUEST_V2_STATUS_PENDING)
			{
#ifdef DATA_DEBUG
				std::cerr << "RsGxsDataAccess::processRequests() Processing Token: " << req->token << " Status: "
						<< req->status << " ReqType: " << req->reqType << " Age: "
						<< now - req->reqTime << std::endl;
#endif

                                req->status = GXS_REQUEST_V2_STATUS_PARTIAL;

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
                else if((mri = dynamic_cast<MsgRelatedInfoReq*>(req)) != NULL)
                {
                        getMsgRelatedInfo(mri);
                }
                else if((gsr = dynamic_cast<GroupStatisticRequest*>(req)) != NULL)
                {
                    getGroupStatistic(gsr);
                }
                else if((ssr = dynamic_cast<ServiceStatisticRequest*>(req)) != NULL)
                {
                    getServiceStatistic(ssr);
                }
				else
				{
					std::cerr << "RsGxsDataAccess::processRequests() Failed to process request, token: "
							  << req->token << std::endl;

                                        req->status = GXS_REQUEST_V2_STATUS_FAILED;
				}
			}
                        else if (req->status == GXS_REQUEST_V2_STATUS_PARTIAL)
			{
                                        req->status = GXS_REQUEST_V2_STATUS_COMPLETE;
			}
                        else if (req->status == GXS_REQUEST_V2_STATUS_DONE)
			{
#ifdef DATA_DEBUG
				std::cerr << "RsGxsDataAccess::processrequests() Clearing Done Request Token: "
						  << req->token;
				std::cerr << std::endl;
#endif
				toClear.push_back(req->token);
			}
                        else if (now - req->reqTime > MAX_REQUEST_AGE)
			{
#ifdef DATA_DEBUG
				std::cerr << "RsGxsDataAccess::processrequests() Clearing Old Request Token: " << req->token;
				std::cerr << std::endl;
#endif
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

bool RsGxsDataAccess::getGroupStatistic(const uint32_t &token, GxsGroupStatistic &grpStatistic)
{
    RsStackMutex stack(mDataMutex);

    GxsRequest* req = locked_retrieveRequest(token);

    if(req == NULL){

                std::cerr << "RsGxsDataAccess::getGroupStatistic() Unable to retrieve grp stats" << std::endl;
        return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                GroupStatisticRequest* gsreq = dynamic_cast<GroupStatisticRequest*>(req);

        if(gsreq)
                {
                    grpStatistic = gsreq->mGroupStatistic;
                    locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
                }
                else{
            std::cerr << "RsGxsDataAccess::getGroupStatistic() Req found, failed caste" << std::endl;
            return false;
        }
    }else{
        std::cerr << "RsGxsDataAccess::getGroupStatistic() Req not ready" << std::endl;
        return false;
    }

    return true;
}

bool RsGxsDataAccess::getServiceStatistic(const uint32_t &token, GxsServiceStatistic &servStatistic)
{
    RsStackMutex stack(mDataMutex);

    GxsRequest* req = locked_retrieveRequest(token);

    if(req == NULL){

                std::cerr << "RsGxsDataAccess::getServiceStatistic() Unable to retrieve grp stats" << std::endl;
        return false;
        }else  if(req->status == GXS_REQUEST_V2_STATUS_COMPLETE){

                ServiceStatisticRequest* ssreq = dynamic_cast<ServiceStatisticRequest*>(req);

        if(ssreq)
                {
                    servStatistic = ssreq->mServiceStatistic;
                    locked_updateRequestStatus(token, GXS_REQUEST_V2_STATUS_DONE);
                }
                else{
            std::cerr << "RsGxsDataAccess::getServiceStatistic() Req found, failed caste" << std::endl;
            return false;
        }
    }else{
        std::cerr << "RsGxsDataAccess::getServiceStatistic() Req not ready" << std::endl;
        return false;
    }

    return true;
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

        for(; lit != lit_end; lit++)
        {
            grpData[*lit] = NULL;
        }

        bool ok = mDataStore->retrieveNxsGrps(grpData, true, true);

	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grpData.begin();
	for(; mit != grpData.end(); mit++)
		req->mGroupData.push_back(mit->second);

        return ok;
}

bool RsGxsDataAccess::getGroupSummary(GroupMetaReq* req)
{

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;

        std::list<RsGxsGroupId> grpIdsOut;

        getGroupList(req->mGroupIds, req->Options, grpIdsOut);

        if(grpIdsOut.empty())
            return true;

        std::list<RsGxsGroupId>::const_iterator lit = grpIdsOut.begin();

        for(; lit != grpIdsOut.end(); lit++)
		grpMeta[*lit] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

	for(; mit != grpMeta.end(); mit++)
		req->mGroupMetaData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupList(GroupIdReq* req)
{
    getGroupList(req->mGroupIds, req->Options, req->mGroupIdResult);

    return true;
}

bool RsGxsDataAccess::getGroupList(const std::list<RsGxsGroupId>& grpIdsIn, const RsTokReqOptions& opts, std::list<RsGxsGroupId>& grpIdsOut)
{
    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;

    std::list<RsGxsGroupId>::const_iterator lit = grpIdsIn.begin();

    for(; lit != grpIdsIn.end(); lit++)
            grpMeta[*lit] = NULL;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

    for(; mit != grpMeta.end(); mit++)
    {
            grpIdsOut.push_back(mit->first);
    }

    filterGrpList(grpIdsOut, opts, grpMeta);

    for(mit = grpMeta.begin(); mit != grpMeta.end(); mit++)
    {
            delete mit->second; // so wasteful!!
    }

    return true;
}

bool RsGxsDataAccess::getMsgData(MsgDataReq* req)
{
	GxsMsgResult result;

	GxsMsgReq msgIdOut;

	// filter based on options
	getMsgList(req->mMsgIds, req->Options, msgIdOut);

	mDataStore->retrieveNxsMsgs(msgIdOut, result, true, true);

	req->mMsgData = result;
	return true;
}


bool RsGxsDataAccess::getMsgSummary(MsgMetaReq* req)
{
        GxsMsgMetaResult result;

        GxsMsgReq msgIdOut;

        // filter based on options
        getMsgList(req->mMsgIds, req->Options, msgIdOut);

        mDataStore->retrieveGxsMsgMetaData(msgIdOut, result);
	req->mMsgMetaData = result;

	return true;
}


bool RsGxsDataAccess::getMsgList(const GxsMsgReq& msgIds, const RsTokReqOptions& opts, GxsMsgReq& msgIdsOut)
{
    GxsMsgMetaResult result;

    mDataStore->retrieveGxsMsgMetaData(msgIds, result);

    /* CASEs this handles.
     * Input is groupList + Flags.
     * 1) No Flags => All Messages in those Groups.
     *
     */
    std::cerr << "RsGxsDataAccess::getMsgList()";
    std::cerr << std::endl;


    bool onlyOrigMsgs = false;
    bool onlyLatestMsgs = false;
    bool onlyThreadHeadMsgs = false;

    // Can only choose one of these two.
    if (opts.mOptions & RS_TOKREQOPT_MSG_ORIGMSG)
    {
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_ORIGMSG";
            std::cerr << std::endl;
            onlyOrigMsgs = true;
    }
    else if (opts.mOptions & RS_TOKREQOPT_MSG_LATEST)
    {
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_LATEST";
            std::cerr << std::endl;
            onlyLatestMsgs = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_THREAD)
    {
            std::cerr << "RsGxsDataAccess::getMsgList() MSG_THREAD";
            std::cerr << std::endl;
            onlyThreadHeadMsgs = true;
    }

    GxsMsgMetaResult::iterator meta_it;
    MsgMetaFilter metaFilter;

    for(meta_it = result.begin(); meta_it != result.end(); meta_it++)
    {
            const RsGxsGroupId& grpId = meta_it->first;

            metaFilter[grpId] = std::map<RsGxsMessageId, RsGxsMsgMetaData*>();

            const std::vector<RsGxsMsgMetaData*>& metaV = meta_it->second;
            if (onlyLatestMsgs) // THIS ONE IS HARD -> LOTS OF COMP.
            {
                    std::vector<RsGxsMsgMetaData*>::const_iterator vit = metaV.begin();

                    // RUN THROUGH ALL MSGS... in map origId -> TS.
                    std::map<RsGxsMessageId, std::pair<RsGxsMessageId, time_t> > origMsgTs;
                    std::map<RsGxsMessageId, std::pair<RsGxsMessageId, time_t> >::iterator oit;

                    for(; vit != metaV.end(); vit++)
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
                            std::cerr << "RsGxsDataAccess::getMsgList() Found New OrigMsgId: ";
                            std::cerr << msgMeta->mOrigMsgId;
                            std::cerr << " MsgId: " << msgMeta->mMsgId;
                            std::cerr << " TS: " << msgMeta->mPublishTs;
                            std::cerr << std::endl;

                            addMsg = true;
                        }
                        // check timestamps.
                        else if (oit->second.second < msgMeta->mPublishTs)
                        {
                            std::cerr << "RsGxsDataAccess::getMsgList() Found Later Msg. OrigMsgId: ";
                            std::cerr << msgMeta->mOrigMsgId;
                            std::cerr << " MsgId: " << msgMeta->mMsgId;
                            std::cerr << " TS: " << msgMeta->mPublishTs;

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
                    for(oit = origMsgTs.begin(); oit != origMsgTs.end(); oit++)
                    {
                            msgIdsOut[grpId].push_back(oit->second.first);
                    }

            }
            else	// ALL OTHER CASES.
            {
                std::vector<RsGxsMsgMetaData*>::const_iterator vit = metaV.begin();

                for(; vit != metaV.end(); vit++)
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
                        msgIdsOut[grpId].push_back(msgMeta->mMsgId);
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

    std::cerr << "RsGxsDataAccess::getMsgRelatedList()";
    std::cerr << std::endl;

    const RsTokReqOptions& opts = req->Options;

    bool onlyLatestMsgs = false;
    bool onlyAllVersions = false;
    bool onlyChildMsgs = false;
    bool onlyThreadMsgs = false;

    if (opts.mOptions & RS_TOKREQOPT_MSG_LATEST)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_LATEST";
            std::cerr << std::endl;
            onlyLatestMsgs = true;
    }
    else if (opts.mOptions & RS_TOKREQOPT_MSG_VERSIONS)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_VERSIONS";
            std::cerr << std::endl;
            onlyAllVersions = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_PARENT)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_PARENTS";
            std::cerr << std::endl;
            onlyChildMsgs = true;
    }

    if (opts.mOptions & RS_TOKREQOPT_MSG_THREAD)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() MSG_THREAD";
            std::cerr << std::endl;
            onlyThreadMsgs = true;
    }

    if (onlyAllVersions && onlyChildMsgs)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (VERSIONS & PARENT)";
            std::cerr << std::endl;

            return false;
    }

    if (onlyAllVersions && onlyThreadMsgs)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (VERSIONS & THREAD)";
            std::cerr << std::endl;

            return false;
    }

    if ((!onlyLatestMsgs) && onlyChildMsgs)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (!LATEST & PARENT)";
            std::cerr << std::endl;

            return false;
    }

    if ((!onlyLatestMsgs) && onlyThreadMsgs)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (!LATEST & THREAD)";
            std::cerr << std::endl;

            return false;
    }

    if (onlyChildMsgs && onlyThreadMsgs)
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() ERROR Incompatible FLAGS (PARENT & THREAD)";
            std::cerr << std::endl;

            return false;
    }


    /* FALL BACK OPTION */
    if ((!onlyLatestMsgs) && (!onlyAllVersions) && (!onlyChildMsgs) && (!onlyThreadMsgs))
    {
            std::cerr << "RsGxsDataAccess::getMsgRelatedList() FALLBACK -> NO FLAGS -> SIMPLY RETURN nothing";
            std::cerr << std::endl;

            return true;
    }

    std::vector<RsGxsGrpMsgIdPair>::iterator vit_msgIds = req->mMsgIds.begin();

    for(; vit_msgIds != req->mMsgIds.end(); vit_msgIds++)
    {
        MsgMetaFilter filterMap;


        const RsGxsGrpMsgIdPair& grpMsgIdPair = *vit_msgIds;

        // get meta data for all in group
        GxsMsgMetaResult result;
        GxsMsgReq msgIds;
        msgIds.insert(std::make_pair(grpMsgIdPair.first, std::vector<RsGxsMessageId>()));
        mDataStore->retrieveGxsMsgMetaData(msgIds, result);
        std::vector<RsGxsMsgMetaData*>& metaV = result[grpMsgIdPair.first];
        std::vector<RsGxsMsgMetaData*>::iterator vit_meta;

        // msg id to relate to
        const RsGxsMessageId& msgId = grpMsgIdPair.second;
        const RsGxsGroupId& grpId = grpMsgIdPair.first;

        std::vector<RsGxsMessageId> outMsgIds;

        RsGxsMsgMetaData* origMeta = NULL;
        for(vit_meta = metaV.begin(); vit_meta != metaV.end(); vit_meta++)
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
            std::cerr << "RsGxsDataAccess::getMsgRelatedInfo(): Cannot find meta of msgId (to relate to)!"
                      << std::endl;
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
                std::map<RsGxsMessageId, std::pair<RsGxsMessageId, time_t> > origMsgTs;
                std::map<RsGxsMessageId, std::pair<RsGxsMessageId, time_t> >::iterator oit;
                for(vit_meta = metaV.begin(); vit_meta != metaV.end(); vit_meta++)
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
                            std::cerr << "RsGxsDataAccess::getMsgRelatedList() Found New OrigMsgId: ";
                            std::cerr << meta->mOrigMsgId;
                            std::cerr << " MsgId: " << meta->mMsgId;
                            std::cerr << " TS: " << meta->mPublishTs;
                            std::cerr << std::endl;

                            addMsg = true;
                    }
                    // check timestamps.
                    else if (oit->second.second < meta->mPublishTs)
                    {
                            std::cerr << "RsGxsDataAccess::getMsgRelatedList() Found Later Msg. OrigMsgId: ";
                            std::cerr << meta->mOrigMsgId;
                            std::cerr << " MsgId: " << meta->mMsgId;
                            std::cerr << " TS: " << meta->mPublishTs;

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
                for(oit = origMsgTs.begin(); oit != origMsgTs.end(); oit++)
                {
                    outMsgIds.push_back(oit->second.first);
                }
            }
            else
            {

                /* first guess is potentially better than Orig (can't be worse!) */
                time_t latestTs = 0;
                RsGxsMessageId latestMsgId;
                RsGxsMsgMetaData* latestMeta;

                for(vit_meta = metaV.begin(); vit_meta != metaV.end(); vit_meta++)
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
                outMsgIds.push_back(latestMsgId);
                metaMap.insert(std::make_pair(latestMsgId, latestMeta));
            }
        }
        else if (onlyAllVersions)
        {
            for(vit_meta = metaV.begin(); vit_meta != metaV.end(); vit_meta++)
            {
                RsGxsMsgMetaData* meta = *vit_meta;

                if (meta->mOrigMsgId == origMsgId)
                {
                    outMsgIds.push_back(meta->mMsgId);
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
    metaReq[req->mGrpId] = std::vector<RsGxsMessageId>();
    GxsMsgMetaResult metaResult;
    mDataStore->retrieveGxsMsgMetaData(metaReq, metaResult);

    std::vector<RsGxsMsgMetaData*>& msgMetaV = metaResult[req->mGrpId];

    req->mGroupStatistic.mNumMsgs = msgMetaV.size();
    req->mGroupStatistic.mTotalSizeOfMsgs = 0;
    for(int i = 0; i < msgMetaV.size(); i++)
    {
        RsGxsMsgMetaData* m = msgMetaV[i];
        req->mGroupStatistic.mTotalSizeOfMsgs += m->mMsgSize + m->serial_size();
    }

    cleanseMsgMetaMap(metaResult);
    return true;
}

// potentially very expensive!
bool RsGxsDataAccess::getServiceStatistic(ServiceStatisticRequest *req)
{
    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

    req->mServiceStatistic.mNumGrps = grpMeta.size();
    req->mServiceStatistic.mSizeOfGrps = 0;
    req->mServiceStatistic.mSizeOfMsgs = 0;

    for(; mit != grpMeta.end(); mit++)
    {
        RsGxsGrpMetaData* m = mit->second;
        req->mServiceStatistic.mSizeOfGrps += m->mGrpSize + m->serial_size();
        GroupStatisticRequest gr;
        gr.mGrpId = m->mGroupId;
        getGroupStatistic(&gr);
        req->mServiceStatistic.mNumMsgs += gr.mGroupStatistic.mNumMsgs;
        req->mServiceStatistic.mSizeOfMsgs += gr.mGroupStatistic.mTotalSizeOfMsgs;
        delete m;
    }

    req->mServiceStatistic.mSizeStore = req->mServiceStatistic.mSizeOfGrps + req->mServiceStatistic.mSizeOfMsgs;

    return true;
}

bool RsGxsDataAccess::getMsgList(MsgIdReq* req)
{

    GxsMsgMetaResult result;

    mDataStore->retrieveGxsMsgMetaData(req->mMsgIds, result);


    GxsMsgMetaResult::iterator mit = result.begin(), mit_end = result.end();

    for(; mit != mit_end; mit++)
    {
        const RsGxsGroupId grpId = mit->first;
        std::vector<RsGxsMsgMetaData*>& metaV = mit->second;
        std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin(),
        vit_end = metaV.end();

        for(; vit != vit_end; vit++)
        {
            RsGxsMsgMetaData* meta = *vit;
            req->mMsgIdResult[grpId].push_back(meta->mMsgId);
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

        for(; mit !=result.end(); mit++)
	{

            std::vector<RsGxsMsgMetaData*>& msgMetaV = mit->second;
            std::vector<RsGxsMsgMetaData*>::iterator vit = msgMetaV.begin();
                for(; vit != msgMetaV.end(); vit++)
		{
                        delete *vit;
		}
	}

        result.clear();
	return;
}

void RsGxsDataAccess::filterMsgList(GxsMsgIdResult& msgIds, const RsTokReqOptions& opts,
		const MsgMetaFilter& msgMetas) const
{

	GxsMsgIdResult::iterator mit = msgIds.begin();
	for(;mit != msgIds.end(); mit++)
	{

		MsgMetaFilter::const_iterator cit = msgMetas.find(mit->first);

		if(cit == msgMetas.end())
			continue;

		std::vector<RsGxsMessageId>& msgs = mit->second;
		std::vector<RsGxsMessageId>::iterator vit = msgs.begin();
		const std::map<RsGxsMessageId, RsGxsMsgMetaData*>& meta = cit->second;
		std::map<RsGxsMessageId, RsGxsMsgMetaData*>::const_iterator cit2;

		for(; vit != msgs.end();)
		{

			bool keep = false;
			if( (cit2 = meta.find(*vit)) != meta.end() )
			{
				keep = checkMsgFilter(opts, cit2->second);
			}

			if(keep)
			{
				vit++;
			}else
			{
				vit = msgs.erase(vit);
			}
		}
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
            lit++;
        }else
        {
            lit = grpIds.erase(lit);
        }

    }
}


bool RsGxsDataAccess::checkRequestStatus(const uint32_t& token,
		uint32_t& status, uint32_t& reqtype, uint32_t& anstype, time_t& ts)
{

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = locked_retrieveRequest(token);

        if(req == NULL)
		return false;

	anstype = req->ansType;
	reqtype = req->reqType;
	status = req->status;
	ts = req->reqTime;

	return true;
}

bool RsGxsDataAccess::addGroupData(RsNxsGrp* grp) {

	RsStackMutex stack(mDataMutex);

	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grpM;
	grpM.insert(std::make_pair(grp, grp->metaData));
	return mDataStore->storeGroup(grpM);
}

bool RsGxsDataAccess::updateGroupData(RsNxsGrp* grp) {

	RsStackMutex stack(mDataMutex);

	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grpM;
	grpM.insert(std::make_pair(grp, grp->metaData));
	return mDataStore->updateGroup(grpM);
}

bool RsGxsDataAccess::addMsgData(RsNxsMsg* msg) {

	RsStackMutex stack(mDataMutex);

	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgM;
	msgM.insert(std::make_pair(msg, msg->metaData));
	return mDataStore->storeMessage(msgM);
}



void RsGxsDataAccess::tokenList(std::list<uint32_t>& tokens)
{

	RsStackMutex stack(mDataMutex);

	std::map<uint32_t, GxsRequest*>::iterator mit = mRequests.begin();

	for(; mit != mRequests.end(); mit++)
	{
		tokens.push_back(mit->first);
	}
}

bool RsGxsDataAccess::locked_updateRequestStatus(const uint32_t& token,
		const uint32_t& status)
{

	GxsRequest* req = locked_retrieveRequest(token);

	if(req)
		req->status = status;
	else
		return false;

	return true;
}

uint32_t RsGxsDataAccess::generatePublicToken()
{

	uint32_t token;
	generateToken(token);

        {
            RsStackMutex stack(mDataMutex);
            mPublicToken[token] = RsTokenService::GXS_REQUEST_V2_STATUS_PENDING;
        }

	return token;
}



bool RsGxsDataAccess::updatePublicRequestStatus(const uint32_t& token,
		const uint32_t& status)
{
	RsStackMutex stack(mDataMutex);
	std::map<uint32_t, uint32_t>::iterator mit = mPublicToken.find(token);

	if(mit != mPublicToken.end())
	{
		mit->second = status;
	}
	else
	{
		return false;
	}

	return true;
}



bool RsGxsDataAccess::disposeOfPublicToken(const uint32_t& token)
{
	RsStackMutex stack(mDataMutex);
	std::map<uint32_t, uint32_t>::iterator mit = mPublicToken.find(token);

	if(mit != mPublicToken.end())
	{
		mPublicToken.erase(mit);
	}
	else
	{
		return false;
	}

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
bool RsGxsDataAccess::checkMsgFilter(const RsTokReqOptions& opts, const RsGxsMsgMetaData* meta) const
{
	bool statusMatch = false;
        if (opts.mStatusMask)
	{
		// Exact Flags match required.
 		if ((opts.mStatusMask & opts.mStatusFilter) == (opts.mStatusMask & meta->mMsgStatus))
		{
			std::cerr << "checkMsgFilter() Accepting Msg as StatusMatches: ";
			std::cerr << " Mask: " << opts.mStatusMask << " StatusFilter: " << opts.mStatusFilter;
			std::cerr << " MsgStatus: " << meta->mMsgStatus << " MsgId: " << meta->mMsgId;
			std::cerr << std::endl;

			statusMatch = true;
		}
		else
		{
			std::cerr << "checkMsgFilter() Dropping Msg due to !StatusMatch ";
			std::cerr << " Mask: " << opts.mStatusMask << " StatusFilter: " << opts.mStatusFilter;
			std::cerr << " MsgStatus: " << meta->mMsgStatus << " MsgId: " << meta->mMsgId;
			std::cerr << std::endl;
		}
	}
	else
	{
		// no status comparision,
		statusMatch = true;
	}

        bool flagMatch = false;

        if(opts.mMsgFlagMask)
        {
            // Exact Flags match required.
            if ((opts.mMsgFlagMask & opts.mMsgFlagFilter) == (opts.mMsgFlagMask & meta->mMsgFlags))
            {
                    std::cerr << "checkMsgFilter() Accepting Msg as FlagMatches: ";
                    std::cerr << " Mask: " << opts.mMsgFlagMask << " FlagFilter: " << opts.mMsgFlagFilter;
                    std::cerr << " MsgFlag: " << meta->mMsgFlags << " MsgId: " << meta->mMsgId;
                    std::cerr << std::endl;

                    flagMatch = true;
            }
            else
            {
                    std::cerr << "checkMsgFilter() Dropping Msg due to !FlagMatch ";
                    std::cerr << " Mask: " << opts.mMsgFlagMask << " FlagFilter: " << opts.mMsgFlagFilter;
                    std::cerr << " MsgFlag: " << meta->mMsgFlags << " MsgId: " << meta->mMsgId;
                    std::cerr << std::endl;

                    flagMatch = false;
            }
        }else{
            flagMatch = true;
        }

        return statusMatch && flagMatch;
}

