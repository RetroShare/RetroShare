/*******************************************************************************
 * libretroshare/src/services: p3gxsforums.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef P3_GXSFORUMS_SERVICE_HEADER
#define P3_GXSFORUMS_SERVICE_HEADER


#include "retroshare/rsgxsforums.h"
#include "gxs/rsgenexchange.h"

#include "util/rstickevent.h"

#include <map>
#include <string>

/* 
 *
 */

class p3GxsForums: public RsGenExchange, public RsGxsForums, public p3Config,
	public RsTickEvent	/* only needed for testing - remove after */
{
	public:

	p3GxsForums(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs);

virtual RsServiceInfo getServiceInfo();

virtual void service_tick();

	protected:


virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

	virtual RsSerialiser* setupSerialiser();                            // @see p3Config::setupSerialiser()
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList); // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList);               // @see p3Config::loadList(std::list<RsItem *>&)

	public:

virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups);
virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs);
//Not currently used
//virtual bool getRelatedMessages(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs);

        //////////////////////////////////////////////////////////////////////////////
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

	virtual bool createForum(RsGxsForumGroup& forum);
	virtual bool createMessage(RsGxsForumMsg& message);

virtual bool createGroup(uint32_t &token, RsGxsForumGroup &group);
virtual bool createMsg(uint32_t &token, RsGxsForumMsg &msg);

/*!
 * To update forum group with new information
 * @param token the token used to check completion status of update
 * @param group group to be updated, groupId element must be set or will be rejected
 * @return false groupId not set, true if set and accepted (still check token for completion)
 */
virtual bool updateGroup(uint32_t &token, RsGxsForumGroup &group);


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
	
};

#endif 
