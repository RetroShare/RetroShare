/*******************************************************************************
 * libretroshare/src/services: p3gxsforums.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2020  Asociación Civil Altermundi <info@altermundi.net>  *
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

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsForums::p3GxsForums( RsGeneralDataService *gds,
                          RsNetworkExchangeService *nes, RsGixs* gixs ) :
    RsGenExchange( gds, nes, new RsGxsForumSerialiser(),
                   RS_SERVICE_GXS_TYPE_FORUMS, gixs, forumsAuthenPolicy()),
    RsGxsForums(static_cast<RsGxsIface&>(*this)), mGenToken(0),
    mGenActive(false), mGenCount(0), mKnownForumsMutex("GXS forums known forums timestamp cache")
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

void p3GxsForums::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef GXSFORUMS_DEBUG
	std::cerr << "p3GxsForums::notifyChanges() : " << changes.size() << "changes to notify" << std::endl;
#endif

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); ++it)
	{
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (msgChange)
		{
			if (msgChange->getType() == RsGxsNotify::TYPE_RECEIVED_NEW || msgChange->getType() == RsGxsNotify::TYPE_PUBLISHED) /* message received */
				if (rsEvents)
				{
					auto ev = std::make_shared<RsGxsForumEvent>();
					ev->mForumMsgId = msgChange->mMsgId;
					ev->mForumGroupId = msgChange->mGroupId;
					ev->mForumEventCode = RsForumEventCode::NEW_MESSAGE;
					rsEvents->postEvent(ev);
				}

#ifdef NOT_USED_YET
			if (!msgChange->metaChange())
			{
#ifdef GXSCHANNELS_DEBUG
				std::cerr << "p3GxsForums::notifyChanges() Found Message Change Notification";
				std::cerr << std::endl;
#endif

				std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
				for(auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
				{
#ifdef GXSCHANNELS_DEBUG
					std::cerr << "p3GxsForums::notifyChanges() Msgs for Group: " << mit->first;
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
#endif
		}
		else
		{
			if (rsEvents)
			{
				RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange*>(*it);
				if (grpChange)
				{
					switch (grpChange->getType())
					{
					case RsGxsNotify::TYPE_PROCESSED:	// happens when the group is subscribed
					{
							auto ev = std::make_shared<RsGxsForumEvent>();
							ev->mForumGroupId = grpChange->mGroupId;
							ev->mForumEventCode = RsForumEventCode::SUBSCRIBE_STATUS_CHANGED;
							rsEvents->postEvent(ev);
					}
                        break;

                    case RsGxsNotify::TYPE_GROUP_SYNC_PARAMETERS_UPDATED:
                    {
                            auto ev = std::make_shared<RsGxsForumEvent>();
                            ev->mForumGroupId = grpChange->mGroupId;
                            ev->mForumEventCode = RsForumEventCode::SYNC_PARAMETERS_UPDATED;
                            rsEvents->postEvent(ev);
                    }
                        break;

                    case RsGxsNotify::TYPE_PUBLISHED:
					case RsGxsNotify::TYPE_RECEIVED_NEW:
					{
						/* group received */

						RS_STACK_MUTEX(mKnownForumsMutex);

						if(mKnownForums.find(grpChange->mGroupId) == mKnownForums.end())
						{
							mKnownForums.insert( std::make_pair(grpChange->mGroupId, time(nullptr)));
							IndicateConfigChanged();

							auto ev = std::make_shared<RsGxsForumEvent>();
							ev->mForumGroupId = grpChange->mGroupId;
							ev->mForumEventCode = RsForumEventCode::NEW_FORUM;
							rsEvents->postEvent(ev);
						}
						else
							RsInfo() << __PRETTY_FUNCTION__
							         << " Not notifying already known forum "
							         << grpChange->mGroupId << std::endl;
					}
						break;

					case RsGxsNotify::TYPE_STATISTICS_CHANGED:
					{
						auto ev = std::make_shared<RsGxsForumEvent>();
						ev->mForumGroupId = grpChange->mGroupId;
						ev->mForumEventCode = RsForumEventCode::STATISTICS_CHANGED;
						rsEvents->postEvent(ev);
					}
						break;

                    case RsGxsNotify::TYPE_UPDATED:
					{
						// Happens when the group data has changed. In this case we need to analyse the old and new group in order to detect possible notifications for clients

						RsGxsForumGroupItem *old_forum_grp_item = dynamic_cast<RsGxsForumGroupItem*>(grpChange->mOldGroupItem);
						RsGxsForumGroupItem *new_forum_grp_item = dynamic_cast<RsGxsForumGroupItem*>(grpChange->mNewGroupItem);

						if(old_forum_grp_item == nullptr || new_forum_grp_item == nullptr)
						{
							RsErr() << __PRETTY_FUNCTION__ << " received GxsGroupUpdate item with mOldGroup and mNewGroup not of type RsGxsForumGroupItem. This is inconsistent!" << std::endl;
							delete grpChange;
							continue;
						}

						// First of all, we check if there is a difference between the old and new list of moderators

						std::list<RsGxsId> added_mods, removed_mods;

						for(auto& gxs_id: new_forum_grp_item->mGroup.mAdminList.ids)
							if(old_forum_grp_item->mGroup.mAdminList.ids.find(gxs_id) == old_forum_grp_item->mGroup.mAdminList.ids.end())
								added_mods.push_back(gxs_id);

						for(auto& gxs_id: old_forum_grp_item->mGroup.mAdminList.ids)
							if(new_forum_grp_item->mGroup.mAdminList.ids.find(gxs_id) == new_forum_grp_item->mGroup.mAdminList.ids.end())
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
					}
                        break;


					default:
                        RsErr() << " Got a GXS event of type " << grpChange->getType() << " Currently not handled." << std::endl;
						break;
					}
                }
			}
		}

		/* shouldn't need to worry about groups - as they need to be subscribed to */

        delete *it;
	}
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
