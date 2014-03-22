/*
 * libretroshare/src/services p3posted.cc
 *
 * Posted interface for RetroShare.
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

#include "services/p3posted.h"
#include "serialiser/rsposteditems.h"

#include <math.h>

/****
 * #define POSTED_DEBUG 1
 ****/

RsPosted *rsPosted = NULL;

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    :p3PostBase(gds, nes, gixs, new RsGxsPostedSerialiser(), RS_SERVICE_GXSV2_TYPE_POSTED), 
	RsPosted(this)
{
	return;
}


const std::string GXS_POSTED_APP_NAME = "gxsposted";
const uint16_t GXS_POSTED_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_POSTED_APP_MINOR_VERSION  =       0;
const uint16_t GXS_POSTED_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_POSTED_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3Posted::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXSV2_TYPE_POSTED,
                GXS_POSTED_APP_NAME,
                GXS_POSTED_APP_MAJOR_VERSION,
                GXS_POSTED_APP_MINOR_VERSION,
                GXS_POSTED_MIN_MAJOR_VERSION,
                GXS_POSTED_MIN_MINOR_VERSION);
}



bool p3Posted::getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsPostedGroupItem* item = dynamic_cast<RsGxsPostedGroupItem*>(*vit);
			if (item)
			{
				RsPostedGroup grp = item->mGroup;
				item->mGroup.mMeta = item->meta;
				grp.mMeta = item->mGroup.mMeta;
				delete item;
				groups.push_back(grp);
			}
			else
			{
				std::cerr << "Not a RsGxsPostedGroupItem, deleting!" << std::endl;
				delete *vit;
			}
		}
	}
	return ok;
}

bool p3Posted::getPostData(const uint32_t &token, std::vector<RsPostedPost> &msgs)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	time_t now = time(NULL);

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
				RsGxsPostedPostItem* item = dynamic_cast<RsGxsPostedPostItem*>(*vit);

				if(item)
				{
					RsPostedPost msg = item->mPost;
					msg.mMeta = item->meta;
					msg.calculateScores(now);

					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a PostedPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
		
	return ok;
}


bool p3Posted::getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &msgs)
{
	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
	time_t now = time(NULL);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsPostedPostItem* item = dynamic_cast<RsGxsPostedPostItem*>(*vit);
		
				if(item)
				{
					RsPostedPost msg = item->mPost;
					msg.mMeta = item->meta;
					msg.calculateScores(now);

					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a PostedPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
			
	return ok;
}


/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

/* Switched from having explicit Ranking calculations to calculating the set of scores
 * on each RsPostedPost item.
 *
 * TODO: move this function to be part of RsPostedPost - then the GUI 
 * can reuse is as necessary.
 *
 */

bool RsPostedPost::calculateScores(time_t ref_time)
{
	/* so we want to calculate all the scores for this Post. */

	PostStats stats;
	extractPostCache(mMeta.mServiceString, stats);

	mUpVotes = stats.up_votes;
	mDownVotes = stats.down_votes;
	mComments = stats.comments;
	mHaveVoted = (mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK);

	time_t age_secs = ref_time - mMeta.mPublishTs;
#define POSTED_AGESHIFT (2.0)
#define POSTED_AGEFACTOR (3600.0)

	mTopScore = ((int) mUpVotes - (int) mDownVotes);
	if (mTopScore > 0)
	{
		// score drops with time.
		mHotScore =  mTopScore / pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
	}
	else
	{
		// gets more negative with time.
		mHotScore =  mTopScore * pow(POSTED_AGESHIFT + age_secs / POSTED_AGEFACTOR, 1.5);
	}
	mNewScore = -age_secs;

	return true;
}

/********************************************************************************************/
/********************************************************************************************/

bool p3Posted::createGroup(uint32_t &token, RsPostedGroup &group)
{
	std::cerr << "p3Posted::createGroup()" << std::endl;

	RsGxsPostedGroupItem* grpItem = new RsGxsPostedGroupItem();
	grpItem->mGroup = group;
	grpItem->meta = group.mMeta;

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3Posted::updateGroup(uint32_t &token, RsPostedGroup &group)
{
	std::cerr << "p3Posted::updateGroup()" << std::endl;

	RsGxsPostedGroupItem* grpItem = new RsGxsPostedGroupItem();
	grpItem->mGroup = group;
	grpItem->meta = group.mMeta;

	RsGenExchange::updateGroup(token, grpItem);
	return true;
}


bool p3Posted::createPost(uint32_t &token, RsPostedPost &msg)
{
	std::cerr << "p3Posted::createPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsPostedPostItem* msgItem = new RsGxsPostedPostItem();
	msgItem->mPost = msg;
	msgItem->meta = msg.mMeta;
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}

/********************************************************************************************/
