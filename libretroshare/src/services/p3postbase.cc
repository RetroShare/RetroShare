/*******************************************************************************
 * libretroshare/src/services: p3postbase.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2012 Robert Fernie <retroshare@lunamutt.com>                 *
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
#include <retroshare/rsidentity.h>

#include "retroshare/rsgxsflags.h"
#include <stdio.h>
#include <math.h>

#include "services/p3postbase.h"
#include "rsitems/rsgxscommentitems.h"

#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define POSTBASE_DEBUG 1
 ****/

#define POSTBASE_BACKGROUND_PROCESSING	0x0002
#define PROCESSING_START_PERIOD		30
#define PROCESSING_INC_PERIOD		15

#define POSTBASE_ALL_GROUPS 		0x0011
#define POSTBASE_UNPROCESSED_MSGS	0x0012
#define POSTBASE_ALL_MSGS 		0x0013
#define POSTBASE_BG_POST_META		0x0014
/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PostBase::p3PostBase(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs,
	RsSerialType* serviceSerialiser, uint16_t serviceType)
    : RsGenExchange(gds, nes, serviceSerialiser, serviceType, gixs, postBaseAuthenPolicy()), GxsTokenQueue(this), RsTickEvent(), mPostBaseMtx("PostBaseMtx")
{
	mBgProcessing = false;

	mCommentService = new p3GxsCommentService(this,  serviceType);
	RsTickEvent::schedule_in(POSTBASE_BACKGROUND_PROCESSING, PROCESSING_START_PERIOD);
}


uint32_t p3PostBase::postBaseAuthenPolicy()
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

void p3PostBase::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::notifyChanges()";
	std::cerr << std::endl;
#endif

	p3Notify *notify = NULL;
	if (!changes.empty())
	{
		notify = RsServer::notify();
	}

	std::vector<RsGxsNotify *>::iterator it;

	for(it = changes.begin(); it != changes.end(); ++it)
	{
		RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
		if (msgChange)
		{
#ifdef POSTBASE_DEBUG
			std::cerr << "p3PostBase::notifyChanges() Found Message Change Notification";
			std::cerr << std::endl;
#endif

			std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
			for(auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
			{
#ifdef POSTBASE_DEBUG
				std::cerr << "p3PostBase::notifyChanges() Msgs for Group: " << mit->first;
				std::cerr << std::endl;
#endif
				// To start with we are just going to trigger updates on these groups.
				// FUTURE OPTIMISATION.
				// It could be taken a step further and directly request these msgs for an update.
				addGroupForProcessing(mit->first);

				if (notify && msgChange->getType() == RsGxsNotify::TYPE_RECEIVED_NEW)
				{
					for (auto mit1 = mit->second.begin(); mit1 != mit->second.end(); ++mit1)
					{
						notify->AddFeedItem(RS_FEED_ITEM_POSTED_MSG, mit->first.toStdString(), mit1->toStdString());
					}
				}
			}
		}

		/* pass on Group Changes to GUI */
		if (groupChange)
		{
#ifdef POSTBASE_DEBUG
			std::cerr << "p3PostBase::notifyChanges() Found Group Change Notification";
			std::cerr << std::endl;
#endif

			std::list<RsGxsGroupId> &groupList = groupChange->mGrpIdList;
			std::list<RsGxsGroupId>::iterator git;
			for(git = groupList.begin(); git != groupList.end(); ++git)
			{
#ifdef POSTBASE_DEBUG
				std::cerr << "p3PostBase::notifyChanges() Incoming Group: " << *git;
				std::cerr << std::endl;
#endif

				if (notify && groupChange->getType() == RsGxsNotify::TYPE_RECEIVED_NEW)
				{
					notify->AddFeedItem(RS_FEED_ITEM_POSTED_NEW, git->toStdString());
				}
			}
		}
	}
	receiveHelperChanges(changes);

#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::notifyChanges() -> receiveChanges()";
	std::cerr << std::endl;
#endif
}

void	p3PostBase::service_tick()
{
	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests();

	mCommentService->comment_tick();

	return;
}

/********************************************************************************************/
/********************************************************************************************/

void p3PostBase::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	if (read)
	{
		status = 0;
	}

	setMsgStatusFlags(token, msgId, status, mask);

}


        // Overloaded from RsTickEvent for Event callbacks.
