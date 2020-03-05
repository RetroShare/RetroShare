/*******************************************************************************
 * libretroshare/src/services: p3gxschannels.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
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
#include "services/p3gxschannels.h"
#include "rsitems/rsgxschannelitems.h"
#include "util/radix64.h"
#include "util/rsmemory.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsfiles.h>


#include "retroshare/rsgxsflags.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rspeers.h"

#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"

#include <cstdio>
#include <chrono>
#include <string>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

#ifdef RS_DEEP_CHANNEL_INDEX
#	include "deep_search/channelsindex.hpp"
#endif //  RS_DEEP_CHANNEL_INDEX


/****
 * #define GXSCHANNEL_DEBUG 1
 ****/

/*extern*/ RsGxsChannels* rsGxsChannels = nullptr;


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

p3GxsChannels::p3GxsChannels(
        RsGeneralDataService *gds, RsNetworkExchangeService *nes,
        RsGixs* gixs ) :
    RsGenExchange( gds, nes, new RsGxsChannelSerialiser(),
                   RS_SERVICE_GXS_TYPE_CHANNELS, gixs, channelsAuthenPolicy() ),
    RsGxsChannels(static_cast<RsGxsIface&>(*this)), GxsTokenQueue(this),
    mSubscribedGroupsMutex("GXS channels subscribed groups cache"),
    mKnownChannelsMutex("GXS channels known channels timestamp cache"),
    mSearchCallbacksMapMutex("GXS channels search callbacks map"),
    mDistantChannelsCallbacksMapMutex("GXS channels distant channels callbacks map")
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

static const uint32_t GXS_CHANNELS_CONFIG_MAX_TIME_NOTIFY_STORAGE = 86400*30*2 ; // ignore notifications for 2 months
static const uint8_t  GXS_CHANNELS_CONFIG_SUBTYPE_NOTIFY_RECORD   = 0x01 ;

struct RsGxsChannelNotifyRecordsItem: public RsItem
{

	RsGxsChannelNotifyRecordsItem()
	    : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_GXS_TYPE_CHANNELS_CONFIG,GXS_CHANNELS_CONFIG_SUBTYPE_NOTIFY_RECORD)
	{}

    virtual ~RsGxsChannelNotifyRecordsItem() {}

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{ RS_SERIAL_PROCESS(records); }

	void clear() {}

	std::map<RsGxsGroupId,rstime_t> records;
};

class GxsChannelsConfigSerializer : public RsServiceSerializer
{
public:
	GxsChannelsConfigSerializer() : RsServiceSerializer(RS_SERVICE_GXS_TYPE_CHANNELS_CONFIG) {}
	virtual ~GxsChannelsConfigSerializer() {}

	RsItem* create_item(uint16_t service_id, uint8_t item_sub_id) const
	{
		if(service_id != RS_SERVICE_GXS_TYPE_CHANNELS_CONFIG)
			return NULL;

		switch(item_sub_id)
		{
		case GXS_CHANNELS_CONFIG_SUBTYPE_NOTIFY_RECORD: return new RsGxsChannelNotifyRecordsItem();
		default:
			return NULL;
		}
	}
};

bool p3GxsChannels::saveList(bool &cleanup, std::list<RsItem *>&saveList)
{
	cleanup = true ;

	RsGxsChannelNotifyRecordsItem *item = new RsGxsChannelNotifyRecordsItem ;

	{
		RS_STACK_MUTEX(mKnownChannelsMutex);
		item->records = mKnownChannels;
	}

	saveList.push_back(item) ;
	return true;
}

bool p3GxsChannels::loadList(std::list<RsItem *>& loadList)
{
	while(!loadList.empty())
	{
		RsItem *item = loadList.front();
		loadList.pop_front();

		rstime_t now = time(NULL);

		RsGxsChannelNotifyRecordsItem *fnr = dynamic_cast<RsGxsChannelNotifyRecordsItem*>(item) ;

		if(fnr)
		{
			RS_STACK_MUTEX(mKnownChannelsMutex);
			mKnownChannels.clear();

			for(auto it(fnr->records.begin());it!=fnr->records.end();++it)
				if( now < it->second + GXS_CHANNELS_CONFIG_MAX_TIME_NOTIFY_STORAGE)
					mKnownChannels.insert(*it) ;
		}

		delete item ;
	}
	return true;
}

