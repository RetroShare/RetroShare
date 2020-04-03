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



class SSGxsChannelGroup
{
	public:
	SSGxsChannelGroup(): mAutoDownload(false), mDownloadDirectory("") {}
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
	virtual RsServiceInfo getServiceInfo();

	virtual void service_tick();

protected:


	virtual RsSerialiser* setupSerialiser();                            // @see p3Config::setupSerialiser()
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList); // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList);               // @see p3Config::loadList(std::list<RsItem *>&)

    virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id);
    virtual TurtleRequestId turtleSearchRequest(const std::string& match_string);
    virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSummary> &results) ;
    virtual bool clearDistantSearchResults(TurtleRequestId req);
    virtual bool retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChannelGroup& distant_group);

	// Overloaded to cache new groups.
virtual RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups);
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts, std::vector<RsGxsComment> &cmts);
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts);
//Not currently used
//virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &posts);

        //////////////////////////////////////////////////////////////////////////////

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
	virtual bool groupShareKeys(
	        const RsGxsGroupId &groupId, const std::set<RsPeerId>& peers);

virtual bool createGroup(uint32_t &token, RsGxsChannelGroup &group);
virtual bool createPost(uint32_t &token, RsGxsChannelPost &post);

virtual bool updateGroup(uint32_t &token, RsGxsChannelGroup &group);

// no tokens... should be cached.
virtual bool setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled);
virtual	bool getChannelAutoDownload(const RsGxsGroupId &groupid, bool& enabled);
virtual bool setChannelDownloadDirectory(const RsGxsGroupId &groupId, const std::string& directory);
virtual bool getChannelDownloadDirectory(const RsGxsGroupId &groupId, std::string& directory);

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
virtual bool subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe);

	// Set Statuses.
virtual void setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed);
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);

	// File Interface
	virtual bool ExtraFileHash(const std::string& path);
virtual bool ExtraFileRemove(const RsFileHash &hash);


	/// Implementation of @see RsGxsChannels::getChannelsSummaries
	bool getChannelsSummaries(std::list<RsGroupMetaData>& channels) override;

	/// Implementation of @see RsGxsChannels::getChannelsInfo
	bool getChannelsInfo(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelGroup>& channelsInfo ) override;

	/// Implementation of @see RsGxsChannels::getChannelAllMessages
	bool getChannelAllContent(const RsGxsGroupId& channelId,
	                        std::vector<RsGxsChannelPost>& posts,
	                        std::vector<RsGxsComment>& comments ) override;

	/// Implementation of @see RsGxsChannels::getChannelMessages
	bool getChannelContent(const RsGxsGroupId& channelId,
	                        const std::set<RsGxsMessageId>& contentIds,
	                        std::vector<RsGxsChannelPost>& posts,
	                        std::vector<RsGxsComment>& comments ) override;

	/// Implementation of @see RsGxsChannels::getContentSummaries
	bool getContentSummaries(
	        const RsGxsGroupId& channelId,
	        std::vector<RsMsgMetaData>& summaries ) override;

    /// Implementation of @see RsGxsChannels::getChannelStatistics
    bool getChannelStatistics(const RsGxsGroupId& channelId,GxsGroupStatistic& stat) override;

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
	virtual bool markRead(const RsGxsGrpMsgIdPair& msgId, bool read);

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
	        const RsGxsGroupId& channelId, const std::set<RsPeerId>& peers );

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
	virtual void handleResponse(uint32_t token, uint32_t req_type);


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
};
