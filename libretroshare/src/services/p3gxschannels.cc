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
#include "util/radix64.h"
#include "util/rsmemory.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsfiles.h>


#include "retroshare/rsgxsflags.h"
#include "retroshare/rsfiles.h"

#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"

#include <stdio.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define GXSCHANNEL_DEBUG 1
 ****/

RsGxsChannels *rsGxsChannels = NULL;


#define GXSCHANNEL_STOREPERIOD	(3600 * 24 * 30)

#define	 GXSCHANNELS_SUBSCRIBED_META		1
#define  GXSCHANNELS_UNPROCESSED_SPECIFIC	2
#define  GXSCHANNELS_UNPROCESSED_GENERIC	3

#define CHANNEL_PROCESS	 		0x0001
#define CHANNEL_TESTEVENT_DUMMYDATA	0x0002
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

#define CHANNEL_DOWNLOAD_PERIOD 	(3600 * 24 * 7)
#define CHANNEL_MAX_AUTO_DL		(8 * 1024 * 1024 * 1024ull)	// 8 GB. Just a security ;-)
	
/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsChannels::p3GxsChannels(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsChannelSerialiser(), RS_SERVICE_GXS_TYPE_CHANNELS, gixs, channelsAuthenPolicy()), RsGxsChannels(this), GxsTokenQueue(this)
{
	// For Dummy Msgs.
	mGenActive = false;
	mCommentService = new p3GxsCommentService(this,  RS_SERVICE_GXS_TYPE_CHANNELS);

	RsTickEvent::schedule_in(CHANNEL_PROCESS, 0);

	// Test Data disabled in repo.
	//RsTickEvent::schedule_in(CHANNEL_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);
    mGenToken = 0;
    mGenCount = 0;

}


const std::string GXS_CHANNELS_APP_NAME = "gxschannels";
const uint16_t GXS_CHANNELS_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_CHANNELS_APP_MINOR_VERSION  =       0;
const uint16_t GXS_CHANNELS_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_CHANNELS_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3GxsChannels::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_CHANNELS,
                GXS_CHANNELS_APP_NAME,
                GXS_CHANNELS_APP_MAJOR_VERSION,
                GXS_CHANNELS_APP_MINOR_VERSION,
                GXS_CHANNELS_MIN_MAJOR_VERSION,
                GXS_CHANNELS_MIN_MINOR_VERSION);
}


uint32_t p3GxsChannels::channelsAuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = 0;

	flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}


	/** Overloaded to cache new groups **/
RsGenExchange::ServiceCreate_Return p3GxsChannels::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /* keySet */)
{
	updateSubscribedGroup(grpItem->meta);
	return SERVICE_CREATE_SUCCESS;
}


void p3GxsChannels::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::notifyChanges()";
	std::cerr << std::endl;
