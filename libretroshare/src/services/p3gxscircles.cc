/*******************************************************************************
 * libretroshare/src/services: p3gxscircles.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
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
#include "rsitems/rsgxscircleitems.h"

#include "services/p3gxscircles.h"
#include "retroshare/rsgxsflags.h"
#include "util/rsrandom.h"
#include "util/rsdir.h"
#include "util/radix64.h"
#include "util/rsstring.h"
#include "util/rsdebug.h"
#include "pgp/pgpauxutils.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rspeers.h"
#include "rsserver/p3face.h"

#include <sstream>
#include <stdio.h>

/****
 * #define DEBUG_CIRCLES	 1
 ****/

/*extern*/ RsGxsCircles* rsGxsCircles = nullptr;

/******
 *
 * GxsCircles are used to limit the spread of Gxs Groups and Messages.
 *
 * This is done via GxsCircle parameters in GroupMetaData:
 *      mCircleType (ALL, External, Internal).
 *      mCircleId.
 *
 * The Circle Group contains the definition of who is allowed access to the Group.
 * and GXS asks this service before forwarding any data.
 *
 * The CircleGroup contains:
 *      list of GxsId's
 *      list of GxsCircleId's (subcircles also allowed).
 *
 * This service runs a background task to transform the CircleGroups
 * into a list of friends/peers who are allowed access.
 * These results are cached to provide GXS with quick access to the information. 
 * This involves:
 *      - fetching the GroupData via GXS.
 *      - querying the list of GxsId to see if they are known.
 *		(NB: this will cause caching of GxsId in p3IdService.
 *      - recursively loading subcircles to complete Circle definition.
 *      - saving the result into Cache.
 *
 * For Phase 1, we will only use the list of GxsIds. No subcircles will be allowed.
 * Recursively determining membership via sub-circles is complex and needs more thought.
 * The data-types for the full system, however, will be in-place.
 */


#define CIRCLEREQ_CACHELOAD	  0x0001
#define CIRCLEREQ_CIRCLE_LIST    0x0002
#define CIRCLEREQ_MESSAGE_DATA   0x0003

//#define CIRCLEREQ_PGPHASH 	0x0010
//#define CIRCLEREQ_REPUTATION 	0x0020

//#define CIRCLEREQ_CACHETEST 	0x1000

// Events.
#define CIRCLE_EVENT_LOADIDS		0x0001
#define CIRCLE_EVENT_CACHELOAD 		0x0002
#define CIRCLE_EVENT_RELOADIDS 		0x0003
#define CIRCLE_EVENT_DUMMYSTART		0x0004
#define CIRCLE_EVENT_DUMMYLOAD 		0x0005
#define CIRCLE_EVENT_DUMMYGEN 		0x0006

#define CIRCLE_DUMMY_STARTPERIOD	300  // MUST BE LONG ENOUGH FOR IDS TO HAVE BEEN MADE.
#define CIRCLE_DUMMY_GENPERIOD		10

//#define CIRCLE_EVENT_CACHETEST 		0x1000
//#define CACHETEST_PERIOD	60
//#define OWNID_RELOAD_DELAY		10

#define GXSID_LOAD_CYCLE		10	// GXSID completes a load in this period.

#define MIN_CIRCLE_LOAD_GAP		5
#define GXS_CIRCLE_DELAY_TO_FORCE_MEMBERSHIP_UPDATE	 60	// re-check every 1 mins. Normally this shouldn't be necessary since notifications inform abotu new messages.
#define GXS_CIRCLE_DELAY_TO_CHECK_MEMBERSHIP_UPDATE	 60	// re-check every 1 mins. Normally this shouldn't be necessary since notifications inform abotu new messages.
#define GXS_CIRCLE_DELAY_TO_SEND_CACHE_UPDATED_EVENT 10	// do not send cache update events more often than every 10 secs.

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsCircles::p3GxsCircles(
        RsGeneralDataService *gds, RsNetworkExchangeService *nes,
        p3IdService *identities, PgpAuxUtils *pgpUtils) :
    RsGxsCircleExchange(
        gds, nes, new RsGxsCircleSerialiser(), RS_SERVICE_GXS_TYPE_GXSCIRCLE,
        identities, circleAuthenPolicy() ),
    RsGxsCircles(static_cast<RsGxsIface&>(*this)), GxsTokenQueue(this),
    RsTickEvent(), mIdentities(identities), mPgpUtils(pgpUtils),
    mCircleMtx("p3GxsCircles"),
    mCircleCache(DEFAULT_MEM_CACHE_SIZE, "GxsCircleCache" ),
    mCacheUpdated(false)
{
	// Kick off Cache Testing, + Others.
	//RsTickEvent::schedule_in(CIRCLE_EVENT_CACHETEST, CACHETEST_PERIOD);
    	mLastCacheMembershipUpdateTS = 0 ;
        mLastCacheUpdateEvent = 0;
        
	RsTickEvent::schedule_now(CIRCLE_EVENT_LOADIDS);

	// Dummy Circles.
	//	RsTickEvent::schedule_in(CIRCLE_EVENT_DUMMYSTART, CIRCLE_DUMMY_STARTPERIOD);
	mDummyIdToken = 0;
}

static bool allowedGxsIdFlagTest(uint32_t subscription_flags,bool group_is_self_restricted)
{
    if(group_is_self_restricted)
	    return   (subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST) && (subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE);
    else
	    return   (subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST) && (subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED) && (subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE);
}

const std::string GXS_CIRCLES_APP_NAME = "gxscircle";
const uint16_t GXS_CIRCLES_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_CIRCLES_APP_MINOR_VERSION  =       0;
const uint16_t GXS_CIRCLES_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_CIRCLES_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3GxsCircles::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_GXSCIRCLE,
                GXS_CIRCLES_APP_NAME,
                GXS_CIRCLES_APP_MAJOR_VERSION,
                GXS_CIRCLES_APP_MINOR_VERSION,
                GXS_CIRCLES_MIN_MAJOR_VERSION,
                GXS_CIRCLES_MIN_MINOR_VERSION);
}

bool p3GxsCircles::createCircle(
        const std::string& circleName, RsGxsCircleType circleType,
        RsGxsCircleId& circleId, const RsGxsCircleId& restrictedId,
        const RsGxsId& authorId, const std::set<RsGxsId>& gxsIdMembers,
        const std::set<RsPgpId>& localMembers )
{
	if(circleName.empty())
	{
		RsErr() << __PRETTY_FUNCTION__ << " Circle name is empty" << std::endl;
		return false;
	}

	switch(circleType)
	{
	case RsGxsCircleType::PUBLIC:
		if(!restrictedId.isNull())
		{
			RsErr() << __PRETTY_FUNCTION__ << " restrictedId: " << restrictedId
			        << " must be null with RsGxsCircleType::PUBLIC"
			        << std::endl;
			return false;
		}
		break;
	case RsGxsCircleType::EXTERNAL:
		if(restrictedId.isNull())
		{
			RsErr() << __PRETTY_FUNCTION__ << " restrictedId can't be null "
			        << "with RsGxsCircleType::EXTERNAL" << std::endl;
			return false;
		}
		break;
	case RsGxsCircleType::NODES_GROUP:
		if(localMembers.empty())
		{
			RsErr() << __PRETTY_FUNCTION__ << " localMembers can't be empty "
			        << "with RsGxsCircleType::NODES_GROUP" << std::endl;
			return false;
		}
		break;
	case RsGxsCircleType::LOCAL:
		break;
	case RsGxsCircleType::EXT_SELF:
		if(!restrictedId.isNull())
		{
			RsErr() << __PRETTY_FUNCTION__ << " restrictedId: " << restrictedId
			        << " must be null with RsGxsCircleType::EXT_SELF"
			        << std::endl;
			return false;
		}
		if(gxsIdMembers.empty())
		{
			RsErr() << __PRETTY_FUNCTION__ << " gxsIdMembers can't be empty "
			        << "with RsGxsCircleType::EXT_SELF" << std::endl;
			return false;
		}
		break;
	case RsGxsCircleType::YOUR_EYES_ONLY:
		break;
	default:
		RsErr() << __PRETTY_FUNCTION__ << " Invalid circle type: "
		        << static_cast<uint32_t>(circleType) << std::endl;
		return false;
	}

	RsGxsCircleGroup cData;
	cData.mMeta.mGroupName = circleName;
	cData.mMeta.mAuthorId = authorId;
	cData.mMeta.mCircleType = static_cast<uint32_t>(circleType);
	cData.mMeta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
	cData.mMeta.mCircleId = restrictedId;
	cData.mLocalFriends = localMembers;
	cData.mInvitedMembers = gxsIdMembers;

	uint32_t token;
	createGroup(token, cData);

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, cData.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting created"
		          << " group data." << std::endl;
		return false;
	}

	circleId = static_cast<RsGxsCircleId>(cData.mMeta.mGroupId);
	return true;
};

bool p3GxsCircles::editCircle(RsGxsCircleGroup& cData)
{
	uint32_t token;
	updateGroup(token, cData);

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, cData.mMeta))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting updated"
		          << " group data." << std::endl;
		return false;
	}

	return true;
}

bool p3GxsCircles::getCirclesSummaries(std::list<RsGroupMetaData>& circles)
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	if( !requestGroupInfo(token, opts) || waitToken(token) != RsTokenService::COMPLETE )
    {
        std::cerr << "Cannot get circles summary. Token queue is overloaded?" << std::endl;
        return false;
    }
    else
		return getGroupSummary(token, circles);
}

bool p3GxsCircles::getCirclesInfo( const std::list<RsGxsGroupId>& circlesIds,
                                   std::vector<RsGxsCircleGroup>& circlesInfo )
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if( !requestGroupInfo(token, opts, circlesIds) || waitToken(token) != RsTokenService::COMPLETE )
    {
        std::cerr << "Cannot get circle info. Token queue is overloaded?" << std::endl;
        return false;
    }
	else
		return getGroupData(token, circlesInfo);
}