RsSerialiser* p3GxsChannels::setupSerialiser()
{
	RsSerialiser* rss = new RsSerialiser;
	rss->addSerialType(new GxsChannelsConfigSerializer());

	return rss;
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
	std::cerr << "p3GxsChannels::notifyChanges() : " << changes.size() << "changes to notify" << std::endl;
#endif

	/* iterate through and grab any new messages */
	std::list<RsGxsGroupId> unprocessedGroups;

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); ++it)
	{
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (msgChange)
		{
			if (msgChange->getType() == RsGxsNotify::TYPE_RECEIVED_NEW|| msgChange->getType() == RsGxsNotify::TYPE_PUBLISHED)
			{
				/* message received */
				if (rsEvents)
				{
					std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
					for (auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
						for (auto mit1 = mit->second.begin(); mit1 != mit->second.end(); ++mit1)
						{
							auto ev = std::make_shared<RsGxsChannelEvent>();
							ev->mChannelMsgId = *mit1;
							ev->mChannelGroupId = mit->first;
							ev->mChannelEventCode = RsChannelEventCode::NEW_MESSAGE;
							rsEvents->postEvent(ev);
						}
				}
			}

			if (!msgChange->metaChange())
			{
#ifdef GXSCHANNELS_DEBUG
				std::cerr << "p3GxsChannels::notifyChanges() Found Message Change Notification";
				std::cerr << std::endl;
#endif

				std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
				for(auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
				{
#ifdef GXSCHANNELS_DEBUG
					std::cerr << "p3GxsChannels::notifyChanges() Msgs for Group: " << mit->first;
					std::cerr << std::endl;
#endif
					bool enabled = false;
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

		RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange*>(*it);

		if (grpChange && rsEvents)
		{
			switch (grpChange->getType())
			{
			default:
			case RsGxsNotify::TYPE_PROCESSED:	// happens when the group is subscribed
			{
				std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
				std::list<RsGxsGroupId>::iterator git;
				for (git = grpList.begin(); git != grpList.end(); ++git)
				{
					auto ev = std::make_shared<RsGxsChannelEvent>();
					ev->mChannelGroupId = *git;
					ev->mChannelEventCode = RsChannelEventCode::SUBSCRIBE_STATUS_CHANGED;
					rsEvents->postEvent(ev);
				}

			}
				break;

			case RsGxsNotify::TYPE_PUBLISHED:
			case RsGxsNotify::TYPE_RECEIVED_NEW:
			{
				/* group received */
				std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
				std::list<RsGxsGroupId>::iterator git;
				RS_STACK_MUTEX(mKnownChannelsMutex);
				for (git = grpList.begin(); git != grpList.end(); ++git)
				{
					if(mKnownChannels.find(*git) == mKnownChannels.end())
					{
						mKnownChannels.insert(std::make_pair(*git,time(NULL))) ;
						IndicateConfigChanged();

						auto ev = std::make_shared<RsGxsChannelEvent>();
						ev->mChannelGroupId = *git;
						ev->mChannelEventCode = RsChannelEventCode::NEW_CHANNEL;
						rsEvents->postEvent(ev);
					}
					else
						std::cerr << "(II) Not notifying already known channel " << *git << std::endl;
				}
				break;
			}

			case RsGxsNotify::TYPE_RECEIVED_PUBLISHKEY:
			{
				/* group received */
				std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
				std::list<RsGxsGroupId>::iterator git;
				for (git = grpList.begin(); git != grpList.end(); ++git)
				{
					auto ev = std::make_shared<RsGxsChannelEvent>();
					ev->mChannelGroupId = *git;
					ev->mChannelEventCode = RsChannelEventCode::RECEIVED_PUBLISH_KEY;

					rsEvents->postEvent(ev);
				}
				break;
			}
			}
		}

        RsGxsDistantSearchResultChange *dsrChange = dynamic_cast<RsGxsDistantSearchResultChange*>(*it);

        if(dsrChange && rsEvents)
        {
			auto ev = std::make_shared<RsGxsChannelEvent>();
			ev->mChannelGroupId = dsrChange->mGroupId;
			ev->mChannelEventCode = RsChannelEventCode::RECEIVED_DISTANT_SEARCH_RESULT;
			ev->mDistantSearchRequestId = dsrChange->mRequestId;

			rsEvents->postEvent(ev);
        }

		/* shouldn't need to worry about groups - as they need to be subscribed to */
        delete *it;
	}

	if(!unprocessedGroups.empty())
		request_SpecificSubscribedGroups(unprocessedGroups);
}

void	p3GxsChannels::service_tick()
{
	static rstime_t last_dummy_tick = 0;

	if (time(NULL) > last_dummy_tick + 5)
	{
		dummy_tick();
		last_dummy_tick = time(NULL);
	}

	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests();

	mCommentService->comment_tick();
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

bool p3GxsChannels::groupShareKeys(
        const RsGxsGroupId &groupId, const std::set<RsPeerId>& peers )
{
	RsGenExchange::shareGroupPublishKey(groupId,peers);
	return true;
}


/* Okay - chris is not going to be happy with this...
 * but I can't be bothered with crazy data structures
 * at the moment - fix it up later
 */

bool p3GxsChannels::getPostData(
        const uint32_t &token, std::vector<RsGxsChannelPost> &msgs,
        std::vector<RsGxsComment> &cmts )
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::cerr << std::endl;
#endif

	GxsMsgDataMap msgData;
	if(!RsGenExchange::getMsgData(token, msgData))
	{
		std::cerr << __PRETTY_FUNCTION__ <<" ERROR in request" << std::endl;
		return false;
	}

	GxsMsgDataMap::iterator mit = msgData.begin();

	for(; mit != msgData.end();  ++mit)
	{
		std::vector<RsGxsMsgItem*>& msgItems = mit->second;
		std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

		for(; vit != msgItems.end(); ++vit)
		{
			RsGxsChannelPostItem* postItem =
			        dynamic_cast<RsGxsChannelPostItem*>(*vit);

			if(postItem)
			{
				RsGxsChannelPost msg;
				postItem->toChannelPost(msg, true);
				msgs.push_back(msg);
				delete postItem;
			}
			else
			{
				RsGxsCommentItem* cmtItem =
				        dynamic_cast<RsGxsCommentItem*>(*vit);
				if(cmtItem)
				{
					RsGxsComment cmt;
					RsGxsMsgItem *mi = (*vit);
					cmt = cmtItem->mMsg;
					cmt.mMeta = mi->meta;
#ifdef GXSCOMMENT_DEBUG
					std::cerr << "p3GxsChannels::getPostData Found Comment:" << std::endl;
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
					std::cerr << __PRETTY_FUNCTION__
					          << " Not a GxsChannelPostItem neither a "
					          << "RsGxsCommentItem PacketService=" << std::hex
					          << (int)msg->PacketService() << std::dec
					          << " PacketSubType=" << std::hex
					          << (int)msg->PacketSubType() << std::dec
					          << " , deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return true;
}

bool p3GxsChannels::getPostData(
        const uint32_t& token, std::vector<RsGxsChannelPost>& posts )
{
	std::vector<RsGxsComment> cmts;
	return getPostData(token, posts, cmts);
}

//Not currently used
/*bool p3GxsChannels::getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
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
}*/


/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChannels::setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
	return setAutoDownload(groupId, enabled);
}

	
bool p3GxsChannels::getChannelAutoDownload(const RsGxsGroupId &groupId, bool& enabled)
{
    return autoDownloadEnabled(groupId,enabled);
}
	
bool p3GxsChannels::setChannelDownloadDirectory(
        const RsGxsGroupId &groupId, const std::string& directory )
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " id: " << groupId << " to: "
	          << directory << std::endl;
#endif

	RS_STACK_MUTEX(mSubscribedGroupsMutex);

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;
	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Unknown groupId: "
		          << groupId.toStdString() << std::endl;
		return false;
	}

    /* extract from ServiceString */
    SSGxsChannelGroup ss;
    ss.load(it->second.mServiceString);

	if (directory == ss.mDownloadDirectory)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Warning! groupId: " << groupId
		          << " Was already configured to download into: " << directory
		          << std::endl;
		return false;
	}

    ss.mDownloadDirectory = directory;
    std::string serviceString = ss.save();
    uint32_t token;

    it->second.mServiceString = serviceString; // update Local Cache.
    RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Feiled setting group "
		          << " service string" << std::endl;
		return false;
	}

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

	RS_STACK_MUTEX(mSubscribedGroupsMutex);

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Unknown groupId: "
		          << groupId.toStdString() << std::endl;
		return false;
	}

    /* extract from ServiceString */
    SSGxsChannelGroup ss;
    ss.load(it->second.mServiceString);
    directory = ss.mDownloadDirectory;

	return true;
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


void p3GxsChannels::request_SpecificSubscribedGroups(
        const std::list<RsGxsGroupId> &groups )
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::request_SpecificSubscribedGroups()";
	std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token = 0;

	if(!RsGenExchange::getTokenService()->
	        requestGroupInfo(token, ansType, opts, groups))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Failed requesting groups info!"
		          << std::endl;
		return;
	}

	if(!GxsTokenQueue::queueRequest(token, GXSCHANNELS_SUBSCRIBED_META))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Failed queuing request!"
		          << std::endl;
	}
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

	RS_STACK_MUTEX(mSubscribedGroupsMutex);
	mSubscribedGroups[group.mGroupId] = group;
}