#endif

	p3Notify *notify = NULL;
	if (!changes.empty())
	{
		notify = RsServer::notify();
	}

	/* iterate through and grab any new messages */
	std::list<RsGxsGroupId> unprocessedGroups;

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); ++it)
	{
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
		if (msgChange)
		{
			if (msgChange->getType() == RsGxsNotify::TYPE_RECEIVE)
			{
				/* message received */
				if (notify)
				{
					std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
					std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
					for (mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
					{
						std::vector<RsGxsMessageId>::iterator mit1;
						for (mit1 = mit->second.begin(); mit1 != mit->second.end(); ++mit1)
						{
							notify->AddFeedItem(RS_FEED_ITEM_CHANNEL_MSG, mit->first.toStdString(), mit1->toStdString());
						}
					}
				}
			}

			if (!msgChange->metaChange())
			{
#ifdef GXSCHANNELS_DEBUG
				std::cerr << "p3GxsChannels::notifyChanges() Found Message Change Notification";
				std::cerr << std::endl;
#endif

				std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
				std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
				for(mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
				{
#ifdef GXSCHANNELS_DEBUG
					std::cerr << "p3GxsChannels::notifyChanges() Msgs for Group: " << mit->first;
					std::cerr << std::endl;
#endif
                                bool enabled = false ;

                    if (autoDownloadEnabled(mit->first, enabled) && enabled)
					{
#ifdef GXSCHANNELS_DEBUG
						std::cerr << "p3GxsChannels::notifyChanges() AutoDownload for Group: " << mit->first;
						std::cerr << std::endl;
#endif

						/* problem is most of these will be comments and votes,
						 * should make it occasional - every 5mins / 10minutes TODO */
						unprocessedGroups.push_back(mit->first);
					}
				}
			}
		}
		else
		{
			if (notify)
			{
				RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange*>(*it);
				if (grpChange)
				{
					switch (grpChange->getType())
					{
						case RsGxsNotify::TYPE_PROCESSED:
						case RsGxsNotify::TYPE_PUBLISH:
							break;

						case RsGxsNotify::TYPE_RECEIVE:
						{
							/* group received */
							std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
							std::list<RsGxsGroupId>::iterator git;
							for (git = grpList.begin(); git != grpList.end(); ++git)
							{
								notify->AddFeedItem(RS_FEED_ITEM_CHANNEL_NEW, git->toStdString());
							}
							break;
						}

						case RsGxsNotify::TYPE_PUBLISHKEY:
						{
							/* group received */
							std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
							std::list<RsGxsGroupId>::iterator git;
							for (git = grpList.begin(); git != grpList.end(); ++git)
							{
								notify->AddFeedItem(RS_FEED_ITEM_CHANNEL_PUBLISHKEY, git->toStdString());
							}
							break;
						}
					}
				}
			}
		}

		/* shouldn't need to worry about groups - as they need to be subscribed to */
	}

	request_SpecificSubscribedGroups(unprocessedGroups);

	RsGxsIfaceHelper::receiveChanges(changes);
}

void	p3GxsChannels::service_tick()
{

static  time_t last_dummy_tick = 0;

	if (time(NULL) > last_dummy_tick + 5)
	{
		dummy_tick();
		last_dummy_tick = time(NULL);
	}

	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests();

	mCommentService->comment_tick();

	return;
}

bool p3GxsChannels::getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::getGroupData()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); ++vit)
		{
			RsGxsChannelGroupItem* item = dynamic_cast<RsGxsChannelGroupItem*>(*vit);
			if (item)
			{
				RsGxsChannelGroup grp;
				item->toChannelGroup(grp, true);
				delete item;
				groups.push_back(grp);
			}
			else
			{
				std::cerr << "p3GxsChannels::getGroupData() ERROR in decode";
				std::cerr << std::endl;
				delete(*vit);
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getGroupData() ERROR in request";
		std::cerr << std::endl;
	}

	return ok;
}

bool p3GxsChannels::groupShareKeys(const RsGxsGroupId &groupId, std::set<RsPeerId>& peers)
{
    RsGenExchange::shareGroupPublishKey(groupId,peers) ;
    return true ;
}


/* Okay - chris is not going to be happy with this...
 * but I can't be bothered with crazy data structures
 * at the moment - fix it up later
 */

bool p3GxsChannels::getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::getPostData()";
	std::cerr << std::endl;
#endif

	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);

				if(item)
				{
					RsGxsChannelPost msg;
					item->toChannelPost(msg, true);
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					RsGxsCommentItem* cmt = dynamic_cast<RsGxsCommentItem*>(*vit);
					if(!cmt)
					{
						RsGxsMsgItem* msg = (*vit);
						//const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS    = 0x0217;
						//const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM = 0x03;
						//const uint8_t RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM = 0xf1;
						std::cerr << "Not a GxsChannelPostItem neither a RsGxsCommentItem"
											<< " PacketService=" << std::hex << (int)msg->PacketService() << std::dec
											<< " PacketSubType=" << std::hex << (int)msg->PacketSubType() << std::dec
											<< " , deleting!" << std::endl;
					}
					delete *vit;
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


bool p3GxsChannels::getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::getRelatedPosts()";
	std::cerr << std::endl;
#endif

	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);
		
				if(item)
				{
					RsGxsChannelPost msg;
					item->toChannelPost(msg, true);
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					RsGxsCommentItem* cmt = dynamic_cast<RsGxsCommentItem*>(*vit);
					if(!cmt)
					{
						RsGxsMsgItem* msg = (*vit);
						//const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS    = 0x0217;
						//const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM = 0x03;
						//const uint8_t RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM = 0xf1;
						std::cerr << "Not a GxsChannelPostItem neither a RsGxsCommentItem"
											<< " PacketService=" << std::hex << (int)msg->PacketService() << std::dec
											<< " PacketSubType=" << std::hex << (int)msg->PacketSubType() << std::dec
											<< " , deleting!" << std::endl;
					}
					delete *vit;
				}
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getRelatedPosts() ERROR in request";
		std::cerr << std::endl;
	}
			
	return ok;
}


