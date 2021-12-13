/*******************************************************************************
 * libretroshare/src/services: p3gxsforums.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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

#include <cstdio>
#include <memory>

#include "services/p3gxsforums.h"
#include "rsitems/rsgxsforumitems.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"
#include "util/rsdebug.h"
#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"
#include "util/rsdebuglevel2.h"
#include "retroshare/rsgxsflags.h"


// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define GXSFORUM_DEBUG 1
 ****/

RsGxsForums *rsGxsForums = NULL;

#define FORUM_TESTEVENT_DUMMYDATA	0x0001
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.
#define FORUM_UNUSED_BY_FRIENDS_DELAY (2*30*86400) 		// unused forums are deleted after 2 months

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsForums::p3GxsForums( RsGeneralDataService *gds,
                          RsNetworkExchangeService *nes, RsGixs* gixs ) :
    RsGenExchange( gds, nes, new RsGxsForumSerialiser(),
                   RS_SERVICE_GXS_TYPE_FORUMS, gixs, forumsAuthenPolicy()),
    RsGxsForums(static_cast<RsGxsIface&>(*this)), mGenToken(0),
    mGenActive(false), mGenCount(0),
    mKnownForumsMutex("GXS forums known forums timestamp cache")
#ifdef RS_DEEP_FORUMS_INDEX
    , mDeepIndex(DeepForumsIndex::dbDefaultPath())
#endif
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

    {
        RS_STACK_MUTEX(mKnownForumsMutex);
        item->records = mKnownForums ;
    }

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
            RS_STACK_MUTEX(mKnownForumsMutex);

			mKnownForums.clear();

			for(auto it(fnr->records.begin());it!=fnr->records.end();++it)
				if( now < it->second + GXS_FORUMS_CONFIG_MAX_TIME_NOTIFY_STORAGE)
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

