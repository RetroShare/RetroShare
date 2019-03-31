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


#include "retroshare/rsgxschannels.h"
#include "services/p3gxscommon.h"
#include "gxs/rsgenexchange.h"
#include "gxs/gxstokenqueue.h"

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
	        rstime_t maxWait = 300 );

	/// @see RsGxsChannels::turtleChannelRequest
	virtual bool turtleChannelRequest(
	        const RsGxsGroupId& channelId,
	        const std::function<void (const RsGxsChannelGroup& result)>& multiCallback,
	        rstime_t maxWait = 300 );

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

	virtual bool createNewComment(uint32_t &token, RsGxsComment &msg) override
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
	virtual bool getChannelsSummaries(std::list<RsGroupMetaData>& channels);

	/// Implementation of @see RsGxsChannels::getChannelsInfo
	virtual bool getChannelsInfo(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelGroup>& channelsInfo );

	/// Implementation of @see RsGxsChannels::getChannelContent
	virtual bool getChannelContent( const RsGxsGroupId& channelId,
	                       const std::set<RsGxsMessageId>& contentsIds,
	                       std::vector<RsGxsChannelPost>& posts,
	                       std::vector<RsGxsComment>& comments );

	/// Implementation of @see RsGxsChannels::getContentSummaries
	virtual bool getContentSummaries( const RsGxsGroupId& channelId,
	                                  std::vector<RsMsgMetaData>& summaries );

#ifdef REMOVED
	/// Implementation of @see RsGxsChannels::createChannel
	virtual bool createChannel(RsGxsChannelGroup& channel);
#endif

	/// Implementation of @see RsGxsChannels::createChannel
	virtual bool createChannel(const std::string& name,
                               const std::string& description,
                               const RsGxsImage& image,
                               const RsGxsId& author_id,
                               uint32_t circle_type,
                               RsGxsCircleId& circle_id,
                               RsGxsGroupId& channel_group_id,
                               std::string& error_message);

#ifdef REMOVED
	/// Implementation of @see RsGxsChannels::createComment
	virtual bool createComment(RsGxsComment& comment);
#endif

	/// Implementation of @see RsGxsChannels::createComment
	virtual bool createComment(const RsGxsGroupId&   groupId,
	                           const RsGxsMessageId& parentMsgId,
                               const std::string&    comment,
	                           RsGxsMessageId&       commentMessageId,
                               std::string&          error_message);

	/// Implementation of @see RsGxsChannels::editChannel
	virtual bool editChannel(RsGxsChannelGroup& channel);

#ifdef REMOVED
	/// Implementation of @see RsGxsChannels::createPost
	virtual bool createPost(RsGxsChannelPost& post);
#endif
	/// Implementation of @see RsGxsChannels::createPost
	virtual bool createPost(const RsGxsGroupId& groupId,
    						const RsGxsMessageId& origMsgId,
	                        const std::string& msgName,
							const std::string& msg,
							const std::list<RsGxsFile>& files,
							const RsGxsImage& thumbnail,
                            RsGxsMessageId &message_id,
                            std::string& error_message) ;

#ifdef REMOVED
	/// Implementation of @see RsGxsChannels::createVote
	virtual bool createVote(RsGxsVote& vote);
#endif

	/// Implementation of @see RsGxsChannels::createVote
	virtual bool createVote(const RsGxsGroupId&         groupId,
                             const RsGxsMessageId&       threadId,
                             const RsGxsMessageId&       commentMessageId,
                             const RsGxsId&              authorId,
                             uint32_t                    voteType,
                             RsGxsMessageId&             voteMessageId,
                             std::string&                error_message);

	/// Implementation of @see RsGxsChannels::subscribeToChannel
	virtual bool subscribeToChannel( const RsGxsGroupId &groupId,
	                                 bool subscribe );

	/// Implementation of @see RsGxsChannels::setPostRead
	virtual bool markRead(const RsGxsGrpMsgIdPair& msgId, bool read);

	virtual bool shareChannelKeys(
	        const RsGxsGroupId& channelId, const std::set<RsPeerId>& peers );

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
		ChannelDummyRef() { return; }
		ChannelDummyRef(const RsGxsGroupId &grpId, const RsGxsMessageId &threadId, const RsGxsMessageId &msgId)
		:mGroupId(grpId), mThreadId(threadId), mMsgId(msgId) { return; }

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

#endif 