/********************************************************************************************/
/********************************************************************************************/

#if 0
bool p3GxsChannels::setChannelAutoDownload(uint32_t &token, const RsGxsGroupId &groupId, bool autoDownload)
{
	std::cerr << "p3GxsChannels::setChannelAutoDownload()";
	std::cerr << std::endl;

	// we don't actually use the token at this point....
	//bool p3GxsChannels::setAutoDownload(const RsGxsGroupId &groupId, bool enabled)
	


	return;
}
#endif

bool p3GxsChannels::setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
	return setAutoDownload(groupId, enabled);
}

	
bool p3GxsChannels::getChannelAutoDownload(const RsGxsGroupId &groupId, bool& enabled)
{
    return autoDownloadEnabled(groupId,enabled);
}
	
bool p3GxsChannels::setChannelDownloadDirectory(const RsGxsGroupId &groupId, const std::string& directory)
{
#ifdef GXSCHANNELS_DEBUG
    std::cerr << "p3GxsChannels::setDownloadDirectory() id: " << groupId << " to: " << directory << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);
    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::setAutoDownload() Missing Group" << std::endl;
#endif
        return false;
    }

    /* extract from ServiceString */
    SSGxsChannelGroup ss;
    ss.load(it->second.mServiceString);

    if (directory == ss.mDownloadDirectory)
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::setDownloadDirectory() WARNING setting looks okay already" << std::endl;
#endif

    }

    /* we are just going to set it anyway. */
    ss.mDownloadDirectory = directory;
    std::string serviceString = ss.save();
    uint32_t token;

    it->second.mServiceString = serviceString; // update Local Cache.
    RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

    /* now reload it */
    std::list<RsGxsGroupId> groups;
    groups.push_back(groupId);

    request_SpecificSubscribedGroups(groups);

    return true;
}

bool p3GxsChannels::getChannelDownloadDirectory(const RsGxsGroupId & groupId,std::string& directory)
{
#ifdef GXSCHANNELS_DEBUG
    std::cerr << "p3GxsChannels::getChannelDownloadDirectory(" << id << ")" << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);

    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::getChannelDownloadDirectory() No Entry" << std::endl;
#endif

        return false;
    }

    /* extract from ServiceString */
    SSGxsChannelGroup ss;
    if (it->second.mServiceString.empty())
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::getChannelDownloadDirectory() No ServiceString make new one." << std::endl;
#endif
        std::string serviceString = ss.save();
        uint32_t token;

        it->second.mServiceString = serviceString; // update Local Cache.
        RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

        /* now reload it */
        std::list<RsGxsGroupId> groups;
        groups.push_back(groupId);

        request_SpecificSubscribedGroups(groups);

        return false;
    }

    ss.load(it->second.mServiceString);
    directory = ss.mDownloadDirectory;

    return true ;
}

void p3GxsChannels::request_AllSubscribedGroups()
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::request_SubscribedGroups()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token = 0;

	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, GXSCHANNELS_SUBSCRIBED_META);

#define PERIODIC_ALL_PROCESS	300 // TESTING every 5 minutes.
	RsTickEvent::schedule_in(CHANNEL_PROCESS, PERIODIC_ALL_PROCESS);
}


void p3GxsChannels::request_SpecificSubscribedGroups(const std::list<RsGxsGroupId> &groups)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::request_SpecificSubscribedGroups()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token = 0;

	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groups);
	GxsTokenQueue::queueRequest(token, GXSCHANNELS_SUBSCRIBED_META);
}