void p3GxsForums::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	RS_DBG2(changes.size(), " changes to notify");

	for(RsGxsNotify* gxsChange: changes)
	{
		// Let the compiler delete the change for us
		std::unique_ptr<RsGxsNotify> gxsChangeDeleter(gxsChange);

		switch(gxsChange->getType())
		{
		case RsGxsNotify::TYPE_RECEIVED_NEW: // [[fallthrough]]
		case RsGxsNotify::TYPE_PUBLISHED:
		{
			auto msgChange = dynamic_cast<RsGxsMsgChange*>(gxsChange);

			if(msgChange) /* Message received */
			{
				uint8_t msgSubtype = msgChange->mNewMsgItem->PacketSubType();
				switch(static_cast<RsGxsForumsItems>(msgSubtype))
				{
				case RsGxsForumsItems::MESSAGE_ITEM:
				{
					auto newForumMessageItem =
					        dynamic_cast<RsGxsForumMsgItem*>(
					            msgChange->mNewMsgItem );

					if(!newForumMessageItem)
					{
						RS_ERR("Received message change with mNewMsgItem type "
						       "mismatching or null");
						print_stacktrace();
						return;
					}

#ifdef RS_DEEP_FORUMS_INDEX
					RsGxsForumMsg tmpPost = newForumMessageItem->mMsg;
					tmpPost.mMeta = newForumMessageItem->meta;
					mDeepIndex.indexForumPost(tmpPost);
#endif
					auto ev = std::make_shared<RsGxsForumEvent>();
					ev->mForumMsgId = msgChange->mMsgId;
					ev->mForumGroupId = msgChange->mGroupId;
					ev->mForumEventCode = RsForumEventCode::NEW_MESSAGE;
					rsEvents->postEvent(ev);
					break;
				}
				default:
					RS_WARN("Got unknown gxs message subtype: ", msgSubtype);
					break;
				}
			}

			auto groupChange = dynamic_cast<RsGxsGroupChange*>(gxsChange);
			if(groupChange) /* Group received */
			{
				bool unknown;
				{
					RS_STACK_MUTEX(mKnownForumsMutex);
					unknown = ( mKnownForums.find(gxsChange->mGroupId)
					            == mKnownForums.end() );
					mKnownForums[gxsChange->mGroupId] = time(nullptr);
					IndicateConfigChanged();
				}

				if(unknown)
				{
					auto ev = std::make_shared<RsGxsForumEvent>();
					ev->mForumGroupId = gxsChange->mGroupId;
					ev->mForumEventCode = RsForumEventCode::NEW_FORUM;
					rsEvents->postEvent(ev);
				}

#ifdef RS_DEEP_FORUMS_INDEX
				uint8_t itemType = groupChange->mNewGroupItem->PacketSubType();
				switch(static_cast<RsGxsForumsItems>(itemType))
				{
				case RsGxsForumsItems::GROUP_ITEM:
				{
					auto newForumGroupItem =
					        static_cast<RsGxsForumGroupItem*>(
					            groupChange->mNewGroupItem );
					mDeepIndex.indexForumGroup(newForumGroupItem->mGroup);
					break;
				}
				default:
					RS_WARN("Got unknown gxs group subtype: ", itemType);
					break;
				}
#endif // def RS_DEEP_FORUMS_INDEX

			}
			break;
		}
		case RsGxsNotify::TYPE_PROCESSED: // happens when the group is subscribed
		{
			auto ev = std::make_shared<RsGxsForumEvent>();
			ev->mForumGroupId = gxsChange->mGroupId;
			ev->mForumEventCode = RsForumEventCode::SUBSCRIBE_STATUS_CHANGED;
			rsEvents->postEvent(ev);
			break;
		}
		case RsGxsNotify::TYPE_GROUP_SYNC_PARAMETERS_UPDATED:
		{
			auto ev = std::make_shared<RsGxsForumEvent>();
			ev->mForumGroupId = gxsChange->mGroupId;
			ev->mForumEventCode = RsForumEventCode::SYNC_PARAMETERS_UPDATED;
			rsEvents->postEvent(ev);
			break;
		}
		case RsGxsNotify::TYPE_MESSAGE_DELETED:
		{
			auto delChange = dynamic_cast<RsGxsMsgDeletedChange*>(gxsChange);
			if(!delChange)
			{
				RS_ERR( "Got mismatching notification type: ",
				        gxsChange->getType() );
				print_stacktrace();
				break;
			}

#ifdef RS_DEEP_FORUMS_INDEX
			mDeepIndex.removeForumPostFromIndex(
			            delChange->mGroupId, delChange->messageId);
#endif

			auto ev = std::make_shared<RsGxsForumEvent>();
			ev->mForumEventCode = RsForumEventCode::DELETED_POST;
			ev->mForumGroupId = delChange->mGroupId;
			ev->mForumMsgId = delChange->messageId;
			break;
		}
		case RsGxsNotify::TYPE_GROUP_DELETED:
		{
#ifdef RS_DEEP_FORUMS_INDEX
			mDeepIndex.removeForumFromIndex(gxsChange->mGroupId);
#endif
			auto ev = std::make_shared<RsGxsForumEvent>();
			ev->mForumGroupId = gxsChange->mGroupId;
			ev->mForumEventCode = RsForumEventCode::DELETED_FORUM;
			rsEvents->postEvent(ev);
			break;
		}
		case RsGxsNotify::TYPE_STATISTICS_CHANGED:
		{
			auto ev = std::make_shared<RsGxsForumEvent>();
			ev->mForumGroupId = gxsChange->mGroupId;
			ev->mForumEventCode = RsForumEventCode::STATISTICS_CHANGED;
			rsEvents->postEvent(ev);

			RS_STACK_MUTEX(mKnownForumsMutex);
			mKnownForums[gxsChange->mGroupId] = time(nullptr);
			IndicateConfigChanged();
			break;
		}
		case RsGxsNotify::TYPE_UPDATED:
		{
			/* Happens when the group data has changed. In this case we need to
			 * analyse the old and new group in order to detect possible
			 * notifications for clients */

			auto grpChange = dynamic_cast<RsGxsGroupChange*>(gxsChange);

			RsGxsForumGroupItem* old_forum_grp_item =
			        dynamic_cast<RsGxsForumGroupItem*>(grpChange->mOldGroupItem);
			RsGxsForumGroupItem* new_forum_grp_item =
			        dynamic_cast<RsGxsForumGroupItem*>(grpChange->mNewGroupItem);

			if( old_forum_grp_item == nullptr || new_forum_grp_item == nullptr)
			{
				RS_ERR( "received GxsGroupUpdate item with mOldGroup and "
				        "mNewGroup not of type RsGxsForumGroupItem or NULL. "
				        "This is inconsistent!");
				print_stacktrace();
				break;
			}

#ifdef RS_DEEP_FORUMS_INDEX
			mDeepIndex.indexForumGroup(new_forum_grp_item->mGroup);
#endif

			/* First of all, we check if there is a difference between the old
			 * and new list of moderators */

			std::list<RsGxsId> added_mods, removed_mods;
			for(auto& gxs_id: new_forum_grp_item->mGroup.mAdminList.ids)
				if( old_forum_grp_item->mGroup.mAdminList.ids.find(gxs_id)
				        == old_forum_grp_item->mGroup.mAdminList.ids.end() )
					added_mods.push_back(gxs_id);

			for(auto& gxs_id: old_forum_grp_item->mGroup.mAdminList.ids)
				if( new_forum_grp_item->mGroup.mAdminList.ids.find(gxs_id)
				        == new_forum_grp_item->mGroup.mAdminList.ids.end() )
					removed_mods.push_back(gxs_id);

			if(!added_mods.empty() || !removed_mods.empty())
			{
				auto ev = std::make_shared<RsGxsForumEvent>();

				ev->mForumGroupId = new_forum_grp_item->meta.mGroupId;
				ev->mModeratorsAdded = added_mods;
				ev->mModeratorsRemoved = removed_mods;
				ev->mForumEventCode = RsForumEventCode::MODERATOR_LIST_CHANGED;

				rsEvents->postEvent(ev);
			}

			// check the list of pinned posts
			std::list<RsGxsMessageId> added_pins, removed_pins;

			for(auto& msg_id: new_forum_grp_item->mGroup.mPinnedPosts.ids)
				if( old_forum_grp_item->mGroup.mPinnedPosts.ids.find(msg_id)
				        == old_forum_grp_item->mGroup.mPinnedPosts.ids.end() )
					added_pins.push_back(msg_id);

			for(auto& msg_id: old_forum_grp_item->mGroup.mPinnedPosts.ids)
				if( new_forum_grp_item->mGroup.mPinnedPosts.ids.find(msg_id)
				        == new_forum_grp_item->mGroup.mPinnedPosts.ids.end() )
					removed_pins.push_back(msg_id);

			if(!added_pins.empty() || !removed_pins.empty())
			{
				auto ev = std::make_shared<RsGxsForumEvent>();
				ev->mForumGroupId = new_forum_grp_item->meta.mGroupId;
				ev->mForumEventCode = RsForumEventCode::PINNED_POSTS_CHANGED;
				rsEvents->postEvent(ev);
			}

			if( old_forum_grp_item->mGroup.mDescription != new_forum_grp_item->mGroup.mDescription
			        || old_forum_grp_item->meta.mGroupName  != new_forum_grp_item->meta.mGroupName
			        || old_forum_grp_item->meta.mGroupFlags != new_forum_grp_item->meta.mGroupFlags
			        || old_forum_grp_item->meta.mAuthorId   != new_forum_grp_item->meta.mAuthorId
			        || old_forum_grp_item->meta.mCircleId   != new_forum_grp_item->meta.mCircleId )
			{
				auto ev = std::make_shared<RsGxsForumEvent>();
				ev->mForumGroupId = new_forum_grp_item->meta.mGroupId;
				ev->mForumEventCode = RsForumEventCode::UPDATED_FORUM;
				rsEvents->postEvent(ev);
			}

			break;
		}

		default:
			RS_ERR( "Got a GXS event of type ", gxsChange->getType(),
			        " Currently not handled." );
			break;
		}
	}
}

