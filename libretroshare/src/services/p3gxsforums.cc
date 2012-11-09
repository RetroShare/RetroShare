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

#include "util/rsrandom.h"
#include <stdio.h>

/****
 * #define GXSFORUM_DEBUG 1
 ****/

RsGxsForums *rsGxsForums = NULL;



/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsForums::p3GxsForums(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
    : RsGenExchange(gds, nes, new RsGxsForumSerialiser(), RS_SERVICE_GXSV1_TYPE_FORUMS), RsGxsForums(this)
{
}

void p3GxsForums::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	receiveChanges(changes);
}

void	p3GxsForums::service_tick()
{
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
			RsGxsForumGroup grp = item->mGroup;
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
	return false;
}


#if 0

bool p3GxsForums::generateDummyData()
{
	/* so we want to generate 100's of forums */
#define MAX_FORUMS 10 //100
#define MAX_THREADS 10 //1000
#define MAX_MSGS 100 //10000

	std::list<RsForumV2Group> mGroups;
	std::list<RsForumV2Group>::iterator git;

	std::list<RsForumV2Msg> mMsgs;
	std::list<RsForumV2Msg>::iterator mit;

#define DUMMY_NAME_MAX_LEN		10000
	char name[DUMMY_NAME_MAX_LEN];
	int i, j;
	time_t now = time(NULL);

	for(i = 0; i < MAX_FORUMS; i++)
	{
		/* generate a new forum */
		RsForumV2Group forum;

		/* generate a temp id */
		forum.mMeta.mGroupId = genRandomId();

		snprintf(name, DUMMY_NAME_MAX_LEN, "TestForum_%d", i+1);

		forum.mMeta.mGroupId = genRandomId();
		forum.mMeta.mGroupName = name;

		forum.mMeta.mPublishTs = now - (RSRandom::random_f32() * 100000);
		/* key fields to fill in:
		 * GroupId.
		 * Name.
		 * Flags.
		 * Pop.
		 */



		/* use probability to decide which are subscribed / own / popularity.
		 */

		float rnd = RSRandom::random_f32();
		if (rnd < 0.1)
		{
			forum.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_ADMIN;

		}
		else if (rnd < 0.3)
		{
			forum.mMeta.mSubscribeFlags = RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED;
		}
		else
		{
			forum.mMeta.mSubscribeFlags = 0;
		}

		forum.mMeta.mPop = (int) (RSRandom::random_f32() * 10.0);

		mGroups.push_back(forum);


		//std::cerr << "p3GxsForums::generateDummyData() Generated Forum: " << forum.mMeta;
		//std::cerr << std::endl;
	}


	for(i = 0; i < MAX_THREADS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsForumV2Group head = mGroups.front();
			mGroups.pop_front();
			mGroups.push_back(head);
		}

		RsForumV2Group forum = mGroups.front();

		/* now create a new thread */

		RsForumV2Msg msg;

		/* fill in key data 
		 * GroupId
		 * MsgId
		 * OrigMsgId
		 * ThreadId
		 * ParentId
		 * PublishTS (take Forum TS + a bit ).
		 *
		 * ChildTS ????
		 */
		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => ThreadMsg_%d", forum.mMeta.mGroupName.c_str(), i+1);
		msg.mMeta.mMsgName = name;

		msg.mMeta.mGroupId = forum.mMeta.mGroupId;
		msg.mMeta.mMsgId = genRandomId();
		msg.mMeta.mOrigMsgId = msg.mMeta.mMsgId;
		msg.mMeta.mThreadId = msg.mMeta.mMsgId;
		msg.mMeta.mParentId = "";

		msg.mMeta.mPublishTs = forum.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (msg.mMeta.mPublishTs > now)
			msg.mMeta.mPublishTs = now - 1;

		mMsgs.push_back(msg);

		//std::cerr << "p3GxsForums::generateDummyData() Generated Thread: " << msg.mMeta;
		//std::cerr << std::endl;
		
	}

	for(i = 0; i < MAX_MSGS; i++)
	{
		/* generate a base thread */

		/* rotate the Forum Groups Around, then pick one.
		 */

		int rnd = (int) (RSRandom::random_f32() * 10.0);

		for(j = 0; j < rnd; j++)
		{
			RsForumV2Msg head = mMsgs.front();
			mMsgs.pop_front();
			mMsgs.push_back(head);
		}

		RsForumV2Msg parent = mMsgs.front();

		/* now create a new child msg */

		RsForumV2Msg msg;

		/* fill in key data 
		 * GroupId
		 * MsgId
		 * OrigMsgId
		 * ThreadId
		 * ParentId
		 * PublishTS (take Forum TS + a bit ).
		 *
		 * ChildTS ????
		 */
		snprintf(name, DUMMY_NAME_MAX_LEN, "%s => Msg_%d", parent.mMeta.mMsgName.c_str(), i+1);
		msg.mMeta.mMsgName = name;
		msg.mMsg = name;

		msg.mMeta.mGroupId = parent.mMeta.mGroupId;
		msg.mMeta.mMsgId = genRandomId();
		msg.mMeta.mOrigMsgId = msg.mMeta.mMsgId;
		msg.mMeta.mThreadId = parent.mMeta.mThreadId;
		msg.mMeta.mParentId = parent.mMeta.mOrigMsgId;

		msg.mMeta.mPublishTs = parent.mMeta.mPublishTs + (RSRandom::random_f32() * 10000);
		if (msg.mMeta.mPublishTs > now)
			msg.mMeta.mPublishTs = now - 1;

		mMsgs.push_back(msg);

		//std::cerr << "p3GxsForums::generateDummyData() Generated Child Msg: " << msg.mMeta;
		//std::cerr << std::endl;

	}


	mUpdated = true;

	/* Then - at the end, we push them all into the Proxy */
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		/* pushback */
		mForumProxy->addForumGroup(*git);

	}

	for(mit = mMsgs.begin(); mit != mMsgs.end(); mit++)
	{
		/* pushback */
		mForumProxy->addForumMsg(*mit);
	}

	return true;
}

#endif
