/*******************************************************************************
 * libretroshare/src/services: p3gxsforums.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
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

#include <map>
#include <string>

#include "retroshare/rsgxsforums.h"
#include "gxs/rsgenexchange.h"
#include "retroshare/rsgxscircles.h"
#include "util/rstickevent.h"
#include "util/rsdebug.h"


class p3GxsForums: public RsGenExchange, public RsGxsForums, public p3Config,
	public RsTickEvent	/* only needed for testing - remove after */
{
public:
	p3GxsForums(
	        RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs);

	virtual RsServiceInfo getServiceInfo();
	virtual void service_tick();

protected:
	virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);
	/// Overloaded from RsTickEvent.
	virtual void handle_event(uint32_t event_type, const std::string &elabel);

	virtual RsSerialiser* setupSerialiser();                            // @see p3Config::setupSerialiser()
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList); // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList);               // @see p3Config::loadList(std::list<RsItem *>&)

public:
	/// @see RsGxsForums::createForumV2
	bool createForumV2(
	        const std::string& name, const std::string& description,
	        const RsGxsId& authorId = RsGxsId(),
	        const std::set<RsGxsId>& moderatorsIds = std::set<RsGxsId>(),
	        RsGxsCircleType circleType = RsGxsCircleType::PUBLIC,
	        const RsGxsCircleId& circleId = RsGxsCircleId(),
	        RsGxsGroupId& forumId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsGxsForums::createPost
	bool createPost(
	        const RsGxsGroupId&   forumId,
	        const std::string&    title,
	        const std::string&    mBody,
	        const RsGxsId&        authorId,
	        const RsGxsMessageId& parentId = RsGxsMessageId(),
	        const RsGxsMessageId& origPostId = RsGxsMessageId(),
	        RsGxsMessageId&       postMsgId = RS_DEFAULT_STORAGE_PARAM(RsGxsMessageId),
	        std::string&          errorMessage     = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsGxsForums::createForum @deprecated
	RS_DEPRECATED_FOR(createForumV2)
	virtual bool createForum(RsGxsForumGroup& forum);

	/// @see RsGxsForums::createMessage  @deprecated
	RS_DEPRECATED_FOR(createPost)
	virtual bool createMessage(RsGxsForumMsg& message);

	/// @see RsGxsForums::editForum
	virtual bool editForum(RsGxsForumGroup& forum) override;

	/// @see RsGxsForums::getForumsSummaries
	virtual bool getForumsSummaries(std::list<RsGroupMetaData>& forums);

	/// @see RsGxsForums::getForumsInfo
	virtual bool getForumsInfo(
	        const std::list<RsGxsGroupId>& forumIds,
	        std::vector<RsGxsForumGroup>& forumsInfo );

    /// Implementation of @see RsGxsForums::getForumStatistics
    bool getForumStatistics(const RsGxsGroupId& ForumId,GxsGroupStatistic& stat) override;

    /// Implementation of @see RsGxsForums::getForumServiceStatistics
	bool getForumServiceStatistics(GxsServiceStatistic& stat) override;

	/// @see RsGxsForums::getForumMsgMetaData
	virtual bool getForumMsgMetaData(const RsGxsGroupId& forumId, std::vector<RsMsgMetaData>& msg_metas) ;

	/// @see RsGxsForums::getForumContent
	virtual bool getForumContent(
	        const RsGxsGroupId& forumId,
	        const std::set<RsGxsMessageId>& msgs_to_request,
	        std::vector<RsGxsForumMsg>& msgs );

	/// @see RsGxsForums::markRead
	virtual bool markRead(const RsGxsGrpMsgIdPair& messageId, bool read);

	/// @see RsGxsForums::subscribeToForum
	virtual bool subscribeToForum( const RsGxsGroupId& forumId,
	                               bool subscribe );

	/// @see RsGxsForums
	bool exportForumLink(
	        std::string& link, const RsGxsGroupId& forumId,
	        bool includeGxsData = true,
	        const std::string& baseUrl = DEFAULT_FORUM_BASE_URL,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsGxsForums
	bool importForumLink(
	        const std::string& link,
	        RsGxsGroupId& forumId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

    /// implementation of rsGxsGorums
    ///
	bool getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups) override;
	bool getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) override;
	void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) override;
	bool createGroup(uint32_t &token, RsGxsForumGroup &group) override;
	bool createMsg(uint32_t &token, RsGxsForumMsg &msg) override;
	bool updateGroup(uint32_t &token, const RsGxsForumGroup &group) override;

	bool getMsgMetaData(const uint32_t &token, GxsMsgMetaMap& msg_metas) ;

private:

static uint32_t forumsAuthenPolicy();

virtual bool generateDummyData();

std::string genRandomId();

void 	dummy_tick();

bool generateMessage(uint32_t &token, const RsGxsGroupId &grpId, 
		const RsGxsMessageId &parentId, const RsGxsMessageId &threadId);
bool generateGroup(uint32_t &token, std::string groupName);

	class ForumDummyRef
	{
		public:
		ForumDummyRef() { return; }
		ForumDummyRef(const RsGxsGroupId &grpId, const RsGxsMessageId &threadId, const RsGxsMessageId &msgId)
		:mGroupId(grpId), mThreadId(threadId), mMsgId(msgId) { return; }

		RsGxsGroupId mGroupId;
		RsGxsMessageId mThreadId;
		RsGxsMessageId mMsgId;
	};

	uint32_t mGenToken;
	bool mGenActive;
	int mGenCount;
	std::vector<ForumDummyRef> mGenRefs;
	RsGxsMessageId mGenThreadId;
    std::map<RsGxsGroupId,rstime_t> mKnownForums ;
	
	RsMutex mKnownForumsMutex;
};
