/*
 * libretroshare/src/services: p3posted.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef P3_POSTED_SERVICE_VEG_HEADER
#define P3_POSTED_SERVICE_VEG_HEADER

#include "services/p3gxsserviceVEG.h"
#include "retroshare/rspostedVEG.h"

#include <map>
#include <string>

/* 
 * Posted Service
 *
 */


class PostedDataProxy: public GxsDataProxyVEG
{
	public:

	bool addGroup(const RsPostedGroup &group);
	bool addPost(const RsPostedPost &post);
	bool addVote(const RsPostedVote &vote);
	bool addComment(const RsPostedComment &comment);

	bool getGroup(const std::string &id, RsPostedGroup &group);
	bool getPost(const std::string &id, RsPostedPost &post);
	bool getVote(const std::string &id, RsPostedVote &vote);
	bool getComment(const std::string &id, RsPostedComment &comment);

        /* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *groupData, RsMsgMetaData &meta);

};



class p3PostedServiceVEG: public p3GxsDataServiceVEG, public RsPostedVEG
{
	public:

	p3PostedServiceVEG(uint16_t type);

virtual int	tick();

	public:

// NEW INTERFACE.
/************* Extern Interface *******/

virtual bool updated();

       /* Data Requests */
virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &msgIds);

        /* Generic Lists */
virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds);
virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds);

        /* Generic Summary */
virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);
virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo);

        /* Actual Data -> specific to Interface */
        /* Specific Service Data */
virtual bool getGroup(const uint32_t &token, RsPostedGroup &group);
virtual bool getPost(const uint32_t &token, RsPostedPost &post);
virtual bool getComment(const uint32_t &token, RsPostedComment &comment);


        /* Poll */
virtual uint32_t requestStatus(const uint32_t token);

        /* Cancel Request */
virtual bool cancelRequest(const uint32_t &token);

        //////////////////////////////////////////////////////////////////////////////
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);
virtual bool setMessageServiceString(const std::string &msgId, const std::string &str);
virtual bool setGroupServiceString(const std::string &grpId, const std::string &str);

virtual bool groupRestoreKeys(const std::string &groupId);
virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

virtual bool submitGroup(uint32_t &token, RsPostedGroup &group, bool isNew);
virtual bool submitPost(uint32_t &token, RsPostedPost &post, bool isNew);
virtual bool submitVote(uint32_t &token, RsPostedVote &vote, bool isNew);
virtual bool submitComment(uint32_t &token, RsPostedComment &comment, bool isNew);

	// Extended Interface for Collated Data View.
virtual bool setViewMode(uint32_t mode);
virtual bool setViewPeriod(uint32_t period);
virtual bool setViewRange(uint32_t first, uint32_t count);

virtual bool requestRanking(uint32_t &token, std::string groupId);
virtual bool getRankedPost(const uint32_t &token, RsPostedPost &post);


	// These are exposed for GUI usage.
virtual bool encodePostedCache(std::string &str, uint32_t votes, uint32_t comments);
virtual bool extractPostedCache(const std::string &str, uint32_t &votes, uint32_t &comments);
virtual float calcPostScore(const RsMsgMetaData &meta);

	private:

	// 
bool 	checkRankingRequest();
bool 	processPosts();

	// background processing of Votes.
	// NB: These should probably be handled by a background thread.
	// At the moment they are run from the tick() thread.

bool 	background_checkTokenRequest();
bool 	background_requestGroups();
bool 	background_requestNewMessages();
bool 	background_processNewMessages();

bool 	background_updateVoteCounts();
bool 	background_cleanup();



std::string genRandomId();
bool 	generateDummyData();
bool 	addExtraDummyData();

	PostedDataProxy *mPostedProxy;

	RsMutex mPostedMtx;
	bool mUpdated;

	// Ranking view mode, stored here.
	uint32_t mViewMode;
	uint32_t mViewPeriod;
	uint32_t mViewStart;
	uint32_t mViewCount;

	// Processing Ranking stuff.
	bool mProcessingRanking;
	uint32_t mRankingState;
	uint32_t mRankingExternalToken;
	uint32_t mRankingInternalToken;

	// background processing - Mutex protected.
	time_t   mLastBgCheck;
	bool 	 mBgProcessing;
	uint32_t mBgPhase;
	uint32_t mBgToken;

	std::map<std::string, uint32_t> mBgVoteMap;	// ParentId -> Vote Count.
	std::map<std::string, uint32_t> mBgCommentMap;  // ThreadId -> Comment Count.

	// extra dummy data.
        std::list<RsPostedVote>    mDummyLaterVotes;
        std::list<RsPostedComment> mDummyLaterComments;


};

#endif 