bool p3GxsCircles::getCircleRequests( const RsGxsGroupId& circleId,
                                      std::vector<RsGxsCircleMsg>& requests )
{
	uint32_t token;
	std::list<RsGxsGroupId> grpIds { circleId };
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	if( !requestMsgInfo(token, opts, grpIds) ||
	        waitToken(token) != RsTokenService::COMPLETE ) return false;

	return getMsgData(token, requests);
}

bool p3GxsCircles::getCircleRequest(const RsGxsGroupId& circleId,const RsGxsMessageId& msgId,RsGxsCircleMsg& msg)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

    std::set<RsGxsMessageId> contentsIds;
	contentsIds.insert(msgId);

	GxsMsgReq msgIds;
	msgIds[circleId] = contentsIds;

	uint32_t token;
	if( !requestMsgInfo(token, opts, msgIds) || waitToken(token) != RsTokenService::COMPLETE )
        return false;

    std::vector<RsGxsCircleMsg> msgs;

	if(getMsgData(token, msgs) && msgs.size() == 1)
    {
        msg = msgs.front();
        return true;
    }
    else
        return false;
}

bool p3GxsCircles::inviteIdsToCircle( const std::set<RsGxsId>& identities,
                                      const RsGxsCircleId& circleId )
{
	const std::list<RsGxsGroupId> circlesIds{ RsGxsGroupId(circleId) };
	std::vector<RsGxsCircleGroup> circlesInfo;

	if(!getCirclesInfo(circlesIds, circlesInfo))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting group data."
		          << std::endl;
		return false;
	}

	if(circlesInfo.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Circle: "
		          << circleId.toStdString() << " not found!" << std::endl;
		return false;
	}

	RsGxsCircleGroup& circleGrp = circlesInfo[0];

	if(!(circleGrp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Attempt to edit non-own "
		          << "circle: " << circleId.toStdString() << std::endl;
		return false;
	}

	circleGrp.mInvitedMembers.insert(identities.begin(), identities.end());

	return editCircle(circleGrp);
}

bool p3GxsCircles::revokeIdsFromCircle( const std::set<RsGxsId>& identities, const RsGxsCircleId& circleId )
{
	const std::list<RsGxsGroupId> circlesIds{ RsGxsGroupId(circleId) };
	std::vector<RsGxsCircleGroup> circlesInfo;

	if(!getCirclesInfo(circlesIds, circlesInfo))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failure getting group data." << std::endl;
		return false;
	}

	if(circlesInfo.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Circle: " << circleId.toStdString() << " not found!" << std::endl;
		return false;
	}

	RsGxsCircleGroup& circleGrp = circlesInfo[0];

	if(!(circleGrp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Attempt to edit non-own " << "circle: " << circleId.toStdString() << std::endl;
		return false;
	}

	circleGrp.mInvitedMembers.erase(identities.begin(), identities.end());

	return editCircle(circleGrp);
}

bool p3GxsCircles::exportCircleLink(
        std::string& link, const RsGxsCircleId& circleId,
        bool includeGxsData, const std::string& baseUrl, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(circleId.isNull()) return failure("circleId cannot be null");

	const bool outputRadix = baseUrl.empty();
	if(outputRadix && !includeGxsData) return
	        failure("includeGxsData must be true if format requested is base64");

	RsGxsGroupId&& groupId = static_cast<RsGxsGroupId>(circleId);
	if( includeGxsData &&
	        !RsGenExchange::exportGroupBase64(link, groupId, errMsg) )
		return failure(errMsg);

	if(outputRadix) return true;

	std::vector<RsGxsCircleGroup> circlesInfo;
	if( !getCirclesInfo(
	            std::list<RsGxsGroupId>({groupId}), circlesInfo )
	        || circlesInfo.empty() )
		return failure("failure retrieving circle information");

	RsUrl inviteUrl(baseUrl);
	inviteUrl.setQueryKV(CIRCLE_URL_ID_FIELD, circleId.toStdString());
	inviteUrl.setQueryKV(CIRCLE_URL_NAME_FIELD, circlesInfo[0].mMeta.mGroupName);
	if(includeGxsData) inviteUrl.setQueryKV(CIRCLE_URL_DATA_FIELD, link);

	link = inviteUrl.toString();
	return true;
}

bool p3GxsCircles::importCircleLink(
        const std::string& link, RsGxsCircleId& circleId,
        std::string& errMsg )
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
	const auto qIt = query.find(CIRCLE_URL_DATA_FIELD);
	if(qIt != query.end()) radixPtr = &qIt->second;

	if(radixPtr->empty()) return failure(CIRCLE_URL_DATA_FIELD + " is empty");

	if(!RsGenExchange::importGroupBase64(
	            *radixPtr, reinterpret_cast<RsGxsGroupId&>(circleId), errMsg) )
		return failure(errMsg);

	return true;
}

uint32_t p3GxsCircles::circleAuthenPolicy()
{
	uint32_t policy = 0;
	uint8_t flag = 0;

	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}


void	p3GxsCircles::service_tick()
{
	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests(); // GxsTokenQueue handles all requests.
    
	rstime_t now = time(NULL);

    if(mCacheUpdated && now > mLastCacheUpdateEvent + GXS_CIRCLE_DELAY_TO_SEND_CACHE_UPDATED_EVENT)
    {
        if(rsEvents)
        {
            auto ev = std::make_shared<RsGxsCircleEvent>();
            ev->mCircleEventType = RsGxsCircleEventCode::CACHE_DATA_UPDATED;
            rsEvents->postEvent(ev);
        }

        mLastCacheUpdateEvent = now;
        mCacheUpdated = false;
    }

	if(now > mLastCacheMembershipUpdateTS + GXS_CIRCLE_DELAY_TO_CHECK_MEMBERSHIP_UPDATE)
	{
		checkCircleCache();
		mLastCacheMembershipUpdateTS = now ;
	}
	return;
}


void p3GxsCircles::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::notifyChanges()";
	std::cerr << std::endl;
#endif

	p3Notify *notify = RsServer::notify();

	for(auto it = changes.begin(); it != changes.end(); ++it)
	{
		RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
		RsGxsNotify *c = *it;

		if (msgChange)
		{
#ifdef DEBUG_CIRCLES
			std::cerr << "  Found circle Message Change Notification" << std::endl;
#endif
			for(auto mit = msgChange->msgChangeMap.begin(); mit != msgChange->msgChangeMap.end(); ++mit)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "    Msgs for Group: " << mit->first << std::endl;
#endif
                RsGxsCircleId circle_id(mit->first);

				force_cache_reload(circle_id);

                RsGxsCircleDetails details;
				getCircleDetails(circle_id,details);

				if(rsEvents && (c->getType() == RsGxsNotify::TYPE_RECEIVED_NEW|| c->getType() == RsGxsNotify::TYPE_PUBLISHED) )
					for (auto msgIdIt(mit->second.begin()), end(mit->second.end()); msgIdIt != end; ++msgIdIt)
					{
						RsGxsCircleMsg msg;
						getCircleRequest(RsGxsGroupId(circle_id),*msgIdIt,msg);

						auto ev = std::make_shared<RsGxsCircleEvent>();
						ev->mCircleId = circle_id;
						ev->mGxsId = msg.mMeta.mAuthorId;

						if (msg.stuff == "SUBSCRIPTION_REQUEST_UNSUBSCRIBE")
							ev->mCircleEventType = RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_LEAVE;
						else if(details.mAllowedGxsIds.find(msg.mMeta.mAuthorId) != details.mAllowedGxsIds.end())
							ev->mCircleEventType = RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_JOIN;
						else
							ev->mCircleEventType = RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_REQUEST;

						rsEvents->postEvent(ev);
					}

				mCircleCache.erase(circle_id);
                mCacheUpdated = true;
			}
		}

	    /* add groups to ExternalIdList (Might get Personal Circles here until NetChecks in place) */
	    if (groupChange && !groupChange->metaChange())
	    {
#ifdef DEBUG_CIRCLES
		    std::cerr << "  Found Group Change Notification" << std::endl;
#endif
		    for(std::list<RsGxsGroupId>::iterator git = groupChange->mGrpIdList.begin(); git != groupChange->mGrpIdList.end(); ++git)
		    {
#ifdef DEBUG_CIRCLES
			    std::cerr << "    Incoming Group: " << *git << ". Forcing cache load." << std::endl;
#endif

			    // for new circles we need to add them to the list.
			    // we don't know the type of this circle here
			    // original behavior was to add all ids to the external ids list

			    addCircleIdToList(RsGxsCircleId(*git), 0);

			    // reset the cached circle data for this id
			    {
				    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
				    mCircleCache.erase(RsGxsCircleId(*git));
					mCacheUpdated = true;
			    }
		    }
	    }

		if(groupChange)
        {
            std::list<RsGxsId> own_ids;
            rsIdentity->getOwnIds(own_ids);

			for(std::list<RsGxsGroupId>::const_iterator git(groupChange->mGrpIdList.begin());git!=groupChange->mGrpIdList.end();++git)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "  forcing cache loading for circle " << *git << " in order to trigger subscribe update." << std::endl;
#endif

#ifdef TODO
                // This code will not work: we would like to detect changes in the circle data that reflects the fact that one of the
                // owned GXS ids is invited. But there's no way to compare the old circle data to the new if cache has to be updated.
                // For this we need to add the old metadata and group data in the RsGxsGroupChange structure and account for it.

                if(rsEvents && (c->getType() == RsGxsNotify::TYPE_RECEIVED_NEW) )
                {
                    RsGxsCircleId circle_id(*git);
					force_cache_reload(circle_id);

					RsGxsCircleDetails details;
					getCircleDetails(circle_id,details);

                    // We check that the change corresponds to one of our own ids. Since we do not know what the change is, we notify
                    // for whatever is different from what is currently known. Other ids, that get invited only trigger a notification when the
                    // ID also accepts the invitation, so it becomes a member of the circle.

                    for(auto own_id: own_ids)
					{
                        auto it = details.mSubscriptionFlags.find(own_id);

                        if(it == details.mSubscriptionFlags.end())
                            continue;

						bool invited ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST );
						bool subscrb ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED );

                        if(std::find(details.mAllowedGxsIds.begin(),details.mAllowedGxsIds.end(),id) != details.mAllowedGxsIds.end() && !me_in_circle)
						{
							auto ev = std::make_shared<RsGxsCircleEvent>();

							ev->mType = RsGxsCircleEvent::CIRCLE_MEMBERSHIP_INVITE;
							ev->mCircleId = circle_id;
							ev->mGxsId = ;

							rsEvents->sendEvent(ev);
						}
					}

				}