void p3GxsChannels::load_SubscribedGroups(const uint32_t &token)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::load_SubscribedGroups()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	std::list<RsGroupMetaData> groups;
	std::list<RsGxsGroupId> groupList;

	getGroupMeta(token, groups);

	std::list<RsGroupMetaData>::iterator it;
	for(it = groups.begin(); it != groups.end(); ++it)
	{
		if (it->mSubscribeFlags & 
			(GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
			GXS_SERV::GROUP_SUBSCRIBE_PUBLISH |
			GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
		{
#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::load_SubscribedGroups() updating Subscribed Group: " << it->mGroupId;
			std::cerr << std::endl;
#endif

			updateSubscribedGroup(*it);
            bool enabled = false ;

            if (autoDownloadEnabled(it->mGroupId,enabled) && enabled)
			{
#ifdef GXSCHANNELS_DEBUG
				std::cerr << "p3GxsChannels::load_SubscribedGroups() remembering AutoDownload Group: " << it->mGroupId;
				std::cerr << std::endl;
#endif
				groupList.push_back(it->mGroupId);
			}
		}
		else
		{
#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::load_SubscribedGroups() clearing unsubscribed Group: " << it->mGroupId;
			std::cerr << std::endl;
#endif
			clearUnsubscribedGroup(it->mGroupId);
		}
	}

	/* Query for UNPROCESSED POSTS from checkGroupList */
	request_GroupUnprocessedPosts(groupList);
}



void p3GxsChannels::updateSubscribedGroup(const RsGroupMetaData &group)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::updateSubscribedGroup() id: " << group.mGroupId;
	std::cerr << std::endl;
#endif

	mSubscribedGroups[group.mGroupId] = group;
}


void p3GxsChannels::clearUnsubscribedGroup(const RsGxsGroupId &id)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::clearUnsubscribedGroup() id: " << id;
	std::cerr << std::endl;
#endif

	//std::map<RsGxsGroupId, RsGrpMetaData> mSubscribedGroups;
	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(id);
	if (it != mSubscribedGroups.end())
	{
		mSubscribedGroups.erase(it);
	}
}


bool p3GxsChannels::subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::subscribedToGroup() id: " << groupId << " subscribe: " << subscribe;
	std::cerr << std::endl;
#endif

	std::list<RsGxsGroupId> groups;
	groups.push_back(groupId);

	// Call down to do the real work.
	bool response = RsGenExchange::subscribeToGroup(token, groupId, subscribe);

	// reload Group afterwards.
	request_SpecificSubscribedGroups(groups);

	return response;
}


void p3GxsChannels::request_SpecificUnprocessedPosts(std::list<std::pair<RsGxsGroupId, RsGxsMessageId> > &ids)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::request_SpecificUnprocessedPosts()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	// Only Fetch UNPROCESSED messages.
	opts.mStatusFilter = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	opts.mStatusMask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

	uint32_t token = 0;

	/* organise Ids how they want them */
	GxsMsgReq msgIds;
	std::list<std::pair<RsGxsGroupId, RsGxsMessageId> >::iterator it;
	for(it = ids.begin(); it != ids.end(); ++it)
	{
		std::vector<RsGxsMessageId> &vect_msgIds = msgIds[it->first];
		vect_msgIds.push_back(it->second);
	}

	RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, msgIds);
	GxsTokenQueue::queueRequest(token, GXSCHANNELS_UNPROCESSED_SPECIFIC);
}


void p3GxsChannels::request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::request_GroupUnprocessedPosts()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	// Only Fetch UNPROCESSED messages.
	opts.mStatusFilter = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	opts.mStatusMask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	
	uint32_t token = 0;

	RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, grouplist);
	GxsTokenQueue::queueRequest(token, GXSCHANNELS_UNPROCESSED_GENERIC);
}


void p3GxsChannels::load_SpecificUnprocessedPosts(const uint32_t &token)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::load_SpecificUnprocessedPosts";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	if (!getPostData(token, posts))
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::load_SpecificUnprocessedPosts ERROR";
		std::cerr << std::endl;
