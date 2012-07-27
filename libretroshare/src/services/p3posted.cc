/*
 * libretroshare/src/services p3photoservice.cc
 *
 * Photo Service for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "services/p3posted.h"
#include "util/rsrandom.h"
#include <iostream>
#include <stdio.h>
#include <math.h>

/****
 * #define POSTED_DEBUG 1
 ****/

RsPosted *rsPosted = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PostedService::p3PostedService(uint16_t type)
	:p3GxsDataService(type, new PostedDataProxy()), mPostedMtx("p3PostedService"), mUpdated(true)
{
	{
     		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mPostedProxy = (PostedDataProxy *) mProxy;

		mViewMode = RSPOSTED_VIEWMODE_HOT;
		mViewPeriod = RSPOSTED_PERIOD_WEEK;
		mViewStart = 0;
		mViewCount = 50;

		mProcessingRanking = false;
		mRankingState = 0;
		mRankingExternalToken = 0;
		mRankingInternalToken = 0;

		mLastBgCheck = 0;
		mBgProcessing = 0;
		mBgPhase = 0;
		mBgToken = 0;
	
	}

	generateDummyData();
	return;
}

#define POSTED_BACKGROUND_PERIOD	60

int	p3PostedService::tick()
{
	//std::cerr << "p3PostedService::tick()";
	//std::cerr << std::endl;

	fakeprocessrequests();

	
	// Contine Ranking Request.
	checkRankingRequest();

	// Run Background Stuff.	
	background_checkTokenRequest();

	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doCheck = false;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (now -  mLastBgCheck > POSTED_BACKGROUND_PERIOD)
		{
			doCheck = true;
			mLastBgCheck = now;
		}
	}

	if (doCheck)
	{
		addExtraDummyData();
		background_requestGroups();
	}



	// Add in new votes + comments.
	return 0;
}

bool p3PostedService::updated()
{
	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



       /* Data Requests */
bool p3PostedService::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3PostedService::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3PostedService::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3PostedService::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3PostedService::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3PostedService::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3PostedService::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3PostedService::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PostedService::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3PostedService::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3PostedService::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PostedService::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3PostedService::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3PostedService::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PostedService::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3PostedService::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3PostedService::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PostedService::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getMsgSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> msgIds;
	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsMsgMetaData */
	mProxy->getMsgSummary(msgIds, msgInfo);

	return ans;
}


        /* Specific Service Data */
bool p3PostedService::getGroup(const uint32_t &token, RsPostedGroup &group)
{
	std::cerr << "p3PostedService::getGroup() Token: " << token;
	std::cerr << std::endl;

	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3PostedService::getGroup() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PostedService::getGroup() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getGroup() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsPostedGroup */
	bool ans = mPostedProxy->getGroup(id, group);
	return ans;
}


bool p3PostedService::getPost(const uint32_t &token, RsPostedPost &post)
{
	std::cerr << "p3PostedService::getPost() Token: " << token;
	std::cerr << std::endl;

	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3PostedService::getPost() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PostedService::getPost() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getPost() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsPhotoAlbum */
	bool ans = mPostedProxy->getPost(id, post);
	return ans;
}


bool p3PostedService::getComment(const uint32_t &token, RsPostedComment &comment)
{
	std::cerr << "p3PostedService::getComment() Token: " << token;
	std::cerr << std::endl;

	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3PostedService::getComment() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PostedService::getComment() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PostedService::getComment() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsPhotoAlbum */
	bool ans = mPostedProxy->getComment(id, comment);
	return ans;
}





        /* Poll */
/*** 
 * THE STANDARD ONE IS REPLACED - SO WE CAN HANDLE RANKING REQUESTS
 * Its defined lower - next to the ranking code.
 ***/

#if 0
uint32_t p3PostedService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;

	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}

#endif


        /* Cancel Request */
bool p3PostedService::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////



bool p3PostedService::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
        return mPostedProxy->setMessageStatus(msgId, status, statusMask);
}

bool p3PostedService::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
        return mPostedProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3PostedService::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
        return mPostedProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3PostedService::setMessageServiceString(const std::string &msgId, const std::string &str)
{
        return mPostedProxy->setMessageServiceString(msgId, str);
}

bool p3PostedService::setGroupServiceString(const std::string &grpId, const std::string &str)
{
        return mPostedProxy->setGroupServiceString(grpId, str);
}


bool p3PostedService::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3PostedService::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


