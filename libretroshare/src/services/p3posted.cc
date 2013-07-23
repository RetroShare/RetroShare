/*
 * libretroshare/src/services p3posted.cc
 *
 * Posted interface for RetroShare.
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
#include "serialiser/rsposteditems.h"

#include <retroshare/rsidentity.h>


#include "retroshare/rsgxsflags.h"
#include <stdio.h>
#include <math.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define POSTED_DEBUG 1
 ****/
#define POSTED_DEBUG 1

RsPosted *rsPosted = NULL;

//const uint32_t RsPosted::FLAG_MSGTYPE_POST = 0x0001;
//const uint32_t RsPosted::FLAG_MSGTYPE_MASK = 0x000f;


#define POSTED_TESTEVENT_DUMMYDATA	0x0001
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

#define POSTED_BACKGROUND_PROCESSING	0x0002
#define PROCESSING_START_PERIOD		30
#define PROCESSING_INC_PERIOD		15

#define POSTED_ALL_GROUPS 		0x0011
#define POSTED_UNPROCESSED_MSGS		0x0012
#define POSTED_ALL_MSGS 		0x0013
#define POSTED_BG_POST_META		0x0014
/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsPostedSerialiser(), RS_SERVICE_GXSV2_TYPE_POSTED, gixs, postedAuthenPolicy()), RsPosted(this), GxsTokenQueue(this), RsTickEvent(), mPostedMtx("PostedMtx")
{
	mBgProcessing = false;

	// For Dummy Msgs.
	mGenActive = false;
	mCommentService = new p3GxsCommentService(this,  RS_SERVICE_GXSV2_TYPE_POSTED);

	// Test Data disabled in repo.
	//RsTickEvent::schedule_in(POSTED_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);

	RsTickEvent::schedule_in(POSTED_BACKGROUND_PROCESSING, PROCESSING_START_PERIOD);
}



uint32_t p3Posted::postedAuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = 0;

	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}





void p3Posted::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	std::cerr << "p3Posted::notifyChanges()";
	std::cerr << std::endl;

	std::vector<RsGxsNotify *> changesForGUI;
	std::vector<RsGxsNotify *>::iterator it;

	for(it = changes.begin(); it != changes.end(); it++)
	{
	       RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(*it);
	       RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
	       if (msgChange)
	       {
			std::cerr << "p3Posted::notifyChanges() Found Message Change Notification";
			std::cerr << std::endl;

			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
			for(mit = msgChangeMap.begin(); mit != msgChangeMap.end(); mit++)
			{
				std::cerr << "p3Posted::notifyChanges() Msgs for Group: " << mit->first;
				std::cerr << std::endl;
				// To start with we are just going to trigger updates on these groups.
				// FUTURE OPTIMISATION.
				// It could be taken a step further and directly request these msgs for an update.
				addGroupForProcessing(mit->first);
			}
			delete msgChange;
	       }

	       /* pass on Group Changes to GUI */
		if (groupChange)
		{
			std::cerr << "p3Posted::notifyChanges() Found Group Change Notification";
			std::cerr << std::endl;

			std::list<RsGxsGroupId> &groupList = groupChange->mGrpIdList;
			std::list<RsGxsGroupId>::iterator git;
			for(git = groupList.begin(); git != groupList.end(); git++)
			{
				std::cerr << "p3Posted::notifyChanges() Incoming Group: " << *git;
				std::cerr << std::endl;
			}
			changesForGUI.push_back(groupChange);
		}
	}
	changes.clear();
	RsGxsIfaceHelper::receiveChanges(changesForGUI);

	std::cerr << "p3Posted::notifyChanges() -> receiveChanges()";
	std::cerr << std::endl;
}

void	p3Posted::service_tick()
{
	dummy_tick();
	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests();

	mCommentService->comment_tick();

	return;
}

bool p3Posted::getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsPostedGroupItem* item = dynamic_cast<RsGxsPostedGroupItem*>(*vit);
			if (item)
			{
				RsPostedGroup grp = item->mGroup;
				item->mGroup.mMeta = item->meta;
				grp.mMeta = item->mGroup.mMeta;
				delete item;
				groups.push_back(grp);
			}
			else
			{
				std::cerr << "Not a RsGxsPostedGroupItem, deleting!" << std::endl;
				delete *vit;
			}
		}
	}
	return ok;
}

