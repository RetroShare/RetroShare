/*
 * libretroshare/src/services p3gxsforums.cc
 *
 * GxsForums interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#include "services/p3gxsforums.h"
#include "serialiser/rsgxsforumitems.h"

#include <retroshare/rsidentity.h>


#include "retroshare/rsgxsflags.h"
#include <stdio.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define GXSFORUM_DEBUG 1
 ****/

RsGxsForums *rsGxsForums = NULL;


#define FORUM_TESTEVENT_DUMMYDATA	0x0001
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsForums::p3GxsForums(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsForumSerialiser(), RS_SERVICE_GXSV2_TYPE_FORUMS, gixs, forumsAuthenPolicy()), RsGxsForums(this)
{
	// For Dummy Msgs.
	mGenActive = false;

	// Test Data disabled in Repo.
	//RsTickEvent::schedule_in(FORUM_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);

}

uint32_t p3GxsForums::forumsAuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}


void p3GxsForums::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	RsGxsIfaceHelper::receiveChanges(changes);
}

void	p3GxsForums::service_tick()
{
	dummy_tick();
	RsTickEvent::tick_events();
	return;
}

bool p3GxsForums::getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsForumGroupItem* item = dynamic_cast<RsGxsForumGroupItem*>(*vit);
			if (item)
			{
				RsGxsForumGroup grp = item->mGroup;
				item->mGroup.mMeta = item->meta;
				grp.mMeta = item->mGroup.mMeta;
				delete item;
				groups.push_back(grp);
			}
			else
			{
				std::cerr << "Not a GxsForumGrpItem, deleting!" << std::endl;
				delete *vit;
			}
		}
	}
	return ok;
}

/* Okay - chris is not going to be happy with this...
 * but I can't be bothered with crazy data structures
 * at the moment - fix it up later
 */

