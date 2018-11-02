/*******************************************************************************
 * libretroshare/src/services: p3gxschannels.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#ifndef P3_GXSCHANNELS_SERVICE_HEADER
#define P3_GXSCHANNELS_SERVICE_HEADER


#include "gxs/gxstokenqueue.h"
#include "gxs/rsgenexchange.h"
#include "retroshare/rsgxschannels.h"
#include "services/p3gxscommon.h"
#include <util/cxx11retrocompat.h>
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
	virtual RsServiceInfo getServiceInfo() override;

	virtual void service_tick() override;

protected:


	virtual RsSerialiser* setupSerialiser() override;                            // @see p3Config::setupSerialiser()
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList) override; // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList) override;               // @see p3Config::loadList(std::list<RsItem *>&)

    virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id) override;
    virtual TurtleRequestId turtleSearchRequest(const std::string& match_string) override;
    virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSummary> &results) override;
    virtual bool clearDistantSearchResults(TurtleRequestId req) override;
    virtual bool retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChannelGroup& distant_group) override;

	// Overloaded to cache new groups.
virtual RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet) override;

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) override;

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel) override;

public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups) override;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts, std::vector<RsGxsComment> &cmts) override;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts) override {	std::vector<RsGxsComment> cmts; return getPostData( token, posts, cmts);}
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

	/// @see RsGxsChannels::turtleSearchRequest
	virtual bool turtleSearchRequest(const std::string& matchString,
	        const std::function<void (const RsGxsGroupSummary&)>& multiCallback,
	        rstime_t maxWait = 300 ) override;

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

virtual bool createComment(uint32_t &token, RsGxsComment &msg) override
	{
		return mCommentService->createGxsComment(token, msg);
	}

virtual bool createVote(uint32_t &token, RsGxsVote &msg) override
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
	virtual bool getChannelsSummaries(std::list<RsGroupMetaData>& channels) override;

	/// Implementation of @see RsGxsChannels::getChannelsInfo
	virtual bool getChannelsInfo(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelGroup>& channelsInfo ) override;

	/// Implementation of @see RsGxsChannels::getChannelContent
	virtual bool getChannelsContent(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelPost>& posts,
	        std::vector<RsGxsComment>& comments ) override;

	/// Implementation of @see RsGxsChannels::createChannel
	virtual bool createChannel(RsGxsChannelGroup& channel) override;

	/// Implementation of @see RsGxsChannels::createPost
	virtual bool createPost(RsGxsChannelPost& post) override;

protected:
	// Overloaded from GxsTokenQueue for Request callbacks.
	virtual void handleResponse(uint32_t token, uint32_t req_type) override;


private:

static uint32_t channelsAuthenPolicy();

	// Handle Processing.
	void request_AllSubscribedGroups();
	void request_SpecificSubscribedGroups(const std::list<RsGxsGroupId> &groups);
	void load_SubscribedGroups(const uint32_t &token);

	void request_SpecificUnprocessedPosts(std::list<std::pair<RsGxsGroupId, RsGxsMessageId> > &ids);
	void load_SpecificUnprocessedPosts(const uint32_t &token);

	void request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist);
	void load_GroupUnprocessedPosts(const uint32_t &token);

	void handleUnprocessedPost(const RsGxsChannelPost &msg);

	// Local Cache of Subscribed Groups. and AutoDownload Flag.
	void updateSubscribedGroup(const RsGroupMetaData &group);
	void clearUnsubscribedGroup(const RsGxsGroupId &id);
	bool setAutoDownload(const RsGxsGroupId &groupId, bool enabled);
	bool autoDownloadEnabled(const RsGxsGroupId &groupId, bool &enabled);



	std::map<RsGxsGroupId, RsGroupMetaData> mSubscribedGroups;


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
		ChannelDummyRef() { return; }
		ChannelDummyRef(const RsGxsGroupId &grpId, const RsGxsMessageId &threadId, const RsGxsMessageId &msgId)
		:mGroupId(grpId), mThreadId(threadId), mMsgId(msgId) { return; }

		RsGxsGroupId mGroupId;
		RsGxsMessageId mThreadId;
		RsGxsMessageId mMsgId;
	};

	uint32_t mGenToken;
	bool mGenActive;
	int mGenCount;
	std::vector<ChannelDummyRef> mGenRefs;
	RsGxsMessageId mGenThreadId;

	p3GxsCommentService *mCommentService;
    std::map<RsGxsGroupId,rstime_t> mKnownChannels;

	/** Store search callbacks with timeout*/
	std::map<
	    TurtleRequestId,
	    std::pair<
	        std::function<void (const RsGxsGroupSummary&)>,
	        std::chrono::system_clock::time_point >
	 > mSearchCallbacksMap;
	RsMutex mSearchCallbacksMapMutex;

	/// Cleanup mSearchCallbacksMap
	void cleanTimedOutSearches();
};

#endif 
