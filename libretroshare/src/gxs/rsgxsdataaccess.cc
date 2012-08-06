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

	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_FAILED = 0;
	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_PENDING = 1;
	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_PARTIAL = 2;
	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_FINISHED_INCOMPLETE = 3;
	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_COMPLETE = 4;
	const uint8_t RsTokenServiceV2::GXS_REQUEST_STATUS_DONE = 5;			 // ONCE ALL DATA RETRIEVED.

RsGxsDataAccess::RsGxsDataAccess(RsGeneralDataService* ds)
 : mDataStore(ds), mDataMutex("RsGxsDataAccess")
{
}


bool RsGxsDataAccess::requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsV2 &opts,
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
		const RsTokReqOptionsV2 &opts, const GxsMsgReq &msgIds)
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

bool RsGxsDataAccess::requestSetGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId &grpId, uint32_t subscribeFlags,
		uint32_t subscribeMask)
{

	generateToken(token);

	GroupSetFlagReq* req = new GroupSetFlagReq();

	req->flag = subscribeFlags;
	req->flagMask = subscribeMask;
	req->grpId = grpId;

	std::cerr << "RsGxsDataAccess::requestSetGroupSubscribeFlags() gets Token: " << token << std::endl;
	storeRequest(req);

	return false;
}

bool RsGxsDataAccess::requestSetGroupStatus(uint32_t& token, const RsGxsGroupId& grpId, uint32_t status, uint32_t statusMask)
{

	generateToken(token);

	GroupSetFlagReq* req = new GroupSetFlagReq();

	req->flag = status;
	req->flagMask = statusMask;
	req->grpId = grpId;

	std::cerr << "RsGxsDataAccess::requestSetGroupStatus() gets Token: " << token << std::endl;
	storeRequest(req);

	return true;
}

bool RsGxsDataAccess::requestSetMessageStatus(uint32_t& token, const RsGxsGrpMsgIdPair &msgId, uint32_t status,
		uint32_t statusMask)
{

	generateToken(token);

	MessageSetFlagReq* req = new MessageSetFlagReq();

	req->flag = status;
	req->flagMask = statusMask;
	req->msgId = msgId;

	std::cerr << "RsGxsDataAccess::requestSetGroupStatus() gets Token: " << token << std::endl;
	storeRequest(req);

	return true;
}

void RsGxsDataAccess::setReq(GxsRequest* req, const uint32_t& token, const uint32_t& ansType, const RsTokReqOptionsV2& opts) const
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
	}else  if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		GroupMetaReq* gmreq = dynamic_cast<GroupMetaReq*>(req);

		if(gmreq)
		{
			groupInfo = gmreq->mGroupMetaData;
			updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
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

	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupData() Unable to retrieve group data" << std::endl;
		return false;
	}else  if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		GroupDataReq* gmreq = dynamic_cast<GroupDataReq*>(req);

		if(gmreq)
		{
			grpData = gmreq->mGroupData;
			updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
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
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgData() Unable to retrieve group data" << std::endl;
		return false;
	}else if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		MsgDataReq* mdreq = dynamic_cast<MsgDataReq*>(req);

		if(mdreq)
		{
		 msgData = mdreq->mMsgData;
		 updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		}else{
			std::cerr << "RsGxsDataAccess::getMsgData() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getMsgData() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getMsgSummary(const uint32_t& token, GxsMsgMetaResult& msgInfo)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgSummary() Unable to retrieve group data" << std::endl;
		return false;
	}else  if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		MsgMetaReq* mmreq = dynamic_cast<MsgMetaReq*>(req);

		if(mmreq)
		{
		 msgInfo = mmreq->mMsgMetaData;
		 updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

		}else{
			std::cerr << "RsGxsDataAccess::getMsgSummary() Req found, failed caste" << std::endl;
			return false;
		}
	}else{
		std::cerr << "RsGxsDataAccess::getMsgSummary() Req not ready" << std::endl;
		return false;
	}

	return true;
}