bool p3Posted::getPostData(const uint32_t &token, std::vector<RsPostedPost> &msgs)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	time_t now = time(NULL);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
		
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsPostedPostItem* item = dynamic_cast<RsGxsPostedPostItem*>(*vit);

				if(item)
				{
					RsPostedPost msg = item->mPost;
					msg.mMeta = item->meta;
					msg.calculateScores(now);

					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a PostedPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
		
	return ok;
}


bool p3Posted::getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &msgs)
{
	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
	time_t now = time(NULL);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsPostedPostItem* item = dynamic_cast<RsGxsPostedPostItem*>(*vit);
		
				if(item)
				{
					RsPostedPost msg = item->mPost;
					msg.mMeta = item->meta;
					msg.calculateScores(now);

					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a PostedPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
			
	return ok;
}


/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

/* Switched from having explicit Ranking calculations to calculating the set of scores
 * on each RsPostedPost item.
 *
 * TODO: move this function to be part of RsPostedPost - then the GUI 
 * can reuse is as necessary.
 *
 */

bool RsPostedPost::calculateScores(time_t ref_time)
{
	/* so we want to calculate all the scores for this Post. */

	PostStats stats;
	extractPostedCache(mMeta.mServiceString, stats);

	mUpVotes = stats.up_votes;
	mDownVotes = stats.down_votes;
	mComments = stats.comments;
	mHaveVoted = (mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK);

	time_t age_secs = ref_time - mMeta.mPublishTs;
#define POSTED_AGESHIFT (2.0)
#define POSTED_AGEFACTOR (3600.0)

	mTopScore = ((int) mUpVotes - (int) mDownVotes);
	if (mTopScore > 0)
	{
		// score drops with time.
		mHotScore =  mTopScore / pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
	}
	else
	{
		// gets more negative with time.
		mHotScore =  mTopScore * pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
	}
	mNewScore = -age_secs;

	return true;
}


/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

bool p3Posted::createGroup(uint32_t &token, RsPostedGroup &group)
{
	std::cerr << "p3Posted::createGroup()" << std::endl;

	RsGxsPostedGroupItem* grpItem = new RsGxsPostedGroupItem();
	grpItem->mGroup = group;
	grpItem->meta = group.mMeta;

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3Posted::createPost(uint32_t &token, RsPostedPost &msg)
{
	std::cerr << "p3Posted::createPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsPostedPostItem* msgItem = new RsGxsPostedPostItem();
	msgItem->mPost = msg;
	msgItem->meta = msg.mMeta;
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}


/********************************************************************************************/
/********************************************************************************************/

void p3Posted::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNREAD | GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNREAD;
	if (read)
	{
		status = 0;
	}

	setMsgStatusFlags(token, msgId, status, mask);

}





/********************************************************************************************/
/********************************************************************************************/

/* so we need the same tick idea as wiki for generating dummy channels
 */

#define 	MAX_GEN_GROUPS		5
#define 	MAX_GEN_MESSAGES	100

std::string p3Posted::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

bool p3Posted::generateDummyData()
{
	mGenCount = 0;
	mGenRefs.resize(MAX_GEN_MESSAGES);

	std::string groupName;
	rs_sprintf(groupName, "TestTopic_%d", mGenCount);

	std::cerr << "p3Posted::generateDummyData() Starting off with Group: " << groupName;
	std::cerr << std::endl;

	/* create a new group */
	generateGroup(mGenToken, groupName);

	mGenActive = true;

	return true;
}


void p3Posted::dummy_tick()
{
	/* check for a new callback */

	if (mGenActive)
	{
		std::cerr << "p3Posted::dummyTick() AboutActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
		if (status != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3Posted::dummy_tick() Status: " << status;
			std::cerr << std::endl;

			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
				std::cerr << "p3Posted::dummy_tick() generateDummyMsgs() FAILED";
				std::cerr << std::endl;
				mGenActive = false;
			}
			return;
		}

		if (mGenCount < MAX_GEN_GROUPS)
		{
			/* get the group Id */
			RsGxsGroupId groupId;
			RsGxsMessageId emptyId;
			if (!acknowledgeTokenGrp(mGenToken, groupId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3Posted::dummy_tick() Acknowledged GroupId: " << groupId;
			std::cerr << std::endl;

			PostedDummyRef ref(groupId, emptyId, emptyId);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_MESSAGES)
		{
			/* get the msg Id, and generate next snapshot */
			RsGxsGrpMsgIdPair msgId;
			if (!acknowledgeTokenMsg(mGenToken, msgId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3Posted::dummy_tick() Acknowledged <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;

			/* store results for later selection */

			PostedDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else
		{
			std::cerr << "p3Posted::dummy_tick() Finished";
			std::cerr << std::endl;

			/* done */
			mGenActive = false;
			return;
		}

		mGenCount++;

		if (mGenCount < MAX_GEN_GROUPS)
		{
			std::string groupName;
			rs_sprintf(groupName, "TestTopic_%d", mGenCount);

			std::cerr << "p3Posted::dummy_tick() Generating Group: " << groupName;
			std::cerr << std::endl;

			/* create a new group */
			generateGroup(mGenToken, groupName);
		}
		else
		{
			/* create a new message */
			uint32_t idx = (uint32_t) (mGenCount * RSRandom::random_f32());
			PostedDummyRef &ref = mGenRefs[idx];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.empty())
			{
				mGenThreadId = parentId;
			}

			std::cerr << "p3Posted::dummy_tick() Generating Msg ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;

			if (parentId.empty())
			{
				generatePost(mGenToken, grpId);
			}
			else
			{
				generateComment(mGenToken, grpId, parentId, mGenThreadId);
			}
		}
	}
}


bool p3Posted::generatePost(uint32_t &token, const RsGxsGroupId &grpId)
{
	RsPostedPost msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mNotes, "Posted Msg: GroupId: %s, some randomness: %s", 
		grpId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mNotes;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = "";
	msg.mMeta.mParentId = "";

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	uint32_t i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++);

	if (it != ownIds.end())
	{
		std::cerr << "p3Posted::generateMessage() Author: " << *it;
		std::cerr << std::endl;
		msg.mMeta.mAuthorId = *it;
	} 
	else
	{
		std::cerr << "p3Posted::generateMessage() No Author!";
		std::cerr << std::endl;
	} 

	createPost(token, msg);

	return true;
}


bool p3Posted::generateComment(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsComment msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mComment, "Posted Comment: GroupId: %s, ThreadId: %s, ParentId: %s + some randomness: %s", 
		grpId.c_str(), threadId.c_str(), parentId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mComment;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = threadId;
	msg.mMeta.mParentId = parentId;

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	uint32_t i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++);

	if (it != ownIds.end())
	{
		std::cerr << "p3Posted::generateMessage() Author: " << *it;
		std::cerr << std::endl;
		msg.mMeta.mAuthorId = *it;
	} 
	else
	{
		std::cerr << "p3Posted::generateMessage() No Author!";
		std::cerr << std::endl;
	} 

	createComment(token, msg);

	return true;
}


bool p3Posted::generateGroup(uint32_t &token, std::string groupName)
{
	/* generate a new group */
	RsPostedGroup group;
	group.mMeta.mGroupName = groupName;

	createGroup(token, group);

	return true;
}


        // Overloaded from RsTickEvent for Event callbacks.
void p3Posted::handle_event(uint32_t event_type, const std::string & /* elabel */)
{
	std::cerr << "p3Posted::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
		case POSTED_TESTEVENT_DUMMYDATA:
			generateDummyData();
			break;

		case POSTED_BACKGROUND_PROCESSING:
			background_tick();
			break;

		default:
			/* error */
			std::cerr << "p3Posted::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}


/*********************************************************************************
 * Background Calculations.
 *
 * Get list of change groups from Notify....
 * this doesn't imclude your own submissions (at this point). 
 * So they will not be processed until someone else changes something.
 * TODO FIX: Must push for that change.
 *
 * Eventually, we should just be able to get the new messages from Notify, 
 * and only process them!
 */

void p3Posted::background_tick()
{

#if 0
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (mBgGroupList.empty())
		{
			background_requestAllGroups();
		}
	}
#endif

	background_requestUnprocessedGroup();

	RsTickEvent::schedule_in(POSTED_BACKGROUND_PROCESSING, PROCESSING_INC_PERIOD);

}

bool p3Posted::background_requestAllGroups()
{
	std::cerr << "p3Posted::background_requestAllGroups()";
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;

	uint32_t token = 0;
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, POSTED_ALL_GROUPS);

	return true;
}


void p3Posted::background_loadGroups(const uint32_t &token)
{
	/* get messages */
	std::cerr << "p3Posted::background_loadGroups()";
	std::cerr << std::endl;

	std::list<RsGxsGroupId> groupList;
	bool ok = RsGenExchange::getGroupList(token, groupList);

	if (!ok)
	{
		return;
	}

	std::list<RsGxsGroupId>::iterator it;
	for(it = groupList.begin(); it != groupList.end(); it++)
	{
		addGroupForProcessing(*it);
	}
}


void p3Posted::addGroupForProcessing(RsGxsGroupId grpId)
{
#ifdef POSTED_DEBUG
	std::cerr << "p3Posted::addGroupForProcessing(" << grpId << ")";
	std::cerr << std::endl;
#endif // POSTED_DEBUG

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		// no point having multiple lookups queued.
		if (mBgGroupList.end() == std::find(mBgGroupList.begin(), 
						mBgGroupList.end(), grpId))
		{
			mBgGroupList.push_back(grpId);
		}
	}
}