void	p3GxsForums::service_tick()
{
	dummy_tick();
	RsTickEvent::tick_events();
	return;
}

rstime_t p3GxsForums::service_getLastGroupSeenTs(const RsGxsGroupId& gid)
{
     rstime_t now = time(nullptr);

    RS_STACK_MUTEX(mKnownForumsMutex);

    auto it = mKnownForums.find(gid);
    bool unknown_forum = it == mKnownForums.end();

    if(unknown_forum)
    {
        mKnownForums[gid] = now;
        IndicateConfigChanged();
        return now;
    }
    else
        return it->second;
}
bool p3GxsForums::service_checkIfGroupIsStillUsed(const RsGxsGrpMetaData& meta)
{
#ifdef GXSFORUMS_DEBUG
    std::cerr << "p3gxsForums: Checking unused forums: called by GxsCleaning." << std::endl;
#endif

    // request all group infos at once

    rstime_t now = time(nullptr);

    RS_STACK_MUTEX(mKnownForumsMutex);

    auto it = mKnownForums.find(meta.mGroupId);
    bool unknown_forum = it == mKnownForums.end();

#ifdef GXSFORUMS_DEBUG
    std::cerr << "  Forum " << meta.mGroupId ;
#endif

    if(unknown_forum)
    {
        // This case should normally not happen. It does because this forum was never registered since it may
        // arrived before this code was here

#ifdef GXSFORUMS_DEBUG
        std::cerr << ". Not known yet. Adding current time as new TS." << std::endl;
#endif
        mKnownForums[meta.mGroupId] = now;
        IndicateConfigChanged();

        return true;
    }
    else
    {
        bool used_by_friends = (now < it->second + FORUM_UNUSED_BY_FRIENDS_DELAY);
        bool subscribed = static_cast<bool>(meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);

#ifdef GXSFORUMS_DEBUG
        std::cerr << ". subscribed: " << subscribed << ", used_by_friends: " << used_by_friends << " last TS: " << now - it->second << " secs ago (" << (now-it->second)/86400 << " days)";
#endif

        if(!subscribed && !used_by_friends)
        {
#ifdef GXSFORUMS_DEBUG
            std::cerr << ". Scheduling for deletion" << std::endl;
#endif
            return false;
        }
        else
        {
#ifdef GXSFORUMS_DEBUG
            std::cerr << ". Keeping!" << std::endl;
#endif
            return true;
        }
    }
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

bool p3GxsForums::createForumV2(
        const std::string& name, const std::string& description,
        const RsGxsId& authorId, const std::set<RsGxsId>& moderatorsIds,
        RsGxsCircleType circleType, const RsGxsCircleId& circleId,
        RsGxsGroupId& forumId, std::string& errorMessage )
{
	auto createFail = [&](std::string mErr)
	{
		errorMessage = mErr;
		RsErr() << __PRETTY_FUNCTION__ << " " << errorMessage << std::endl;
		return false;
	};

	if(name.empty()) return createFail("Forum name is required");

	if(!authorId.isNull() && !rsIdentity->isOwnId(authorId))
		return createFail("Author must be iether null or and identity owned by "
		                  "this node");

	switch(circleType)
	{
	case RsGxsCircleType::PUBLIC: // fallthrough
	case RsGxsCircleType::LOCAL: // fallthrough
	case RsGxsCircleType::YOUR_EYES_ONLY:
		break;
	case RsGxsCircleType::EXTERNAL:
		if(circleId.isNull())
			return createFail("circleType is EXTERNAL but circleId is null");
		break;
	case RsGxsCircleType::NODES_GROUP:
	{
		RsGroupInfo ginfo;
		if(!rsPeers->getGroupInfo(RsNodeGroupId(circleId), ginfo))
			return createFail("circleType is NODES_GROUP but circleId does not "
			                  "correspond to an actual group of friends");
		break;
	}
	default: return createFail("circleType has invalid value");
	}

	// Create a consistent channel group meta from the information supplied
	RsGxsForumGroup forum;

	forum.mMeta.mGroupName = name;
	forum.mMeta.mAuthorId = authorId;
	forum.mMeta.mCircleType = static_cast<uint32_t>(circleType);

	forum.mMeta.mSignFlags = GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_NONEREQ
	        | GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_REQUIRED;

	/* This flag have always this value even for circle restricted forums due to
	 * how GXS distribute/verify groups */
	forum.mMeta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	forum.mMeta.mCircleId.clear();
	forum.mMeta.mInternalCircle.clear();

	switch(circleType)
	{
	case RsGxsCircleType::NODES_GROUP:
		forum.mMeta.mInternalCircle = circleId; break;
	case RsGxsCircleType::EXTERNAL:
		forum.mMeta.mCircleId = circleId; break;
	default: break;
	}

	forum.mDescription = description;
	forum.mAdminList.ids = moderatorsIds;

	uint32_t token;
	if(!createGroup(token, forum))
		return createFail("Failed creating GXS group.");

	// wait for the group creation to complete.
	RsTokenService::GxsRequestStatus wSt =
	        waitToken( token, std::chrono::milliseconds(5000),
	                   std::chrono::milliseconds(20) );
	if(wSt != RsTokenService::COMPLETE)
		return createFail( "GXS operation waitToken failed with: "
		                   + std::to_string(wSt) );

	if(!RsGenExchange::getPublishedGroupMeta(token, forum.mMeta))
		return createFail("Failure getting updated group data.");

	forumId = forum.mMeta.mGroupId;

	return true;
}

bool p3GxsForums::createPost(
        const RsGxsGroupId& forumId, const std::string& title,
        const std::string& mBody,
        const RsGxsId& authorId, const RsGxsMessageId& parentId,
        const RsGxsMessageId& origPostId, RsGxsMessageId& postMsgId,
        std::string& errorMessage )
{
	RsGxsForumMsg post;

	auto failure = [&](std::string errMsg)
	{
		errorMessage = errMsg;
		RsErr() << __PRETTY_FUNCTION__ << " " << errorMessage << std::endl;
		return false;
	};

	if(title.empty()) return failure("Title is required");

	if(authorId.isNull()) return failure("Author id is needed");

	if(!rsIdentity->isOwnId(authorId))
		return failure( "Author id: " + authorId.toStdString() + " is not of"
		                "own identity" );

	if(!parentId.isNull())
	{
		std::vector<RsGxsForumMsg> msgs;
		if( getForumContent(forumId, std::set<RsGxsMessageId>({parentId}), msgs)
		        && msgs.size() == 1 )
		{
			post.mMeta.mParentId = parentId;
			post.mMeta.mThreadId = msgs[0].mMeta.mThreadId;
		}
		else return failure("Parent post " + parentId.toStdString()
		                    + " doesn't exists locally");
	}

	std::vector<RsGxsForumGroup> forumInfo;
	if(!getForumsInfo(std::list<RsGxsGroupId>({forumId}), forumInfo))
		return failure( "Forum with Id " + forumId.toStdString()
		                + " does not exist locally." );

	if(!origPostId.isNull())
	{
		std::vector<RsGxsForumMsg> msgs;
		if( getForumContent( forumId,
		                     std::set<RsGxsMessageId>({origPostId}), msgs)
		        && msgs.size() == 1 )
			post.mMeta.mOrigMsgId = origPostId;
		else return failure("Original post " + origPostId.toStdString()
		                    + " doesn't exists locally");
	}

	post.mMeta.mGroupId  = forumId;
	post.mMeta.mMsgName = title;
	post.mMeta.mAuthorId = authorId;
	post.mMsg = mBody;

	uint32_t token;
	if( !createMsg(token, post)
	        || waitToken(
	            token,
	            std::chrono::milliseconds(5000) ) != RsTokenService::COMPLETE )
		return failure("Failure creating GXS message");

	if(!RsGenExchange::getPublishedMsgMeta(token, post.mMeta))
		return failure("Failure getting created GXS message metadata");

	postMsgId = post.mMeta.mMsgId;
	return true;
}

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

bool p3GxsForums::getForumsInfo( const std::list<RsGxsGroupId>& forumIds, std::vector<RsGxsForumGroup>& forumsInfo )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

    if(forumIds.empty())
    {
		if( !requestGroupInfo(token, opts) || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE )
            return false;
    }
	else
    {
		if( !requestGroupInfo(token, opts, forumIds, forumIds.size()==1) || waitToken(token,std::chrono::milliseconds(5000)) != RsTokenService::COMPLETE )
            return false;
    }
	return getGroupData(token, forumsInfo);
}

