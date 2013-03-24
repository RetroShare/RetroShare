/*
 * libretroshare/src/services: p3gxschannels.h
 *
 * GxsChannel interface for RetroShare.
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

#ifndef P3_GXSCHANNELS_SERVICE_HEADER
#define P3_GXSCHANNELS_SERVICE_HEADER


#include "retroshare/rsgxschannels.h"
#include "services/p3gxscommon.h"
#include "gxs/rsgenexchange.h"
#include "gxs/gxstokenqueue.h"

#include "util/rstickevent.h"

#include <map>
#include <string>

/* 
 *
 */


class SSGxsChannelGroup
{
	public:

	bool load(const std::string &input);
	std::string save() const;

	bool mAutoDownload;
};


class p3GxsChannels: public RsGenExchange, public RsGxsChannels, 
	public GxsTokenQueue,
	public RsTickEvent	/* only needed for testing - remove after */
{
	public:

	p3GxsChannels(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs);

virtual void service_tick();

	protected:


	// Overloaded to cache new groups.
virtual RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

	public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups);
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts);

virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &posts);

        //////////////////////////////////////////////////////////////////////////////

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

virtual bool createGroup(uint32_t &token, RsGxsChannelGroup &group);
virtual bool createPost(uint32_t &token, RsGxsChannelPost &post);

// no tokens... should be cached.
virtual bool setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled);
virtual	bool getChannelAutoDownload(const RsGxsGroupId &groupid);

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


	// Overloaded from RsGxsIface.
virtual bool subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe);

	// Set Statuses.
virtual void setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed);
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);

	// File Interface
virtual bool ExtraFileHash(const std::string &path, std::string filename);
virtual bool ExtraFileRemove(const std::string &hash);

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
	void load_SpecificUnprocessedPosts(const uint32_t &token);

	void request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist);
	void load_GroupUnprocessedPosts(const uint32_t &token);

	void handleUnprocessedPost(const RsGxsChannelPost &msg);

	// Local Cache of Subscribed Groups. and AutoDownload Flag.
	void updateSubscribedGroup(const RsGroupMetaData &group);
	void clearUnsubscribedGroup(const RsGxsGroupId &id);
	bool setAutoDownload(const RsGxsGroupId &groupId, bool enabled);
	bool autoDownloadEnabled(const RsGxsGroupId &id);



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
};

#endif 