bool p3PostedService::submitGroup(uint32_t &token, RsPostedGroup &group, bool isNew)
{
	/* check if its a modification or a new album */

	/* add to database */

	/* check if its a mod or new photo */
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
		group.mMeta.mPublishTs = time(NULL);

		std::cerr << "p3PostedService::submitGroup() Generated New GroupID: " << group.mMeta.mGroupId;
		std::cerr << std::endl;
	}

	//group.mModFlags = 0; // These are always cleared.

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mPostedProxy->addGroup(group);
	}

	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3PostedService::submitGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}


bool p3PostedService::submitPost(uint32_t &token, RsPostedPost &post, bool isNew)
{
	if (post.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3PostedService::submitPost() Missing GroupID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* generate a new id */
	post.mMeta.mMsgId = genRandomId();
	post.mMeta.mPublishTs = time(NULL);

	if (isNew)
	{
		/* new (Original Msg) photo */
		post.mMeta.mOrigMsgId = post.mMeta.mMsgId; 
		std::cerr << "p3PostedService::submitPost() New Msg";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "p3PostedService::submitPost() Updated Msg";
		std::cerr << std::endl;
	}

	//post.mModFlags = 0; // These are always cleared.

	std::cerr << "p3PostedService::submitPost() OrigMsgId: " << post.mMeta.mOrigMsgId;
	std::cerr << " MsgId: " << post.mMeta.mMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mPostedProxy->addPost(post);
	}

	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(post.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3PostedService::submitPost() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



bool p3PostedService::submitVote(uint32_t &token, RsPostedVote &vote, bool isNew)
{
	if (vote.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3PostedService::submitVote() Missing GroupID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* generate a new id */
	vote.mMeta.mMsgId = genRandomId();
	vote.mMeta.mPublishTs = time(NULL);

	if (isNew)
	{
		/* new (Original Msg) photo */
		vote.mMeta.mOrigMsgId = vote.mMeta.mMsgId; 
		std::cerr << "p3PostedService::submitVote() New Msg";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "p3PostedService::submitVote() Updated Msg";
		std::cerr << std::endl;
	}

	//vote.mModFlags = 0; // These are always cleared.

	std::cerr << "p3PostedService::submitVote() OrigMsgId: " << vote.mMeta.mOrigMsgId;
	std::cerr << " MsgId: " << vote.mMeta.mMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mPostedProxy->addVote(vote);
	}

	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(vote.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3PostedService::submitVote() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);


	return true;
}


bool p3PostedService::submitComment(uint32_t &token, RsPostedComment &comment, bool isNew)
{
	if (comment.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3PostedService::submitPost() Missing GroupID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* generate a new id */
	comment.mMeta.mMsgId = genRandomId();
	comment.mMeta.mPublishTs = time(NULL);

	if (isNew)
	{
		/* new (Original Msg) photo */
		comment.mMeta.mOrigMsgId = comment.mMeta.mMsgId; 
		std::cerr << "p3PostedService::submitComment() New Msg";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "p3PostedService::submitComment() Updated Msg";
		std::cerr << std::endl;
	}

	//comment.mModFlags = 0; // These are always cleared.

	std::cerr << "p3PostedService::submitComment() OrigMsgId: " << comment.mMeta.mOrigMsgId;
	std::cerr << " MsgId: " << comment.mMeta.mMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mPostedProxy->addComment(comment);
	}

	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(comment.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3PostedService::submitComment() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/

bool PostedDataProxy::getGroup(const std::string &id, RsPostedGroup &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsPostedGroup *pG = (RsPostedGroup *) groupData;
		group = *pG;

		group.mMeta = meta;

		std::cerr << "PostedDataProxy::getGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "PostedDataProxy::getGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}



bool PostedDataProxy::getPost(const std::string &id, RsPostedPost &post)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsPostedMsg *pM = (RsPostedMsg *) msgData;

		if (pM->postedType == RSPOSTED_MSGTYPE_POST)
		{
			RsPostedPost *pP = (RsPostedPost *) pM;
			post = *pP;
	
			// update definitive version of the metadata.
			post.mMeta = meta;

			std::cerr << "PostedDataProxy::getPost() Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return true;
		}
		else
		{
			std::cerr << "PostedDataProxy::getPost() ERROR NOT POST Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return false;
		}
	}

	std::cerr << "PostedDataProxy::getPost() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}




bool PostedDataProxy::getVote(const std::string &id, RsPostedVote &vote)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsPostedMsg *pM = (RsPostedMsg *) msgData;
		
		if (pM->postedType == RSPOSTED_MSGTYPE_VOTE)
		{
			RsPostedVote *pP = (RsPostedVote *) pM;
			vote = *pP;
	
			// update definitive version of the metadata.
			vote.mMeta = meta;

			std::cerr << "PostedDataProxy::getVote() Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return true;
		}
		else
		{
			std::cerr << "PostedDataProxy::getVote() ERROR NOT VOTE Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return false;
		}
	}

	std::cerr << "PostedDataProxy::getVote() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}




bool PostedDataProxy::getComment(const std::string &id, RsPostedComment &comment)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsPostedMsg *pM = (RsPostedMsg *) msgData;
		
		if (pM->postedType == RSPOSTED_MSGTYPE_COMMENT)
		{
			RsPostedComment *pP = (RsPostedComment *) pM;
			comment = *pP;
	
			// update definitive version of the metadata.
			comment.mMeta = meta;

			std::cerr << "PostedDataProxy::getComment() Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return true;
		}
		else
		{
			std::cerr << "PostedDataProxy::getComment() ERROR NOT POST Id: " << id;
			std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
			std::cerr << std::endl;
			return false;
		}
	}

	std::cerr << "PostedDataProxy::getComment() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}




bool PostedDataProxy::addGroup(const RsPostedGroup &group)
{
	// Make duplicate.
	RsPostedGroup *pG = new RsPostedGroup();
	*pG = group;

	std::cerr << "PostedDataProxy::addGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool PostedDataProxy::addPost(const RsPostedPost &post)
{
	// Make duplicate.
	RsPostedPost *pP = new RsPostedPost();
	*pP = post;

	std::cerr << "PostedDataProxy::addPost()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}


bool PostedDataProxy::addVote(const RsPostedVote &vote)
{
	// Make duplicate.
	RsPostedVote *pP = new RsPostedVote();
	*pP = vote;

	std::cerr << "PostedDataProxy::addVote()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}


bool PostedDataProxy::addComment(const RsPostedComment &comment)
{
	// Make duplicate.
	RsPostedComment *pP = new RsPostedComment();
	*pP = comment;

	std::cerr << "PostedDataProxy::addComment()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}


        /* These Functions must be overloaded to complete the service */
bool PostedDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsPostedGroup *group = (RsPostedGroup *) groupData;
	meta = group->mMeta;

	return true;
}

bool PostedDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsPostedMsg *msg = (RsPostedMsg *) msgData;
	meta = msg->mMeta;

	return true;
}


/********************************************************************************************/

std::string p3PostedService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
	
/********************************************************************************************/

std::ostream &operator<<(std::ostream &out, const RsPostedPost &post)
{
	out << "RsPostedPost [ ";
	out << "Title: " << post.mMeta.mMsgName;
	out << "]";
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsPostedVote &vote)
{
	out << "RsPostedVote [ ";
	out << "Title: " << vote.mMeta.mMsgName;
	out << "]";
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsPostedComment &comment)
{
	out << "RsPostedComment [ ";
	out << "Title: " << comment.mMeta.mMsgName;
	out << "]";
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsPostedGroup &group)
{
	out << "RsPostedGroup [ ";
	out << "Title: " << group.mMeta.mGroupName;
	out << "]";
	return out;
}


/********************************************************************************************/
/********************************************************************************************/

bool p3PostedService::generateDummyData()
{
#define MAX_GROUPS 10 //100
#define MAX_POSTS 100 //1000
#define MAX_COMMENTS 5000 //10000
#define MAX_VOTES 10000 //10000

	std::list<RsPostedGroup> mGroups;
	std::list<RsPostedGroup>::iterator git;

	std::list<RsPostedPost> mPosts;
	std::list<RsPostedPost>::iterator pit;

	std::list<RsPostedVote> mVotes;
	std::list<RsPostedVote>::iterator vit;

	std::list<RsPostedComment> mComments;
	std::list<RsPostedComment>::iterator cit;

#define DUMMY_NAME_MAX_LEN		10000
	char name[DUMMY_NAME_MAX_LEN];
	int i, j;
	time_t now = time(NULL);

	for(i = 0; i < MAX_GROUPS; i++)
	{
		/* generate a new forum */
		RsPostedGroup group;

		snprintf(name, DUMMY_NAME_MAX_LEN, "TestTopic_%d", i+1);

		group.mMeta.mGroupId = genRandomId();
		group.mMeta.mGroupName = name;

		group.mMeta.mPublishTs = now - (RSRandom::random_f32() * 100000);
		/* key fields to fill in:
		 * GroupId.
		 * Name.
		 * Flags.
		 * Pop.
		 */



		/* use probability to decide which are subscribed / own / popularity.
		 */

		float rnd = RSRandom::random_f32();
		if (rnd < 0.1)
		{
			group.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_ADMIN | RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED;

		}
		else if (rnd < 0.3)
		{
			group.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED;
		}
		else
		{
			group.mMeta.mSubscribeFlags = 0;
		}

		group.mMeta.mPop = (int) (RSRandom::random_f32() * 10.0);
		mGroups.push_back(group);

	}

	for(i = 0; i < MAX_POSTS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsPostedGroup head = mGroups.front();
			mGroups.pop_front();
			mGroups.push_back(head);
		}

		RsPostedGroup group = mGroups.front();

		/* now create a new thread */

		RsPostedPost post;

		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => Post_%d", group.mMeta.mGroupName.c_str(), i+1);
		post.mMeta.mMsgName = name;

		post.mMeta.mGroupId = group.mMeta.mGroupId;
		post.mMeta.mMsgId = genRandomId();
		post.mMeta.mOrigMsgId = post.mMeta.mMsgId;
		post.mMeta.mThreadId = post.mMeta.mMsgId;
		post.mMeta.mParentId = "";

		post.mMeta.mPublishTs = group.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (post.mMeta.mPublishTs > now)
			post.mMeta.mPublishTs = now - 1;

		mPosts.push_back(post);
		
	}

	for(i = 0; i < MAX_COMMENTS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsPostedPost head = mPosts.front();
			mPosts.pop_front();
			mPosts.push_back(head);
		}

		RsPostedPost parent = mPosts.front();

		/* now create a new child msg */

		RsPostedComment comment;

		/* fill in key data 
		 * GroupId
		 * MsgId
		 * OrigMsgId
		 * ThreadId
		 * ParentId
		 * PublishTS (take Forum TS + a bit ).
		 *
		 * ChildTS ????
		 */
		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => Comment_%d", parent.mMeta.mMsgName.c_str(), i+1);
		comment.mMeta.mMsgName = name;
		//comment.mMsg = name;

		comment.mMeta.mGroupId = parent.mMeta.mGroupId;
		comment.mMeta.mMsgId = genRandomId();
		comment.mMeta.mOrigMsgId = comment.mMeta.mMsgId;
		comment.mMeta.mThreadId = parent.mMeta.mThreadId;
		comment.mMeta.mParentId = parent.mMeta.mOrigMsgId;

		comment.mMeta.mPublishTs = parent.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (comment.mMeta.mPublishTs > now)
			comment.mMeta.mPublishTs = now - 1;

		mComments.push_back(comment);
	}


	for(i = 0; i < MAX_VOTES; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsPostedPost head = mPosts.front();
			mPosts.pop_front();
			mPosts.push_back(head);
		}

		RsPostedPost parent = mPosts.front();

		/* now create a new child msg */

		RsPostedVote vote;

		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => Vote_%d", parent.mMeta.mMsgName.c_str(), i+1);
		vote.mMeta.mMsgName = name;
		//vote.mMsg = name;

		vote.mMeta.mGroupId = parent.mMeta.mGroupId;
		vote.mMeta.mMsgId = genRandomId();
		vote.mMeta.mOrigMsgId = vote.mMeta.mMsgId;
		vote.mMeta.mThreadId = parent.mMeta.mThreadId;
		vote.mMeta.mParentId = parent.mMeta.mOrigMsgId;

		vote.mMeta.mPublishTs = parent.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (vote.mMeta.mPublishTs > now)
			vote.mMeta.mPublishTs = now - 1;

		mVotes.push_back(vote);
	}


	mUpdated = true;

	/* Then - at the end, we push them all into the Proxy */
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		/* pushback */
		mPostedProxy->addGroup(*git);

	}

	for(pit = mPosts.begin(); pit != mPosts.end(); pit++)
	{
		/* pushback */
		mPostedProxy->addPost(*pit);
	}

	for(cit = mComments.begin(); cit != mComments.end(); cit++)
	{
		/* pushback */
#define COMMENT_FRAC_FOR_LATER	(0.70)
		if (RSRandom::random_f32() > COMMENT_FRAC_FOR_LATER)
		{
			mPostedProxy->addComment(*cit);
		}
		else
		{
			mDummyLaterComments.push_back(*cit);
		}
	}


	for(vit = mVotes.begin(); vit != mVotes.end(); vit++)
	{
		/* pushback */

#define VOTE_FRAC_FOR_LATER	(0.70)
		if (RSRandom::random_f32() > VOTE_FRAC_FOR_LATER)
		{
			mPostedProxy->addVote(*vit);
		}
		else
		{
			mDummyLaterVotes.push_back(*vit);
		}
	}

	return true;
}

