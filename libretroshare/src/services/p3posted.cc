/*******************************************************************************
 * libretroshare/src/services: p3posted.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2013 Robert Fernie <retroshare@lunamutt.com>                 *
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
#include "services/p3posted.h"
#include "rsitems/rsposteditems.h"

#include <math.h>
#include <typeinfo>


/****
 * #define POSTED_DEBUG 1
 ****/

RsPosted *rsPosted = NULL;

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3Posted::p3Posted(
        RsGeneralDataService *gds, RsNetworkExchangeService *nes,
        RsGixs* gixs ) :
    p3PostBase( gds, nes, gixs, new RsGxsPostedSerialiser(),
                RS_SERVICE_GXS_TYPE_POSTED ),
    RsPosted(static_cast<RsGxsIface&>(*this)) {}

const std::string GXS_POSTED_APP_NAME = "gxsposted";
const uint16_t GXS_POSTED_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_POSTED_APP_MINOR_VERSION  =       0;
const uint16_t GXS_POSTED_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_POSTED_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3Posted::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_POSTED,
                GXS_POSTED_APP_NAME,
                GXS_POSTED_APP_MAJOR_VERSION,
                GXS_POSTED_APP_MINOR_VERSION,
                GXS_POSTED_MIN_MAJOR_VERSION,
                GXS_POSTED_MIN_MINOR_VERSION);
}

bool p3Posted::groupShareKeys(const RsGxsGroupId& groupId,const std::set<RsPeerId>& peers)
{
        RsGenExchange::shareGroupPublishKey(groupId,peers) ;
        return true ;
}

bool p3Posted::getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); ++vit)
		{
			RsGxsPostedGroupItem* item = dynamic_cast<RsGxsPostedGroupItem*>(*vit);
			if (item)
			{
				RsPostedGroup grp;
				item->toPostedGroup(grp, true);
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

bool p3Posted::getPostData(const uint32_t &token, std::vector<RsPostedPost> &msgs, std::vector<RsGxsComment> &cmts)
{
#ifdef POSTED_DEBUG
	std::cerr << "p3Posted::getPostData()";
	std::cerr << std::endl;
#endif

	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	rstime_t now = time(NULL);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end(); ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
		
			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsPostedPostItem* postItem = dynamic_cast<RsGxsPostedPostItem*>(*vit);

				if(postItem)
				{
					RsPostedPost msg = postItem->mPost;
					msg.mMeta = postItem->meta;
					postItem->toPostedPost(msg, true);
					msg.calculateScores(now);

					msgs.push_back(msg);
					delete postItem;
				}
				else
				{
					RsGxsCommentItem* cmtItem = dynamic_cast<RsGxsCommentItem*>(*vit);
					if(cmtItem)
					{
						RsGxsComment cmt;
						RsGxsMsgItem *mi = (*vit);
						cmt = cmtItem->mMsg;
						cmt.mMeta = mi->meta;
#ifdef GXSCOMMENT_DEBUG
						std::cerr << "p3Posted::getPostData Found Comment:" << std::endl;
						cmt.print(std::cerr,"  ", "cmt");
#endif
						cmts.push_back(cmt);
						delete cmtItem;
					}
					else
					{
						RsGxsMsgItem* msg = (*vit);
						//const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS    = 0x0217;
						//const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM = 0x03;
						//const uint8_t RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM = 0xf1;
						std::cerr << "Not a PostedPostItem neither a RsGxsCommentItem"
											<< " PacketService=" << std::hex << (int)msg->PacketService() << std::dec
											<< " PacketSubType=" << std::hex << (int)msg->PacketSubType() << std::dec
                                            << " type name    =" << typeid(*msg).name()
											<< " , deleting!" << std::endl;
						delete *vit;
					}
				}
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getPostData() ERROR in request";
		std::cerr << std::endl;
	}

	return ok;
}

//Not currently used
/*bool p3Posted::getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &msgs)
{
	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
	rstime_t now = time(NULL);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end(); ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); ++vit)
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
}*/


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

bool RsPostedPost::calculateScores(rstime_t ref_time)
{
	/* so we want to calculate all the scores for this Post. */

	PostStats stats;
	extractPostCache(mMeta.mServiceString, stats);

	mUpVotes = stats.up_votes;
	mDownVotes = stats.down_votes;
	mComments = stats.comments;
	mHaveVoted = (mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK);

	rstime_t age_secs = ref_time - mMeta.mPublishTs;
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
	grpItem->fromPostedGroup(group, true);


	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3Posted::updateGroup(uint32_t &token, RsPostedGroup &group)
{
	std::cerr << "p3Posted::updateGroup()" << std::endl;

	RsGxsPostedGroupItem* grpItem = new RsGxsPostedGroupItem();
	grpItem->fromPostedGroup(group, true);


	RsGenExchange::updateGroup(token, grpItem);
	return true;
}


bool p3Posted::createPost(uint32_t &token, RsPostedPost &msg)
{
	std::cerr << "p3Posted::createPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsPostedPostItem* msgItem = new RsGxsPostedPostItem();
	//msgItem->mPost = msg;
	//msgItem->meta = msg.mMeta;
	msgItem->fromPostedPost(msg, true);
		
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}

/********************************************************************************************/