#endif
		return;
	}


	std::vector<RsGxsChannelPost>::iterator it;
	for(it = posts.begin(); it != posts.end(); ++it)
	{
		/* autodownload the files */
		handleUnprocessedPost(*it);
	}
}

	
void p3GxsChannels::load_GroupUnprocessedPosts(const uint32_t &token)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::load_GroupUnprocessedPosts";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	if (!getPostData(token, posts))
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::load_GroupUnprocessedPosts ERROR";
		std::cerr << std::endl;
#endif
		return;
	}


	std::vector<RsGxsChannelPost>::iterator it;
	for(it = posts.begin(); it != posts.end(); ++it)
	{
		handleUnprocessedPost(*it);
	}
}

void p3GxsChannels::handleUnprocessedPost(const RsGxsChannelPost &msg)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::handleUnprocessedPost() GroupId: " << msg.mMeta.mGroupId << " MsgId: " << msg.mMeta.mMsgId;
	std::cerr << std::endl;
#endif

	if (!IS_MSG_UNPROCESSED(msg.mMeta.mMsgStatus))
	{
		std::cerr << "p3GxsChannels::handleUnprocessedPost() Msg already Processed";
		std::cerr << std::endl;
		std::cerr << "p3GxsChannels::handleUnprocessedPost() ERROR - this should not happen";
		std::cerr << std::endl;
		return;
	}

    bool enabled = false ;

	/* check that autodownload is set */
    if (autoDownloadEnabled(msg.mMeta.mGroupId,enabled) && enabled )
	{
			
		
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::handleUnprocessedPost() AutoDownload Enabled ... handling";
		std::cerr << std::endl;
#endif

		/* check the date is not too old */
		time_t age = time(NULL) - msg.mMeta.mPublishTs;

		if (age < (time_t) CHANNEL_DOWNLOAD_PERIOD )
    {
        /* start download */
        // NOTE WE DON'T HANDLE PRIVATE CHANNELS HERE.
        // MORE THOUGHT HAS TO GO INTO THAT STUFF.

#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::handleUnprocessedPost() START DOWNLOAD";
        std::cerr << std::endl;
#endif

        std::list<RsGxsFile>::const_iterator fit;
        for(fit = msg.mFiles.begin(); fit != msg.mFiles.end(); ++fit)
        {
            std::string fname = fit->mName;
            Sha1CheckSum hash  = Sha1CheckSum(fit->mHash);
            uint64_t size     = fit->mSize;

            std::list<RsPeerId> srcIds;
            std::string localpath = "";
            TransferRequestFlags flags = RS_FILE_REQ_BACKGROUND | RS_FILE_REQ_ANONYMOUS_ROUTING;

            if (size < CHANNEL_MAX_AUTO_DL)
            {
                std::string directory ;
                if(getChannelDownloadDirectory(msg.mMeta.mGroupId,directory))
                    localpath = directory ;

                rsFiles->FileRequest(fname, hash, size, localpath, flags, srcIds);
            }
            else
                std::cerr << "WARNING: Channel file is not auto-downloaded because its size exceeds the threshold of " << CHANNEL_MAX_AUTO_DL << " bytes." << std::endl;
        }
    }

		/* mark as processed */
		uint32_t token;
		RsGxsGrpMsgIdPair msgId(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
		setMessageProcessedStatus(token, msgId, true);
	}
#ifdef GXSCHANNELS_DEBUG
	else
	{
		std::cerr << "p3GxsChannels::handleUnprocessedPost() AutoDownload Disabled ... skipping";
		std::cerr << std::endl;
	}
#endif
}


	// Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsChannels::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::handleResponse(" << token << "," << req_type << ")";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	// stuff.
	switch(req_type)
	{
		case GXSCHANNELS_SUBSCRIBED_META:
			load_SubscribedGroups(token);
			break;

		case GXSCHANNELS_UNPROCESSED_SPECIFIC:
			load_SpecificUnprocessedPosts(token);
			break;

		case GXSCHANNELS_UNPROCESSED_GENERIC:
			load_SpecificUnprocessedPosts(token);
			break;

		default:
			/* error */
			std::cerr << "p3GxsService::handleResponse() Unknown Request Type: " << req_type;
			std::cerr << std::endl;
			break;
	}
}