void p3GxsChannels::clearUnsubscribedGroup(const RsGxsGroupId &id)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::clearUnsubscribedGroup() id: " << id;
	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(mSubscribedGroupsMutex);
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
		msgIds[it->first].insert(it->second);

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


void p3GxsChannels::load_unprocessedPosts(uint32_t token)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::load_SpecificUnprocessedPosts" << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	if (!getPostData(token, posts))
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR getting post data!"
		          << std::endl;
		return;
	}

	std::vector<RsGxsChannelPost>::iterator it;
	for(it = posts.begin(); it != posts.end(); ++it)
	{
		/* autodownload the files */
		handleUnprocessedPost(*it);
	}
}

void p3GxsChannels::handleUnprocessedPost(const RsGxsChannelPost &msg)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " GroupId: " << msg.mMeta.mGroupId
	          << " MsgId: " << msg.mMeta.mMsgId << std::endl;
#endif

	if (!IS_MSG_UNPROCESSED(msg.mMeta.mMsgStatus))
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR Msg already Processed! "
		          << "mMsgId: " << msg.mMeta.mMsgId << std::endl;
		return;
	}

	/* check that autodownload is set */
	bool enabled = false;
	if (autoDownloadEnabled(msg.mMeta.mGroupId, enabled) && enabled)
	{
#ifdef GXSCHANNELS_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " AutoDownload Enabled... handling"
		          << std::endl;
#endif

		/* check the date is not too old */
		rstime_t age = time(NULL) - msg.mMeta.mPublishTs;

		if (age < (rstime_t) CHANNEL_DOWNLOAD_PERIOD )
    {
        /* start download */
        // NOTE WE DON'T HANDLE PRIVATE CHANNELS HERE.
        // MORE THOUGHT HAS TO GO INTO THAT STUFF.

#ifdef GXSCHANNELS_DEBUG
			std::cerr << __PRETTY_FUNCTION__ << " START DOWNLOAD" << std::endl;
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
				std::cerr << __PRETTY_FUNCTION__ << "Channel file is not auto-"
				          << "downloaded because its size exceeds the threshold"
				          << " of " << CHANNEL_MAX_AUTO_DL << " bytes."
				          << std::endl;
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
		load_unprocessedPosts(token);
		break;

	case GXSCHANNELS_UNPROCESSED_GENERIC:
		load_unprocessedPosts(token);
		break;

	default:
		std::cerr << __PRETTY_FUNCTION__ << "ERROR Unknown Request Type: "
		          << req_type << std::endl;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// Blocking API implementation begin
////////////////////////////////////////////////////////////////////////////////

bool p3GxsChannels::getChannelsSummaries(
        std::list<RsGroupMetaData>& channels )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	if( !requestGroupInfo(token, opts)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return getGroupSummary(token, channels);
}

bool p3GxsChannels::getChannelsInfo(
        const std::list<RsGxsGroupId>& chanIds,
        std::vector<RsGxsChannelGroup>& channelsInfo )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if( !requestGroupInfo(token, opts, chanIds)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return getGroupData(token, channelsInfo) && !channelsInfo.empty();
}

bool p3GxsChannels::getContentSummaries(
        const RsGxsGroupId& channelId, std::vector<RsMsgMetaData>& summaries )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;

	std::list<RsGxsGroupId> channelIds;
	channelIds.push_back(channelId);

	if( !requestMsgInfo(token, opts, channelIds) ||
	        waitToken(token, std::chrono::seconds(5)) != RsTokenService::COMPLETE )
		return false;

	GxsMsgMetaMap metaMap;
	bool res = RsGenExchange::getMsgMeta(token, metaMap);
	summaries = metaMap[channelId];

	return res;
}

bool p3GxsChannels::getChannelContent( const RsGxsGroupId& channelId,
                       const std::set<RsGxsMessageId>& contentsIds,
                       std::vector<RsGxsChannelPost>& posts,
                       std::vector<RsGxsComment>& comments )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	GxsMsgReq msgIds;
	msgIds[channelId] = contentsIds;

	if( !requestMsgInfo(token, opts, msgIds) ||
	        waitToken(token) != RsTokenService::COMPLETE ) return false;

	return getPostData(token, posts, comments);
}