#define EXTRA_COMMENT_ADD	(20)
#define EXTRA_VOTE_ADD		(50)

bool p3PostedService::addExtraDummyData()
{
	std::cerr << "p3PostedService::addExtraDummyData()";
	std::cerr << std::endl;

	int i = 0;

	std::list<RsPostedVote>::iterator vit;
	std::list<RsPostedComment>::iterator cit;

	for(cit = mDummyLaterComments.begin(); (cit != mDummyLaterComments.end()) && (i < EXTRA_COMMENT_ADD); i++)
	{
		mPostedProxy->addComment(*cit);
		cit = mDummyLaterComments.erase(cit);
	}

	i = 0;
	for(vit = mDummyLaterVotes.begin(); (vit != mDummyLaterVotes.end()) && (i < EXTRA_VOTE_ADD); i++)
	{
		mPostedProxy->addVote(*vit);
		vit = mDummyLaterVotes.erase(vit);
	}

	return true;
}




/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/


/* This is the part that will be kept for the final Version.
 * we provide a processed view of the data...
 *
 * start off crude -> then make it efficient.
 */


bool p3PostedService::setViewMode(uint32_t mode)
{
     	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mViewMode = mode;

	return true;
}

bool p3PostedService::setViewPeriod(uint32_t period)
{
     	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mViewPeriod = period;

	return true;
}