#endif

				if(rsEvents && (c->getType() == RsGxsNotify::TYPE_RECEIVED_NEW|| c->getType() == RsGxsNotify::TYPE_PUBLISHED) )
                {
					auto ev = std::make_shared<RsGxsCircleEvent>();
					ev->mCircleId = RsGxsCircleId(*git);
					ev->mCircleEventType = RsGxsCircleEventCode::NEW_CIRCLE;
					rsEvents->postEvent(ev);
                }

                // reset circle from cache since the number of invitee may have changed.
				{
				    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
				    mCircleCache.erase(RsGxsCircleId(*git));
					mCacheUpdated = true;
			    }

            }
        }

		RsGxsGroupUpdate *grpUpdate = dynamic_cast<RsGxsGroupUpdate*>(*it);

		if (grpUpdate && rsEvents)
		{
			// Happens when the group data has changed. In this case we need to analyse the old and new group in order to detect possible notifications for clients

			RsGxsCircleGroupItem *old_circle_grp_item = dynamic_cast<RsGxsCircleGroupItem*>(grpUpdate->mOldGroupItem);
			RsGxsCircleGroupItem *new_circle_grp_item = dynamic_cast<RsGxsCircleGroupItem*>(grpUpdate->mNewGroupItem);

            const RsGxsCircleId circle_id ( old_circle_grp_item->meta.mGroupId );

			if(old_circle_grp_item == nullptr || new_circle_grp_item == nullptr)
			{
				RsErr() << __PRETTY_FUNCTION__ << " received GxsGroupUpdate item with mOldGroup and mNewGroup not of type RsGxsCircleGroupItem. This is inconsistent!" << std::endl;
				delete grpUpdate;
				continue;
			}

			// First of all, we check if there is a difference between the old and new list of invited members

			std::list<RsGxsId> added_identities, removed_identities;
            std::list<RsGxsId> own_ids_lst;
            rsIdentity->getOwnIds(own_ids_lst,false);		// retrieve own identities

            std::set<RsGxsId> own_ids;
            for(auto& id:own_ids_lst)
                own_ids.insert(id);		// put them in a std::set for O(log(n)) search

			for(auto& gxs_id: new_circle_grp_item->gxsIdSet.ids)
				if(old_circle_grp_item->gxsIdSet.ids.find(gxs_id) == old_circle_grp_item->gxsIdSet.ids.end() && own_ids.find(gxs_id)!=own_ids.end())
					added_identities.push_back(gxs_id);

			for(auto& gxs_id: old_circle_grp_item->gxsIdSet.ids)
				if(new_circle_grp_item->gxsIdSet.ids.find(gxs_id) == old_circle_grp_item->gxsIdSet.ids.end() && own_ids.find(gxs_id)!=own_ids.end())
					removed_identities.push_back(gxs_id);

            for(auto& gxs_id:added_identities)
            {
				auto ev = std::make_shared<RsGxsCircleEvent>();

				ev->mCircleEventType = RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_INVITE;
				ev->mCircleId = circle_id;
				ev->mGxsId = gxs_id;

				rsEvents->sendEvent(ev);
            }
            for(auto& gxs_id:removed_identities)
            {
				auto ev = std::make_shared<RsGxsCircleEvent>();

				ev->mCircleEventType = RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_REVOKED;
				ev->mCircleId = circle_id;
				ev->mGxsId = gxs_id;

				rsEvents->sendEvent(ev);
            }
        }

        delete *it;
	}
}

/********************************************************************************/
/******************* RsCircles Interface  ***************************************/
/********************************************************************************/

bool p3GxsCircles::getCircleDetails(
        const RsGxsCircleId& id, RsGxsCircleDetails& details)
{

#ifdef DEBUG_CIRCLES
    std::cerr << "p3GxsCircles::getCircleDetails(" << id << ")";
    std::cerr << std::endl;
#endif // DEBUG_CIRCLES

    {
	    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	    if (mCircleCache.is_cached(id))
	    {
		    RsGxsCircleCache &data = mCircleCache.ref(id);

		    // should also have meta data....
		    details.mCircleId = id;
		    details.mCircleName = data.mCircleName;

		    details.mCircleType = data.mCircleType;
		    details.mRestrictedCircleId = data.mRestrictedCircleId;

		    details.mAllowedNodes = data.mAllowedNodes;
		    details.mSubscriptionFlags.clear();
		    details.mAllowedGxsIds.clear();
		    details.mAmIAllowed = false ;
		    details.mAmIAdmin = bool(data.mGroupSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

		    for(std::map<RsGxsId,RsGxsCircleMembershipStatus>::const_iterator it(data.mMembershipStatus.begin());it!=data.mMembershipStatus.end();++it)
		    {
			    details.mSubscriptionFlags[it->first] = it->second.subscription_flags ;

			    if(it->second.subscription_flags == GXS_EXTERNAL_CIRCLE_FLAGS_ALLOWED)
                		{
				    details.mAllowedGxsIds.insert(it->first) ;
                    
                    			if(rsIdentity->isOwnId(it->first))
		    				details.mAmIAllowed = true ;
                    		}
		    }

		    return true;
	    }
    }

    /* it isn't there - add to public requests */
    cache_request_load(id);

    return false;
}

bool p3GxsCircles:: getCircleExternalIdList(std::list<RsGxsCircleId> &circleIds)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::getCircleIdList()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (circleIds.empty())
	{
		circleIds = mCircleExternalIdList;
	}
	else
	{
		std::list<RsGxsCircleId>::const_iterator it;
		for(it = mCircleExternalIdList.begin(); it != mCircleExternalIdList.begin(); ++it)
		{
			circleIds.push_back(*it);
		}
	}

	return true;
}


/********************************************************************************/
/******************* RsGcxs Interface     ***************************************/
/********************************************************************************/

bool p3GxsCircles::isLoaded(const RsGxsCircleId &circleId)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	return mCircleCache.is_cached(circleId);
}

bool p3GxsCircles::loadCircle(const RsGxsCircleId &circleId)
{
	return cache_request_load(circleId);
}


int p3GxsCircles::canSend(const RsGxsCircleId &circleId, const RsPgpId &id, bool& should_encrypt)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (mCircleCache.is_cached(circleId))
	{
		RsGxsCircleCache &data = mCircleCache.ref(circleId);
		should_encrypt = (data.mCircleType == GXS_CIRCLE_TYPE_EXTERNAL);
                
		if (data.isAllowedPeer(id))
			return 1;
        
		return 0;
	}
	return -1;
}

int p3GxsCircles::canReceive(const RsGxsCircleId &circleId, const RsPgpId &id)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (mCircleCache.is_cached(circleId))
	{
		RsGxsCircleCache &data = mCircleCache.ref(circleId);
		if (data.isAllowedPeer(id))
		{
			return 1;
		}
		return 0;
	}
	return -1;
}

bool p3GxsCircles::recipients(const RsGxsCircleId &circleId, std::list<RsPgpId>& friendlist)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (mCircleCache.is_cached(circleId))
	{
		RsGxsCircleCache &data = mCircleCache.ref(circleId);
		data.getAllowedPeersList(friendlist);
		return true;
	}
	return false;
}

bool p3GxsCircles::isRecipient(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, const RsGxsId& id) 
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (mCircleCache.is_cached(circleId))
	{
		const RsGxsCircleCache &data = mCircleCache.ref(circleId);
		return data.isAllowedPeer(id,destination_group);
	}
	return false;
}

// This function uses the destination group for the transaction in order to decide which list of
// keys to ecnrypt to. When sending to a self-restricted group, the list of recipients is extended to
// the admin list rather than just the members list.

bool p3GxsCircles::recipients(const RsGxsCircleId& circleId, const RsGxsGroupId& dest_group, std::list<RsGxsId>& gxs_ids)
{
	gxs_ids.clear() ;

	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (!mCircleCache.is_cached(circleId))
		return false ;

	const RsGxsCircleCache& cache = mCircleCache.ref(circleId);

	for(std::map<RsGxsId,RsGxsCircleMembershipStatus>::const_iterator it(cache.mMembershipStatus.begin());it!=cache.mMembershipStatus.end();++it)
        	if(allowedGxsIdFlagTest(it->second.subscription_flags, RsGxsCircleId(dest_group) == circleId))
			gxs_ids.push_back(it->first) ;
		
	return true;
}

/********************************************************************************/
/******************* Get/Set Data      ******************************************/
/********************************************************************************/

bool p3GxsCircles::getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups)
{

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsCircleGroupItem* item = dynamic_cast<RsGxsCircleGroupItem*>(*vit);
			if (item)
			{
				RsGxsCircleGroup group;
				item->convertTo(group);

				// If its cached - add that info (TODO).
				groups.push_back(group);
				delete(item);
			}
			else
			{
				std::cerr << "p3GxsCircles::getGroupData()";
				std::cerr << " Not a RsGxsCircleGroupItem, deleting!";
				std::cerr << std::endl;
				delete *vit;
			}
		}
	}

	return ok;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