void p3PostBase::handle_event(uint32_t event_type, const std::string & /* elabel */)
{
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::handle_event(" << event_type << ")";
	std::cerr << std::endl;
#endif

	// stuff.
	switch(event_type)
	{
		case POSTBASE_BACKGROUND_PROCESSING:
			background_tick();
			break;

		default:
			/* error */
#ifdef POSTBASE_DEBUG
			std::cerr << "p3PostBase::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
#endif
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

void p3PostBase::background_tick()
{

#if 0
	{
		RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
		if (mBgGroupList.empty())
		{
			background_requestAllGroups();
		}
	}
#endif

	background_requestUnprocessedGroup();

	RsTickEvent::schedule_in(POSTBASE_BACKGROUND_PROCESSING, PROCESSING_INC_PERIOD);

}

bool p3PostBase::background_requestAllGroups()
{
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::background_requestAllGroups()";
	std::cerr << std::endl;
#endif

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;

	uint32_t token = 0;
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, POSTBASE_ALL_GROUPS);

	return true;
}


void p3PostBase::background_loadGroups(const uint32_t &token)
{
	/* get messages */
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::background_loadGroups()";
	std::cerr << std::endl;
#endif

	std::list<RsGxsGroupId> groupList;
	bool ok = RsGenExchange::getGroupList(token, groupList);

	if (!ok)
	{
		return;
	}

	std::list<RsGxsGroupId>::iterator it;
	for(it = groupList.begin(); it != groupList.end(); ++it)
	{
		addGroupForProcessing(*it);
	}
}


void p3PostBase::addGroupForProcessing(RsGxsGroupId grpId)
{
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::addGroupForProcessing(" << grpId << ")";
	std::cerr << std::endl;
#endif // POSTBASE_DEBUG

	{
		RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
		// no point having multiple lookups queued.
		if (mBgGroupList.end() == std::find(mBgGroupList.begin(), 
						mBgGroupList.end(), grpId))
		{
			mBgGroupList.push_back(grpId);
		}
	}
}


void p3PostBase::background_requestUnprocessedGroup()
{
#ifdef POSTBASE_DEBUG
	std::cerr << "p3PostBase::background_requestUnprocessedGroup()";
	std::cerr << std::endl;
#endif // POSTBASE_DEBUG


	RsGxsGroupId grpId;
	{
		RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
		if (mBgProcessing)
		{
#ifdef POSTBASE_DEBUG
            std::cerr << "p3PostBase::background_requestUnprocessedGroup() Already Active";
            std::cerr << std::endl;
#endif
			return;
		}
		if (mBgGroupList.empty())
		{
#ifdef POSTBASE_DEBUG
            std::cerr << "p3PostBase::background_requestUnprocessedGroup() No Groups to Process";
            std::cerr << std::endl;
#endif
			return;
		}

		grpId = mBgGroupList.front();
		mBgGroupList.pop_front();
		mBgProcessing = true;
	}

	background_requestGroupMsgs(grpId, true);
}





void p3PostBase::background_requestGroupMsgs(const RsGxsGroupId &grpId, bool unprocessedOnly)
{
#ifdef POSTBASE_DEBUG
    std::cerr << "p3PostBase::background_requestGroupMsgs() id: " << grpId;
    std::cerr << std::endl;
#endif

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
		GxsTokenQueue::queueRequest(token, POSTBASE_UNPROCESSED_MSGS);
	}
	else
	{
		GxsTokenQueue::queueRequest(token, POSTBASE_ALL_MSGS);
	}
}




void p3PostBase::background_loadUnprocessedMsgs(const uint32_t &token)
{
	background_loadMsgs(token, true);
}


void p3PostBase::background_loadAllMsgs(const uint32_t &token)
{
	background_loadMsgs(token, false);
}


/* This function is generalised to support any collection of messages, across multiple groups */