bool p3PostedService::setViewRange(uint32_t first, uint32_t count)
{
     	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mViewStart = first;
	mViewCount = count;

	return true;
}

float p3PostedService::calcPostScore(const RsMsgMetaData &meta)
{
     	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	float score = 0;
	uint32_t votes = 0;
	uint32_t comments = 0;
	time_t now = time(NULL);
	time_t age_secs = now - meta.mPublishTs;
 	// This is a potential problem for gaming the system... post into the future.
	// Should fix this by discarding/hiding until ts valid XXX.
	if (age_secs < 0)
	{
		age_secs = 0; 
	}

	if (!extractPostedCache(meta.mServiceString, votes, comments))
	{
		/* no votes/comments yet */
	}

	/* this is dependent on View Mode */
	switch(mViewMode)
	{
		default:
		case RSPOSTED_VIEWMODE_LATEST:
		{
			score = -age_secs; // 

			break;
		}
		case RSPOSTED_VIEWMODE_TOP:
		{
			score = votes;
			break;
		}
// Potentially only 
// This is effectively HackerNews Algorithm: which is (p-1)/(t+2)^1.5, where p is votes and t is age in hours.
		case RSPOSTED_VIEWMODE_HOT:
		{
#define POSTED_AGESHIFT (2.0)
#define POSTED_AGEFACTOR (3600.0)
			score = votes / pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
			break;
		}
// Like HOT, but using number of Comments.
		case RSPOSTED_VIEWMODE_COMMENTS:
		{
			score = comments / pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
			break;
		}
	}

	return score;
}

