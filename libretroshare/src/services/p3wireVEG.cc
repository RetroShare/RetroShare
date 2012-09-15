/*
 * libretroshare/src/services p3wire.cc
 *
 * Wire interface for RetroShare.
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

#include "services/p3wireVEG.h"

#include "util/rsrandom.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWireVEG *rsWireVEG = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3WireVEG::p3WireVEG(uint16_t type)
	:p3GxsDataServiceVEG(type, new WireDataProxy()), mWireMtx("p3Wire"), mUpdated(true)
{
     	RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

	mWireProxy = (WireDataProxy *) mProxy;
	return;
}


int	p3WireVEG::tick()
{
	//std::cerr << "p3WireVEG::tick()";
	//std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool p3WireVEG::updated()
{
	RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



       /* Data Requests */
bool p3WireVEG::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3WireVEG::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3WireVEG::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3WireVEG::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3WireVEG::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3WireVEG::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3WireVEG::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3WireVEG::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WireVEG::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3WireVEG::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3WireVEG::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WireVEG::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3WireVEG::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3WireVEG::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WireVEG::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3WireVEG::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3WireVEG::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WireVEG::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getMsgSummary() ERROR Status Incomplete" << std::endl;
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
bool p3WireVEG::getGroupData(const uint32_t &token, RsWireGroup &group)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3WireVEG::getGroupData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WireVEG::getGroupData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWireGroup */
	bool ans = mWireProxy->getGroup(id, group);
	return ans;
}


bool p3WireVEG::getMsgData(const uint32_t &token, RsWirePulse &pulse)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3WireVEG::getMsgData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WireVEG::getMsgData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WireVEG::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWirePulse */
	bool ans = mWireProxy->getPulse(id, pulse);
	return ans;
}



        /* Poll */
uint32_t p3WireVEG::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3WireVEG::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////

bool p3WireVEG::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
        return mWireProxy->setMessageStatus(msgId, status, statusMask);
}


bool p3WireVEG::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
        return mWireProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3WireVEG::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
	return mWireProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3WireVEG::setMessageServiceString(const std::string &msgId, const std::string &str)
{
        return mWireProxy->setMessageServiceString(msgId, str);
}

bool p3WireVEG::setGroupServiceString(const std::string &grpId, const std::string &str)
{
        return mWireProxy->setGroupServiceString(grpId, str);
}


bool p3WireVEG::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3WireVEG::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


/********************************************************************************************/

	
std::string p3WireVEG::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	

bool p3WireVEG::createGroup(uint32_t &token, RsWireGroup &group, bool isNew)
{
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3WireVEG::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}

	{	
		RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWireProxy->addGroup(group);
	}
	
	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptionsVEG opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3Wiree::createGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);
	
	return true;
}




bool p3WireVEG::createPulse(uint32_t &token, RsWirePulse &pulse, bool isNew)
{
	if (pulse.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3WireVEG::createPulse() Missing PulseID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new pulse */
	if (pulse.mMeta.mOrigMsgId.empty())
	{
		std::cerr << "p3WireVEG::createPulse() New Pulse";
		std::cerr << std::endl;

		/* new pulse, generate a new OrigPulseId */
		pulse.mMeta.mOrigMsgId = genRandomId();
		pulse.mMeta.mMsgId = pulse.mMeta.mOrigMsgId;
	}
	else
	{
		std::cerr << "p3WireVEG::createPulse() Modified Pulse";
		std::cerr << std::endl;

		/* mod pulse, keep orig pulse id, generate a new PulseId */
		pulse.mMeta.mMsgId = genRandomId();
	}

	std::cerr << "p3WireVEG::createPulse() GroupId: " << pulse.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3WireVEG::createPulse() PulseId: " << pulse.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "p3WireVEG::createPulse() OrigPulseId: " << pulse.mMeta.mOrigMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWireProxy->addPulse(pulse);
	}
	
	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptionsVEG opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(pulse.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3WireVEG::createPulse() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/


	
bool WireDataProxy::getGroup(const std::string &id, RsWireGroup &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsWireGroup *pG = (RsWireGroup *) groupData;
		group = *pG;

		// update definitive version of the metadata.
		group.mMeta = meta;

		std::cerr << "WireDataProxy::getGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WireDataProxy::getGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WireDataProxy::getPulse(const std::string &id, RsWirePulse &pulse)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsWirePulse *pP = (RsWirePulse *) msgData;
		// Shallow copy of thumbnail.
		pulse = *pP;
	
		// update definitive version of the metadata.
		pulse.mMeta = meta;

		std::cerr << "WireDataProxy::getPulse() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WireDataProxy::getPulse() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WireDataProxy::addGroup(const RsWireGroup &group)
{
	// Make duplicate.
	RsWireGroup *pG = new RsWireGroup();
	*pG = group;

	std::cerr << "WireDataProxy::addGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool WireDataProxy::addPulse(const RsWirePulse &pulse)
{
	// Make duplicate.
	RsWirePulse *pP = new RsWirePulse();
	*pP = pulse;

	std::cerr << "WireDataProxy::addPulse()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}



        /* These Functions must be overloaded to complete the service */
bool WireDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsWireGroup *group = (RsWireGroup *) groupData;
	meta = group->mMeta;

	return true;
}

bool WireDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsWirePulse *page = (RsWirePulse *) msgData;
	meta = page->mMeta;

	return true;
}



/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


#if 0

bool p3WireVEG::generateDummyData()
{
#define MAX_GROUPS 100
#define MAX_POSTS  1000

#define MAX_BASE_COMMENTS 1000 //10000
#define MAX_COMMENTS 4000 //10000

#define MAX_VOTES 10000 //10000

	std::list<RsWireGroup> mGroups;
	std::list<RsWireGroup>::iterator git;

	std::list<RsWirePulse> mPosts;
	std::list<RsWirePulse>::iterator pit;

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

	for(i = 0; i < MAX_BASE_COMMENTS; i++)
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


	for(i = 0; i < MAX_COMMENTS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsPostedComment head = mComments.front();
			mComments.pop_front();
			mComments.push_back(head);
		}

		RsPostedComment parent = mComments.front();

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

#endif