bool RsGxsDataAccess::getMsgList(const uint32_t& token, GxsMsgIdResult& msgIds)
{
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getMsgList() Unable to retrieve group data" << std::endl;
		return false;
	}else  if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		MsgIdReq* mireq = dynamic_cast<MsgIdReq*>(req);

		if(mireq)
		{
		 msgIds = mireq->mMsgIdResult;
		 updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

		}else{
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
	GxsRequest* req = retrieveRequest(token);

	if(req == NULL){

		std::cerr << "RsGxsDataAccess::getGroupList() Unable to retrieve group data,"
				"\nRequest does not exist" << std::endl;
		return false;
	}else if(req->token == GXS_REQUEST_STATUS_COMPLETE){

		GroupIdReq* gireq = dynamic_cast<GroupIdReq*>(req);

		if(gireq)
		{
		 groupIds = gireq->mGroupIdResult;
		 updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

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


GxsRequest* RsGxsDataAccess::retrieveRequest(const uint32_t& token)
{

	RsStackMutex stack(mDataMutex);

	if(mRequests.find(token) == mRequests.end()) return NULL;

	GxsRequest* req = mRequests[token];

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
				std::cerr << "RsGxsDataAccess::processRequests() Processing Token: " << req->token << " Status: "
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

	std::map<RsGxsGroupId, RsNxsGrp*> grpData;
	mDataStore->retrieveNxsGrps(grpData, true, true);

	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grpData.begin();
	for(; mit != grpData.end(); mit++)
		req->mGroupData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupSummary(GroupMetaReq* req)
{

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;

	std::list<RsGxsGroupId>::const_iterator lit = req->mGroupIds.begin();

	for(; lit != req->mGroupIds.end(); lit++)
		grpMeta[*lit] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.begin();

	for(; mit != grpMeta.end(); mit++)
		req->mGroupMetaData.push_back(mit->second);

	return true;
}

bool RsGxsDataAccess::getGroupList(GroupIdReq* req)
{
	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;

	std::list<RsGxsGroupId>::const_iterator lit = req->mGroupIds.begin();

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
	std::vector<RsGxsGroupId> groupIds;
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
	std::vector<RsGxsGroupId> groupIds;
	GxsMsgReq::iterator mit = req->mMsgIds.begin();

	const RsTokReqOptionsV2& opts = req->Options;

	for(; mit != req->mMsgIds.end(); mit++)
		groupIds.push_back(mit->first);

	{
		RsStackMutex stack(mDataMutex);
		mDataStore->retrieveGxsMsgMetaData(groupIds, result);
	}


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
			std::map<RsGxsGroupId, std::pair<RsGxsMessageId, time_t> > origMsgTs;
			std::map<RsGxsGroupId, std::pair<RsGxsMessageId, time_t> >::iterator oit;

			for(; vit != metaV.end(); vit++)
			{
				RsGxsMsgMetaData* msgMeta = *vit;

				/* if we are grabbing thread Head... then parentId == empty. */
				if (onlyThreadHeadMsgs)
				{
					if (!(msgMeta->mParentId.empty()))
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
				req->mMsgIds[grpId].push_back(oit->second.first);

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
					if (!(msgMeta->mParentId.empty()))
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
					req->mMsgIdResult[grpId].push_back(msgMeta->mMsgId);
				}

			}
		}
	}

	filterMsgList(req->mMsgIdResult, opts, metaFilter);

	// delete the data
	cleanseMetaFilter(metaFilter);

	return true;
}

void RsGxsDataAccess::cleanseMetaFilter(MsgMetaFilter& filter)
{
	MsgMetaFilter::iterator mit = filter.begin();

	for(; mit !=filter.end(); mit++)
	{
		std::map<RsGxsMessageId, RsGxsMsgMetaData*>& metaM =
				mit->second;
		std::map<RsGxsMessageId, RsGxsMsgMetaData*>::iterator mit2
		= metaM.begin();

		for(; mit2 != metaM.end(); mit2++)
		{
			delete mit2->second;
		}
	}

	filter.clear();
	return;
}

void RsGxsDataAccess::filterMsgList(GxsMsgIdResult& msgIds, const RsTokReqOptionsV2& opts,
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


bool RsGxsDataAccess::checkRequestStatus(const uint32_t& token,
		uint32_t& status, uint32_t& reqtype, uint32_t& anstype, time_t& ts)
{

	GxsRequest* req = retrieveRequest(token);

	RsStackMutex stack(mDataMutex);

	if(!req)
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



bool RsGxsDataAccess::addMsgData(RsNxsMsg* msg) {

	RsStackMutex stack(mDataMutex);

	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgM;
	msgM.insert(std::make_pair(msg, msg->metaData));
	return mDataStore->storeMessage(msgM);
}



void RsGxsDataAccess::tokenList(std::list<uint32_t>& tokens) {

	RsStackMutex stack(mDataMutex);

	std::map<uint32_t, GxsRequest*>::iterator mit = mRequests.begin();

	for(; mit != mRequests.end(); mit++)
	{
		tokens.push_back(mit->first);
	}
}

bool RsGxsDataAccess::updateRequestStatus(const uint32_t& token,
		const uint32_t& status) {

	RsStackMutex stack(mDataMutex);

	GxsRequest* req = retrieveRequest(token);

	if(req)
		req->status = status;
	else
		return false;

	return true;
}

bool RsGxsDataAccess::checkMsgFilter(const RsTokReqOptionsV2& opts, const RsGxsMsgMetaData* meta) const
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
	return statusMatch;
}