static uint32_t convertPeriodFlagToSeconds(uint32_t periodMode)
{
	float secs = 1;
	switch(periodMode)
	{
		// Fallthrough all of them.
		case RSPOSTED_PERIOD_YEAR:
			secs *= 12;
		case RSPOSTED_PERIOD_MONTH:
			secs *= 4.3;  // average ~30.4 days = 4.3 weeks.
		case RSPOSTED_PERIOD_WEEK:
			secs *= 7;
		case RSPOSTED_PERIOD_DAY:
			secs *= 24;
		case RSPOSTED_PERIOD_HOUR:
			secs *= 3600;
	}

	return (uint32_t) secs;
}

#define POSTED_RANKINGS_INITIAL_CHECK	1
#define POSTED_RANKINGS_DATA_REQUEST	2
#define POSTED_RANKINGS_DATA_DONE	3

        /* Poll */
uint32_t p3PostedService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;

	uint32_t int_token = token;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if ((mProcessingRanking) && (token == mRankingExternalToken))
		{
			{
				switch(mRankingState)
				{
					case POSTED_RANKINGS_INITIAL_CHECK:
						status = GXS_REQUEST_STATUS_PENDING;
						return status;
						break;
					case POSTED_RANKINGS_DATA_REQUEST:
						// Switch to real token.
						int_token = mRankingInternalToken;
						break;
				}
			}
		}
	}
	
	checkRequestStatus(int_token, status, reqtype, anstype, ts);

	return status;
}