bool p3GxsChannels::createChannelV2(
        const std::string& name, const std::string& description,
        const RsGxsImage& thumbnail, const RsGxsId& authorId,
        RsGxsCircleType circleType, const RsGxsCircleId& circleId,
        RsGxsGroupId& channelId, std::string& errorMessage )
{
	const auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errorMessage = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(!authorId.isNull() && !rsIdentity->isOwnId(authorId))
		return failure("authorId must be either null, or of an owned identity");

	if(        circleType != RsGxsCircleType::PUBLIC
	        && circleType != RsGxsCircleType::EXTERNAL
	        && circleType != RsGxsCircleType::NODES_GROUP
	        && circleType != RsGxsCircleType::LOCAL
	        && circleType != RsGxsCircleType::YOUR_EYES_ONLY)
		return failure("circleType has invalid value");

	switch(circleType)
	{
	case RsGxsCircleType::EXTERNAL:
		if(circleId.isNull())
			return failure("circleType is EXTERNAL but circleId is null");
		break;
	case RsGxsCircleType::NODES_GROUP:
	{
		RsGroupInfo ginfo;

		if(!rsPeers->getGroupInfo(RsNodeGroupId(circleId), ginfo))
			return failure( "circleType is NODES_GROUP but circleId does not "
			                "correspond to an actual group of friends" );
		break;
	}
	default:
		if(!circleId.isNull())
			return failure( "circleType requires a null circleId, but a non "
			                "null circleId (" + circleId.toStdString() +
			                ") was supplied" );
		break;
	}

	// Create a consistent channel group meta from the information supplied
	RsGxsChannelGroup channel;

	channel.mMeta.mGroupName = name;
	channel.mMeta.mAuthorId = authorId;
	channel.mMeta.mCircleType = static_cast<uint32_t>(circleType);

	channel.mMeta.mSignFlags = GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_NONEREQ
	        | GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_REQUIRED;

	channel.mMeta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	channel.mMeta.mCircleId.clear();
	channel.mMeta.mInternalCircle.clear();

	switch(circleType)
	{
	case RsGxsCircleType::NODES_GROUP:
		channel.mMeta.mInternalCircle = circleId; break;
	case RsGxsCircleType::EXTERNAL:
		channel.mMeta.mCircleId = circleId; break;
	default: break;
	}

	// Create the channel
	channel.mDescription = description;
	channel.mImage = thumbnail;

	uint32_t token;
	if(!createGroup(token, channel))
		return failure("Failure creating GXS group");

	// wait for the group creation to complete.
	RsTokenService::GxsRequestStatus wSt =
	        waitToken( token, std::chrono::seconds(5),
	                   std::chrono::milliseconds(50) );
	if(wSt != RsTokenService::COMPLETE)
		return failure( "GXS operation waitToken failed with: " +
		                std::to_string(wSt) );

	if(!RsGenExchange::getPublishedGroupMeta(token, channel.mMeta))
		return failure("Failure getting updated group data.");

	channelId = channel.mMeta.mGroupId;

#ifdef RS_DEEP_CHANNEL_INDEX
	DeepChannelsIndex::indexChannelGroup(channel);
#endif //  RS_DEEP_CHANNEL_INDEX

	return true;
}

bool p3GxsChannels::createChannel(RsGxsChannelGroup& channel)
{
	uint32_t token;
	if(!createGroup(token, channel))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failed creating group."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, channel.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failure getting updated "
		          << " group data." << std::endl;
		return false;
	}

#ifdef RS_DEEP_CHANNEL_INDEX
	DeepChannelsIndex::indexChannelGroup(channel);
#endif //  RS_DEEP_CHANNEL_INDEX

	return true;
}

bool p3GxsChannels::createVoteV2(
        const RsGxsGroupId& channelId, const RsGxsMessageId& postId,
        const RsGxsMessageId& commentId, const RsGxsId& authorId,
        RsGxsVoteType tVote, RsGxsMessageId& voteId, std::string& errorMessage )
{
	std::vector<RsGxsChannelGroup> channelsInfo;
	if(!getChannelsInfo(std::list<RsGxsGroupId>({channelId}),channelsInfo))
	{
		errorMessage = "Channel with Id " + channelId.toStdString()
		        + " does not exist.";
		return false;
	}

	if(commentId.isNull())
	{
		errorMessage = "You cannot vote on null comment "
		        + commentId.toStdString();
		return false;
	}

	std::set<RsGxsMessageId> s({commentId});
	std::vector<RsGxsChannelPost> posts;
	std::vector<RsGxsComment> comments;

	if(!getChannelContent(channelId, s, posts, comments))
	{
		errorMessage = "You cannot vote on comment "
		        + commentId.toStdString() + " of channel with Id "
		        + channelId.toStdString()
		        + ": this comment does not exists locally!";
		return false;
	}

	// is the ID a comment ID or a post ID?
	// It should be comment => should have a parent ID
	if(posts.front().mMeta.mParentId.isNull())
	{
		errorMessage = "You cannot vote on channel message "
		        + commentId.toStdString() + " of channel with Id "
		        + channelId.toStdString()
		        + ": given id refers to a post, not a comment!";
		return false;
	}

	if( tVote != RsGxsVoteType::NONE
	        && tVote != RsGxsVoteType::UP
	        && tVote != RsGxsVoteType::DOWN )
	{
		errorMessage = "Your vote to channel with Id "
		        + channelId.toStdString() + " has wrong vote type. "
		        + " Only RsGxsVoteType::NONE, RsGxsVoteType::UP, "
		        + "RsGxsVoteType::DOWN are accepted.";
		return false;
	}

	if(!rsIdentity->isOwnId(authorId))
	{
		errorMessage = "You cannot vote to channel with Id "
		        + channelId.toStdString() + " with identity "
		        + authorId.toStdString() + " because it is not yours.";
		return false;
	}

	// Create the vote
	RsGxsVote vote;
	vote.mMeta.mGroupId = channelId;
	vote.mMeta.mThreadId = postId;
	vote.mMeta.mParentId = commentId;
	vote.mMeta.mAuthorId = authorId;
	vote.mVoteType = static_cast<uint32_t>(tVote);

	uint32_t token;
	if(!createNewVote(token, vote))
	{
		errorMessage = "Error! Failed creating vote.";
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		errorMessage = "GXS operation failed.";
		return false;
	}

	if(!RsGenExchange::getPublishedMsgMeta(token, vote.mMeta))
	{
		errorMessage = "Failure getting generated vote data.";
		return false;
	}

	voteId = vote.mMeta.mMsgId;
	return true;
}

