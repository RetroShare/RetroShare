/*******************************************************************************
 * libretroshare/src/services: p3gxsforums.cc                                  *
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
#include "services/p3gxsforums.h"
#include "rsitems/rsgxsforumitems.h"

#include <retroshare/rsidentity.h>

#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"

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

p3GxsForums::p3GxsForums( RsGeneralDataService *gds,
                          RsNetworkExchangeService *nes, RsGixs* gixs ) :
    RsGenExchange( gds, nes, new RsGxsForumSerialiser(),
                   RS_SERVICE_GXS_TYPE_FORUMS, gixs, forumsAuthenPolicy()),
    RsGxsForums(static_cast<RsGxsIface&>(*this)), mGenToken(0),
    mGenActive(false), mGenCount(0)
{
	// Test Data disabled in Repo.
	//RsTickEvent::schedule_in(FORUM_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);
}


const std::string GXS_FORUMS_APP_NAME = "gxsforums";
const uint16_t GXS_FORUMS_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_FORUMS_APP_MINOR_VERSION  =       0;
const uint16_t GXS_FORUMS_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_FORUMS_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3GxsForums::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_FORUMS,
                GXS_FORUMS_APP_NAME,
                GXS_FORUMS_APP_MAJOR_VERSION,
                GXS_FORUMS_APP_MINOR_VERSION,
                GXS_FORUMS_MIN_MAJOR_VERSION,
                GXS_FORUMS_MIN_MINOR_VERSION);
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

static const uint32_t GXS_FORUMS_CONFIG_MAX_TIME_NOTIFY_STORAGE = 86400*30*2 ; // ignore notifications for 2 months
static const uint8_t  GXS_FORUMS_CONFIG_SUBTYPE_NOTIFY_RECORD   = 0x01 ;

struct RsGxsForumNotifyRecordsItem: public RsItem
{

	RsGxsForumNotifyRecordsItem()
	    : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_GXS_TYPE_FORUMS_CONFIG,GXS_FORUMS_CONFIG_SUBTYPE_NOTIFY_RECORD)
	{}

    virtual ~RsGxsForumNotifyRecordsItem() {}

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{ RS_SERIAL_PROCESS(records); }

	void clear() {}

	std::map<RsGxsGroupId,rstime_t> records;
};

class GxsForumsConfigSerializer : public RsServiceSerializer
{
public:
	GxsForumsConfigSerializer() : RsServiceSerializer(RS_SERVICE_GXS_TYPE_FORUMS_CONFIG) {}
	virtual ~GxsForumsConfigSerializer() {}

	RsItem* create_item(uint16_t service_id, uint8_t item_sub_id) const
	{
		if(service_id != RS_SERVICE_GXS_TYPE_FORUMS_CONFIG)
			return NULL;

		switch(item_sub_id)
		{
		case GXS_FORUMS_CONFIG_SUBTYPE_NOTIFY_RECORD: return new RsGxsForumNotifyRecordsItem();
		default:
			return NULL;
		}
	}
};

bool p3GxsForums::saveList(bool &cleanup, std::list<RsItem *>&saveList)
{
	cleanup = true ;

	RsGxsForumNotifyRecordsItem *item = new RsGxsForumNotifyRecordsItem ;

	item->records = mKnownForums ;

	saveList.push_back(item) ;
	return true;
}

bool p3GxsForums::loadList(std::list<RsItem *>& loadList)
{
	while(!loadList.empty())
	{
		RsItem *item = loadList.front();
		loadList.pop_front();

		rstime_t now = time(NULL);

		RsGxsForumNotifyRecordsItem *fnr = dynamic_cast<RsGxsForumNotifyRecordsItem*>(item) ;

		if(fnr != NULL)
		{
			mKnownForums.clear();

			for(auto it(fnr->records.begin());it!=fnr->records.end();++it)
				if( it->second + GXS_FORUMS_CONFIG_MAX_TIME_NOTIFY_STORAGE < now)
					mKnownForums.insert(*it) ;
		}

		delete item ;
	}
	return true;
}

RsSerialiser* p3GxsForums::setupSerialiser()
{
	RsSerialiser* rss = new RsSerialiser;
	rss->addSerialType(new GxsForumsConfigSerializer());

	return rss;
}

void p3GxsForums::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	if (!changes.empty())
	{
		p3Notify *notify = RsServer::notify();

		if (notify)
		{
			std::vector<RsGxsNotify*>::iterator it;
			for(it = changes.begin(); it != changes.end(); ++it)
			{
				RsGxsNotify *c = *it;

				switch (c->getType())
				{
                default:
					case RsGxsNotify::TYPE_PROCESSED:
					case RsGxsNotify::TYPE_PUBLISHED:
						break;

					case RsGxsNotify::TYPE_RECEIVED_NEW:
					{
						RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange*>(c);
						if (msgChange)
						{
							std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;

							for (auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
							{
								for (auto mit1 = mit->second.begin(); mit1 != mit->second.end(); ++mit1)
									notify->AddFeedItem(RS_FEED_ITEM_FORUM_MSG, mit->first.toStdString(), mit1->toStdString());
							}
							break;
						}

						RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
						if (grpChange)
						{
							/* group received */
							std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
							std::list<RsGxsGroupId>::iterator git;

							for (git = grpList.begin(); git != grpList.end(); ++git)
							{
                                if(mKnownForums.find(*git) == mKnownForums.end())
                                {
									notify->AddFeedItem(RS_FEED_ITEM_FORUM_NEW, git->toStdString());
                                    mKnownForums.insert(std::make_pair(*git,time(NULL))) ;

									IndicateConfigChanged();
                                }
                                else
                                    std::cerr << "(II) Not notifying already known forum " << *git << std::endl;
							}
							break;
						}
						break;
					}

					case RsGxsNotify::TYPE_RECEIVED_PUBLISHKEY:
					{
						RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
						if (grpChange)
						{
							/* group received */
							std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
							std::list<RsGxsGroupId>::iterator git;
							for (git = grpList.begin(); git != grpList.end(); ++git)
							{
								notify->AddFeedItem(RS_FEED_ITEM_FORUM_PUBLISHKEY, git->toStdString());
							}
							break;
						}
						break;
					}
				}
			}
		}
	}

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
		
		for(; vit != grpData.end(); ++vit)
		{
			RsGxsForumGroupItem* item = dynamic_cast<RsGxsForumGroupItem*>(*vit);
			if (item)
			{
				RsGxsForumGroup grp = item->mGroup;
				grp.mMeta = item->meta;
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

bool p3GxsForums::getMsgMetaData(const uint32_t &token, GxsMsgMetaMap& msg_metas)
{
	return RsGenExchange::getMsgMeta(token, msg_metas);
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

		for(; mit != msgData.end(); ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); ++vit)
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

//Not currently used
/*bool p3GxsForums::getRelatedMessages(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs)
{
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
}*/

/********************************************************************************************/

bool p3GxsForums::createForum(RsGxsForumGroup& forum)
{
	uint32_t token;
	if(!createGroup(token, forum))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failed creating group."
		          << std::endl;
		return false;
	}

	if(waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, forum.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting updated "
		          << " group data." << std::endl;
		return false;
	}

	return true;
}

bool p3GxsForums::editForum(RsGxsForumGroup& forum)
{
	uint32_t token;
	if(!updateGroup(token, forum))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failed updating group."
		          << std::endl;
		return false;
	}

	if(waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, forum.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting updated "
		          << " group data." << std::endl;
		return false;
	}

	return true;
}

