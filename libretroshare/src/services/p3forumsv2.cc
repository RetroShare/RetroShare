/*
 * libretroshare/src/services p3forumsv2.cc
 *
 * ForumsV2 interface for RetroShare.
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

#include "services/p3forumsv2.h"

#include "util/rsrandom.h"
#include <stdio.h>

/****
 * #define FORUMV2_DEBUG 1
 ****/

RsForumsV2 *rsForumsV2 = NULL;



/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3ForumsV2::p3ForumsV2(uint16_t type)
	:p3GxsDataService(type, new ForumDataProxy()), mForumMtx("p3ForumsV2"), mUpdated(true)
{
	{
     		RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

		mForumProxy = (ForumDataProxy *) mProxy;
	}

	generateDummyData();

	return;
}


int	p3ForumsV2::tick()
{
	std::cerr << "p3ForumsV2::tick()";
	std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool p3ForumsV2::updated()
{
	RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



       /* Data Requests */
bool p3ForumsV2::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3ForumsV2::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3ForumsV2::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3ForumsV2::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3ForumsV2::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3ForumsV2::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3ForumsV2::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3ForumsV2::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3ForumsV2::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3ForumsV2::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3ForumsV2::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3ForumsV2::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3ForumsV2::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3ForumsV2::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3ForumsV2::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3ForumsV2::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3ForumsV2::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3ForumsV2::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getMsgSummary() ERROR Status Incomplete" << std::endl;
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
bool p3ForumsV2::getGroupData(const uint32_t &token, RsForumV2Group &group)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3ForumsV2::getGroupData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3ForumsV2::getGroupData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsForumAlbum */
	bool ans = mForumProxy->getForumGroup(id, group);
	return ans;
}


bool p3ForumsV2::getMsgData(const uint32_t &token, RsForumV2Msg &msg)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3ForumsV2::getMsgData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3ForumsV2::getMsgData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3ForumsV2::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsForumAlbum */
	bool ans = mForumProxy->getForumMsg(id, msg);
	return ans;
}



        /* Poll */
uint32_t p3ForumsV2::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3ForumsV2::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////

bool p3ForumsV2::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return mForumProxy->setMessageStatus(msgId, status, statusMask);
}

bool p3ForumsV2::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
	return mForumProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3ForumsV2::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
	return mForumProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3ForumsV2::setMessageServiceString(const std::string &msgId, const std::string &str)
{
	return mForumProxy->setMessageServiceString(msgId, str);
}

bool p3ForumsV2::setGroupServiceString(const std::string &grpId, const std::string &str)
{
	return mForumProxy->setGroupServiceString(grpId, str);
}



bool p3ForumsV2::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3ForumsV2::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}




/********************************************************************************************/

	
std::string p3ForumsV2::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
bool p3ForumsV2::createGroup(uint32_t &token, RsForumV2Group &group, bool isNew)
{
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3ForumsV2::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}

	{	
		RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mForumProxy->addForumGroup(group);
	}
	
	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3ForumsV2::createGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);
	
	return true;
}




