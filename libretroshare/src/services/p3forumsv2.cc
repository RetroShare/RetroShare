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
     	RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

	mForumProxy = (ForumDataProxy *) mProxy;
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
        /* Functions from Forums -> need to be implemented generically */
bool p3ForumsV2::groupsChanged(std::list<std::string> &groupIds)
{
	return false;
}

        // Get Message Status - is retrived via MessageSummary.
bool p3ForumsV2::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return false;
}


        // 
bool p3ForumsV2::groupSubscribe(const std::string &groupId, bool subscribe)
{
	return false;
}


bool p3ForumsV2::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3ForumsV2::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}

#if 0
/* details are updated in album - to choose Album ID, and storage path */
bool p3ForumsV2::submitAlbumDetails(RsForumAlbum &album)
{
	return false;
}

bool p3ForumsV2::submitForum(RsForumForum &photo)
{
	return false;
}

#endif




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
	
bool p3ForumsV2::createGroup(RsForumV2Group &group)
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
	
	RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	mForumProxy->addForumGroup(group);

	return true;
}




bool p3ForumsV2::createMsg(RsForumV2Msg &msg)
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

	RsStackMutex stack(mForumMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	mForumProxy->addForumMsg(msg);

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


#if 0

bool p3ForumsV2::generateDummyData()
{
	/* so we want to generate 100's of forums */
#define MAX_FORUMS 100
#define MAX_THREADS 1000
#define MAX_MSGS 10000

	std::list<RsForumV2Group> mGroups;
	std::list<RsForumV2Group>::iterator git;

	std::list<RsForumV2Msg> mMsgs;
	std::list<RsForumV2Msg>::iterator mit;

	for(int i = 0; i < MAX_FORUMS; i++)
	{
		/* generate a new forum */
		RsForumV2Group forum;

		/* key fields to fill in:
		 * GroupId.
		 * Name.
		 * Flags.
		 * Pop.
		 */

		/* use probability to decide which are subscribed / own / popularity.
		 */


		mGroups.push_back(forum);
	}


	for(i = 0; i < MAX_THREADS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd;

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

		mMsgs.push_back(msg);
		
	}

	for(i = 0; i < MAX_MSGS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd;

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


		
	}



	/* Then - at the end, we push them all into the Proxy */
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		/* pushback */
	}

	for(mit = mMsgs.begin(); mit != mMsgs.end(); mit++)
	{
		/* pushback */
	}

}

#endif

