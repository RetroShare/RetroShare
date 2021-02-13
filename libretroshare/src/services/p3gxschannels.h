/*******************************************************************************
 * libretroshare/src/services: p3gxschannels.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#pragma once


#include "retroshare/rsgxschannels.h"
#include "services/p3gxscommon.h"
#include "gxs/rsgenexchange.h"
#include "gxs/gxstokenqueue.h"
#include "util/rsmemory.h"
#include "util/rsdebug.h"
#include "util/rstickevent.h"

#include <map>
#include <string>


// This class is only a helper to parse the channel group service string.

class GxsChannelGroupInfo
{
	public:
    GxsChannelGroupInfo(): mAutoDownload(false), mDownloadDirectory("") {}
	bool load(const std::string &input);
	std::string save() const;

	bool mAutoDownload;
	std::string mDownloadDirectory;
};


class p3GxsChannels: public RsGenExchange, public RsGxsChannels, 
	public GxsTokenQueue, public p3Config,
	public RsTickEvent	/* only needed for testing - remove after */
{
public:
	p3GxsChannels( RsGeneralDataService* gds, RsNetworkExchangeService* nes,
	               RsGixs* gixs );
    virtual RsServiceInfo getServiceInfo() override;

    virtual void service_tick() override;

protected:

    virtual bool service_checkIfGroupIsStillUsed(const RsGxsGrpMetaData& meta) override;	// see RsGenExchange

    virtual RsSerialiser* setupSerialiser() override;                            // @see p3Config::setupSerialiser()
    virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList) override; // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
    virtual bool loadList(std::list<RsItem *>& loadList) override;               // @see p3Config::loadList(std::list<RsItem *>&)

    virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id) override;
    virtual TurtleRequestId turtleSearchRequest(const std::string& match_string) override;
    virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSearchResults> &results)  override;
    virtual bool clearDistantSearchResults(TurtleRequestId req) override;
    virtual bool getDistantSearchResultGroupData(const RsGxsGroupId& group_id,RsGxsChannelGroup& distant_group) override;
    virtual DistantSearchGroupStatus getDistantSearchStatus(const RsGxsGroupId& group_id)  override;

	// Overloaded to cache new groups.
virtual RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet) override;

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) override;

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel) override;

public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups ) override;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts, std::vector<RsGxsComment> &cmts, std::vector<RsGxsVote> &vots) override;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts, std::vector<RsGxsComment> &cmts) override;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts) override;
//Not currently used
//virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &posts);

        //////////////////////////////////////////////////////////////////////////////

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
	virtual bool groupShareKeys(
            const RsGxsGroupId &groupId, const std::set<RsPeerId>& peers) override;

virtual bool createGroup(uint32_t &token, RsGxsChannelGroup &group) override;
virtual bool createPost(uint32_t &token, RsGxsChannelPost &post) override;

virtual bool updateGroup(uint32_t &token, RsGxsChannelGroup &group) override;

// no tokens... should be cached.
virtual bool setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled) override;
virtual	bool getChannelAutoDownload(const RsGxsGroupId &groupid, bool& enabled) override;
virtual bool setChannelDownloadDirectory(const RsGxsGroupId &groupId, const std::string& directory) override;
virtual bool getChannelDownloadDirectory(const RsGxsGroupId &groupId, std::string& directory) override;

#ifdef TO_REMOVE
	/// @see RsGxsChannels::turtleSearchRequest
	virtual bool turtleSearchRequest(const std::string& matchString,
	        const std::function<void (const RsGxsGroupSummary&)>& multiCallback,
	        rstime_t maxWait = 300 ) override;

	/// @see RsGxsChannels::turtleChannelRequest
	virtual bool turtleChannelRequest(
	        const RsGxsGroupId& channelId,
	        const std::function<void (const RsGxsChannelGroup& result)>& multiCallback,
	        rstime_t maxWait = 300 ) override;

	/// @see RsGxsChannels::localSearchRequest
	virtual bool localSearchRequest(const std::string& matchString,
	        const std::function<void (const RsGxsGroupSummary& result)>& multiCallback,
	        rstime_t maxWait = 30 ) override;