/// @deprecated use createVoteV2 instead
bool p3GxsChannels::createVote(RsGxsVote& vote)
{
	uint32_t token;
	if(!createNewVote(token, vote))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failed creating vote."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedMsgMeta(token, vote.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failure getting generated "
		          << " vote data." << std::endl;
		return false;
	}

	return true;
}

bool p3GxsChannels::editChannel(RsGxsChannelGroup& channel)
{
	uint32_t token;
	if(!updateGroup(token, channel))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failed updating group."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, channel.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failure getting updated "
		          << " group data." << std::endl;
		return false;
	}

#ifdef RS_DEEP_CHANNEL_INDEX
	DeepChannelsIndex::indexChannelGroup(channel);
#endif //  RS_DEEP_CHANNEL_INDEX

	return true;
}

bool p3GxsChannels::createPostV2(
        const RsGxsGroupId& channelId, const std::string& title,
        const std::string& body, const std::list<RsGxsFile>& files,
        const RsGxsImage& thumbnail, const RsGxsMessageId& origPostId,
        RsGxsMessageId& postId, std::string& errorMessage )
{
	// Do some checks

	std::vector<RsGxsChannelGroup> channelsInfo;

	if(!getChannelsInfo(std::list<RsGxsGroupId>({channelId}),channelsInfo))
	{
		errorMessage = "Channel with Id " + channelId.toStdString() +
		        " does not exist.";
		return false;
	}

	const RsGxsChannelGroup& cg(*channelsInfo.begin());

	if(!(cg.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH))
	{
		errorMessage = "You cannot post to channel with Id " +
		        channelId.toStdString() + ": missing publish rights!";
		return false;
	}

	if(!origPostId.isNull())
	{
		std::set<RsGxsMessageId> s({origPostId});
		std::vector<RsGxsChannelPost> posts;
		std::vector<RsGxsComment> comments;

		if(!getChannelContent(channelId,s,posts,comments))
		{
			errorMessage = "You cannot edit post " + origPostId.toStdString()
			        + " of channel with Id " + channelId.toStdString()
			        + ": this post does not exist locally!";
			return false;
		}
	}

	// Create the post
	RsGxsChannelPost post;

	post.mMeta.mGroupId = channelId;
	post.mMeta.mOrigMsgId = origPostId;
	post.mMeta.mMsgName = title;

	post.mMsg = body;
	post.mFiles = files;
	post.mThumbnail = thumbnail;

	uint32_t token;
	if(!createPost(token, post) || waitToken(token) != RsTokenService::COMPLETE)
	{
		errorMessage = "GXS operation failed";
		return false;
	}

	if(RsGenExchange::getPublishedMsgMeta(token,post.mMeta))
	{
#ifdef RS_DEEP_CHANNEL_INDEX
		DeepChannelsIndex::indexChannelPost(post);
#endif //  RS_DEEP_CHANNEL_INDEX

		postId = post.mMeta.mMsgId;
		return true;
	}

	errorMessage = "Failed to retrive created post metadata";
	return false;
}