bool p3GxsForums::getForumContent(
                    const RsGxsGroupId& forumId,
                    const std::set<RsGxsMessageId>& msgs_to_request,
                    std::vector<RsGxsForumMsg>& msgs )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	GxsMsgReq msgIds;
	msgIds[forumId] = msgs_to_request;

    if( !requestMsgInfo(token, opts, msgIds) || waitToken(token,std::chrono::seconds(5)) != RsTokenService::COMPLETE )
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

    RsGxsGrpMsgIdPair p;
    acknowledgeMsg(token,p);

	return true;
}

bool p3GxsForums::subscribeToForum(const RsGxsGroupId& groupId, bool subscribe )
{
	uint32_t token;
	if( !RsGenExchange::subscribeToGroup(token, groupId, subscribe) ||
	        waitToken(token) != RsTokenService::COMPLETE ) return false;

	RsGxsGroupId grp;
	acknowledgeGrp(token, grp);

	/* Since subscribe has been requested, the caller is most probably
	 * interested in getting the group messages ASAP so check updates from peers
	 * without waiting GXS sync timer.
	 * Do it here as this is meaningful or not depending on the service.
	 * Do it only after the token has been completed otherwise the pull have no
	 * effect. */
	if(subscribe) RsGenExchange::netService()->checkUpdatesFromPeers();

	return true;
}

