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
	}

	generateDummyData();
	return;
}


int	p3PostedService::tick()
{
	//std::cerr << "p3PostedService::tick()";
	//std::cerr << std::endl;

	fakeprocessrequests();
	
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
	
	/* convert to RsPhotoAlbum */
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
uint32_t p3PostedService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3PostedService::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////
        /* Functions from Forums -> need to be implemented generically */
bool p3PostedService::groupsChanged(std::list<std::string> &groupIds)
{
	return false;
}

        // Get Message Status - is retrived via MessageSummary.
bool p3PostedService::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return false;
}


        // 
bool p3PostedService::groupSubscribe(const std::string &groupId, bool subscribe)
{
	return false;
}


bool p3PostedService::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3PostedService::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


bool p3PostedService::submitGroup(RsPostedGroup &group, bool isNew)
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

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mPostedProxy->addGroup(group);

	return true;
}


bool p3PostedService::submitPost(RsPostedPost &post, bool isNew)
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

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	mPostedProxy->addPost(post);

	return true;
}



bool p3PostedService::submitVote(RsPostedVote &vote, bool isNew)
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

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	mPostedProxy->addVote(vote);

	return true;
}



bool p3PostedService::submitComment(RsPostedComment &comment, bool isNew)
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

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	mPostedProxy->addComment(comment);

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

		if (pM->postedType == RSPOSTED_MSG_POST)
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
		
		if (pM->postedType == RSPOSTED_MSG_VOTE)
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
		
		if (pM->postedType == RSPOSTED_MSG_COMMENT)
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
#define MAX_COMMENTS 100 //10000
#define MAX_VOTES 100 //10000

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
			group.mMeta.mSubscribeFlags = RS_DISTRIB_ADMIN;

		}
		else if (rnd < 0.3)
		{
			group.mMeta.mSubscribeFlags = RS_DISTRIB_SUBSCRIBED;
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
		mPostedProxy->addComment(*cit);
	}


	for(vit = mVotes.begin(); vit != mVotes.end(); vit++)
	{
		/* pushback */
		mPostedProxy->addVote(*vit);
	}

	return true;
}


/********************************************************************************************/
/********************************************************************************************/