bool p3PostedService::getRankedPost(const uint32_t &token, RsPostedPost &post)
{
	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
	if (!mProcessingRanking)
	{

		return false;
	}

	if (token != mRankingExternalToken)
	{


		return false;
	}

	if (mRankingState != POSTED_RANKINGS_DATA_REQUEST)
	{

		return false;

	}

	
	if (!getPost(mRankingInternalToken, post))
	{
		/* clean up */
		mProcessingRanking = false;
		mRankingExternalToken = 0;
		mRankingInternalToken = 0;
		mRankingState = POSTED_RANKINGS_DATA_DONE;

		return false;
	}

	return true;
}


bool p3PostedService::requestRanking(uint32_t &token, std::string groupId)
{
	std::cerr << "p3PostedService::requestRanking()";
	std::cerr << std::endl;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (mProcessingRanking)
		{
			std::cerr << "p3PostedService::requestRanking() ERROR Request already running - ignoring";
			std::cerr << std::endl;

			return false;
		}
	}

	generateToken(token);

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mProcessingRanking = true;
		mRankingState = POSTED_RANKINGS_INITIAL_CHECK;
		mRankingExternalToken = token;
	}

	/* now we request all the posts within the timeframe */

	uint32_t posttoken; 
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; 

	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;
	//uint32_t age = convertPeriodFlagToSeconds(mViewPeriod);

	std::list<std::string> groupIds;
	groupIds.push_back(groupId);

	requestMsgInfo(posttoken, ansType, opts, groupIds);

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mRankingInternalToken = posttoken;
	}
	return true;
}

bool p3PostedService::checkRankingRequest()
{
	uint32_t token = 0;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (!mProcessingRanking)
		{
			return false;
		}

		if (mRankingState != POSTED_RANKINGS_INITIAL_CHECK)
		{
			return false;
		}

		/* here it actually running! */
		token = mRankingInternalToken;
	}


	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	
	if (status == GXS_REQUEST_STATUS_COMPLETE)
	{
		processPosts();
	}
	return true;
}


bool p3PostedService::processPosts()
{
	std::cerr << "p3PostedService::processPosts()";
	std::cerr << std::endl;

	uint32_t token = 0;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (!mProcessingRanking)
		{
			std::cerr << "p3PostedService::processPosts() ERROR Ranking Request not running";
			std::cerr << std::endl;

			return false;
		}

		if (mRankingState != POSTED_RANKINGS_INITIAL_CHECK)
		{
			std::cerr << "p3PostedService::processPosts() ERROR Ranking Request not running";
			std::cerr << std::endl;

			return false;
		}
		token = mRankingInternalToken;
	}

	/* extract the info -> and sort */
	std::list<RsMsgMetaData> postList;
	std::list<RsMsgMetaData>::const_iterator it;

	if (!getMsgSummary(token, postList))
	{
		std::cerr << "p3PostedService::processPosts() ERROR getting postList";
		std::cerr << std::endl;
		return false;
	}

	std::multimap<float, std::string> postMap;
	std::multimap<float, std::string>::reverse_iterator mit;

	for(it = postList.begin(); it != postList.end(); it++)
	{
		float score = calcPostScore(*it);
		postMap.insert(std::make_pair(score, it->mMsgId));
	}

	/* now grab the N required, and request the data again...
	 * -> this is what will be passed back to GUI
	 */

	std::list<std::string> msgList;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		unsigned int i = 0;
		for(mit = postMap.rbegin(); (mit != postMap.rend()) && (i < mViewStart); mit++, i++)
		{
			std::cerr << "p3PostedService::processPosts() Skipping PostId: " << mit->second;
			std::cerr << " with score: " << mit->first;
			std::cerr << std::endl;
		}

	
		for(i = 0; (mit != postMap.rend()) && (i < mViewCount); mit++, i++)
		{
			std::cerr << "p3PostedService::processPosts() Adding PostId: " << mit->second;
			std::cerr << " with score: " << mit->first;
			std::cerr << std::endl;
			msgList.push_back(mit->second);
		}
	}

	token = 0; 
	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts; 

	requestMsgRelatedInfo(token, ansType, opts, msgList);

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mRankingState = POSTED_RANKINGS_DATA_REQUEST;
		mRankingInternalToken = token;
	}
	return true;
}



/***** Background Processing ****
 *
 * Process Each Message - as it arrives.
 *
 * Update 
 *
 */

#define POSTED_BG_REQUEST_GROUPS		1
#define POSTED_BG_REQUEST_UNPROCESSED		2
#define POSTED_BG_REQUEST_PARENTS		3
#define POSTED_BG_PROCESS_VOTES			4