void p3PostBase::background_loadMsgs(const uint32_t &token, bool unprocessed)
{
	/* get messages */
#ifdef POSTBASE_DEBUG
    std::cerr << "p3PostBase::background_loadMsgs()";
    std::cerr << std::endl;
#endif

	std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> > msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if (!ok)
	{
		std::cerr << "p3PostBase::background_loadMsgs() Failed to getMsgData()";
		std::cerr << std::endl;

		/* cleanup */
		background_cleanup();
		return;

	}

	{
		RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
		mBgStatsMap.clear();
		mBgIncremental = unprocessed;
	}

	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > postMap;

	// generate vector of changes to push to the GUI.
	std::vector<RsGxsNotify *> changes;
	RsGxsMsgChange *msgChanges = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED, false);


	RsGxsGroupId groupId;
	std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> >::iterator mit;
	std::vector<RsGxsMsgItem*>::iterator vit;
	for (mit = msgData.begin(); mit != msgData.end(); ++mit)
	{
		  groupId = mit->first;
		  for (vit = mit->second.begin(); vit != mit->second.end(); ++vit)
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
				RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
				if (mBgIncremental)
				{
#ifdef POSTBASE_DEBUG
                    std::cerr << "p3PostBase::background_loadMsgs() Msg already Processed - Skipping";
					std::cerr << std::endl;
					std::cerr << "p3PostBase::background_loadMsgs() ERROR This should not happen";
                    std::cerr << std::endl;
#endif
					delete(*vit);
					continue;
				}
			}
	
			/* 3 types expected: PostedPost, Comment and Vote */
			if (parentId.isNull())
			{
#ifdef POSTBASE_DEBUG
                /* we don't care about top-level (Posts) */
				std::cerr << "\tIgnoring TopLevel Item";
                std::cerr << std::endl;
#endif

				/* but we need to notify GUI about them */	
				msgChanges->msgChangeMap[mit->first].insert((*vit)->meta.mMsgId);
			}
			else if (NULL != (commentItem = dynamic_cast<RsGxsCommentItem *>(*vit)))
			{
#ifdef POSTBASE_DEBUG
                /* comment - want all */
				/* Comments are counted by Thread Id */
				std::cerr << "\tProcessing Comment: " << commentItem;
                std::cerr << std::endl;
#endif
	
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
#ifdef POSTBASE_DEBUG
                    std::cerr << "\tProcessing Vote: " << voteItem;
					std::cerr << std::endl;
#endif

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
				std::cerr << "p3PostBase::background_processNewMessages() ERROR Strange NEW Message:";
				std::cerr << std::endl;
				std::cerr << "\t" << (*vit)->meta;
				std::cerr << std::endl;
	
			}
	
			if (inc_counters)
			{
				RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
	
				std::map<RsGxsMessageId, PostStats>::iterator sit = mBgStatsMap.find(threadId);
				if (sit == mBgStatsMap.end())
				{
					// add to map of ones to update.		
					postMap[groupId].insert(threadId);

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
		
#ifdef POSTBASE_DEBUG
                std::cerr << "\tThreadId: " << threadId;
				std::cerr << " Comment Total: " << sit->second.comments;
				std::cerr << " UpVote Total: " << sit->second.up_votes;
                std::cerr << " DownVote Total: " << sit->second.down_votes;
                std::cerr << std::endl;
#endif
			}
		
			/* flag all messages as processed and new for the gui */
			if ((*vit)->meta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)
			{
				uint32_t token_a;
				RsGxsGrpMsgIdPair msgId = std::make_pair(groupId, (*vit)->meta.mMsgId);
				RsGenExchange::setMsgStatusFlags(token_a, msgId, GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD, GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD);
			}
			delete(*vit);
		}
	}

	/* push updates of new Posts */
	if (msgChanges->msgChangeMap.size() > 0)
	{
#ifdef POSTBASE_DEBUG
        std::cerr << "p3PostBase::background_processNewMessages() -> receiveChanges()";
        std::cerr << std::endl;
#endif

		changes.push_back(msgChanges);
	 	receiveHelperChanges(changes);
	}
	else
	{
		delete(msgChanges);
	}

	/* request the summary info from the parents */
	uint32_t token_b;
	uint32_t anstype = RS_TOKREQ_ANSTYPE_SUMMARY; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
	RsGenExchange::getTokenService()->requestMsgInfo(token_b, anstype, opts, postMap);

	GxsTokenQueue::queueRequest(token_b, POSTBASE_BG_POST_META);
	return;
}


#define RSGXS_MAX_SERVICE_STRING	1024
bool encodePostCache(std::string &str, const PostStats &s)
{
	char line[RSGXS_MAX_SERVICE_STRING];

	snprintf(line, RSGXS_MAX_SERVICE_STRING, "%d %d %d", s.comments, s.up_votes, s.down_votes);

	str = line;
	return true;
}

bool extractPostCache(const std::string &str, PostStats &s)
{

	uint32_t iupvotes, idownvotes, icomments;
	if (3 == sscanf(str.c_str(), "%u %u %u", &icomments, &iupvotes, &idownvotes))
	{
		s.comments = icomments;
		s.up_votes = iupvotes;
		s.down_votes = idownvotes;
		return true;
	}
	return false;
}