void p3Posted::background_requestUnprocessedGroup()
{
#ifdef POSTED_DEBUG
	std::cerr << "p3Posted::background_requestUnprocessedGroup()";
	std::cerr << std::endl;
#endif // POSTED_DEBUG


	RsGxsGroupId grpId;
	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		if (mBgProcessing)
		{
			std::cerr << "p3Posted::background_requestUnprocessedGroup() Already Active";
			std::cerr << std::endl;
			return;
		}
		if (mBgGroupList.empty())
		{
			std::cerr << "p3Posted::background_requestUnprocessedGroup() No Groups to Process";
			std::cerr << std::endl;
			return;
		}

		grpId = mBgGroupList.front();
		mBgGroupList.pop_front();
		mBgProcessing = true;
	}

	background_requestGroupMsgs(grpId, true);
}





void p3Posted::background_requestGroupMsgs(const RsGxsGroupId &grpId, bool unprocessedOnly)
{
	std::cerr << "p3Posted::background_requestGroupMsgs() id: " << grpId;
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	if (unprocessedOnly)
	{
		opts.mStatusFilter = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
		opts.mStatusMask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	}

	std::list<RsGxsGroupId> grouplist;
	grouplist.push_back(grpId);

	uint32_t token = 0;

	RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, grouplist);

	if (unprocessedOnly)
	{
		GxsTokenQueue::queueRequest(token, POSTED_UNPROCESSED_MSGS);
	}
	else
	{
		GxsTokenQueue::queueRequest(token, POSTED_ALL_MSGS);
	}
}