bool p3GxsForums::exportForumLink(
        std::string& link, const RsGxsGroupId& forumId, bool includeGxsData,
        const std::string& baseUrl, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(forumId.isNull()) return failure("forumId cannot be null");

	const bool outputRadix = baseUrl.empty();
	if(outputRadix && !includeGxsData) return
	        failure("includeGxsData must be true if format requested is base64");

	if( includeGxsData &&
	        !RsGenExchange::exportGroupBase64(link, forumId, errMsg) )
		return failure(errMsg);

	if(outputRadix) return true;

	std::vector<RsGxsForumGroup> forumsInfo;
	if( !getForumsInfo(std::list<RsGxsGroupId>({forumId}), forumsInfo)
	        || forumsInfo.empty() )
		return failure("failure retrieving forum information");

	RsUrl inviteUrl(baseUrl);
	inviteUrl.setQueryKV(FORUM_URL_ID_FIELD, forumId.toStdString());
	inviteUrl.setQueryKV(FORUM_URL_NAME_FIELD, forumsInfo[0].mMeta.mGroupName);
	if(includeGxsData) inviteUrl.setQueryKV(FORUM_URL_DATA_FIELD, link);

	link = inviteUrl.toString();
	return true;
}

bool p3GxsForums::importForumLink(
        const std::string& link, RsGxsGroupId& forumId, std::string& errMsg )
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
	const auto qIt = query.find(FORUM_URL_DATA_FIELD);
	if(qIt != query.end()) radixPtr = &qIt->second;

	if(radixPtr->empty()) return failure(FORUM_URL_DATA_FIELD + " is empty");

	if(!RsGenExchange::importGroupBase64(*radixPtr, forumId, errMsg))
		return failure(errMsg);

	return true;
}