/********************************************************************************************/
/********************************************************************************************/


bool p3GxsChannels::autoDownloadEnabled(const RsGxsGroupId &groupId,bool& enabled)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::autoDownloadEnabled(" << id << ")";
	std::cerr << std::endl;
#endif

	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::autoDownloadEnabled() No Entry";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	if (it->second.mServiceString.empty())
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::autoDownloadEnabled() No ServiceString make new one." << std::endl;
#endif
		std::string serviceString = ss.save();
		uint32_t token;

		it->second.mServiceString = serviceString; // update Local Cache.
		RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

		/* now reload it */
		std::list<RsGxsGroupId> groups;
		groups.push_back(groupId);

		request_SpecificSubscribedGroups(groups);

		return false;
	}

	ss.load(it->second.mServiceString);
	enabled = ss.mAutoDownload;

	return true;
}

bool SSGxsChannelGroup::load(const std::string &input)
{
    int download_val;
    mAutoDownload = false;
    mDownloadDirectory.clear();

    if(input.empty())
    {
        std::cerr << "(EE) SSGxsChannelGroup::load() asked to load a null string. Weird." << std::endl;
        return false ;
    }
    
    RsTemporaryMemory tmpmem(input.length());

    if (1 == sscanf(input.c_str(), "D:%d", &download_val))
    {
        if (download_val == 1)
            mAutoDownload = true;
    }
    else  if( 2 == sscanf(input.c_str(),"v2 {D:%d} {P:%[^}]}",&download_val,(unsigned char*)tmpmem))
    {
        if (download_val == 1)
            mAutoDownload = true;

        std::vector<uint8_t> vals = Radix64::decode(std::string((char*)(unsigned char *)tmpmem)) ;
        mDownloadDirectory = std::string((char*)vals.data(),vals.size());
    }
    else  if( 1 == sscanf(input.c_str(),"v2 {D:%d}",&download_val))
    {
        if (download_val == 1)
            mAutoDownload = true;
    }
    else
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "SSGxsChannelGroup::load(): could not parse string \"" << input << "\"" << std::endl;
#endif
        return false ;
    }

#ifdef GXSCHANNELS_DEBUG
    std::cerr << "DECODED STRING: autoDL=" << mAutoDownload << ", directory=\"" << mDownloadDirectory << "\"" << std::endl;
#endif

    return true;
}

std::string SSGxsChannelGroup::save() const
{
    std::string output = "v2 ";

    if (mAutoDownload)
        output += "{D:1}";
    else
        output += "{D:0}";

    if(!mDownloadDirectory.empty())
    {
        std::string encoded_str ;
        Radix64::encode(mDownloadDirectory.c_str(),mDownloadDirectory.length(),encoded_str);

        output += " {P:" + encoded_str + "}";
    }

#ifdef GXSCHANNELS_DEBUG
    std::cerr << "ENCODED STRING: " << output << std::endl;
#endif

    return output;
}

bool p3GxsChannels::setAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::setAutoDownload() id: " << groupId << " enabled: " << enabled;
	std::cerr << std::endl;
#endif

	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::setAutoDownload() Missing Group";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	ss.load(it->second.mServiceString);
	if (enabled == ss.mAutoDownload)
	{
		/* it should be okay! */
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::setAutoDownload() WARNING setting looks okay already";
		std::cerr << std::endl;
#endif

	}

	/* we are just going to set it anyway. */
	ss.mAutoDownload = enabled;
	std::string serviceString = ss.save();
	uint32_t token;

	it->second.mServiceString = serviceString; // update Local Cache.
	RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

	/* now reload it */
	std::list<RsGxsGroupId> groups;
	groups.push_back(groupId);

	request_SpecificSubscribedGroups(groups);

	return true;
}

/********************************************************************************************/
/********************************************************************************************/

void p3GxsChannels::setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::setMessageProcessedStatus()";
	std::cerr << std::endl;