void p3Posted::background_loadUnprocessedMsgs(const uint32_t &token)
{
	background_loadMsgs(token, true);
}


void p3Posted::background_loadAllMsgs(const uint32_t &token)
{
	background_loadMsgs(token, false);
}


/* This function is generalised to support any collection of messages, across multiple groups */

void p3Posted::background_loadMsgs(const uint32_t &token, bool unprocessed)
{
	/* get messages */
	std::cerr << "p3Posted::background_loadMsgs()";
	std::cerr << std::endl;

	std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> > msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if (!ok)
	{
		std::cerr << "p3Posted::background_loadMsgs() Failed to getMsgData()";
		std::cerr << std::endl;

		/* cleanup */
		background_cleanup();
		return;

	}

	{
		RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
		mBgStatsMap.clear();
		mBgIncremental = unprocessed;
	}

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > postMap;

	// generate vector of changes to push to the GUI.
	std::vector<RsGxsNotify *> changes;
	RsGxsMsgChange *msgChanges = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED);


	RsGxsGroupId groupId;
	std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> >::iterator mit;
	std::vector<RsGxsMsgItem*>::iterator vit;
	for (mit = msgData.begin(); mit != msgData.end(); mit++)
	{
		  groupId = mit->first;
		  for (vit = mit->second.begin(); vit != mit->second.end(); vit++)
		  {
			RsGxsMessageId parentId = (*vit)->meta.mParentId;
			RsGxsMessageId threadId = (*vit)->meta.mThreadId;
				
	
			bool inc_counters = false;
			uint32_t vote_up_inc = 0;
			uint32_t vote_down_inc = 0;
			uint32_t comment_inc = 0;
	
			bool add_voter = false;
			RsGxsId voterId;
			RsGxsCommentItem *commentItem;
			RsGxsVoteItem    *voteItem;
	
			/* THIS Should be handled by UNPROCESSED Filter - but isn't */
			if (!IS_MSG_UNPROCESSED((*vit)->meta.mMsgStatus))
			{
				RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
				if (mBgIncremental)
				{
					std::cerr << "p3Posted::background_loadMsgs() Msg already Processed - Skipping";
					std::cerr << std::endl;
					std::cerr << "p3Posted::background_loadMsgs() ERROR This should not happen";
					std::cerr << std::endl;
					delete(*vit);
					continue;
				}
			}
	
			/* 3 types expected: PostedPost, Comment and Vote */
			if (parentId.empty())
			{
				/* we don't care about top-level (Posts) */
				std::cerr << "\tIgnoring TopLevel Item";
				std::cerr << std::endl;

				/* but we need to notify GUI about them */	
				msgChanges->msgChangeMap[mit->first].push_back((*vit)->meta.mMsgId);
			}
			else if (NULL != (commentItem = dynamic_cast<RsGxsCommentItem *>(*vit)))
			{
				/* comment - want all */
				/* Comments are counted by Thread Id */
				std::cerr << "\tProcessing Comment: " << commentItem;
				std::cerr << std::endl;
	
				inc_counters = true;
				comment_inc = 1;
			}
			else if (NULL != (voteItem = dynamic_cast<RsGxsVoteItem *>(*vit)))
			{
				/* vote - only care about direct children */
				if (parentId == threadId)
				{
					/* Votes are organised by Parent Id,
					 * ie. you can vote for both Posts and Comments
					 */
					std::cerr << "\tProcessing Vote: " << voteItem;
					std::cerr << std::endl;
	
					inc_counters = true;
					add_voter = true;
					voterId = voteItem->meta.mAuthorId;
	
					if (voteItem->mMsg.mVoteType == GXS_VOTE_UP)
					{
						vote_up_inc = 1;
					}
					else
					{
						vote_down_inc = 1;
					}
				}
			}
			else
			{
				/* unknown! */
				std::cerr << "p3Posted::background_processNewMessages() ERROR Strange NEW Message:";
				std::cerr << std::endl;
				std::cerr << "\t" << (*vit)->meta;
				std::cerr << std::endl;
	
			}
	
			if (inc_counters)
			{
				RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
	
				std::map<RsGxsMessageId, PostStats>::iterator sit = mBgStatsMap.find(threadId);
				if (sit == mBgStatsMap.end())
				{
					// add to map of ones to update.		
					postMap[groupId].push_back(threadId);	

					mBgStatsMap[threadId] = PostStats(0,0,0);
					sit = mBgStatsMap.find(threadId);
				}
		
				sit->second.comments += comment_inc;
				sit->second.up_votes += vote_up_inc;
				sit->second.down_votes += vote_down_inc;


				if (add_voter)
				{
					sit->second.voters.push_back(voterId);
				}
		
				std::cerr << "\tThreadId: " << threadId;
				std::cerr << " Comment Total: " << sit->second.comments;
				std::cerr << " UpVote Total: " << sit->second.up_votes;
				std::cerr << " DownVote Total: " << sit->second.down_votes;
				std::cerr << std::endl;
			}
		
			/* flag all messages as processed */
			if ((*vit)->meta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)
			{
				uint32_t token_a;
				RsGxsGrpMsgIdPair msgId = std::make_pair(groupId, (*vit)->meta.mMsgId);
				RsGenExchange::setMsgStatusFlags(token_a, msgId, 0, GXS_SERV::GXS_MSG_STATUS_UNPROCESSED);
			}
			delete(*vit);
		}
	}

	/* push updates of new Posts */
	if (msgChanges->msgChangeMap.size() > 0)
	{
		std::cerr << "p3Posted::background_processNewMessages() -> receiveChanges()";
		std::cerr << std::endl;

		changes.push_back(msgChanges);
	 	RsGxsIfaceHelper::receiveChanges(changes);
	}

	/* request the summary info from the parents */
	uint32_t token_b;
	uint32_t anstype = RS_TOKREQ_ANSTYPE_SUMMARY; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
	RsGenExchange::getTokenService()->requestMsgInfo(token_b, anstype, opts, postMap);

	GxsTokenQueue::queueRequest(token_b, POSTED_BG_POST_META);
	return;
}