bool p3PostedService::background_checkTokenRequest()
{
	uint32_t token = 0;
	uint32_t phase = 0;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (!mBgProcessing)
		{
			return false;
		}

		token = mBgToken;
		phase = mBgPhase;
	}


	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	
	if (status == GXS_REQUEST_STATUS_COMPLETE)
	{
		switch(phase)
		{
			case POSTED_BG_REQUEST_GROUPS:
				background_requestNewMessages();
				break;
			case POSTED_BG_REQUEST_UNPROCESSED:
				background_processNewMessages();
				break;
			case POSTED_BG_REQUEST_PARENTS:
				background_updateVoteCounts();
				break;
			default:
				break;
		}
	}
	return true;
}


bool p3PostedService::background_requestGroups()
{
	std::cerr << "p3PostedService::background_requestGroups()";
	std::cerr << std::endl;

	// grab all the subscribed groups.
	uint32_t token = 0;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

		mBgProcessing = true;
		mBgPhase = POSTED_BG_REQUEST_GROUPS;
		mBgToken = 0;
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST;
	RsTokReqOptions opts; 
	std::list<std::string> groupIds;

	opts.mSubscribeFilter = RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED;

	requestGroupInfo(token, ansType, opts, groupIds);
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}

	return true;
}


bool p3PostedService::background_requestNewMessages()
{
	std::cerr << "p3PostedService::background_requestNewMessages()";
	std::cerr << std::endl;

	std::list<std::string> groupIds;
	uint32_t token = 0;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getGroupList(token, groupIds))
	{
		std::cerr << "p3PostedService::background_requestNewMessages() ERROR No Group List";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = POSTED_BG_REQUEST_UNPROCESSED;
		mBgToken = 0;
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY; 
	RsTokReqOptions opts; 
	token = 0;

	opts.mStatusFilter = RSGXS_MSG_STATUS_UNPROCESSED;
	opts.mStatusMask = RSGXS_MSG_STATUS_UNPROCESSED;

	requestMsgInfo(token, ansType, opts, groupIds);

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}


bool p3PostedService::background_processNewMessages()
{
	std::cerr << "p3PostedService::background_processNewMessages()";
	std::cerr << std::endl;

	std::list<RsMsgMetaData> newMsgList;
	std::list<RsMsgMetaData>::iterator it;
	uint32_t token = 0;

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getMsgSummary(token, newMsgList))
	{
		std::cerr << "p3PostedService::background_processNewMessages() ERROR No New Msgs";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = POSTED_BG_REQUEST_PARENTS;
		mBgToken = 0;
		mBgVoteMap.clear();
		mBgCommentMap.clear();
	}

	/* loop through and sort by parents.
	 *  - grab 
	 */

	std::list<std::string> parentList;

	std::map<std::string, uint32_t>::iterator vit;

	for(it = newMsgList.begin(); it != newMsgList.end(); it++)
	{
		std::cerr << "Found New MsgId: " << it->mMsgId;
		std::cerr << std::endl;

		/* discard threadheads */
		if (it->mParentId.empty())
		{
			std::cerr << "\tIgnoring ThreadHead: " << *it;
			std::cerr << std::endl;
		}
		else if (it->mMsgFlags & RSPOSTED_MSGTYPE_COMMENT)
		{
			/* Comments are counted by Thread Id */
			std::cerr << "\tProcessing Comment: " << *it;
			std::cerr << std::endl;

			vit = mBgCommentMap.find(it->mThreadId);
			if (vit == mBgCommentMap.end())
			{
				mBgCommentMap[it->mThreadId] = 1;

				/* check VoteMap too before adding to parentList */
				if (mBgVoteMap.end() == mBgVoteMap.find(it->mThreadId))
				{
					parentList.push_back(it->mThreadId);
				}
	
				std::cerr << "\tThreadId: " << it->mThreadId;
				std::cerr << " Comment Total: " << mBgCommentMap[it->mThreadId];
				std::cerr << std::endl;
			}
			else
			{
				mBgVoteMap[it->mThreadId]++;
				std::cerr << "\tThreadId: " << it->mThreadId;
				std::cerr << " Comment Total: " << mBgCommentMap[it->mThreadId];
				std::cerr << std::endl;
			}
		}
		else if (it->mMsgFlags & RSPOSTED_MSGTYPE_VOTE)
		{
			/* Votes are organised by Parent Id,
			 * ie. you can vote for both Posts and Comments
			 */
			std::cerr << "\tProcessing Vote: " << *it;
			std::cerr << std::endl;

			vit = mBgVoteMap.find(it->mParentId);
			if (vit == mBgVoteMap.end())
			{
				mBgVoteMap[it->mParentId] = 1;

				/* check CommentMap too before adding to parentList */
				if (mBgCommentMap.end() == mBgCommentMap.find(it->mParentId))
				{
					parentList.push_back(it->mParentId);
				}
	
				std::cerr << "\tParentId: " << it->mParentId;
				std::cerr << " Vote Total: " << mBgVoteMap[it->mParentId];
				std::cerr << std::endl;
			}
			else
			{
				mBgVoteMap[it->mParentId]++;
				std::cerr << "\tParentId: " << it->mParentId;
				std::cerr << " Vote Total: " << mBgVoteMap[it->mParentId];
				std::cerr << std::endl;
			}
		}
		else
		{
			/* unknown! */
			std::cerr << "p3PostedService::background_processNewMessages() ERROR Strange NEW Message:";
			std::cerr << std::endl;
			std::cerr << "\t" << *it;
			std::cerr << std::endl;

		}
	
		/* flag each new vote as processed */
		setMessageStatus(it->mMsgId, 0, RSGXS_MSG_STATUS_UNPROCESSED);
	}


	/* request the summary info from the parents */
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY; 
	token = 0;
	RsTokReqOptions opts; 
	requestMsgRelatedInfo(token, ansType, opts, parentList);

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}