void p3PostBase::background_updateVoteCounts(const uint32_t &token)
{
#ifdef POSTBASE_DEBUG
    std::cerr << "p3PostBase::background_updateVoteCounts()";
    std::cerr << std::endl;
#endif

	GxsMsgMetaMap parentMsgList;
	GxsMsgMetaMap::iterator mit;
	std::vector<RsMsgMetaData>::iterator vit;

	bool ok = RsGenExchange::getMsgMeta(token, parentMsgList);

	if (!ok)
	{
#ifdef POSTBASE_DEBUG
        std::cerr << "p3PostBase::background_updateVoteCounts() ERROR";
        std::cerr << std::endl;
#endif
		background_cleanup();
		return;
	}

	// generate vector of changes to push to the GUI.
	std::vector<RsGxsNotify *> changes;
	RsGxsMsgChange *msgChanges = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED, false);

	for(mit = parentMsgList.begin(); mit != parentMsgList.end(); ++mit)
	{
		for(vit = mit->second.begin(); vit != mit->second.end(); ++vit)
		{
#ifdef POSTBASE_DEBUG
            std::cerr << "p3PostBase::background_updateVoteCounts() Processing Msg(" << mit->first;
            std::cerr << ", " << vit->mMsgId << ")";
            std::cerr << std::endl;
#endif
	
			RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/
	
			/* extract current vote count */
			PostStats stats;
			if (mBgIncremental)
			{
				if (!extractPostCache(vit->mServiceString, stats))
				{
					if (!(vit->mServiceString.empty()))
					{
						std::cerr << "p3PostBase::background_updateVoteCounts() Failed to extract Votes";
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
#ifdef POSTBASE_DEBUG
                std::cerr << "p3PostBase::background_updateVoteCounts() Adding to msgChangeMap: ";
				std::cerr << mit->first << " MsgId: " << vit->mMsgId;
                std::cerr << std::endl;
#endif

				stats.increment(it->second);
				msgChanges->msgChangeMap[mit->first].insert(vit->mMsgId);
			}
			else
			{
#ifdef POSTBASE_DEBUG
                // warning.
				std::cerr << "p3PostBase::background_updateVoteCounts() Warning No New Votes found.";
				std::cerr << " For MsgId: " << vit->mMsgId;
                std::cerr << std::endl;
#endif
			}
	
			std::string str;
			if (!encodePostCache(str, stats))
            {
				std::cerr << "p3PostBase::background_updateVoteCounts() Failed to encode Votes";
				std::cerr << std::endl;
			}
			else
			{
#ifdef POSTBASE_DEBUG
                std::cerr << "p3PostBase::background_updateVoteCounts() Encoded String: " << str;
                std::cerr << std::endl;
#endif
				/* store new result */
				uint32_t token_c;
				RsGxsGrpMsgIdPair msgId = std::make_pair(vit->mGroupId, vit->mMsgId);
				RsGenExchange::setMsgServiceString(token_c, msgId, str);
			}
		}
	}

	if (msgChanges->msgChangeMap.size() > 0)
	{
#ifdef POSTBASE_DEBUG
        std::cerr << "p3PostBase::background_updateVoteCounts() -> receiveChanges()";
        std::cerr << std::endl;
#endif

		changes.push_back(msgChanges);
	 	receiveHelperChanges(changes);
	}
	else
	{
		delete(msgChanges);
	}

	// DONE!.
	background_cleanup();
	return;

}


bool p3PostBase::background_cleanup()
{
#ifdef POSTBASE_DEBUG
    std::cerr << "p3PostBase::background_cleanup()";
    std::cerr << std::endl;
#endif

	RsStackMutex stack(mPostBaseMtx); /********** STACK LOCKED MTX ******/

	// Cleanup.
	mBgStatsMap.clear();
	mBgProcessing = false;

	return true;
}


	// Overloaded from GxsTokenQueue for Request callbacks.
void p3PostBase::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef POSTBASE_DEBUG
    std::cerr << "p3PostBase::handleResponse(" << token << "," << req_type << ")";
    std::cerr << std::endl;
#endif

	// stuff.
	switch(req_type)
	{
		case POSTBASE_ALL_GROUPS:
			background_loadGroups(token);
			break;
		case POSTBASE_UNPROCESSED_MSGS:
			background_loadUnprocessedMsgs(token);
			break;
		case POSTBASE_ALL_MSGS:
			background_loadAllMsgs(token);
			break;
		case POSTBASE_BG_POST_META:
			background_updateVoteCounts(token);
			break;
		default:
			/* error */
			std::cerr << "p3PostBase::handleResponse() Unknown Request Type: " << req_type;
			std::cerr << std::endl;
			break;
	}
}