#endif

	/**
	 * Receive results from turtle search @see RsGenExchange @see RsNxsObserver
	 * @see RsGxsNetService::receiveTurtleSearchResults
	 * @see p3turtle::handleSearchResult
	 */
	void receiveDistantSearchResults( TurtleRequestId id,
	                                  const RsGxsGroupId& grpId ) override;

	/* Comment service - Provide RsGxsCommentService - redirect to p3GxsCommentService */
	virtual bool getCommentData(uint32_t token, std::vector<RsGxsComment> &msgs) override
	{ return mCommentService->getGxsCommentData(token, msgs); }

	virtual bool getRelatedComments( uint32_t token,
	                                 std::vector<RsGxsComment> &msgs ) override
	{ return mCommentService->getGxsRelatedComments(token, msgs); }

	virtual bool createNewComment(uint32_t &token, const RsGxsComment &msg) override
	{
		return mCommentService->createGxsComment(token, msg);
	}

    virtual bool setCommentAsRead(uint32_t& token,const RsGxsGroupId& gid,const RsGxsMessageId& comment_msg_id) override
    {
        setMessageReadStatus(token,RsGxsGrpMsgIdPair(gid,comment_msg_id),true);
        return true;
    }

	virtual bool createNewVote(uint32_t &token, RsGxsVote &msg) override
	{
		return mCommentService->createGxsVote(token, msg);
	}

	virtual bool acknowledgeComment(uint32_t token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId) override
	{
		return acknowledgeMsg(token, msgId);
	}

	virtual bool acknowledgeVote(uint32_t token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId) override
	{
		if (mCommentService->acknowledgeVote(token, msgId))
		{
			return true;
		}
		return acknowledgeMsg(token, msgId);
	}


	// Overloaded from RsGxsIface.
virtual bool subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe) override;

	// Set Statuses.
virtual void setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed);
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) override;

	// File Interface
	virtual bool ExtraFileHash(const std::string& path) override;