bool p3GxsChannels::createCommentV2(
        const RsGxsGroupId&   channelId,
        const RsGxsMessageId& threadId,
        const std::string&    comment,
        const RsGxsId&        authorId,
        const RsGxsMessageId& parentId,
        const RsGxsMessageId& origCommentId,
        RsGxsMessageId&       commentMessageId,
        std::string&          errorMessage )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errorMessage = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(channelId.isNull()) return  failure("channelId cannot be null");
	if(threadId.isNull()) return  failure("threadId cannot be null");
	if(parentId.isNull()) return failure("parentId cannot be null");

	std::vector<RsGxsChannelGroup> channelsInfo;
	if(!getChannelsInfo(std::list<RsGxsGroupId>({channelId}),channelsInfo))
		return failure( "Channel with Id " + channelId.toStdString()
		                + " does not exist." );

	std::vector<RsGxsChannelPost> posts;
	std::vector<RsGxsComment> comments;

	if(!getChannelContent( // does the post thread exist?
	            channelId,std::set<RsGxsMessageId>({threadId}), posts, comments ))
		return failure( "You cannot comment post " + threadId.toStdString() +
		                " of channel with Id " + channelId.toStdString() +
		                ": this post does not exists locally!" );

	// check that the post thread Id is actually that of a post thread
	if(posts.size() != 1 || !posts[0].mMeta.mParentId.isNull())
		return failure( "You cannot comment post " + threadId.toStdString() +
		                " of channel with Id " + channelId.toStdString() +
		                ": supplied threadId is not a thread, or parentMsgId is"
		                " not a comment!");

	if(!getChannelContent( // does the post thread exist?
	                       channelId, std::set<RsGxsMessageId>({parentId}),
	                       posts, comments ))
		return failure( "You cannot comment post " + parentId.toStdString() +
		                ": supplied parent doesn't exists locally!" );

	if(!origCommentId.isNull())
	{
		std::set<RsGxsMessageId> s({origCommentId});
		std::vector<RsGxsChannelPost> posts;
		std::vector<RsGxsComment> comments;

		if( !getChannelContent(channelId, s, posts, comments) ||
		        comments.size() != 1 )
			return failure( "You cannot edit comment " +
			                origCommentId.toStdString() +
			                " of channel with Id " + channelId.toStdString() +
			                ": this comment does not exist locally!");

		const RsGxsId& commentAuthor = comments[0].mMeta.mAuthorId;
		if(commentAuthor != authorId)
			return failure( "Editor identity and creator doesn't match "
			                + authorId.toStdString() + " != "
			                + commentAuthor.toStdString() );
	}

	if(!rsIdentity->isOwnId(authorId)) // is the author ID actually ours?
		return failure( "You cannot comment to channel with Id " +
		                channelId.toStdString() + " with identity " +
		                authorId.toStdString() + " because it is not yours." );

	// Now create the comment
	RsGxsComment cmt;
	cmt.mMeta.mGroupId  = channelId;
	cmt.mMeta.mThreadId = threadId;
	cmt.mMeta.mParentId = parentId;
	cmt.mMeta.mAuthorId = authorId;
	cmt.mMeta.mOrigMsgId = origCommentId;
	cmt.mComment = comment;

	uint32_t token;
	if(!createNewComment(token, cmt))
		return failure("createNewComment failed");

	RsTokenService::GxsRequestStatus wSt = waitToken(token);
	if(wSt != RsTokenService::COMPLETE)
		return failure( "GXS operation waitToken failed with: " +
		                std::to_string(wSt) );

	if(!RsGenExchange::getPublishedMsgMeta(token, cmt.mMeta))
		return failure("Failure getting created comment data.");

	commentMessageId = cmt.mMeta.mMsgId;
	return true;
}

bool p3GxsChannels::createComment(RsGxsComment& comment) // deprecated
{
	uint32_t token;
	if(!createNewComment(token, comment))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failed creating comment."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedMsgMeta(token, comment.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! Failure getting generated "
		          << " comment data." << std::endl;
		return false;
	}

	return true;
}

bool p3GxsChannels::subscribeToChannel(
        const RsGxsGroupId& groupId, bool subscribe )
{
	uint32_t token;
	if( !subscribeToGroup(token, groupId, subscribe)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return true;
}

bool p3GxsChannels::markRead(const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t token;
	setMessageReadStatus(token, msgId, read);
	if(waitToken(token) != RsTokenService::COMPLETE ) return false;
	return true;
}

bool p3GxsChannels::shareChannelKeys(
        const RsGxsGroupId& channelId, const std::set<RsPeerId>& peers)
{
	return groupShareKeys(channelId, peers);
}


////////////////////////////////////////////////////////////////////////////////
/// Blocking API implementation end
////////////////////////////////////////////////////////////////////////////////


/********************************************************************************************/
/********************************************************************************************/


bool p3GxsChannels::autoDownloadEnabled(const RsGxsGroupId &groupId,bool& enabled)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::autoDownloadEnabled(" << groupId << ")";
	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(mSubscribedGroupsMutex);
	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;
	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
		std::cerr << __PRETTY_FUNCTION__ << " WARNING requested channel: "
		          << groupId << " is not subscribed" << std::endl;
		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	ss.load(it->second.mServiceString);
	enabled = ss.mAutoDownload;

	return true;
}

bool SSGxsChannelGroup::load(const std::string &input)
{
    if(input.empty())
    {
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "SSGxsChannelGroup::load() asked to load a null string." << std::endl;
#endif
        return true ;
    }
    int download_val;
    mAutoDownload = false;
    mDownloadDirectory.clear();

    
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
        Radix64::encode((unsigned char*)mDownloadDirectory.c_str(),mDownloadDirectory.length(),encoded_str);

        output += " {P:" + encoded_str + "}";
    }

#ifdef GXSCHANNELS_DEBUG
    std::cerr << "ENCODED STRING: " << output << std::endl;
#endif

    return output;
}

bool p3GxsChannels::setAutoDownload(const RsGxsGroupId& groupId, bool enabled)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " id: " << groupId
	          << " enabled: " << enabled << std::endl;
#endif

	RS_STACK_MUTEX(mSubscribedGroupsMutex);
	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;
	it = mSubscribedGroups.find(groupId);
	if (it == mSubscribedGroups.end())
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR requested channel: "
		          << groupId.toStdString() << " is not subscribed!" << std::endl;
		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	ss.load(it->second.mServiceString);
	if (enabled == ss.mAutoDownload)
	{
		std::cerr << __PRETTY_FUNCTION__ << " WARNING mAutoDownload was already"
		          << " properly set to: " << enabled << " for channel:"
		          << groupId.toStdString() << std::endl;
		return false;
	}

	ss.mAutoDownload = enabled;
	std::string serviceString = ss.save();

	uint32_t token;
	RsGenExchange::setGroupServiceString(token, groupId, serviceString);

	if(waitToken(token) != RsTokenService::COMPLETE) return false;

	it->second.mServiceString = serviceString; // update Local Cache.

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

void p3GxsChannels::setMessageReadStatus( uint32_t& token,
                                          const RsGxsGrpMsgIdPair& msgId,
                                          bool read )
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << "p3GxsChannels::setMessageReadStatus()";
	std::cerr << std::endl;
#endif

	/* Always remove status unprocessed */
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	if (read) status = 0;

	setMsgStatusFlags(token, msgId, status, mask);

	if (rsEvents)
	{
		auto ev = std::make_shared<RsGxsChannelEvent>();

		ev->mChannelMsgId = msgId.second;
		ev->mChannelGroupId = msgId.first;
		ev->mChannelEventCode = RsChannelEventCode::READ_STATUS_CHANGED;
		rsEvents->postEvent(ev);
	}
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