bool p3PostedService::encodePostedCache(std::string &str, uint32_t votes, uint32_t comments)
{
	char line[RSGXS_MAX_SERVICE_STRING];

	snprintf(line, RSGXS_MAX_SERVICE_STRING, "%d %d", votes, comments);

	str = line;
	return true;
}

bool p3PostedService::extractPostedCache(const std::string &str, uint32_t &votes, uint32_t &comments)
{

	uint32_t ivotes, icomments;
	if (2 == sscanf(str.c_str(), "%d %d", &ivotes, &icomments))
	{
		votes = ivotes;
		comments = icomments;
		return true;
	}

	return false;
}


bool p3PostedService::background_updateVoteCounts()
{
	std::cerr << "p3PostedService::background_updateVoteCounts()";
	std::cerr << std::endl;

	std::list<RsMsgMetaData> parentMsgList;
	std::list<RsMsgMetaData>::iterator it;

	if (!getMsgSummary(mBgToken, parentMsgList))
	{
		std::cerr << "p3PostedService::background_updateVoteCounts() ERROR";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = POSTED_BG_PROCESS_VOTES;
		mBgToken = 0;
	}

	for(it = parentMsgList.begin(); it != parentMsgList.end(); it++)
	{
		/* extract current vote count */
		uint32_t votes = 0;
		uint32_t comments = 0;

		if (!extractPostedCache(it->mServiceString, votes, comments))
		{
			if (!(it->mServiceString.empty()))
			{
				std::cerr << "p3PostedService::background_updateVoteCounts() Failed to extract Votes";
				std::cerr << std::endl;
				std::cerr << "\tFrom String: " << it->mServiceString;
				std::cerr << std::endl;
			}
		}

		/* find increment in votemap */
		{
			RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

			std::map<std::string, uint32_t>::iterator vit;
			vit = mBgVoteMap.find(it->mMsgId);
			if (vit != mBgVoteMap.end())
			{
				votes += vit->second;
			}
			else
			{
				// warning.
				std::cerr << "p3PostedService::background_updateVoteCounts() Warning No New Votes found.";
				std::cerr << " For MsgId: " << it->mMsgId;
				std::cerr << std::endl;
			}

		}

		{
			RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

			std::map<std::string, uint32_t>::iterator cit;
			cit = mBgCommentMap.find(it->mMsgId);
			if (cit != mBgCommentMap.end())
			{
				comments += cit->second;
			}
			else
			{
				// warning.
				std::cerr << "p3PostedService::background_updateVoteCounts() Warning No New Comments found.";
				std::cerr << " For MsgId: " << it->mMsgId;
				std::cerr << std::endl;
			}

		}

		std::string str;
		if (!encodePostedCache(str, votes, comments))
		{
			std::cerr << "p3PostedService::background_updateVoteCounts() Failed to encode Votes";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "p3PostedService::background_updateVoteCounts() Encoded String: " << str;
			std::cerr << std::endl;
			/* store new result */
			setMessageServiceString(it->mMsgId, str);
		}
	}

	// DONE!.
	background_cleanup();
	return true;

}


bool p3PostedService::background_cleanup()
{
	std::cerr << "p3PostedService::background_cleanup()";
	std::cerr << std::endl;

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	// Cleanup.
	mBgVoteMap.clear();
	mBgCommentMap.clear();
	mBgProcessing = false;
	mBgToken = 0;

	return true;
}