virtual bool ExtraFileRemove(const RsFileHash &hash) override;


	/// Implementation of @see RsGxsChannels::getChannelsSummaries
	bool getChannelsSummaries(std::list<RsGroupMetaData>& channels) override;

    /// get the metadata of a single channel
    bool getChannelSummary(const RsGxsGroupId& grpId,RsGroupMetaData& meta);

    /// Implementation of @see RsGxsChannels::getChannelsInfo
	bool getChannelsInfo(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelGroup>& channelsInfo ) override;

	/// Implementation of @see RsGxsChannels::getChannelAllMessages
	bool getChannelAllContent(const RsGxsGroupId& channelId,
	                          std::vector<RsGxsChannelPost>& posts,
	                          std::vector<RsGxsComment>& comments,
	                          std::vector<RsGxsVote>& votes ) override;

	/// Implementation of @see RsGxsChannels::getChannelMessages
	bool getChannelContent(const RsGxsGroupId& channelId,
	                        const std::set<RsGxsMessageId>& contentIds,
	                        std::vector<RsGxsChannelPost>& posts,
	                        std::vector<RsGxsComment>& comments,
	                        std::vector<RsGxsVote>& votes ) override;

	/// Implementation of @see RsGxsChannels::getChannelComments
	virtual bool getChannelComments(const RsGxsGroupId &channelId,
	                                const std::set<RsGxsMessageId> &contentIds,
	                                std::vector<RsGxsComment> &comments) override;

	/// Implementation of @see RsGxsChannels::getContentSummaries
	bool getContentSummaries(
	        const RsGxsGroupId& channelId,
	        std::vector<RsMsgMetaData>& summaries ) override;

    /// Implementation of @see RsGxsChannels::getChannelStatistics
    bool getChannelStatistics(const RsGxsGroupId& channelId,GxsGroupStatistic& stat) override;

    /// Iplementation of @see RsGxsChannels::getChannelServiceStatistics
    bool getChannelServiceStatistics(GxsServiceStatistic& stat) override;

	/// Implementation of @see RsGxsChannels::createChannelV2
	bool createChannelV2(
	        const std::string& name, const std::string& description,
	        const RsGxsImage& thumbnail = RsGxsImage(),
	        const RsGxsId& authorId = RsGxsId(),
	        RsGxsCircleType circleType = RsGxsCircleType::PUBLIC,
	        const RsGxsCircleId& circleId = RsGxsCircleId(),
	        RsGxsGroupId& channelId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// Implementation of @see RsGxsChannels::createComment
	bool createCommentV2(
	        const RsGxsGroupId&   channelId,
	        const RsGxsMessageId& threadId,
	        const std::string&    comment,
	        const RsGxsId&        authorId,
	        const RsGxsMessageId& parentId = RsGxsMessageId(),
	        const RsGxsMessageId& origCommentId = RsGxsMessageId(),
	        RsGxsMessageId&       commentMessageId = RS_DEFAULT_STORAGE_PARAM(RsGxsMessageId),
	        std::string&          errorMessage     = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// Implementation of @see RsGxsChannels::editChannel
	bool editChannel(RsGxsChannelGroup& channel) override;

	/// Implementation of @see RsGxsChannels::createPostV2
	bool createPostV2(
	        const RsGxsGroupId& channelId, const std::string& title,
	        const std::string& body,
	        const std::list<RsGxsFile>& files = std::list<RsGxsFile>(),
	        const RsGxsImage& thumbnail = RsGxsImage(),
	        const RsGxsMessageId& origPostId = RsGxsMessageId(),
	        RsGxsMessageId& postId = RS_DEFAULT_STORAGE_PARAM(RsGxsMessageId),
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// Implementation of @see RsGxsChannels::createVoteV2
	bool createVoteV2(
	        const RsGxsGroupId& channelId, const RsGxsMessageId& postId,
	        const RsGxsMessageId& commentId, const RsGxsId& authorId,
	        RsGxsVoteType vote,
	        RsGxsMessageId& voteId = RS_DEFAULT_STORAGE_PARAM(RsGxsMessageId),
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// Implementation of @see RsGxsChannels::subscribeToChannel
	bool subscribeToChannel( const RsGxsGroupId &groupId,
	                                 bool subscribe ) override;

	/// @see RsGxsChannels
	virtual bool markRead(const RsGxsGrpMsgIdPair& msgId, bool read) override;

	/// @see RsGxsChannels
	bool exportChannelLink(
	        std::string& link, const RsGxsGroupId& chanId,
	        bool includeGxsData = true,
	        const std::string& baseUrl = DEFAULT_CHANNEL_BASE_URL,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsGxsChannels
	bool importChannelLink(
	        const std::string& link,
	        RsGxsGroupId& chanId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	virtual bool shareChannelKeys(
	        const RsGxsGroupId& channelId, const std::set<RsPeerId>& peers ) override;

	/// Implementation of @see RsGxsChannels::createChannel
	RS_DEPRECATED_FOR(createChannelV2)
	bool createChannel(RsGxsChannelGroup& channel) override;

	/// @deprecated Implementation of @see RsGxsChannels::createPost
	RS_DEPRECATED_FOR(createPostV2)
	bool createPost(RsGxsChannelPost& post) override;

	/// @deprecated Implementation of @see RsGxsChannels::createComment
	RS_DEPRECATED_FOR(createCommentV2)
	bool createComment(RsGxsComment &comment) override;

	/// @deprecated Implementation of @see RsGxsChannels::createVote
	RS_DEPRECATED_FOR(createVoteV2)
	bool createVote(RsGxsVote& vote) override;


protected:
	// Overloaded from GxsTokenQueue for Request callbacks.
	virtual void handleResponse(uint32_t token, uint32_t req_type
	                            , RsTokenService::GxsRequestStatus status) override;


private:

static uint32_t channelsAuthenPolicy();

	// Handle Processing.
	void request_AllSubscribedGroups();
	void request_SpecificSubscribedGroups(const std::list<RsGxsGroupId> &groups);
	void load_SubscribedGroups(const uint32_t &token);

	void request_SpecificUnprocessedPosts(std::list<std::pair<RsGxsGroupId, RsGxsMessageId> > &ids);
	void request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist);
	void load_unprocessedPosts(uint32_t token);

	void handleUnprocessedPost(const RsGxsChannelPost &msg);

	// Local Cache of Subscribed Groups. and AutoDownload Flag.
	void updateSubscribedGroup(const RsGroupMetaData &group);
	void clearUnsubscribedGroup(const RsGxsGroupId &id);
	bool setAutoDownload(const RsGxsGroupId &groupId, bool enabled);
	bool autoDownloadEnabled(const RsGxsGroupId &groupId, bool &enabled);
    bool checkForOldAndUnusedChannels();

// DUMMY DATA,
virtual bool generateDummyData();

std::string genRandomId();

void 	dummy_tick();

bool generatePost(uint32_t &token, const RsGxsGroupId &grpId);
bool generateComment(uint32_t &token, const RsGxsGroupId &grpId, 
		const RsGxsMessageId &parentId, const RsGxsMessageId &threadId);
bool generateVote(uint32_t &token, const RsGxsGroupId &grpId, 
		const RsGxsMessageId &parentId, const RsGxsMessageId &threadId);

bool generateGroup(uint32_t &token, std::string groupName);

	class ChannelDummyRef
	{
		public:
		ChannelDummyRef() {}
		ChannelDummyRef(
		        const RsGxsGroupId &grpId, const RsGxsMessageId &threadId,
		        const RsGxsMessageId &msgId ) :
		    mGroupId(grpId), mThreadId(threadId), mMsgId(msgId) {}

		RsGxsGroupId mGroupId;
		RsGxsMessageId mThreadId;
		RsGxsMessageId mMsgId;
	};

	std::map<RsGxsGroupId, RsGroupMetaData> mSubscribedGroups;
	RsMutex mSubscribedGroupsMutex;

	/** G10h4ck: Is this stuff really used? And for what? BEGIN */
	uint32_t mGenToken;
	bool mGenActive;
	int mGenCount;
	std::vector<ChannelDummyRef> mGenRefs;
	RsGxsMessageId mGenThreadId;
	/** G10h4ck: Is this stuff really used? And for what? END */

	p3GxsCommentService* mCommentService;

	std::map<RsGxsGroupId,rstime_t> mKnownChannels;
	RsMutex mKnownChannelsMutex;

    rstime_t mLastDistantSearchNotificationTS;

    std::map<TurtleRequestId,std::set<RsGxsGroupId> > mSearchResultsToNotify;
#ifdef TO_REMOVE
	/** Store search callbacks with timeout*/
	std::map<
	    TurtleRequestId,
	    std::pair<
	        std::function<void (const RsGxsGroupSummary&)>,
	        std::chrono::system_clock::time_point >
	 > mSearchCallbacksMap;
	RsMutex mSearchCallbacksMapMutex;

	/** Store distant channels requests callbacks with timeout*/
	std::map<
	    TurtleRequestId,
	    std::pair<
	        std::function<void (const RsGxsChannelGroup&)>,
	        std::chrono::system_clock::time_point >
	 > mDistantChannelsCallbacksMap;
	RsMutex mDistantChannelsCallbacksMapMutex;

	/// Cleanup mSearchCallbacksMap and mDistantChannelsCallbacksMap
	void cleanTimedOutCallbacks();
#endif
};