bool p3GxsCircles::getMsgData(const uint32_t &token, std::vector<RsGxsCircleMsg> &msgs)
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
				RsGxsCircleMsgItem* item = dynamic_cast<RsGxsCircleMsgItem*>(*vit);
				RsGxsCircleSubscriptionRequestItem* rsItem = dynamic_cast<RsGxsCircleSubscriptionRequestItem*>(*vit);
				if(item)
				{
					RsGxsCircleMsg msg = item->mMsg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else if (rsItem)
				{
					RsGxsCircleMsg msg ;//= rsItem->mMsg;
					msg.mMeta = rsItem->meta;
					switch (rsItem->subscription_type)
					{
						case RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_UNKNOWN:
							msg.stuff.clear();
						break;
						case RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_SUBSCRIBE:
							msg.stuff="SUBSCRIPTION_REQUEST_SUBSCRIBE";
						break;
						case RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_UNSUBSCRIBE:
							msg.stuff="SUBSCRIPTION_REQUEST_UNSUBSCRIBE";
						break;
					}
					msgs.push_back(msg);
					delete rsItem;
				}
				else
				{
					std::cerr << "Not a GxsCircleMsgItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}

void p3GxsCircles::createGroup(uint32_t& token, RsGxsCircleGroup &group)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::createGroup()";
        std::cerr << " CircleType: " << (uint32_t) group.mMeta.mCircleType;
        std::cerr << " CircleId: " << group.mMeta.mCircleId.toStdString();
        std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	RsGxsCircleGroupItem* item = new RsGxsCircleGroupItem();
	item->convertFrom(group);

	RsGenExchange::publishGroup(token, item);
}

void p3GxsCircles::updateGroup(uint32_t &token, RsGxsCircleGroup &group)
{
    // note: refresh of circle cache gets triggered in the RsGenExchange::notifyChanges() callback
    RsGxsCircleGroupItem* item = new RsGxsCircleGroupItem();
    item->convertFrom(group);

    RsGenExchange::updateGroup(token, item);
}

RsGenExchange::ServiceCreate_Return p3GxsCircles::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::service_CreateGroup()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	RsGxsCircleGroupItem *item = dynamic_cast<RsGxsCircleGroupItem *>(grpItem);
	if (!item)
	{
		std::cerr << "p3GxsCircles::service_CreateGroup() ERROR invalid cast";
		std::cerr << std::endl;
		return SERVICE_CREATE_FAIL;
	}

	// Now copy the GroupId into the mCircleId, and set the mode.
	if (item->meta.mCircleType == GXS_CIRCLE_TYPE_EXT_SELF)
	{
		item->meta.mCircleType = GXS_CIRCLE_TYPE_EXTERNAL;
		item->meta.mCircleId = RsGxsCircleId(item->meta.mGroupId);
	}

    // the advantage of adding the id to the list now is, that we know the cirlce type at this point
    // this is not the case in NotifyChanges()
    addCircleIdToList(RsGxsCircleId(item->meta.mGroupId), item->meta.mCircleType);

	return SERVICE_CREATE_SUCCESS;
}



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* 
 * Cache of recently used circles.
 */

RsGxsCircleCache::RsGxsCircleCache() 
{ 
	mCircleType = GXS_CIRCLE_TYPE_EXTERNAL;
	mIsExternal = true;
	mUpdateTime = 0;
	mGroupStatus = 0;
	mGroupSubscribeFlags = 0;
	mLastUpdatedMembershipTS = 0 ;

	return; 
}


bool RsGxsCircleCache::loadBaseCircle(const RsGxsCircleGroup &circle)
{

	mCircleId = RsGxsCircleId(circle.mMeta.mGroupId);
	mCircleName = circle.mMeta.mGroupName;
	mUpdateTime = time(NULL);
//	mProcessedCircles.insert(mCircleId);

	mCircleType = circle.mMeta.mCircleType;
	mIsExternal = (mCircleType != GXS_CIRCLE_TYPE_LOCAL);
	mGroupStatus = circle.mMeta.mGroupStatus;
	mGroupSubscribeFlags = circle.mMeta.mSubscribeFlags;
	mOriginator = circle.mMeta.mOriginator ;
        
        mAllowedNodes = circle.mLocalFriends ;
        mRestrictedCircleId = circle.mMeta.mCircleId ;
        
        mMembershipStatus.clear() ;
        
        for(std::set<RsGxsId>::const_iterator it(circle.mInvitedMembers.begin());it!=circle.mInvitedMembers.end();++it)
        {
            RsGxsCircleMembershipStatus& s(mMembershipStatus[*it]) ;
            s.last_subscription_TS = 0 ;
            s.subscription_flags = GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST ;
        }

#ifdef DEBUG_CIRCLES
	std::cerr << "RsGxsCircleCache::loadBaseCircle(" << mCircleId << ")";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	return true;
}

bool RsGxsCircleCache::loadSubCircle(const RsGxsCircleCache &subcircle)
{
	/* copy across all the lists */

	/* should not be any unprocessed circles or peers */
#ifdef DEBUG_CIRCLES
#endif // DEBUG_CIRCLES

	std::cerr << "RsGxsCircleCache::loadSubCircle(" << subcircle.mCircleId << ") TODO";
	std::cerr << std::endl;

	return true;
}

bool RsGxsCircleCache::getAllowedPeersList(std::list<RsPgpId>& friendlist) const
{
    friendlist.clear() ;
    
	for(std::set<RsPgpId>::const_iterator it = mAllowedNodes.begin(); it != mAllowedNodes.end(); ++it)
		friendlist.push_back(*it) ;
	
	return true;
}

bool RsGxsCircleCache::isAllowedPeer(const RsGxsId& id,const RsGxsGroupId& destination_group) const
{
	std::map<RsGxsId,RsGxsCircleMembershipStatus>::const_iterator it = mMembershipStatus.find(id) ;
    
    	if(it == mMembershipStatus.end())
            return false ;
        
        return allowedGxsIdFlagTest(it->second.subscription_flags, RsGxsGroupId(mCircleId) == destination_group) ;
}

bool RsGxsCircleCache::isAllowedPeer(const RsPgpId &id) const
{
    return mAllowedNodes.find(id) != mAllowedNodes.end() ;
}

bool RsGxsCircleCache::addLocalFriend(const RsPgpId &pgpId)
{
	/* empty list as no GxsID associated */
            mAllowedNodes.insert(pgpId) ;
	return true;
}


/************************************************************************************/
/************************************************************************************/

bool p3GxsCircles::request_CircleIdList()
{
	/* trigger request to load missing ids into cache */
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::request_CircleIdList()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, CIRCLEREQ_CIRCLE_LIST);	
	return true;
}


bool p3GxsCircles::load_CircleIdList(uint32_t token)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::load_CircleIdList() : " << token;
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

    std::list<RsGroupMetaData> groups;
    bool ok = RsGenExchange::getGroupMeta(token, groups);

	if(ok)
	{
		// Save List 
        {
            RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
            mCirclePersonalIdList.clear();
            mCircleExternalIdList.clear();
        }

        for(std::list<RsGroupMetaData>::iterator it = groups.begin(); it != groups.end(); ++it)
		{
            addCircleIdToList(RsGxsCircleId(it->mGroupId), it->mCircleType);
		}
	}
	else
	{
		std::cerr << "p3GxsCircles::load_CircleIdList() ERROR no data";
		std::cerr << std::endl;

		return false;
	}
	return true;
}



/****************************************************************************/
// ID STUFF. \/ \/ \/ \/ \/ \/ \/     :)
/****************************************************************************/
#if 0

/************************************************************************************/
/************************************************************************************/

bool p3GxsCircles::cachetest_getlist()
{
	std::cerr << "p3GxsCircles::cachetest_getlist() making request";
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, CIRCLEREQ_CACHETEST);	

	// Schedule Next Event.
	RsTickEvent::schedule_in(CIRCLE_EVENT_CACHETEST, CACHETEST_PERIOD);
	return true;
}

bool p3GxsCircles::cachetest_handlerequest(uint32_t token)
{
	std::cerr << "p3GxsCircles::cachetest_handlerequest() token: " << token;
	std::cerr << std::endl;

		std::list<RsGxsId> grpIds;
		bool ok = RsGenExchange::getGroupList(token, grpIds);

	if(ok)
	{
		std::list<RsGxsId>::iterator vit = grpIds.begin();
		for(; vit != grpIds.end(); ++vit)
		{
			/* 5% chance of checking it! */
			if (RSRandom::random_f32() < 0.25)
			{
				std::cerr << "p3GxsCircles::cachetest_request() Testing Id: " << *vit;
				std::cerr << std::endl;

				/* try the cache! */
				if (!haveKey(*vit))
				{
					std::list<PeerId> nullpeers;
					requestKey(*vit, nullpeers);

					std::cerr << "p3GxsCircles::cachetest_request() Requested Key Id: " << *vit;
					std::cerr << std::endl;
				}
				else
				{
					RsTlvSecurityKey seckey;
					if (getKey(*vit, seckey))
					{
						std::cerr << "p3GxsCircles::cachetest_request() Got Key OK Id: " << *vit;
						std::cerr << std::endl;

						// success!
							seckey.print(std::cerr, 10);
						std::cerr << std::endl;


					}
					else
					{
						std::cerr << "p3GxsCircles::cachetest_request() ERROR no Key for Id: " << *vit;
						std::cerr << std::endl;
					}
				}

				/* try private key too! */
				if (!havePrivateKey(*vit))
				{
					requestPrivateKey(*vit);
					std::cerr << "p3GxsCircles::cachetest_request() Requested PrivateKey Id: " << *vit;
					std::cerr << std::endl;
				}
				else
				{
					RsTlvSecurityKey seckey;
					if (getPrivateKey(*vit, seckey))
					{
						// success!
						std::cerr << "p3GxsCircles::cachetest_request() Got PrivateKey OK Id: " << *vit;
						std::cerr << std::endl;
					}
					else
					{
						std::cerr << "p3GxsCircles::cachetest_request() ERROR no PrivateKey for Id: " << *vit;
						std::cerr << std::endl;
					}
				}
			}
		}
	}
	else
	{
		std::cerr << "p3GxsCircles::cache_load_for_token() ERROR no data";
		std::cerr << std::endl;

		return false;
	}
	return true;
}

