/*
 * libretroshare/src/services p3gxschannels.cc
 *
 * GxsChannels interface for RetroShare.
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

#include "services/p3gxschannels.h"
#include "serialiser/rsgxschannelitems.h"

#include <retroshare/rsidentity.h>


#include "retroshare/rsgxsflags.h"
#include <stdio.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define GXSCHANNEL_DEBUG 1
 ****/
#define GXSCHANNEL_DEBUG 1

RsGxsChannels *rsGxsChannels = NULL;


#define CHANNEL_TESTEVENT_DUMMYDATA	0x0001
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsChannels::p3GxsChannels(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsChannelSerialiser(), RS_SERVICE_GXSV1_TYPE_CHANNELS, gixs, channelsAuthenPolicy()), RsGxsChannels(this)
{
	// For Dummy Msgs.
	mGenActive = false;
	mCommentService = new p3GxsCommentService(this,  RS_SERVICE_GXSV1_TYPE_CHANNELS);

//#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET

	RsTickEvent::schedule_in(CHANNEL_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);

//#endif

}



uint32_t p3GxsChannels::channelsAuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = 0;

	flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	//RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN; 
	//RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	//RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	//RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	return policy;
}





void p3GxsChannels::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	 RsGxsIfaceHelper::receiveChanges(changes);
}

void	p3GxsChannels::service_tick()
{
	dummy_tick();
	RsTickEvent::tick_events();
	return;
}

bool p3GxsChannels::getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsChannelGroupItem* item = dynamic_cast<RsGxsChannelGroupItem*>(*vit);
			RsGxsChannelGroup grp = item->mGroup;
			item->mGroup.mMeta = item->meta;
			grp.mMeta = item->mGroup.mMeta;
			delete item;
			groups.push_back(grp);
		}
	}
	return ok;
}

/* Okay - chris is not going to be happy with this...
 * but I can't be bothered with crazy data structures
 * at the moment - fix it up later
 */

bool p3GxsChannels::getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
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
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);
		
				if(item)
				{
					RsGxsChannelPost msg = item->mMsg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsChannelPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
		
	return ok;
}


bool p3GxsChannels::getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
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
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);
		
				if(item)
				{
					RsGxsChannelPost msg = item->mMsg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsChannelPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
			
	return ok;
}


/********************************************************************************************/

bool p3GxsChannels::createGroup(uint32_t &token, RsGxsChannelGroup &group)
{
	std::cerr << "p3GxsChannels::createGroup()" << std::endl;

	RsGxsChannelGroupItem* grpItem = new RsGxsChannelGroupItem();
	grpItem->mGroup = group;
	grpItem->meta = group.mMeta;

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3GxsChannels::createPost(uint32_t &token, RsGxsChannelPost &msg)
{
	std::cerr << "p3GxsChannels::createChannelPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsChannelPostItem* msgItem = new RsGxsChannelPostItem();
	msgItem->mMsg = msg;
	msgItem->meta = msg.mMeta;
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}


/********************************************************************************************/
/********************************************************************************************/

void p3GxsChannels::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
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

/* so we need the same tick idea as wiki for generating dummy channels
 */

#define 	MAX_GEN_GROUPS		5
#define 	MAX_GEN_MESSAGES	100

std::string p3GxsChannels::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

bool p3GxsChannels::generateDummyData()
{
	mGenCount = 0;
	mGenRefs.resize(MAX_GEN_MESSAGES);

	std::string groupName;
	rs_sprintf(groupName, "TestChannel_%d", mGenCount);

	std::cerr << "p3GxsChannels::generateDummyData() Starting off with Group: " << groupName;
	std::cerr << std::endl;

	/* create a new group */
	generateGroup(mGenToken, groupName);

	mGenActive = true;

	return true;
}


void p3GxsChannels::dummy_tick()
{
	/* check for a new callback */

	if (mGenActive)
	{
		std::cerr << "p3GxsChannels::dummyTick() AboutActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
		if (status != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3GxsChannels::dummy_tick() Status: " << status;
			std::cerr << std::endl;

			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
				std::cerr << "p3GxsChannels::dummy_tick() generateDummyMsgs() FAILED";
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

			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged GroupId: " << groupId;
			std::cerr << std::endl;

			ChannelDummyRef ref(groupId, emptyId, emptyId);
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

			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;

			/* store results for later selection */

			ChannelDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else
		{
			std::cerr << "p3GxsChannels::dummy_tick() Finished";
			std::cerr << std::endl;

			/* done */
			mGenActive = false;
			return;
		}

		mGenCount++;

		if (mGenCount < MAX_GEN_GROUPS)
		{
			std::string groupName;
			rs_sprintf(groupName, "TestChannel_%d", mGenCount);

			std::cerr << "p3GxsChannels::dummy_tick() Generating Group: " << groupName;
			std::cerr << std::endl;

			/* create a new group */
			generateGroup(mGenToken, groupName);
		}
		else
		{
			/* create a new message */
			uint32_t idx = (uint32_t) (mGenCount * RSRandom::random_f32());
			ChannelDummyRef &ref = mGenRefs[idx];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.empty())
			{
				mGenThreadId = parentId;
			}

			std::cerr << "p3GxsChannels::dummy_tick() Generating Msg ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;

			if (parentId.empty())
			{
				generatePost(mGenToken, grpId);
			}
			else
			{
				generateComment(mGenToken, grpId, parentId, mGenThreadId);
			}
		}
	}
}


bool p3GxsChannels::generatePost(uint32_t &token, const RsGxsGroupId &grpId)
{
	RsGxsChannelPost msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mMsg, "Channel Msg: GroupId: %s, some randomness: %s", 
		grpId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mMsg;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = "";
	msg.mMeta.mParentId = "";

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	createPost(token, msg);

	return true;
}


bool p3GxsChannels::generateComment(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsComment msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mComment, "Channel Comment: GroupId: %s, ThreadId: %s, ParentId: %s + some randomness: %s", 
		grpId.c_str(), threadId.c_str(), parentId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mComment;

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
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++);

	if (it != ownIds.end())
	{
		std::cerr << "p3GxsChannels::generateMessage() Author: " << *it;
		std::cerr << std::endl;
		msg.mMeta.mAuthorId = *it;
	} 
	else
	{
		std::cerr << "p3GxsChannels::generateMessage() No Author!";
		std::cerr << std::endl;
	} 

	createComment(token, msg);

	return true;
}


bool p3GxsChannels::generateGroup(uint32_t &token, std::string groupName)
{
	/* generate a new channel */
	RsGxsChannelGroup channel;
	channel.mMeta.mGroupName = groupName;

	createGroup(token, channel);

	return true;
}


        // Overloaded from RsTickEvent for Event callbacks.
void p3GxsChannels::handle_event(uint32_t event_type, const std::string &elabel)
{
	std::cerr << "p3GxsChannels::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
		case CHANNEL_TESTEVENT_DUMMYDATA:
			generateDummyData();
			break;

		default:
			/* error */
			std::cerr << "p3GxsChannels::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}

