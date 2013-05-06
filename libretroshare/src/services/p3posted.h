/*
 * libretroshare/src/services: p3posted.h
 *
 * GxsChannel interface for RetroShare.
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#ifndef P3_POSTED_SERVICE_HEADER
#define P3_POSTED_SERVICE_HEADER


#include "retroshare/rsposted.h"
#include "services/p3gxscommon.h"
#include "gxs/rsgenexchange.h"

#include "util/rstickevent.h"

#include <retroshare/rsidentity.h>

#include <map>
#include <string>
#include <list>

/* 
 *
 */


class PostStats
{
	public:
	PostStats() :up_votes(0), down_votes(0), comments(0) { return; }
	PostStats(int up, int down, int c) :up_votes(up), down_votes(down), comments(c) { return; }

	void increment(const PostStats &s) 
	{ 
		up_votes += s.up_votes;
		down_votes += s.down_votes;
		comments += s.comments;
		return;
	}

	int up_votes;
	int down_votes;
	int comments;
	std::list<RsGxsId> voters;
};

bool encodePostedCache(std::string &str, const PostStats &s);
bool extractPostedCache(const std::string &str, PostStats &s);


class p3Posted: public RsGenExchange, public RsPosted, 
	public GxsTokenQueue, 
	public RsTickEvent	/* only needed for testing - remove after */
{
	public:

	p3Posted(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs);

virtual void service_tick();

	protected:


virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

        // Overloaded from GxsTokenQueue for Request callbacks.
virtual void handleResponse(uint32_t token, uint32_t req_type);

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

	public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups);
virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts);

virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts);

        //////////////////////////////////////////////////////////////////////////////

virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

virtual bool createGroup(uint32_t &token, RsPostedGroup &group);
virtual bool createPost(uint32_t &token, RsPostedPost &post);



	/* Comment service - Provide RsGxsCommentService - redirect to p3GxsCommentService */
virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &msgs)
	{
        	return mCommentService->getGxsCommentData(token, msgs);
	}

virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &msgs)
	{
		return mCommentService->getGxsRelatedComments(token, msgs);
	}

virtual bool createComment(uint32_t &token, RsGxsComment &msg)
	{
		return mCommentService->createGxsComment(token, msg);
	}

virtual bool createVote(uint32_t &token, RsGxsVote &msg)
	{
		return mCommentService->createGxsVote(token, msg);
	}

virtual bool acknowledgeComment(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
	{
		return acknowledgeMsg(token, msgId);
	}

virtual bool acknowledgeVote(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
	{
		if (mCommentService->acknowledgeVote(token, msgId))
		{
			return true;
		}
		return acknowledgeMsg(token, msgId);
	}


	private:

static uint32_t postedAuthenPolicy();

	bool calculateScores(RsPostedPost &post, time_t ref_time);

	// Background processing.
	void background_tick();

	bool background_requestAllGroups();
	void background_loadGroups(const uint32_t &token);

	void addGroupForProcessing(RsGxsGroupId grpId);
	void background_requestUnprocessedGroup();

	void background_requestGroupMsgs(const RsGxsGroupId &grpId, bool unprocessedOnly);
	void background_loadUnprocessedMsgs(const uint32_t &token);
	void background_loadAllMsgs(const uint32_t &token);
	void background_loadMsgs(const uint32_t &token, bool unprocessed);


	void background_updateVoteCounts(const uint32_t &token);
	bool background_cleanup();


	RsMutex mPostedMtx; 

	bool mBgProcessing;
	bool mBgIncremental;
        std::list<RsGxsGroupId> mBgGroupList;
        std::map<RsGxsMessageId, PostStats> mBgStatsMap; 




// DUMMY DATA,
virtual bool generateDummyData();

std::string genRandomId();

void 	dummy_tick();

bool generatePost(uint32_t &token, const RsGxsGroupId &grpId);
bool generateComment(uint32_t &token, const RsGxsGroupId &grpId, 
		const RsGxsMessageId &parentId, const RsGxsMessageId &threadId);
bool generateGroup(uint32_t &token, std::string groupName);

	class PostedDummyRef
	{
		public:
		PostedDummyRef() { return; }
		PostedDummyRef(const RsGxsGroupId &grpId, const RsGxsMessageId &threadId, const RsGxsMessageId &msgId)
		:mGroupId(grpId), mThreadId(threadId), mMsgId(msgId) { return; }

		RsGxsGroupId mGroupId;
		RsGxsMessageId mThreadId;
		RsGxsMessageId mMsgId;
	};

	uint32_t mGenToken;
	bool mGenActive;
	int mGenCount;
	std::vector<PostedDummyRef> mGenRefs;
	RsGxsMessageId mGenThreadId;

	p3GxsCommentService *mCommentService;	
};









#endif 