bool p3ForumsV2::createMsg(uint32_t &token, RsForumV2Msg &msg, bool isNew)
{
	if (msg.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3ForumsV2::createForumMsg() Missing MsgID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new msg */
	if (msg.mMeta.mOrigMsgId.empty())
	{
		std::cerr << "p3ForumsV2::createForumMsg() New Msg";
		std::cerr << std::endl;

		/* new msg, generate a new OrigMsgId */
		msg.mMeta.mOrigMsgId = genRandomId();
		msg.mMeta.mMsgId = msg.mMeta.mOrigMsgId;
	}
	else
	{
		std::cerr << "p3ForumsV2::createForumMsg() Modified Msg";
		std::cerr << std::endl;

		/* mod msg, keep orig msg id, generate a new MsgId */
		msg.mMeta.mMsgId = genRandomId();
	}

	std::cerr << "p3ForumsV2::createForumMsg() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3ForumsV2::createForumMsg() MsgId: " << msg.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "p3ForumsV2::createForumMsg() OrigMsgId: " << msg.mMeta.mOrigMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mForumProxy->addForumMsg(msg);
	}
	
	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(msg.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3ForumsV2::createMsg() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/

	
bool ForumDataProxy::getForumGroup(const std::string &id, RsForumV2Group &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsForumV2Group *pG = (RsForumV2Group *) groupData;
		group = *pG;

		// update definitive version of the metadata.
		group.mMeta = meta;

		std::cerr << "ForumDataProxy::getForumGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "ForumDataProxy::getForumGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool ForumDataProxy::getForumMsg(const std::string &id, RsForumV2Msg &page)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsForumV2Msg *pP = (RsForumV2Msg *) msgData;
		// Shallow copy of thumbnail.
		page = *pP;
	
		// update definitive version of the metadata.
		page.mMeta = meta;

		std::cerr << "ForumDataProxy::getForumMsg() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "ForumDataProxy::getForumMsg() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool ForumDataProxy::addForumGroup(const RsForumV2Group &group)
{
	// Make duplicate.
	RsForumV2Group *pG = new RsForumV2Group();
	*pG = group;

	std::cerr << "ForumDataProxy::addForumGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool ForumDataProxy::addForumMsg(const RsForumV2Msg &msg)
{
	// Make duplicate.
	RsForumV2Msg *pM = new RsForumV2Msg();
	*pM = msg;

	std::cerr << "ForumDataProxy::addForumMsg()";
	std::cerr << " MetaData: " << pM->mMeta << " DataPointer: " << pM;
	std::cerr << std::endl;

	return createMsg(pM);
}



        /* These Functions must be overloaded to complete the service */
bool ForumDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsForumV2Group *group = (RsForumV2Group *) groupData;
	meta = group->mMeta;

	return true;
}

bool ForumDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsForumV2Msg *page = (RsForumV2Msg *) msgData;
	meta = page->mMeta;

	return true;
}


/********************************************************************************************/



bool p3ForumsV2::generateDummyData()
{
	/* so we want to generate 100's of forums */
#define MAX_FORUMS 10 //100
#define MAX_THREADS 10 //1000
#define MAX_MSGS 100 //10000

	std::list<RsForumV2Group> mGroups;
	std::list<RsForumV2Group>::iterator git;

	std::list<RsForumV2Msg> mMsgs;
	std::list<RsForumV2Msg>::iterator mit;

#define DUMMY_NAME_MAX_LEN		10000
	char name[DUMMY_NAME_MAX_LEN];
	int i, j;
	time_t now = time(NULL);

	for(i = 0; i < MAX_FORUMS; i++)
	{
		/* generate a new forum */
		RsForumV2Group forum;

		/* generate a temp id */
		forum.mMeta.mGroupId = genRandomId();

		snprintf(name, DUMMY_NAME_MAX_LEN, "TestForum_%d", i+1);

		forum.mMeta.mGroupId = genRandomId();
		forum.mMeta.mGroupName = name;

		forum.mMeta.mPublishTs = now - (RSRandom::random_f32() * 100000);
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
			forum.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_ADMIN;

		}
		else if (rnd < 0.3)
		{
			forum.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED;
		}
		else
		{
			forum.mMeta.mSubscribeFlags = 0;
		}

		forum.mMeta.mPop = (int) (RSRandom::random_f32() * 10.0);

		mGroups.push_back(forum);


		//std::cerr << "p3ForumsV2::generateDummyData() Generated Forum: " << forum.mMeta;
		//std::cerr << std::endl;
	}


	for(i = 0; i < MAX_THREADS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsForumV2Group head = mGroups.front();
			mGroups.pop_front();
			mGroups.push_back(head);
		}

		RsForumV2Group forum = mGroups.front();

		/* now create a new thread */

		RsForumV2Msg msg;

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
		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => ThreadMsg_%d", forum.mMeta.mGroupName.c_str(), i+1);
		msg.mMeta.mMsgName = name;

		msg.mMeta.mGroupId = forum.mMeta.mGroupId;
		msg.mMeta.mMsgId = genRandomId();
		msg.mMeta.mOrigMsgId = msg.mMeta.mMsgId;
		msg.mMeta.mThreadId = msg.mMeta.mMsgId;
		msg.mMeta.mParentId = "";

		msg.mMeta.mPublishTs = forum.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (msg.mMeta.mPublishTs > now)
			msg.mMeta.mPublishTs = now - 1;

		mMsgs.push_back(msg);

		//std::cerr << "p3ForumsV2::generateDummyData() Generated Thread: " << msg.mMeta;
		//std::cerr << std::endl;
		
	}

	for(i = 0; i < MAX_MSGS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsForumV2Msg head = mMsgs.front();
			mMsgs.pop_front();
			mMsgs.push_back(head);
		}

		RsForumV2Msg parent = mMsgs.front();

		/* now create a new child msg */

		RsForumV2Msg msg;

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
		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => Msg_%d", parent.mMeta.mMsgName.c_str(), i+1);
		msg.mMeta.mMsgName = name;
		msg.mMsg = name;

		msg.mMeta.mGroupId = parent.mMeta.mGroupId;
		msg.mMeta.mMsgId = genRandomId();
		msg.mMeta.mOrigMsgId = msg.mMeta.mMsgId;
		msg.mMeta.mThreadId = parent.mMeta.mThreadId;
		msg.mMeta.mParentId = parent.mMeta.mOrigMsgId;

		msg.mMeta.mPublishTs = parent.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (msg.mMeta.mPublishTs > now)
			msg.mMeta.mPublishTs = now - 1;

		mMsgs.push_back(msg);

		//std::cerr << "p3ForumsV2::generateDummyData() Generated Child Msg: " << msg.mMeta;
		//std::cerr << std::endl;

	}


	mUpdated = true;

	/* Then - at the end, we push them all into the Proxy */
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		/* pushback */
		mForumProxy->addForumGroup(*git);

	}

	for(mit = mMsgs.begin(); mit != mMsgs.end(); mit++)
	{
		/* pushback */
		mForumProxy->addForumMsg(*mit);
	}

	return true;
}