#endif

	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	if (processed)
	{
		status = 0;
	}
	setMsgStatusFlags(token, msgId, status, mask);
}

void p3GxsChannels::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::setMessageReadStatus()";
	std::cerr << std::endl;
#endif

	/* Always remove status unprocessed */
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	if (read)
	{
		status = 0;
	}
	setMsgStatusFlags(token, msgId, status, mask);
}


/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChannels::createGroup(uint32_t &token, RsGxsChannelGroup &group)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::createGroup()" << std::endl;
#endif

	RsGxsChannelGroupItem* grpItem = new RsGxsChannelGroupItem();
	grpItem->fromChannelGroup(group, true);

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3GxsChannels::updateGroup(uint32_t &token, RsGxsChannelGroup &group)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::updateGroup()" << std::endl;
#endif

	RsGxsChannelGroupItem* grpItem = new RsGxsChannelGroupItem();
	grpItem->fromChannelGroup(group, true);

	RsGenExchange::updateGroup(token, grpItem);
	return true;
}



bool p3GxsChannels::createPost(uint32_t &token, RsGxsChannelPost &msg)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::createChannelPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;
#endif

	RsGxsChannelPostItem* msgItem = new RsGxsChannelPostItem();
	msgItem->fromChannelPost(msg, true);
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}

/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChannels::ExtraFileHash(const std::string &path, std::string filename)
{
	/* extract filename */
	filename = RsDirUtil::getTopDir(path);


	TransferRequestFlags flags = RS_FILE_REQ_ANONYMOUS_ROUTING;
	if(!rsFiles->ExtraFileHash(path, GXSCHANNEL_STOREPERIOD, flags))
		return false;

	return true;
}


bool p3GxsChannels::ExtraFileRemove(const RsFileHash &hash)
{
	TransferRequestFlags tflags = RS_FILE_REQ_ANONYMOUS_ROUTING | RS_FILE_REQ_EXTRA;
	RsFileHash fh = RsFileHash(hash);
	return rsFiles->ExtraFileRemove(fh, tflags);
}


/********************************************************************************************/
/********************************************************************************************/

/* so we need the same tick idea as wiki for generating dummy channels
 */

#define 	MAX_GEN_GROUPS		20
#define 	MAX_GEN_POSTS		500
#define 	MAX_GEN_COMMENTS	600
#define 	MAX_GEN_VOTES		700

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
	mGenRefs.resize(MAX_GEN_VOTES);

	std::string groupName;
	rs_sprintf(groupName, "TestChannel_%d", mGenCount);

#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::generateDummyData() Starting off with Group: " << groupName;
	std::cerr << std::endl;
#endif

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
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::dummyTick() Gen Active";
		std::cerr << std::endl;