/// @deprecated use createPostV2 instead
bool p3GxsChannels::createPost(RsGxsChannelPost& post)
{
	uint32_t token;
	if( !createPost(token, post)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;

	if(RsGenExchange::getPublishedMsgMeta(token,post.mMeta))
	{
#ifdef RS_DEEP_CHANNEL_INDEX
		DeepChannelsIndex::indexChannelPost(post);
#endif //  RS_DEEP_CHANNEL_INDEX

		return true;
	}

	return false;
}


bool p3GxsChannels::createPost(uint32_t &token, RsGxsChannelPost &msg)
{
#ifdef GXSCHANNELS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " GroupId: " << msg.mMeta.mGroupId
	          << std::endl;
#endif

	RsGxsChannelPostItem* msgItem = new RsGxsChannelPostItem();
	msgItem->fromChannelPost(msg, true);
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}

/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChannels::ExtraFileHash(const std::string& path)
{
	TransferRequestFlags flags = RS_FILE_REQ_ANONYMOUS_ROUTING;
	return rsFiles->ExtraFileHash(path, GXSCHANNEL_STOREPERIOD, flags);
}


bool p3GxsChannels::ExtraFileRemove(const RsFileHash& hash)
{ return rsFiles->ExtraFileRemove(hash); }


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
		if (status != RsTokenService::COMPLETE)
		{
#ifdef GXSCHANNELS_DEBUG
			std::cerr << "p3GxsChannels::dummy_tick() Status: " << status;
			std::cerr << std::endl;
#endif

			if (status == RsTokenService::FAILED)
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

	cleanTimedOutCallbacks();
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

	createNewComment(token, msg);

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

	createNewVote(token, vote);

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

TurtleRequestId p3GxsChannels::turtleGroupRequest(const RsGxsGroupId& group_id)
{
    return netService()->turtleGroupRequest(group_id) ;
}
TurtleRequestId p3GxsChannels::turtleSearchRequest(const std::string& match_string)
{
	return netService()->turtleSearchRequest(match_string);
}

bool p3GxsChannels::clearDistantSearchResults(TurtleRequestId req)
{
    return netService()->clearDistantSearchResults(req);
}
bool p3GxsChannels::retrieveDistantSearchResults(TurtleRequestId req,std::map<RsGxsGroupId,RsGxsGroupSummary>& results)
{
    return netService()->retrieveDistantSearchResults(req,results);
}

bool p3GxsChannels::retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChannelGroup& distant_group)
{
	RsGxsGroupSummary gs;

    if(netService()->retrieveDistantGroupSummary(group_id,gs))
    {
        // This is a placeholder information by the time we receive the full group meta data.
		distant_group.mMeta.mGroupId         = gs.mGroupId ;
		distant_group.mMeta.mGroupName       = gs.mGroupName;
		distant_group.mMeta.mGroupFlags      = GXS_SERV::FLAG_PRIVACY_PUBLIC ;
		distant_group.mMeta.mSignFlags       = gs.mSignFlags;

		distant_group.mMeta.mPublishTs       = gs.mPublishTs;
		distant_group.mMeta.mAuthorId        = gs.mAuthorId;

    	distant_group.mMeta.mCircleType      = GXS_CIRCLE_TYPE_PUBLIC ;// guessed, otherwise the group would not be search-able.

		// other stuff.
		distant_group.mMeta.mAuthenFlags     = 0;	// wild guess...

    	distant_group.mMeta.mSubscribeFlags  = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED ;

		distant_group.mMeta.mPop             = gs.mPopularity; 			// Popularity = number of friend subscribers
		distant_group.mMeta.mVisibleMsgCount = gs.mNumberOfMessages; 	// Max messages reported by friends
		distant_group.mMeta.mLastPost        = gs.mLastMessageTs; 		// Timestamp for last message. Not used yet.

		return true ;
    }
    else
        return false ;
}

bool p3GxsChannels::turtleSearchRequest(
        const std::string& matchString,
        const std::function<void (const RsGxsGroupSummary&)>& multiCallback,
        rstime_t maxWait )
{
	if(matchString.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << " match string can't be empty!"
		          << std::endl;
		return false;
	}

	TurtleRequestId sId = turtleSearchRequest(matchString);

	{
	RS_STACK_MUTEX(mSearchCallbacksMapMutex);
	mSearchCallbacksMap.emplace(
	            sId,
	            std::make_pair(
	                multiCallback,
	                std::chrono::system_clock::now() +
	                std::chrono::seconds(maxWait) ) );
	}

	return true;
}

/// @see RsGxsChannels::turtleChannelRequest
bool p3GxsChannels::turtleChannelRequest(
        const RsGxsGroupId& channelId,
        const std::function<void (const RsGxsChannelGroup& result)>& multiCallback,
        rstime_t maxWait)
{
	if(channelId.isNull())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error! channelId can't be null!"
		          << std::endl;
		return false;
	}

	TurtleRequestId sId = turtleGroupRequest(channelId);

	{
	RS_STACK_MUTEX(mDistantChannelsCallbacksMapMutex);
	mDistantChannelsCallbacksMap.emplace(
	            sId,
	            std::make_pair(
	                multiCallback,
	                std::chrono::system_clock::now() +
	                std::chrono::seconds(maxWait) ) );
	}

	return true;
}

/// @see RsGxsChannels::localSearchRequest
bool p3GxsChannels::localSearchRequest(
            const std::string& matchString,
            const std::function<void (const RsGxsGroupSummary& result)>& multiCallback,
            rstime_t maxWait )
{
	if(matchString.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << " match string can't be empty!"
		          << std::endl;
		return false;
	}

	auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(maxWait);
	RsThread::async([=]()
	{
		std::list<RsGxsGroupSummary> results;
		RsGenExchange::localSearch(matchString, results);
		if(std::chrono::steady_clock::now() < timeout)
			for(const RsGxsGroupSummary& result : results) multiCallback(result);
	});

	return true;
}