std::error_condition p3GxsForums::getChildPosts(
        const RsGxsGroupId& forumId, const RsGxsMessageId& parentId,
        std::vector<RsGxsForumMsg>& childPosts )
{
	RS_DBG3("forumId: ", forumId, " parentId: ", parentId);

	if(forumId.isNull() || parentId.isNull())
		return std::errc::invalid_argument;

	std::vector<RsGxsGrpMsgIdPair> msgIds;
	msgIds.push_back(RsGxsGrpMsgIdPair(forumId, parentId));

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_PARENT | RS_TOKREQOPT_MSG_LATEST;

	uint32_t token;
	if( !requestMsgRelatedInfo(token, opts, msgIds) ||
	        waitToken(token) != RsTokenService::COMPLETE )
		return std::errc::timed_out;

	GxsMsgRelatedDataMap msgData;
	if(!getMsgRelatedData(token, msgData))
		return std::errc::no_message_available;

	for(auto& mit: msgData)
	{
		for(auto& vit: mit.second)
		{
			auto msgItem = dynamic_cast<RsGxsForumMsgItem*>(vit);
			if(msgItem)
			{
				RsGxsForumMsg post = msgItem->mMsg;
				post.mMeta = msgItem->meta;
				childPosts.push_back(post);
			}
			else RS_WARN("Got item of unexpected type: ", vit);

			delete vit;
		}
	}

	return std::error_condition();
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

bool p3GxsForums::getForumServiceStatistics(GxsServiceStatistic& stat)
{
    uint32_t token;
	if(!RsGxsIfaceHelper::requestServiceStatistic(token) || waitToken(token) != RsTokenService::COMPLETE)
        return false;

    return RsGenExchange::getServiceStatistic(token,stat);
}

bool p3GxsForums::getForumStatistics(const RsGxsGroupId& ForumId,GxsGroupStatistic& stat)
{
	uint32_t token;
	if(!RsGxsIfaceHelper::requestGroupStatistic(token, ForumId) || waitToken(token) != RsTokenService::COMPLETE)
        return false;

    return RsGenExchange::getGroupStatistic(token,stat);
}

bool p3GxsForums::updateGroup(uint32_t &token, const RsGxsForumGroup &group)
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
		status = 0;

	setMsgStatusFlags(token, msgId, status, mask);

	/* WARNING: The event may be received before the operation is completed!
	 * TODO: move notification to blocking method markRead(...) which wait the
	 * operation to complete */
	if (rsEvents)
	{
		auto ev = std::make_shared<RsGxsForumEvent>();

		ev->mForumMsgId = msgId.second;
		ev->mForumGroupId = msgId.first;
		ev->mForumEventCode = RsForumEventCode::READ_STATUS_CHANGED;
		rsEvents->postEvent(ev);
	}
}

/********************************************************************************************/
/********************************************************************************************/

std::error_condition p3GxsForums::setPostKeepForever(
        const RsGxsGroupId& forumId, const RsGxsMessageId& postId,
        bool keepForever )
{
	if(forumId.isNull() || postId.isNull()) return std::errc::invalid_argument;

	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_KEEP_FOREVER;
	uint32_t status = keepForever ? GXS_SERV::GXS_MSG_STATUS_KEEP_FOREVER : 0;

	uint32_t token;
	setMsgStatusFlags(token, RsGxsGrpMsgIdPair(forumId, postId), status, mask);

	switch(waitToken(token))
	{
	case RsTokenService::PENDING: // [[fallthrough]];
	case RsTokenService::PARTIAL: return std::errc::timed_out;
	case RsTokenService::COMPLETE: // [[fallthrough]];
	case RsTokenService::DONE:
	{
		auto ev = std::make_shared<RsGxsForumEvent>();
		ev->mForumGroupId = forumId;
		ev->mForumMsgId = postId;
		ev->mForumEventCode = RsForumEventCode::UPDATED_MESSAGE;
		rsEvents->postEvent(ev);
		return std::error_condition();
	}
	case RsTokenService::CANCELLED: return std::errc::operation_canceled;
	default: return std::errc::bad_message;
	}
}

std::error_condition p3GxsForums::requestSynchronization()
{
	auto errc = RsGenExchange::netService()->checkUpdatesFromPeers();
	if(errc) return errc;
	return RsGenExchange::netService()->requestPull();
}

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

void RsGxsForumGroup::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(mMeta);
	RS_SERIAL_PROCESS(mDescription);

	/* Work around to have usable JSON API, without breaking binary
	 * serialization retrocompatibility */
	switch (j)
	{
	case RsGenericSerializer::TO_JSON: // fallthrough
	case RsGenericSerializer::FROM_JSON:
		RsTypeSerializer::serial_process( j, ctx,
		                                  mAdminList.ids, "mAdminList" );
		RsTypeSerializer::serial_process( j, ctx,
		                                  mPinnedPosts.ids, "mPinnedPosts" );
		break;
	default:
		RS_SERIAL_PROCESS(mAdminList);
		RS_SERIAL_PROCESS(mPinnedPosts);
	}
}

bool RsGxsForumGroup::canEditPosts(const RsGxsId& id) const
{
	return mAdminList.ids.find(id) != mAdminList.ids.end() ||
	        id == mMeta.mAuthorId;
}