/****************************************************************************/
// ID STUFF. /\ /\ /\ /\ /\ /\ /\ /\    :)
/****************************************************************************/
#endif



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
// Complicated deal of loading Circles.

bool p3GxsCircles::force_cache_reload(const RsGxsCircleId& id)
{
#ifdef DEBUG_CIRCLES
    std::cerr << "p3GxsCircles::force_cache_reload(): Forcing cache reload of Circle ID " << id << std::endl;
#endif
    
    {
	    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_CIRCLES
	    std::cerr << "  clearing from existing cache entries..." << std::endl;
#endif

	    std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it = mLoadingCache.find(id);
	    if (it != mLoadingCache.end())
	    {
		    mLoadingCache.erase(it) ;
#ifdef DEBUG_CIRCLES
		    std::cerr << "  removed item from currently loading cache entries..." << std::endl;
#endif
	    }
	    mCircleCache.erase(id) ;
    }
    cache_request_load(id) ;
    
    return true ;
}

bool p3GxsCircles::cache_request_load(const RsGxsCircleId &id)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::cache_request_load(" << id << ")";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	{
		RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

		/* check its not loaded */
		if (mCircleCache.is_cached(id))
			return true;

		/* check it is not already being loaded */
		std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
		it = mLoadingCache.find(id);
		if (it != mLoadingCache.end())
		{
			// Already loading.
			return true;
		}
		// Put it into the Loading Cache - so we will detect it later.
		mLoadingCache[id] = RsGxsCircleCache();
		mCacheLoad_ToCache.push_back(id);
	}

	if (RsTickEvent::event_count(CIRCLE_EVENT_CACHELOAD) > 0)
	{
		/* its already scheduled */
		return true;
	}

	int32_t age = 0;
	if (RsTickEvent::prev_event_ago(CIRCLE_EVENT_CACHELOAD, age))
	{
		if (age < MIN_CIRCLE_LOAD_GAP)
		{
			RsTickEvent::schedule_in(CIRCLE_EVENT_CACHELOAD,  MIN_CIRCLE_LOAD_GAP - age);
			return true;
		}
	}

	RsTickEvent::schedule_now(CIRCLE_EVENT_CACHELOAD);
	return true;
}


bool p3GxsCircles::cache_start_load()
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::cache_start_load()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLESmatch

	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
	{
		RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		std::list<RsGxsCircleId>::iterator it;
		for(it = mCacheLoad_ToCache.begin(); it != mCacheLoad_ToCache.end(); ++it)
		{
#ifdef DEBUG_CIRCLES
			std::cerr << "p3GxsCircles::cache_start_load() GroupId: " << *it;
			std::cerr << std::endl;
#endif // DEBUG_CIRCLES

			groupIds.push_back(RsGxsGroupId(it->toStdString())); // might need conversion?
		}

		mCacheLoad_ToCache.clear();
	}

	if (groupIds.size() > 0)
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::cache_start_load() #Groups: " << groupIds.size();
		std::cerr << std::endl;
#endif // DEBUG_CIRCLES

		uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
		uint32_t token = 0;
	
		RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groupIds);
		GxsTokenQueue::queueRequest(token, CIRCLEREQ_CACHELOAD);	
	}
	return 1;
}


bool p3GxsCircles::cache_load_for_token(uint32_t token)
{
#ifdef DEBUG_CIRCLES
    std::cerr << "p3GxsCircles::cache_load_for_token() : " << token << std::endl;
#endif // DEBUG_CIRCLES

    std::vector<RsGxsGrpItem*> grpData;

    if(!RsGenExchange::getGroupData(token, grpData))
    {
	    std::cerr << "p3GxsCircles::cache_load_for_token() ERROR no data";
	    std::cerr << std::endl;

	    return false;
    }

    std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

    for(; vit != grpData.end(); ++vit)
    {
	    RsGxsCircleGroupItem *item = dynamic_cast<RsGxsCircleGroupItem*>(*vit);
	    if (!item)
	    {
		    std::cerr << "  Not a RsGxsCircleGroupItem Item, deleting!" << std::endl;
		    delete(*vit);
		    continue;
	    }
	    RsGxsCircleGroup group;
	    item->convertTo(group);

#ifdef DEBUG_CIRCLES
	    std::cerr << "  Loaded Id with Meta: " << item->meta << std::endl;
#endif // DEBUG_CIRCLES


	    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

	    /* should already have a LoadingCache entry */
	    RsGxsCircleId id = RsGxsCircleId(item->meta.mGroupId) ;

	    // (cyril) I'm not sure this logic is needed. The token system would avoid duplicates normally.

	    std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it = mLoadingCache.find(id);

	    if (it == mLoadingCache.end())
	    {
		    std::cerr << "p3GxsCircles::cache_load_for_token() Load ERROR: ";
		    std::cerr << item->meta;
		    std::cerr << std::endl;
		    delete(item);
		    // ERROR.
		    continue;
	    }

	    RsGxsCircleCache& cache = it->second;

	    cache.loadBaseCircle(group);
	    delete item;

	    if(locked_processLoadingCacheEntry(it->second)) 
	    {
#ifdef DEBUG_CIRCLES
		    std::cerr << "  All peers available. Moving to cache..." << std::endl;
#endif
		    mLoadingCache.erase(it);
	    }
	    else
	    {
#ifdef DEBUG_CIRCLES
		    std::cerr << "  Unprocessed peers. Requesting reload..." << std::endl;
#endif

		    /* schedule event to try reload gxsIds */
		    RsTickEvent::schedule_in(CIRCLE_EVENT_RELOADIDS, GXSID_LOAD_CYCLE, id.toStdString());
	    }

		mCacheUpdated = true;
    }

    return true;
}

// This method parses the cache entry and makes sure that all ids are known. If not, requests the missing ids
// when done, the entry is removed from mLoadingCache

bool p3GxsCircles::locked_processLoadingCacheEntry(RsGxsCircleCache& cache)
{
	//bool isUnprocessedPeers = false;

	if (cache.mIsExternal)
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "Processing External Circle " << cache.mCircleId << std::endl;
#endif

        // Do we actually need to retrieve the missing keys for all members of a circle???
        // These keys are needed for subscribtion request signature checking. But this is only
        // when a subscription msg is posted, which would trigger retrieval of the key anyway
        // Maybe this can be made an option of p3GxsCircles, or of rsIdentity.

		// need to trigger the searches.
		for(std::map<RsGxsId,RsGxsCircleMembershipStatus>::iterator pit = cache.mMembershipStatus.begin(); pit != cache.mMembershipStatus.end(); ++pit)
		{
#ifdef DEBUG_CIRCLES
			std::cerr << "  Member status: " << pit->first << " : " << pit->second.subscription_flags;
#endif

			/* check cache */
			if(!(pit->second.subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE))
			{
				if(mIdentities->haveKey(pit->first))
				{
					pit->second.subscription_flags |= GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE;

                    mCacheUpdated = true;
#ifdef DEBUG_CIRCLES
					std::cerr << "    Key is now available!"<< std::endl;
#endif
				}
				else
				{
                    std::list<RsPeerId> peers;

					if(!cache.mOriginator.isNull())
					{
						peers.push_back(cache.mOriginator) ;
#ifdef DEBUG_CIRCLES
						std::cerr << "    Requesting unknown/unloaded identity: " << pit->first << " to originator " << cache.mOriginator << std::endl;
#endif
					}
					else
					{
						std::cerr << "    (WW) cache entry for circle " << cache.mCircleId << " has empty originator. Asking info for GXS id " << pit->first << " to all connected friends." << std::endl;

						rsPeers->getOnlineList(peers) ;
					}

					mIdentities->requestKey(pit->first, peers,RsIdentityUsage(serviceType(),RsIdentityUsage::CIRCLE_MEMBERSHIP_CHECK,RsGxsGroupId(cache.mCircleId)));
					//isUnprocessedPeers = true;
				}
			}
#ifdef DEBUG_CIRCLES
	    		else
                		std::cerr << "  Key is available. Nothing to process." << std::endl;
#endif
		}

#ifdef HANDLE_SUBCIRCLES
#if 0
		std::list<RsGxsCircleId> &circles = group.mSubCircles;
		std::list<RsGxsCircleId>::const_iterator cit;
		for(cit = circles.begin(); cit != circles.end(); ++cit)
		{
			/* if its cached already -> then its complete. */
			if (mCircleCache.is_loaded(*cit))
			{
				RsGxsCircleCache cachedCircle;
				if (mCircleCache.fetch(&cit, cachedCircle))
				{
					/* copy cached circle into circle */
					cache.loadSubCircle(cachedCircle);
				}
				else
				{
					/* error */
					continue;
				}
			}
			else
			{
				/* push into secondary processing queues */
				std::list<RsGxsCircleId> &proc_circles = mCacheLoad_SubCircle[*cit];
				proc_circles.push_back(id);

				subCirclesToLoad.push_back(*cit);

				isComplete = false;
				isUnprocessedCircles = true;
			}
		}
#endif
#endif
	}
#ifdef DEBUG_CIRCLES
	else
		std::cerr << "  Loading Personal Circle" << std::endl;
#endif

	// We can check for self inclusion in the circle right away, since own ids are always loaded.
	// that allows to subscribe/unsubscribe uncomplete circles 

    locked_checkCircleCacheForAutoSubscribe(cache);
	locked_checkCircleCacheForMembershipUpdate(cache);

    	// always store in cache even if uncomplete. But do not remove the loading items so that they can be kept in loading state.