bool p3GxsForums::getForumsSummaries( std::list<RsGroupMetaData>& forums )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	if( !requestGroupInfo(token, opts)
	        || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE ) return false;
	return getGroupSummary(token, forums);
}

bool p3GxsForums::getForumsInfo(
        const std::list<RsGxsGroupId>& forumIds,
        std::vector<RsGxsForumGroup>& forumsInfo )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if( !requestGroupInfo(token, opts, forumIds)
	        || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE ) return false;
	return getGroupData(token, forumsInfo);
}

bool p3GxsForums::getForumContent(
        const RsGxsGroupId& forumId, std::set<RsGxsMessageId>& msgs_to_request,
        std::vector<RsGxsForumMsg>& msgs )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	GxsMsgReq msgIds;
	msgIds[forumId] = msgs_to_request;

	if( !requestMsgInfo(token, opts, msgIds) ||
	        waitToken(token,std::chrono::seconds(5)) != RsTokenService::COMPLETE )
		return false;

	return getMsgData(token, msgs);
}


bool p3GxsForums::getForumMsgMetaData(const RsGxsGroupId& forumId, std::vector<RsMsgMetaData>& msg_metas)
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;

    GxsMsgMetaMap meta_map;
    std::list<RsGxsGroupId> forumIds;
    forumIds.push_back(forumId);

	if( !requestMsgInfo(token, opts, forumIds) || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE ) return false;

	bool res = getMsgMetaData(token, meta_map);

    msg_metas = meta_map[forumId];

    return res;
}

bool p3GxsForums::markRead(const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t token;
	setMessageReadStatus(token, msgId, read);
	if(waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE ) return false;
	return true;
}

bool p3GxsForums::subscribeToForum(
        const RsGxsGroupId& groupId, bool subscribe )
{
	uint32_t token;
	if( !RsGenExchange::subscribeToGroup(token, groupId, subscribe)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return true;
}

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


        if(group.mMeta.mGroupId.isNull())
		return false;

	RsGxsForumGroupItem* grpItem = new RsGxsForumGroupItem();
	grpItem->mGroup = group;
        grpItem->meta = group.mMeta;

        RsGenExchange::updateGroup(token, grpItem);
	return true;
}

bool p3GxsForums::createMessage(RsGxsForumMsg& message)
{
	uint32_t token;
	if( !createMsg(token, message)
	        || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE ) return false;

	if(RsGenExchange::getPublishedMsgMeta(token, message.mMeta)) return true;

	return false;
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
		if (status != RsTokenService::COMPLETE)
		{
			std::cerr << "p3GxsForums::dummy_tick() Status: " << status;
			std::cerr << std::endl;

			if (status == RsTokenService::FAILED)
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
			if (mGenThreadId.isNull())
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
		grpId.toStdString().c_str(), threadId.toStdString().c_str(), parentId.toStdString().c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mMsg;

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
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); ++it, i++) ;

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
void p3GxsForums::handle_event(uint32_t event_type, const std::string &/*elabel*/)
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