#define RSGXS_MAX_SERVICE_STRING	1024
bool encodePostedCache(std::string &str, const PostStats &s)
{
	char line[RSGXS_MAX_SERVICE_STRING];

	snprintf(line, RSGXS_MAX_SERVICE_STRING, "%d %d %d", s.comments, s.up_votes, s.down_votes);

	str = line;
	return true;
}

bool extractPostedCache(const std::string &str, PostStats &s)
{

	uint32_t iupvotes, idownvotes, icomments;
	if (3 == sscanf(str.c_str(), "%d %d %d", &icomments, &iupvotes, &idownvotes))
	{
		s.comments = icomments;
		s.up_votes = iupvotes;
		s.down_votes = idownvotes;
		return true;
	}
	return false;
}


void p3Posted::background_updateVoteCounts(const uint32_t &token)
{
	std::cerr << "p3Posted::background_updateVoteCounts()";
	std::cerr << std::endl;

	GxsMsgMetaMap parentMsgList;
	GxsMsgMetaMap::iterator mit;
	std::vector<RsMsgMetaData>::iterator vit;

	bool ok = RsGenExchange::getMsgMeta(token, parentMsgList);

	if (!ok)
	{
		std::cerr << "p3Posted::background_updateVoteCounts() ERROR";
		std::cerr << std::endl;
		background_cleanup();
		return;
	}

	// generate vector of changes to push to the GUI.
	std::vector<RsGxsNotify *> changes;
	RsGxsMsgChange *msgChanges = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED);

	for(mit = parentMsgList.begin(); mit != parentMsgList.end(); mit++)
	{
		for(vit = mit->second.begin(); vit != mit->second.end(); vit++)
		{
			std::cerr << "p3Posted::background_updateVoteCounts() Processing Msg(" << mit->first;
			std::cerr << ", " << vit->mMsgId << ")";
			std::cerr << std::endl;
	
			RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/
	
			/* extract current vote count */
			PostStats stats;
			if (mBgIncremental)
			{
				if (!extractPostedCache(vit->mServiceString, stats))
				{
					if (!(vit->mServiceString.empty()))
					{
						std::cerr << "p3Posted::background_updateVoteCounts() Failed to extract Votes";
						std::cerr << std::endl;
						std::cerr << "\tFrom String: " << vit->mServiceString;
						std::cerr << std::endl;
					}
				}
			}
	
			/* get increment */
			std::map<RsGxsMessageId, PostStats>::iterator it;
			it = mBgStatsMap.find(vit->mMsgId);
	
			if (it != mBgStatsMap.end())
			{
				std::cerr << "p3Posted::background_updateVoteCounts() Adding to msgChangeMap: ";
				std::cerr << mit->first << " MsgId: " << vit->mMsgId;
				std::cerr << std::endl;

				stats.increment(it->second);
				msgChanges->msgChangeMap[mit->first].push_back(vit->mMsgId);
			}
			else
			{
				// warning.
				std::cerr << "p3Posted::background_updateVoteCounts() Warning No New Votes found.";
				std::cerr << " For MsgId: " << vit->mMsgId;
				std::cerr << std::endl;
			}
	
			std::string str;
			if (!encodePostedCache(str, stats))
			{
				std::cerr << "p3Posted::background_updateVoteCounts() Failed to encode Votes";
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "p3Posted::background_updateVoteCounts() Encoded String: " << str;
				std::cerr << std::endl;
				/* store new result */
				uint32_t token_c;
				RsGxsGrpMsgIdPair msgId = std::make_pair(vit->mGroupId, vit->mMsgId);
				RsGenExchange::setMsgServiceString(token_c, msgId, str);
			}
		}
	}

	if (msgChanges->msgChangeMap.size() > 0)
	{
		std::cerr << "p3Posted::background_updateVoteCounts() -> receiveChanges()";
		std::cerr << std::endl;

		changes.push_back(msgChanges);
	 	RsGxsIfaceHelper::receiveChanges(changes);
	}

	// DONE!.
	background_cleanup();
	return;

}


bool p3Posted::background_cleanup()
{
	std::cerr << "p3Posted::background_cleanup()";
	std::cerr << std::endl;

	RsStackMutex stack(mPostedMtx); /********** STACK LOCKED MTX ******/

	// Cleanup.
	mBgStatsMap.clear();
	mBgProcessing = false;

	return true;
}


	// Overloaded from GxsTokenQueue for Request callbacks.
void p3Posted::handleResponse(uint32_t token, uint32_t req_type)
{
	std::cerr << "p3Posted::handleResponse(" << token << "," << req_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(req_type)
	{
		case POSTED_ALL_GROUPS:
			background_loadGroups(token);
			break;
		case POSTED_UNPROCESSED_MSGS:
			background_loadUnprocessedMsgs(token);
			break;
		case POSTED_ALL_MSGS:
			background_loadAllMsgs(token);
			break;
		case POSTED_BG_POST_META:
			background_updateVoteCounts(token);
			break;
		default:
			/* error */
			std::cerr << "p3Posted::handleResponse() Unknown Request Type: " << req_type;
			std::cerr << std::endl;
			break;
	}
}