//	if(isUnprocessedPeers)
	//	return false ;
    
	/* move straight into the cache */
	mCircleCache.store(cache.mCircleId, cache);
	mCircleCache.resize();

	std::cerr << "  Loading complete." << std::endl;
        
	return true ;
}

bool p3GxsCircles::cache_reloadids(const RsGxsCircleId &circleId)
{
#ifdef DEBUG_CIRCLES
    std::cerr << "p3GxsCircles::cache_reloadids()";
    std::cerr << std::endl;
#endif // DEBUG_CIRCLES

    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

    /* fetch from loadMap */
    std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
    it = mLoadingCache.find(circleId);
    if (it == mLoadingCache.end())
    {
	    std::cerr << "p3GxsCircles::cache_reloadids() ERROR Id: " << circleId;
	    std::cerr << " Not in mLoadingCache Map";
	    std::cerr << std::endl;

	    // ERROR
	    return false;
    }

    if(locked_processLoadingCacheEntry(it->second))
    {
	    /* remove from loading queue */
	    mLoadingCache.erase(it);
    }
    else
    {
#ifdef DEBUG_CIRCLES
	    std::cerr << "  Unprocessed peers. Requesting reload for circle " << circleId << std::endl;
#endif

	    /* schedule event to try reload gxsIds */
	    RsTickEvent::schedule_in(CIRCLE_EVENT_RELOADIDS, GXSID_LOAD_CYCLE, circleId.toStdString());
    }

    return true;
}
    
bool p3GxsCircles::checkCircleCache()
{
#ifdef DEBUG_CIRCLES
    std::cerr << "checkCircleCache(): calling auto-subscribe check and membership update check." << std::endl;
#endif
    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
    
   mCircleCache.applyToAllCachedEntries(*this,&p3GxsCircles::locked_checkCircleCacheForAutoSubscribe) ;
//    mCircleCache.applyToAllCachedEntries(*this,&p3GxsCircles::locked_checkCircleCacheForMembershipUpdate) ;
    
    return true ;
}

bool p3GxsCircles::locked_checkCircleCacheForMembershipUpdate(RsGxsCircleCache& cache)
{
	rstime_t now = time(NULL) ;

	if(cache.mLastUpdatedMembershipTS + GXS_CIRCLE_DELAY_TO_FORCE_MEMBERSHIP_UPDATE < now)
	{ 
#ifdef DEBUG_CIRCLES
		std::cerr << "Cache entry for circle " << cache.mCircleId << " needs a swab over membership requests. Re-scheduling it." << std::endl;
#endif

		// this should be called regularly

		uint32_t token ;
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		std::list<RsGxsGroupId> grpIds ;

		grpIds.push_back(RsGxsGroupId(cache.mCircleId)) ;

		RsGenExchange::getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY,	opts, grpIds);
		GxsTokenQueue::queueRequest(token, CIRCLEREQ_MESSAGE_DATA);	
	}
	return true ;
}

/* We need to AutoSubscribe if the Circle is relevent to us */

bool p3GxsCircles::locked_checkCircleCacheForAutoSubscribe(RsGxsCircleCache &cache)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::locked_checkCircleCacheForAutoSubscribe() : "<< cache.mCircleId << std::endl;
#endif

	/* if processed already - ignore */
	if (!(cache.mGroupStatus & GXS_SERV::GXS_GRP_STATUS_UNPROCESSED))
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "  Already Processed" << std::endl;
#endif

		return false;
	}

	/* if personal - we created ... is subscribed already */
	if (!cache.mIsExternal)
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "  Personal Circle. Nothing to do." << std::endl;
#endif

		return false;
	}

	/* if we appear in the group - then autosubscribe, and mark as processed. This also applies if we're the group admin */
        
	std::list<RsGxsId> myOwnIds;
                        
	if(!rsIdentity->getOwnIds(myOwnIds))
	{      
		std::cerr << "  own ids not loaded yet." << std::endl;

		/* schedule event to try reload gxsIds */
		RsTickEvent::schedule_in(CIRCLE_EVENT_RELOADIDS, GXSID_LOAD_CYCLE, cache.mCircleId.toStdString());
		return false ;
	}
                            
	bool in_admin_list = false ;
	bool member_request = false ;
	
	for(std::list<RsGxsId>::const_iterator it(myOwnIds.begin());it!=myOwnIds.end() && (!in_admin_list) && (!member_request);++it)
        {
            std::map<RsGxsId,RsGxsCircleMembershipStatus>::const_iterator it2 = cache.mMembershipStatus.find(*it) ;
            
            if(it2 != cache.mMembershipStatus.end())
            {
				in_admin_list = in_admin_list || bool(it2->second.subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST) ;
				member_request= member_request|| bool(it2->second.subscription_flags & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED) ;
            }
        }
                                
    	bool am_I_admin( cache.mGroupSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) ;
                                
#ifdef DEBUG_CIRCLES
	std::cerr << "  own ID in circle: " << in_admin_list << ", own subscribe request: " << member_request << ", am I admin?: " << am_I_admin << std::endl;
#endif
	if(in_admin_list || member_request || am_I_admin)
	{
		uint32_t token, token2;	
        
        	if(! (cache.mGroupSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED))
                {
#ifdef DEBUG_CIRCLES
		    /* we are part of this group - subscribe, clear unprocessed flag */
		    std::cerr << "  I'm allowed in this circle => AutoSubscribing!" << std::endl;
#endif
			RsGenExchange::subscribeToGroup(token, RsGxsGroupId(cache.mCircleId), true);
                }
#ifdef DEBUG_CIRCLES
                else
		    std::cerr << "  I'm allowed in this circle, and already subscribed." << std::endl;
#endif
                
		RsGenExchange::setGroupStatusFlags(token2, RsGxsGroupId(cache.mCircleId), 0, GXS_SERV::GXS_GRP_STATUS_UNPROCESSED);
        
		cache.mGroupStatus &= ~GXS_SERV::GXS_GRP_STATUS_UNPROCESSED;

        mCacheUpdated = true;
		
		return true;
	}
	else 
	{
		/* we know all the peers - we are not part - we can flag as PROCESSED. */
		uint32_t token,token2;	
		RsGenExchange::setGroupStatusFlags(token, RsGxsGroupId(cache.mCircleId.toStdString()), 0, GXS_SERV::GXS_GRP_STATUS_UNPROCESSED);
        
        	if(cache.mGroupSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
                {
			RsGenExchange::subscribeToGroup(token2, RsGxsGroupId(cache.mCircleId), false);
#ifdef DEBUG_CIRCLES
		std::cerr << "  Not part of the group! Let's unsubscribe this circle of unfriendly Napoleons!" << std::endl;
#endif
                }
#ifdef DEBUG_CIRCLES
                else
			std::cerr << "  Not part of the group, and not subscribed either." << std::endl;
#endif
        
		cache.mGroupStatus &= ~GXS_SERV::GXS_GRP_STATUS_UNPROCESSED;
        
        mCacheUpdated = true;
		return true ;
	}
}

void p3GxsCircles::addCircleIdToList(const RsGxsCircleId &circleId, uint32_t circleType)
{
    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

    if (circleType == GXS_CIRCLE_TYPE_LOCAL)
    {
        if(mCirclePersonalIdList.end() == std::find(mCirclePersonalIdList.begin(), mCirclePersonalIdList.end(), circleId)){
            mCirclePersonalIdList.push_back(circleId);
        }
    }
    else
    {
        if(mCircleExternalIdList.end() == std::find(mCircleExternalIdList.begin(), mCircleExternalIdList.end(), circleId)){
            mCircleExternalIdList.push_back(circleId);
        }
    }
}



#ifdef HANDLE_SUBCIRCLES
#if 0
/**** TODO BELOW ****/

bool p3GxsCircles::cache_load_subcircles(uint32_t token)
{
	std::cerr << "p3GxsCircles::cache_load_subcircles() : " << token;
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);

			RsGxsCircleId id = item->meta.mGroupId;
			RsGxsCircleGroup group = item->group;
			group.mMeta = item->meta;
			delete item;

			std::cerr << "p3GxsCircles::cache_load_subcircles() Loaded Id with Meta: ";
			std::cerr << item->meta;
			std::cerr << std::endl;


			RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

			/* stage 2 of loading, load subcircles */
			std::map<RsGxsCircleId, std::list<RsGxsCircleId> >::iterator sit;
			sit = mCacheLoad_SubCircle.find(id)
			if (sit == mCacheLoad_SubCircle.end())
			{
				/* ERROR */
				continue;
			}

			std::list<RsGxsCircleId> updateCaches = sit->second;
			// cleanup while we're here.
			mCacheLoad_SubCircle.erase(sit);

			/* Now iterate through peers / subcircles, and apply
			 * - similarly to base load function
			 */


			RsGxsCircleCache &cache = it->second;
			cache.loadBaseCircle(group);

			bool isComplete = true;

			std::list<RsGxsId> &peers = group.peers;
			std::list<RsGxsId>::const_iterator pit;

			// need to trigger the searches.
			for(pit = peers.begin(); pit != peers.end(); ++pit)
			{
				/* check cache */
				if (mIdentities->is_cached(*pit))
				{
					/* we can process now! */
					RsIdentityDetails details;
					if (mIdentities->getDetails(*pit, details))
					{
						if (details.isPgpKnown)
						{
							// Problem - could have multiple GxsIds here!
							// TODO.
							//cache.mAllowedPeers[details.mPgpId] = *pit;

							for(uit = updateCaches.begin(); uit != updateCaches.end(); ++uit)
							{
								/* fetch the cache - and update */
								mLoadingCache[id] = RsGxsCircleCache();
								std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
								it = mLoadingCache.find(id);
							}
							
						}
						else
						{
							//cache.mUnknownPeers.push_back(*pit);
						}
					}
					else
					{
						// ERROR.
					}
				}
				else
				{
					/* store in to_process queue. */
					cache.mUnprocessedPeers.push_back(*pit);

					if (isComplete)
					{
						/* store reference to update */
						isComplete = false;
						mCacheLoad_KeyWait.push_back(id);
					}
				}
			}

			std::list<RsGxsCircleId> &circles = group.circles;
			std::list<RsGxsCircleId>::const_iterator cit;
			for(cit = circles.begin(); cit != circles.end(); ++cit)
			{
				/* if its cached already -> then its complete. */
				if (mCircleCache.is_loaded(*cit))
				{
					RsGxsCircleCache cachedCircle;
					if (mCircleCache.fetch(&cit, cachedCircle))
					{
						/* copy cached circle into circle */
						cache.loadSubCircle(cachedCircle);
					}
					else
					{
						/* error */
						continue;
					}
				}
				else
				{
					/* push into secondary processing queues */
					std::list<RsGxsCircleId> &proc_circles = mCacheLoad_SubCircle[id];
					proc_circles.push_back(id);

					subCirclesToLoad.push_back(id);

					isComplete = false;
				}
			}

			if (isComplete)
			{
				/* move straight into the cache */
				mCircleCache.store(id, cache);

				/* remove from loading queue */
				mLoadingCache.erase(it);
			}
		}
	}
	else
	{
		std::cerr << "p3GxsCircles::cache_load_for_token() ERROR no data";
		std::cerr << std::endl;

		return false;
	}

	if (!keysToLoad.empty())
	{
		/* schedule event to try reload gxsIds */

	}

	if (!subCirclesToLoad.empty())
	{
		/* request load of subcircles */


	}
	return true;
}