std::error_condition p3GxsForums::getContentSummaries(
        const RsGxsGroupId& forumId,
        const std::set<RsGxsMessageId>& contentIds,
        std::vector<RsMsgMetaData>& summaries )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;

	GxsMsgReq msgReq;
	msgReq[forumId] = contentIds;


	if(!requestMsgInfo(token, opts, msgReq))
	{
		RS_ERR("requestMsgInfo failed");
		return std::errc::invalid_argument;
	}

	switch(waitToken(token, std::chrono::seconds(5)))
	{
	case RsTokenService::COMPLETE:
	{
		GxsMsgMetaMap metaMap;
		if(!RsGenExchange::getMsgMeta(token, metaMap))
			return std::errc::result_out_of_range;
		summaries = metaMap[forumId];
		return std::error_condition();
	}
	case RsTokenService::PARTIAL: // [[fallthrough]];
	case RsTokenService::PENDING:
		return std::errc::timed_out;
	default:
		return std::errc::not_supported;
	}
}

#ifdef RS_DEEP_FORUMS_INDEX
std::error_condition p3GxsForums::handleDistantSearchRequest(
        rs_view_ptr<uint8_t> requestData, uint32_t requestSize,
        rs_owner_ptr<uint8_t>& resultData, uint32_t& resultSize )
{
	RS_DBG1("");

	RsGxsForumsSearchRequest request;
	{
		RsGenericSerializer::SerializeContext ctx(requestData, requestSize);
		RsGenericSerializer::SerializeJob j =
		        RsGenericSerializer::SerializeJob::DESERIALIZE;
		RS_SERIAL_PROCESS(request);
	}

	if(request.mType != RsGxsForumsItems::SEARCH_REQUEST)
	{
		// If more types are implemented we would put a switch on mType instead
		RS_WARN( "Got search request with unkown type: ",
		         static_cast<uint32_t>(request.mType) );
		return std::errc::bad_message;
	}

	RsGxsForumsSearchReply reply;
	auto mErr = prepareSearchResults(request.mQuery, true, reply.mResults);
	if(mErr || reply.mResults.empty()) return mErr;

	{
		RsGenericSerializer::SerializeContext ctx;
		RsGenericSerializer::SerializeJob j =
		        RsGenericSerializer::SerializeJob::SIZE_ESTIMATE;
		RS_SERIAL_PROCESS(reply);
		resultSize = ctx.mOffset;
	}

	resultData = rs_malloc<uint8_t>(resultSize);
	RsGenericSerializer::SerializeContext ctx(resultData, resultSize);
	RsGenericSerializer::SerializeJob j =
	        RsGenericSerializer::SerializeJob::SERIALIZE;
	RS_SERIAL_PROCESS(reply);

	return std::error_condition();
}

std::error_condition p3GxsForums::distantSearchRequest(
        const std::string& matchString, TurtleRequestId& searchId )
{
	RsGxsForumsSearchRequest request;
	request.mQuery = matchString;

	uint32_t requestSize;
	{
		RsGenericSerializer::SerializeContext ctx;
		RsGenericSerializer::SerializeJob j =
		        RsGenericSerializer::SerializeJob::SIZE_ESTIMATE;
		RS_SERIAL_PROCESS(request);
		requestSize = ctx.mOffset;
	}

	std::error_condition ec;
	auto requestData = rs_malloc<uint8_t>(requestSize, &ec);
	if(!requestData) return ec;
	{
		RsGenericSerializer::SerializeContext ctx(requestData, requestSize);
		RsGenericSerializer::SerializeJob j =
		        RsGenericSerializer::SerializeJob::SERIALIZE;
		RS_SERIAL_PROCESS(request);
	}

	return netService()->distantSearchRequest(
	            requestData, requestSize,
	            static_cast<RsServiceType>(serviceType()), searchId );
}

std::error_condition p3GxsForums::localSearch(
        const std::string& matchString,
        std::vector<RsGxsSearchResult>& searchResults )
{ return prepareSearchResults(matchString, false, searchResults); }