bool p3GxsForums::getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();

		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); vit++)
			{
				RsGxsForumMsgItem* item = dynamic_cast<RsGxsForumMsgItem*>(*vit);

				if(item)
				{
					RsGxsForumMsg msg = item->mMsg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsForumMsgItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool p3GxsForums::getRelatedMessages(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs)
{
	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsForumMsgItem* item = dynamic_cast<RsGxsForumMsgItem*>(*vit);
		
				if(item)
				{
					RsGxsForumMsg msg = item->mMsg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsForumMsgItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
			
	return ok;
}

/********************************************************************************************/

bool p3GxsForums::createGroup(uint32_t &token, RsGxsForumGroup &group)
{
	std::cerr << "p3GxsForums::createGroup()" << std::endl;

	RsGxsForumGroupItem* grpItem = new RsGxsForumGroupItem();
	grpItem->mGroup = group;
	grpItem->meta = group.mMeta;

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}

bool p3GxsForums::updateGroup(uint32_t &token, RsGxsForumGroup &group)
{
	std::cerr << "p3GxsForums::updateGroup()" << std::endl;

	RsGxsForumGroupItem* grpItem = new RsGxsForumGroupItem();
	grpItem->mGroup = group;
        grpItem->meta = group.mMeta;

        RsGenExchange::updateGroup(token, grpItem);
	return true;
}

bool p3GxsForums::createMsg(uint32_t &token, RsGxsForumMsg &msg)
{
	std::cerr << "p3GxsForums::createForumMsg() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsForumMsgItem* msgItem = new RsGxsForumMsgItem();
	msgItem->mMsg = msg;
	msgItem->meta = msg.mMeta;
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}


/********************************************************************************************/
/********************************************************************************************/

void p3GxsForums::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNREAD | GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNREAD;
	if (read)
	{
		status = 0;
	}

	setMsgStatusFlags(token, msgId, status, mask);

}

/********************************************************************************************/
/********************************************************************************************/

/* so we need the same tick idea as wiki for generating dummy forums
 */

#define 	MAX_GEN_GROUPS		5
#define 	MAX_GEN_MESSAGES	100

std::string p3GxsForums::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

bool p3GxsForums::generateDummyData()
{
	mGenCount = 0;
	mGenRefs.resize(MAX_GEN_MESSAGES);

	std::string groupName;
	rs_sprintf(groupName, "TestForum_%d", mGenCount);

	std::cerr << "p3GxsForums::generateDummyData() Starting off with Group: " << groupName;
	std::cerr << std::endl;

	/* create a new group */
	generateGroup(mGenToken, groupName);

	mGenActive = true;

	return true;
}


void p3GxsForums::dummy_tick()
{
	/* check for a new callback */

	if (mGenActive)
	{
		std::cerr << "p3GxsForums::dummyTick() AboutActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
		if (status != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3GxsForums::dummy_tick() Status: " << status;
			std::cerr << std::endl;

			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
				std::cerr << "p3GxsForums::dummy_tick() generateDummyMsgs() FAILED";
				std::cerr << std::endl;
				mGenActive = false;
			}
			return;
		}

		if (mGenCount < MAX_GEN_GROUPS)
		{
			/* get the group Id */
			RsGxsGroupId groupId;
			RsGxsMessageId emptyId;
			if (!acknowledgeTokenGrp(mGenToken, groupId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3GxsForums::dummy_tick() Acknowledged GroupId: " << groupId;
			std::cerr << std::endl;

			ForumDummyRef ref(groupId, emptyId, emptyId);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_MESSAGES)
		{
			/* get the msg Id, and generate next snapshot */
			RsGxsGrpMsgIdPair msgId;
			if (!acknowledgeTokenMsg(mGenToken, msgId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3GxsForums::dummy_tick() Acknowledged <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;

			/* store results for later selection */

			ForumDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else
		{
			std::cerr << "p3GxsForums::dummy_tick() Finished";
			std::cerr << std::endl;

			/* done */
			mGenActive = false;
			return;
		}

		mGenCount++;

		if (mGenCount < MAX_GEN_GROUPS)
		{
			std::string groupName;
			rs_sprintf(groupName, "TestForum_%d", mGenCount);

			std::cerr << "p3GxsForums::dummy_tick() Generating Group: " << groupName;
			std::cerr << std::endl;

			/* create a new group */
			generateGroup(mGenToken, groupName);
		}
		else
		{
			/* create a new message */
			uint32_t idx = (uint32_t) (mGenCount * RSRandom::random_f32());
			ForumDummyRef &ref = mGenRefs[idx];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.empty())
			{
				mGenThreadId = parentId;
			}

			std::cerr << "p3GxsForums::dummy_tick() Generating Msg ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;

			generateMessage(mGenToken, grpId, parentId, mGenThreadId);
		}
	}
}


bool p3GxsForums::generateMessage(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsForumMsg msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mMsg, "Forum Msg: GroupId: %s, ThreadId: %s, ParentId: %s + some randomness: %s", 
		grpId.c_str(), threadId.c_str(), parentId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mMsg;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = threadId;
	msg.mMeta.mParentId = parentId;

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	int i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++) ;

	if (it != ownIds.end())
	{
		std::cerr << "p3GxsForums::generateMessage() Author: " << *it;
		std::cerr << std::endl;
		msg.mMeta.mAuthorId = *it;
	} 
	else
	{
		std::cerr << "p3GxsForums::generateMessage() No Author!";
		std::cerr << std::endl;
	} 

	createMsg(token, msg);

	return true;
}


bool p3GxsForums::generateGroup(uint32_t &token, std::string groupName)
{
	/* generate a new forum */
	RsGxsForumGroup forum;
	forum.mMeta.mGroupName = groupName;

	createGroup(token, forum);

	return true;
}


        // Overloaded from RsTickEvent for Event callbacks.
void p3GxsForums::handle_event(uint32_t event_type, const std::string &elabel)
{
	std::cerr << "p3GxsForums::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
		case FORUM_TESTEVENT_DUMMYDATA:
			generateDummyData();
			break;

		default:
			/* error */
			std::cerr << "p3GxsForums::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}