#endif
#endif


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

std::string p3GxsCircles::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
void p3GxsCircles::generateDummyData()
{
	// request Id Data...
#ifdef DEBUG_CIRCLES
        std::cerr << "p3GxsCircles::generateDummyData() getting Id List";
        std::cerr << std::endl;
#endif // DEBUG_CIRCLES

        uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	rsIdentity->getTokenService()->requestGroupInfo(token, ansType, opts);

	{
		RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
        	mDummyIdToken = token;
	}

	RsTickEvent::schedule_in(CIRCLE_EVENT_DUMMYLOAD, CIRCLE_DUMMY_GENPERIOD);
}


void p3GxsCircles::checkDummyIdData()
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::checkDummyIdData()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	// check the token.
        uint32_t status =  rsIdentity->getTokenService()->requestStatus(mDummyIdToken);
        if ( (RsTokenService::FAILED == status) ||
                         (RsTokenService::COMPLETE == status) )
	{
		std::vector<RsGxsIdGroup> ids;
		if (!rsIdentity->getGroupData(mDummyIdToken, ids))
		{
			std::cerr << "p3GxsCircles::checkDummyIdData() ERROR getting data";
			std::cerr << std::endl;
			/* error */
			return;
		}

		std::vector<RsGxsIdGroup>::iterator it;
		for(it = ids.begin(); it != ids.end(); ++it)
		{
                        if (it->mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::checkDummyIdData() PgpLinkedId: " << it->mMeta.mGroupId;
				std::cerr << std::endl;
#endif // DEBUG_CIRCLES
				mDummyPgpLinkedIds.push_back(RsGxsId(it->mMeta.mGroupId.toStdString()));

				if (it->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
				{
#ifdef DEBUG_CIRCLES
					std::cerr << "p3GxsCircles::checkDummyIdData() OwnId: " << it->mMeta.mGroupId;
					std::cerr << std::endl;
#endif // DEBUG_CIRCLES
					mDummyOwnIds.push_back(RsGxsId(it->mMeta.mGroupId.toStdString()));
				}
			}
			else
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::checkDummyIdData() Other Id: " << it->mMeta.mGroupId;
				std::cerr << std::endl;
#endif // DEBUG_CIRCLES
			}
		}
			
		/* schedule the generate events */
#define MAX_CIRCLES 10
		for(int i = 0; i < MAX_CIRCLES; i++)
		{
			RsTickEvent::schedule_in(CIRCLE_EVENT_DUMMYGEN, i * CIRCLE_DUMMY_GENPERIOD);
		}
		return;
	}

	// Otherwise - reschedule to come back here.
	RsTickEvent::schedule_in(CIRCLE_EVENT_DUMMYLOAD, CIRCLE_DUMMY_GENPERIOD);
        return;
}


void p3GxsCircles::generateDummyCircle()
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::generateDummyCircle()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	int npgps = mDummyPgpLinkedIds.size();

	if(npgps == 0)
		return ;

	RsGxsCircleGroup group;

	std::set<RsGxsId> idset;
	// select a random number of them.
#define MAX_PEERS_PER_CIRCLE_GROUP	20
	int nIds = 1 + (RSRandom::random_u32() % MAX_PEERS_PER_CIRCLE_GROUP);
	for(int i = 0; i < nIds; i++)
	{

		int selection = (RSRandom::random_u32() % npgps);
		std::list<RsGxsId>::iterator it = mDummyPgpLinkedIds.begin();
		for(int j = 0; (it != mDummyPgpLinkedIds.end()) && (j < selection); j++, ++it) ;
		if (it != mDummyPgpLinkedIds.end())
		{
			idset.insert(*it);
		}
	}

	/* be sure to add one of our IDs too (otherwise we wouldn't get the group)
	 */
	{

		int selection = (RSRandom::random_u32() % mDummyOwnIds.size());
		std::list<RsGxsId>::iterator it = mDummyOwnIds.begin();
					mDummyOwnIds.push_back(*it);
		for(int j = 0; (it != mDummyOwnIds.end()) && (j < selection); j++, ++it) ;
		if (it != mDummyOwnIds.end())
		{
			idset.insert(*it);
		}
	}

	group.mMeta.mGroupName = genRandomId();
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::generateDummyCircle() Name: " << group.mMeta.mGroupName;
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	std::set<RsGxsId>::iterator it;
	for(it = idset.begin(); it != idset.end(); ++it)
	{
        group.mInvitedMembers.insert(*it);
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::generateDummyCircle() Adding: " << *it;
		std::cerr << std::endl;
#endif // DEBUG_CIRCLES
	}

	uint32_t dummyToken;
	createGroup(dummyToken, group);
}

	// Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsCircles::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::handleResponse(" << token << "," << req_type << ")";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	// stuff.
	switch(req_type)
	{
		case CIRCLEREQ_CIRCLE_LIST:
			load_CircleIdList(token);
			break;

		case CIRCLEREQ_MESSAGE_DATA:
			processMembershipRequests(token);
			break;

		case CIRCLEREQ_CACHELOAD:
			cache_load_for_token(token);
			break;
	default:
		RsErr() << __PRETTY_FUNCTION__ << " Unknown Request Type: "
		        << req_type << std::endl;
		break;
	}
}

	// Overloaded from RsTickEvent for Event callbacks.
void p3GxsCircles::handle_event(uint32_t event_type, const std::string &elabel)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::handle_event(" << event_type << ")";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	// stuff.
	switch(event_type)
	{
		case CIRCLE_EVENT_LOADIDS:
			request_CircleIdList();
			break;

		case CIRCLE_EVENT_CACHELOAD:
			cache_start_load();
			break;

		case CIRCLE_EVENT_RELOADIDS:
			cache_reloadids(RsGxsCircleId(elabel));
			break;

	case CIRCLE_EVENT_DUMMYSTART:
			generateDummyData();
			break;

		case CIRCLE_EVENT_DUMMYLOAD:
			checkDummyIdData();
			break;

		case CIRCLE_EVENT_DUMMYGEN:
			generateDummyCircle();
			break;

	default:
		RsErr() << __PRETTY_FUNCTION__ << " Unknown Event Type: " << event_type
		        << std::endl;
		break;
	}
}

// Circle membership is requested/denied by posting a message into the cicle group, according to the following rules:
// 
//	- a subscription request is a RsItem (which serialises into a radix64 message, that is further signed by the group message publishing system)
//	  The item contains:
//		* subscribe order (yes/no), boolean
//		* circle ID (this is important, otherwise people can copy subscribe messages from one circle to another)
//		* subscribe date
//		* subscribe timeout (how long is the message kept. When timed out, the message is removed and subscription cancelled)
//
//	- subscribe messages follow the following rules, which are enforced by a timer-based method:
//		* subscription requests from a given user are always replaced by the last subscription request
//		* a complete list of who's subscribed to a given group is kept, saved, and regularly updated when new subscribe messages are received, or when admin list is changed.
//		* getGroupDetails reads this list in order to respond who's subscribed to a group. The list of 
//
//	- compatibility with self-restricted circles:
//		* subscription should be based on admin list, so that non subscribed peers still receive the invitation
//
//	- two possible subscription models for circle member list (Restricted forums only propagate to members):
//		1 - list of admin who have not opposed subscription
//			- solves propagation issue. Only admin see data. They can however unsubscribe using a negative req. Admin needs to remove them.
//			- bad for security. Admin can refuse to remove them => back to square one
//		2 - list of admin who have also requested membership
//			- propagation is ok. On restricted circle, the circle msgs/group should be sent to admin list, instead of member list.
//			- solves membership issue since people need to actively be in the group.
//          => choose 2
//			- forum  group : encrypted for Member list
//			- circle group : clear / encrypted for admin list (for self-restricted)
//		We decide between the two by comparing the group we're sending and the circle id it is restricted to.
//
//	- Use cases
//		* user sees group (not self restricted) and requests to subscribe => RS subscribes the group and the user can propagate the response
//		* user is invited to self-restricted circle. He will see it and can subscribe, so he will be in admin list and receive e.g. forum posts.
//		* 
//
//	- Threat model
//		* a malicious user forges a new subscription request: NP-hard as it needs to break the RSA key of the GXS id.
//		* a malicious corrupts a subscription request: NP-hard. Messages are signed.
//		* a malicious user copies an old subscription of someone else and inserts it in the system.
//			=> not possible. Either this existing old susbscription already exists, or it has been replaced by a more recent one, which
//			   will always replace the old one because of the date.
//		* a malicious user removes someone's subscription messages. This is possible, but the mesh nature of the network will allow the message to propagate anyway.
//		* a malicious user creates a circle with an incriminating name/content and adds everyone in it
//			=> people can oppose their membership in the circle using a msg
//
//
//	- the table below summarizes the various choices: forum and circle propagation when restricted to a circle, and group subscribe to the circle
//
//                                  +------------------------------+-----------------------------+
//                                  |   User in admin list         |   User not in admin list    |
//                    +-------------+------------------------------+-----------------------------+
//                    | User request|   Forum  Grp/Msg: YES        |   Forum  Grp/Msg: NO        |         
//                    | Subscription|   Circle Grp/Msg: YES/YES    |   Circle Grp/Msg: YES/NO    |         
//                    |             |   Grp Subscribed: YES        |   Grp Subscribed: YES       |
//                    +-------------+------------------------------+-----------------------------+
//                    | No request  |   Forum  Grp/Msg: NO         |   Forum  Grp/Msg: NO        |         
//                    | Subscription|   Circle Grp/Msg: YES/YES    |   Circle Grp/Msg: YES/NO    |         
//                    |             |   Grp Subscribed: NO         |   Grp Subscribed: NO        |         
//                    +-------------+------------------------------+-----------------------------+