std::error_condition p3GxsForums::prepareSearchResults(
        const std::string& matchString, bool publicOnly,
        std::vector<RsGxsSearchResult>& searchResults )
{
	std::vector<DeepForumsSearchResult> results;
	auto mErr = mDeepIndex.search(matchString, results);
	if(mErr) return mErr;

	searchResults.clear();
	for(auto uRes: results)
	{
		RsUrl resUrl(uRes.mUrl);
		const auto forumIdStr = resUrl.getQueryV(RsGxsForums::FORUM_URL_ID_FIELD);
		if(!forumIdStr)
		{
			RS_ERR( "Forum URL retrieved from deep index miss ID. ",
			        "Should never happen! ", uRes.mUrl );
			print_stacktrace();
			return std::errc::address_not_available;
		}

		std::vector<RsGxsForumGroup> forumsInfo;
		RsGxsGroupId forumId(*forumIdStr);
		if(forumId.isNull())
		{
			RS_ERR( "Forum ID retrieved from deep index is invalid. ",
			        "Should never happen! ", uRes.mUrl );
			print_stacktrace();
			return std::errc::bad_address;
		}

		if( !getForumsInfo(std::list<RsGxsGroupId>{forumId}, forumsInfo) ||
		        forumsInfo.empty() )
		{
			RS_ERR( "Forum just parsed from deep index link not found. "
			        "Should never happen! ", forumId, " ", uRes.mUrl );
			print_stacktrace();
			return std::errc::identifier_removed;
		}

		RsGroupMetaData& fMeta(forumsInfo[0].mMeta);

		// Avoid leaking sensitive information to unkown peers
		if( publicOnly &&
		        ( static_cast<RsGxsCircleType>(fMeta.mCircleType) !=
		          RsGxsCircleType::PUBLIC ) ) continue;

		RsGxsSearchResult res;
		res.mGroupId = forumId;
		res.mGroupName = fMeta.mGroupName;
		res.mAuthorId = fMeta.mAuthorId;
		res.mPublishTs = fMeta.mPublishTs;
		res.mSearchContext = uRes.mSnippet;

		auto postIdStr =
		        resUrl.getQueryV(RsGxsForums::FORUM_URL_MSG_ID_FIELD);
		if(postIdStr)
		{
			RsGxsMessageId msgId(*postIdStr);
			if(msgId.isNull())
			{
				RS_ERR( "Post just parsed from deep index link is invalid. "
				        "Should never happen! ", postIdStr, " ", uRes.mUrl );
				print_stacktrace();
				return std::errc::bad_address;
			}

			std::vector<RsMsgMetaData> msgSummaries;
			auto errc = getContentSummaries(
			            forumId, std::set<RsGxsMessageId>{msgId}, msgSummaries);
			if(errc) return errc;

			if(msgSummaries.size() != 1)
			{
				RS_ERR( "getContentSummaries returned: ", msgSummaries.size(),
				        "should never happen!" );
				return std::errc::result_out_of_range;
			}

			RsMsgMetaData& msgMeta(msgSummaries[0]);
			res.mMsgId = msgMeta.mMsgId;
			res.mMsgName = msgMeta.mMsgName;
			res.mAuthorId = msgMeta.mAuthorId;
		}

		RS_DBG4(res);
		searchResults.push_back(res);
	}

	return std::error_condition();
}

std::error_condition p3GxsForums::receiveDistantSearchResult(
        const TurtleRequestId requestId,
        rs_owner_ptr<uint8_t>& resultData, uint32_t& resultSize )
{
	RsGxsForumsSearchReply reply;
	{
		RsGenericSerializer::SerializeContext ctx(resultData, resultSize);
		RsGenericSerializer::SerializeJob j =
		        RsGenericSerializer::SerializeJob::DESERIALIZE;
		RS_SERIAL_PROCESS(reply);
	}
	free(resultData);

	if(reply.mType != RsGxsForumsItems::SEARCH_REPLY)
	{
		// If more types are implemented we would put a switch on mType instead
		RS_WARN( "Got search request with unkown type: ",
		         static_cast<uint32_t>(reply.mType) );
		return std::errc::bad_message;
	}

	auto event = std::make_shared<RsGxsForumsDistantSearchEvent>();
	event->mSearchId = requestId;
	event->mSearchResults = reply.mResults;
	rsEvents->postEvent(event);
	return std::error_condition();
}

#else  // def RS_DEEP_FORUMS_INDEX

std::error_condition p3GxsForums::distantSearchRequest(
        const std::string&, TurtleRequestId& )
{ return std::errc::function_not_supported; }

std::error_condition p3GxsForums::localSearch(
        const std::string&,
        std::vector<RsGxsSearchResult>& )
{ return std::errc::function_not_supported; }

#endif // def RS_DEEP_FORUMS_INDEX

/*static*/ const std::string RsGxsForums::DEFAULT_FORUM_BASE_URL =
        "retroshare:///forums";
/*static*/ const std::string RsGxsForums::FORUM_URL_NAME_FIELD =
        "forumName";
/*static*/ const std::string RsGxsForums::FORUM_URL_ID_FIELD =
        "forumId";
/*static*/ const std::string RsGxsForums::FORUM_URL_DATA_FIELD =
        "forumData";
/*static*/ const std::string RsGxsForums::FORUM_URL_MSG_TITLE_FIELD =
        "forumMsgTitle";
/*static*/ const std::string RsGxsForums::FORUM_URL_MSG_ID_FIELD =
        "forumMsgId";

RsGxsForumGroup::~RsGxsForumGroup() = default;
RsGxsForumMsg::~RsGxsForumMsg() = default;
RsGxsForums::~RsGxsForums() = default;
RsGxsForumEvent::~RsGxsForumEvent() = default;