#endif

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
		if (status != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Status: " << status;
			std::cerr << std::endl;
#endif

			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
#ifdef GXSCHANNELS_DEBUG
				std::cerr << "p3GxsChannels::dummy_tick() generateDummyMsgs() FAILED";
				std::cerr << std::endl;
#endif
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

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged GroupId: " << groupId;
			std::cerr << std::endl;
#endif

			ChannelDummyRef ref(groupId, emptyId, emptyId);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_POSTS)
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

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged Post <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;
#endif

			/* store results for later selection */

			ChannelDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_COMMENTS)
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

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged Comment <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;
#endif

			/* store results for later selection */

			ChannelDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_VOTES)
		{
			/* get the msg Id, and generate next snapshot */
			RsGxsGrpMsgIdPair msgId;
			if (!acknowledgeVote(mGenToken, msgId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged Vote <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;
#endif

			/* store results for later selection */

			ChannelDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else
		{
#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Finished";
			std::cerr << std::endl;
#endif

			/* done */
			mGenActive = false;
			return;
		}

		mGenCount++;

		if (mGenCount < MAX_GEN_GROUPS)
		{
			std::string groupName;
			rs_sprintf(groupName, "TestChannel_%d", mGenCount);

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Generating Group: " << groupName;
			std::cerr << std::endl;
#endif

			/* create a new group */
			generateGroup(mGenToken, groupName);
		}
		else if (mGenCount < MAX_GEN_POSTS)
		{
			/* create a new post */
			uint32_t idx = (uint32_t) (MAX_GEN_GROUPS * RSRandom::random_f32());
			ChannelDummyRef &ref = mGenRefs[idx];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.isNull())
			{
				mGenThreadId = parentId;
			}

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Generating Post ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;
#endif

			generatePost(mGenToken, grpId);
		}
		else if (mGenCount < MAX_GEN_COMMENTS)
		{
			/* create a new post */
			uint32_t idx = (uint32_t) ((mGenCount - MAX_GEN_GROUPS) * RSRandom::random_f32());
			ChannelDummyRef &ref = mGenRefs[idx + MAX_GEN_GROUPS];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.isNull())
			{
				mGenThreadId = parentId;
			}

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Generating Comment ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;
#endif

			generateComment(mGenToken, grpId, parentId, mGenThreadId);
		}
		else 
		{
			/* create a new post */
			uint32_t idx = (uint32_t) ((MAX_GEN_COMMENTS - MAX_GEN_POSTS) * RSRandom::random_f32());
			ChannelDummyRef &ref = mGenRefs[idx + MAX_GEN_POSTS];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.isNull())
			{
				mGenThreadId = parentId;
			}

#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Generating Vote ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;
#endif

			generateVote(mGenToken, grpId, parentId, mGenThreadId);
		}

	}
}


bool p3GxsChannels::generatePost(uint32_t &token, const RsGxsGroupId &grpId)
{
	RsGxsChannelPost msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mMsg, "Channel Msg: GroupId: %s, some randomness: %s", 
		grpId.toStdString().c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mMsg;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId.clear() ;
	msg.mMeta.mParentId.clear() ;

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

	createPost(token, msg);

	return true;
}


bool p3GxsChannels::generateComment(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsComment msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mComment, "Channel Comment: GroupId: %s, ThreadId: %s, ParentId: %s + some randomness: %s", 
		grpId.toStdString().c_str(), threadId.toStdString().c_str(), parentId.toStdString().c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mComment;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = threadId;
	msg.mMeta.mParentId = parentId;

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	uint32_t i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); ++it, ++i);

	if (it != ownIds.end())
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::generateComment() Author: " << *it;
		std::cerr << std::endl;
#endif
		msg.mMeta.mAuthorId = *it;
	} 
#ifdef GXSCHANNELS_DEBUG
	else
	{
		std::cerr << "p3GxsChannels::generateComment() No Author!";
		std::cerr << std::endl;
	} 
#endif

	createComment(token, msg);

	return true;
}


bool p3GxsChannels::generateVote(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsVote vote;

	vote.mMeta.mGroupId = grpId;
	vote.mMeta.mThreadId = threadId;
	vote.mMeta.mParentId = parentId;
	vote.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	uint32_t i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); ++it, ++i) ;

	if (it != ownIds.end())
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << "p3GxsChannels::generateVote() Author: " << *it;
		std::cerr << std::endl;
#endif
		vote.mMeta.mAuthorId = *it;
	} 
#ifdef GXSCHANNELS_DEBUG
	else
	{
		std::cerr << "p3GxsChannels::generateVote() No Author!";
		std::cerr << std::endl;
	} 
#endif

	if (0.7 > RSRandom::random_f32())
	{
		// 70 % postive votes 
		vote.mVoteType = GXS_VOTE_UP;
	}
	else
	{
		vote.mVoteType = GXS_VOTE_DOWN;
	}

	createVote(token, vote);

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
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::handle_event(" << event_type << ")";
	std::cerr << std::endl;
#endif

	// stuff.
	switch(event_type)
	{
		case CHANNEL_TESTEVENT_DUMMYDATA:
			generateDummyData();
			break;

		case CHANNEL_PROCESS:
			request_AllSubscribedGroups();
			break;

		default:
			/* error */
			std::cerr << "p3GxsChannels::handle_event() Unknown Event Type: " << event_type << " elabel:" << elabel;
			std::cerr << std::endl;
			break;
	}
}