bool p3GxsCircles::pushCircleMembershipRequest(
        const RsGxsId& own_gxsid, const RsGxsCircleId& circle_id,
        uint32_t request_type )
{
	Dbg3() << __PRETTY_FUNCTION__ << "own_gxsid = " << own_gxsid
	       << ", circle=" << circle_id << ", req type=" << request_type
	       << std::endl;

	if( request_type !=
	        RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_SUBSCRIBE &&
	    request_type !=
	        RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_UNSUBSCRIBE )
	{
		RsErr() << __PRETTY_FUNCTION__ << " Unknown request type: "
		        << request_type << std::endl;
		return false;
	}

	if(!rsIdentity->isOwnId(own_gxsid))
	{
		RsErr() << __PRETTY_FUNCTION__ << " Cannot generate membership request "
		        << "from not-own id: " << own_gxsid << std::endl;
		return false;
	}

	if(!getCirclesInfo(
	            std::list<RsGxsGroupId>{static_cast<RsGxsGroupId>(circle_id)},
	            RS_DEFAULT_STORAGE_PARAM(std::vector<RsGxsCircleGroup>) ))
	{
		RsErr() << __PRETTY_FUNCTION__ << " Cannot generate membership request "
		        << "from unknown circle: " << circle_id << std::endl;
		return false;
	}

	// Create a subscribe item

    RsGxsCircleSubscriptionRequestItem *s = new RsGxsCircleSubscriptionRequestItem ;

    s->time_stamp           = time(NULL) ;
    s->time_out             = 0 ;	// means never
    s->subscription_type    = request_type ;

    RsTemporaryMemory tmpmem(circle_id.serial_size() + own_gxsid.serial_size()) ;
    
    uint32_t off = 0 ;
    circle_id.serialise(tmpmem,tmpmem.size(),off) ;
    own_gxsid.serialise(tmpmem,tmpmem.size(),off) ;
    
    s->meta.mGroupId = RsGxsGroupId(circle_id) ;
    s->meta.mMsgId.clear();
    s->meta.mThreadId = RsDirUtil::sha1sum(tmpmem,tmpmem.size()); // make the ID from the hash of the cirle ID and the author ID
    s->meta.mAuthorId = own_gxsid;

    // msgItem->meta.mParentId = ; // leave these blank
    // msgItem->meta.mOrigMsgId= ; 

#ifdef DEBUG_CIRCLES
    std::cerr << "p3GxsCircles::publishSubscribeRequest()" << std::endl;
    std::cerr << "  GroupId    : " << circle_id << std::endl;
    std::cerr << "  AuthorId   : " << s->meta.mAuthorId << std::endl;
    std::cerr << "  ThreadId   : " << s->meta.mThreadId << std::endl;
#endif
    uint32_t token ;
    
    if(request_type == RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_SUBSCRIBE)
	    RsGenExchange::subscribeToGroup(token, RsGxsGroupId(circle_id), true);
    
    RsGenExchange::publishMsg(token, s);
    
    // update the cache.
    force_cache_reload(circle_id);
    
    return true;
}

bool p3GxsCircles::requestCircleMembership(const RsGxsId& own_gxsid,const RsGxsCircleId& circle_id) 
{
    return pushCircleMembershipRequest(own_gxsid,circle_id,RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_SUBSCRIBE) ;
}
bool p3GxsCircles::cancelCircleMembership(const RsGxsId& own_gxsid,const RsGxsCircleId& circle_id) 
{
    return pushCircleMembershipRequest(own_gxsid,circle_id,RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_UNSUBSCRIBE) ;
}


bool p3GxsCircles::processMembershipRequests(uint32_t token)
{
    // Go through membership request messages and process them according to the following rule:
    //	* for each ID only keep the latest membership request. Delete the older ones.
    //	* for each circle, keep a list of IDs sorted into membership categories (e.g. keep updated flags for each IDs)
    // Because msg loading is async-ed, the job in split in two methods: one calls the loading, the other one handles the loaded data.
    
#ifdef DEBUG_CIRCLES
    std::cerr << "Processing circle membership requests." << std::endl;
#endif
    t_RsGxsGenericDataTemporaryMapVector<RsGxsMsgItem> msgItems;

    if(!RsGenExchange::getMsgData(token, msgItems))
    {
	    std::cerr << "(EE) Cannot get msg data for circle. Something's weird." << std::endl;
	    return false;
    }
    
    GxsMsgReq messages_to_delete ;

    for(GxsMsgDataMap::const_iterator it(msgItems.begin());it!=msgItems.end();++it)
    {
	    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
#ifdef DEBUG_CIRCLES
	    std::cerr << "  Circle ID: " << it->first << std::endl;
#endif

	    RsGxsCircleId cid ( it->first );

	    if (!mCircleCache.is_cached(cid))
	    {
		    std::cerr << "    (EE) Circle is not in cache!" << std::endl;
		    continue ;
	    }
            
            // Find the circle ID in cache and process the list of messages to keep the latest order in time.

	    RsGxsCircleCache& data = mCircleCache.ref(cid);
#ifdef DEBUG_CIRCLES
	    std::cerr << "    Circle found in cache!" << std::endl;
	    std::cerr << "    Retrieving messages..." << std::endl;
#endif

            for(uint32_t i=0;i<it->second.size();++i)
            {
#ifdef DEBUG_CIRCLES
	    	std::cerr << "      Group ID: " << it->second[i]->meta.mGroupId << ", Message ID: " << it->second[i]->meta.mMsgId << ": " ;
#endif
            
                RsGxsCircleSubscriptionRequestItem *item = dynamic_cast<RsGxsCircleSubscriptionRequestItem*>(it->second[i]) ;
                
                if(item == NULL)
		{
                    std::cerr << "    (EE) item is not a RsGxsCircleSubscriptionRequestItem. Weird." << std::endl;
                    continue ;
                }
                    
                RsGxsCircleMembershipStatus& info(data.mMembershipStatus[item->meta.mAuthorId]) ;
                
#ifdef DEBUG_CIRCLES
		std::cerr << " is from id " << item->meta.mAuthorId << "  " << time(NULL) - item->time_stamp << " seconds ago, " ;
#endif
                
                if(info.last_subscription_TS < item->time_stamp)
                {
                    info.last_subscription_TS = item->time_stamp ;
                    
                    if(item->subscription_type == RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_SUBSCRIBE)
                    	info.subscription_flags |= GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED;    
                    else if(item->subscription_type == RsGxsCircleSubscriptionRequestItem::SUBSCRIPTION_REQUEST_UNSUBSCRIBE)
                    	info.subscription_flags &= ~GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED;    
                    else
                    	std::cerr << " (EE) unknown subscription order type: " << item->subscription_type ;
                    
					mCacheUpdated = true;
#ifdef DEBUG_CIRCLES
                    std::cerr << " UPDATING" << std::endl;
#endif
                }
                else if(info.last_subscription_TS > item->time_stamp)
                {
#ifdef DEBUG_CIRCLES
                    std::cerr << " Older than last known (" << time(NULL)-info.last_subscription_TS << " seconds ago): deleting." << std::endl;
#endif
                    messages_to_delete[RsGxsGroupId(cid)].insert(it->second[i]->meta.mMsgId) ;
                }
            }
            
            data.mLastUpdatedMembershipTS = time(NULL) ;
			mCacheUpdated = true;
	}
    
    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
    uint32_t token2;
    RsGenExchange::deleteMsgs(token2,messages_to_delete);
    
    return true ;
}

/*static*/ const std::string RsGxsCircles::DEFAULT_CIRCLE_BASE_URL =
        "retroshare:///circles";
/*static*/ const std::string RsGxsCircles::CIRCLE_URL_NAME_FIELD = "circleName";
/*static*/ const std::string RsGxsCircles::CIRCLE_URL_ID_FIELD = "circleId";
/*static*/ const std::string RsGxsCircles::CIRCLE_URL_DATA_FIELD = "circleData";

RsGxsCircles::~RsGxsCircles() = default;
RsGxsCircleMsg::~RsGxsCircleMsg() = default;
RsGxsCircleDetails::~RsGxsCircleDetails() = default;
RsGxsCircleGroup::~RsGxsCircleGroup() = default;
RsGxsCircleEvent::~RsGxsCircleEvent() = default;