void p3GxsChannels::receiveDistantSearchResults(
        TurtleRequestId id, const RsGxsGroupId& grpId )
{
	std::cerr << __PRETTY_FUNCTION__ << "(" << id << ", " << grpId << ")"
	          << std::endl;

	{
		RsGenExchange::receiveDistantSearchResults(id, grpId);
		RsGxsGroupSummary gs;
		gs.mGroupId = grpId;
		netService()->retrieveDistantGroupSummary(grpId, gs);

		{
			RS_STACK_MUTEX(mSearchCallbacksMapMutex);
			auto cbpt = mSearchCallbacksMap.find(id);
			if(cbpt != mSearchCallbacksMap.end())
			{
				cbpt->second.first(gs);
				return;
			}
		} // end RS_STACK_MUTEX(mSearchCallbacksMapMutex);
	}

	{
		RS_STACK_MUTEX(mDistantChannelsCallbacksMapMutex);
		auto cbpt = mDistantChannelsCallbacksMap.find(id);
		if(cbpt != mDistantChannelsCallbacksMap.end())
		{
			std::function<void (const RsGxsChannelGroup&)> callback =
			        cbpt->second.first;
			RsThread::async([this, callback, grpId]()
			{
				std::list<RsGxsGroupId> chanIds({grpId});
				std::vector<RsGxsChannelGroup> channelsInfo;
				if(!getChannelsInfo(chanIds, channelsInfo))
				{
					std::cerr << __PRETTY_FUNCTION__ << " Error! Received "
					          << "distant channel result grpId: " << grpId
					          << " but failed getting channel info"
					          << std::endl;
					return;
				}

				for(const RsGxsChannelGroup& chan : channelsInfo)
					callback(chan);
			} );

			return;
		}
	} // RS_STACK_MUTEX(mDistantChannelsCallbacksMapMutex);
}

void p3GxsChannels::cleanTimedOutCallbacks()
{
	auto now = std::chrono::system_clock::now();

	{
		RS_STACK_MUTEX(mSearchCallbacksMapMutex);
		for( auto cbpt = mSearchCallbacksMap.begin();
		     cbpt != mSearchCallbacksMap.end(); )
			if(cbpt->second.second <= now)
			{
				clearDistantSearchResults(cbpt->first);
				cbpt = mSearchCallbacksMap.erase(cbpt);
			}
			else ++cbpt;
	} // RS_STACK_MUTEX(mSearchCallbacksMapMutex);

	{
		RS_STACK_MUTEX(mDistantChannelsCallbacksMapMutex);
		for( auto cbpt = mDistantChannelsCallbacksMap.begin();
		     cbpt != mDistantChannelsCallbacksMap.end(); )
			if(cbpt->second.second <= now)
			{
				clearDistantSearchResults(cbpt->first);
				cbpt = mDistantChannelsCallbacksMap.erase(cbpt);
			}
			else ++cbpt;
	} // RS_STACK_MUTEX(mDistantChannelsCallbacksMapMutex)
}

bool p3GxsChannels::exportChannelLink(
        std::string& link, const RsGxsGroupId& chanId, bool includeGxsData,
        const std::string& baseUrl, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(chanId.isNull()) return failure("chanId cannot be null");

	const bool outputRadix = baseUrl.empty();
	if(outputRadix && !includeGxsData) return
	        failure("includeGxsData must be true if format requested is base64");

	if( includeGxsData &&
	        !RsGenExchange::exportGroupBase64(link, chanId, errMsg) )
		return failure(errMsg);

	if(outputRadix) return true;

	std::vector<RsGxsChannelGroup> chansInfo;
	if( !getChannelsInfo(std::list<RsGxsGroupId>({chanId}), chansInfo)
	        || chansInfo.empty() )
		return failure("failure retrieving channel information");

	RsUrl inviteUrl(baseUrl);
	inviteUrl.setQueryKV(CHANNEL_URL_ID_FIELD, chanId.toStdString());
	inviteUrl.setQueryKV(CHANNEL_URL_NAME_FIELD, chansInfo[0].mMeta.mGroupName);
	if(includeGxsData) inviteUrl.setQueryKV(CHANNEL_URL_DATA_FIELD, link);

	link = inviteUrl.toString();
	return true;
}

bool p3GxsChannels::importChannelLink(
        const std::string& link, RsGxsGroupId& chanId, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(link.empty()) return failure("link is empty");

	const std::string* radixPtr(&link);

	RsUrl url(link);
	const auto& query = url.query();
	const auto qIt = query.find(CHANNEL_URL_DATA_FIELD);
	if(qIt != query.end()) radixPtr = &qIt->second;

	if(radixPtr->empty()) return failure(CHANNEL_URL_DATA_FIELD + " is empty");

	if(!RsGenExchange::importGroupBase64(*radixPtr, chanId, errMsg))
		return failure(errMsg);

	return true;
}

/*static*/ const std::string RsGxsChannels::DEFAULT_CHANNEL_BASE_URL =
        "retroshare:///channels";
/*static*/ const std::string RsGxsChannels::CHANNEL_URL_NAME_FIELD =
        "chanName";
/*static*/ const std::string RsGxsChannels::CHANNEL_URL_ID_FIELD =
        "chanId";
/*static*/ const std::string RsGxsChannels::CHANNEL_URL_DATA_FIELD =
        "chanData";
/*static*/ const std::string RsGxsChannels::CHANNEL_URL_MSG_TITLE_FIELD =
        "chanMsgTitle";
/*static*/ const std::string RsGxsChannels::CHANNEL_URL_MSG_ID_FIELD =
        "chanMsgId";

RsGxsChannelGroup::~RsGxsChannelGroup() = default;
RsGxsChannelPost::~RsGxsChannelPost() = default;
RsGxsChannels::~RsGxsChannels() = default;
