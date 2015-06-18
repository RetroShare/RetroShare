/*
 * libretroshare/src/services p3gxscircles.cc
 *
 * Circles Interface for RetroShare.
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

#include "services/p3gxscircles.h"
#include "serialiser/rsgxscircleitems.h"
#include "retroshare/rsgxsflags.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"

#include "pgp/pgpauxutils.h"

#include <sstream>
#include <stdio.h>

/****
 * #define DEBUG_CIRCLES	 1
 ****/

RsGxsCircles *rsGxsCircles = NULL;

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


#define CIRCLEREQ_CACHELOAD	0x0001
#define CIRCLEREQ_CIRCLE_LIST   0x0002

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

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsCircles::p3GxsCircles(RsGeneralDataService *gds, RsNetworkExchangeService *nes, 
	p3IdService *identities, PgpAuxUtils *pgpUtils)
	: RsGxsCircleExchange(gds, nes, new RsGxsCircleSerialiser(), 
			RS_SERVICE_GXS_TYPE_GXSCIRCLE, identities, circleAuthenPolicy()), 
	RsGxsCircles(this), GxsTokenQueue(this), RsTickEvent(), 
	mIdentities(identities), 
	mPgpUtils(pgpUtils),
	mCircleMtx("p3GxsCircles"),
        mCircleCache(DEFAULT_MEM_CACHE_SIZE, "GxsCircleCache")

{
	// Kick off Cache Testing, + Others.
	//RsTickEvent::schedule_in(CIRCLE_EVENT_CACHETEST, CACHETEST_PERIOD);

	RsTickEvent::schedule_now(CIRCLE_EVENT_LOADIDS);

	// Dummy Circles.
//	RsTickEvent::schedule_in(CIRCLE_EVENT_DUMMYSTART, CIRCLE_DUMMY_STARTPERIOD);
    mDummyIdToken = 0;

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



uint32_t p3GxsCircles::circleAuthenPolicy()
{

	uint32_t policy = 0;
	uint8_t flag = 0;


	//flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;
	//flag = GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	//flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
	//flag = GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	//flag = GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}


void	p3GxsCircles::service_tick()
{
	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests(); // GxsTokenQueue handles all requests.
	return;
}

void p3GxsCircles::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::notifyChanges()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); ++it)
	{
	       RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(*it);
	       RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
	       if (msgChange && !msgChange->metaChange())
	       {
#ifdef DEBUG_CIRCLES
			std::cerr << "p3GxsCircles::notifyChanges() Found Message Change Notification";
			std::cerr << std::endl;
#endif

			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
			for(mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::notifyChanges() Msgs for Group: " << mit->first;
				std::cerr << std::endl;
#endif
			}
	       }

	       /* add groups to ExternalIdList (Might get Personal Circles here until NetChecks in place) */
		if (groupChange && !groupChange->metaChange())
		{
#ifdef DEBUG_CIRCLES
			std::cerr << "p3GxsCircles::notifyChanges() Found Group Change Notification";
			std::cerr << std::endl;
#endif

			std::list<RsGxsGroupId> &groupList = groupChange->mGrpIdList;
			std::list<RsGxsGroupId>::iterator git;
			for(git = groupList.begin(); git != groupList.end(); ++git)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::notifyChanges() Incoming Group: " << *git;
				std::cerr << std::endl;
#endif

				// for new circles we need to add them to the list.
                // we don't know the type of this circle here
                // original behavior was to add all ids to the external ids list
                addCircleIdToList(RsGxsCircleId(*git), 0);

                // reset the cached circle data for this id
                {
                    RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
                    mCircleCache.erase(RsGxsCircleId(*git));
                }
			}
		}
	}
	RsGxsIfaceHelper::receiveChanges(changes);


}

/********************************************************************************/
/******************* RsCircles Interface  ***************************************/
/********************************************************************************/

bool p3GxsCircles:: getCircleDetails(const RsGxsCircleId &id, RsGxsCircleDetails &details)
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
			details.mIsExternal = data.mIsExternal;

			details.mUnknownPeers = data.mUnknownPeers;
			details.mAllowedPeers = data.mAllowedPeers;
			return true;
		}
	}

	/* it isn't there - add to public requests */
	cache_request_load(id);

	return false;
}


bool p3GxsCircles:: getCirclePersonalIdList(std::list<RsGxsCircleId> &circleIds)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::getCircleIdList()";
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	if (circleIds.empty())
	{
		circleIds = mCirclePersonalIdList;
	}
	else
	{
		std::list<RsGxsCircleId>::const_iterator it;
		for(it = mCirclePersonalIdList.begin(); it != mCirclePersonalIdList.begin(); ++it)
		{
			circleIds.push_back(*it);
		}
	}

	return true;
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

int p3GxsCircles::canSend(const RsGxsCircleId &circleId, const RsPgpId &id)
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

bool p3GxsCircles::recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist)
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

	return; 
}


bool RsGxsCircleCache::loadBaseCircle(const RsGxsCircleGroup &circle)
{

	mCircleId = RsGxsCircleId(circle.mMeta.mGroupId);
	mCircleName = circle.mMeta.mGroupName;
	mUpdateTime = time(NULL);
	mProcessedCircles.insert(mCircleId);

	mCircleType = circle.mMeta.mCircleType;
	mIsExternal = (mCircleType != GXS_CIRCLE_TYPE_LOCAL);
	mGroupStatus = circle.mMeta.mGroupStatus;

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

bool RsGxsCircleCache::getAllowedPeersList(std::list<RsPgpId> &friendlist)
{
	std::map<RsPgpId, std::list<RsGxsId> >::iterator it;
	for(it = mAllowedPeers.begin(); it != mAllowedPeers.end(); ++it)
	{
		friendlist.push_back(it->first);
	}
	return true;
}

bool RsGxsCircleCache::isAllowedPeer(const RsPgpId &id)
{
	std::map<RsPgpId, std::list<RsGxsId> >::iterator it = mAllowedPeers.find(id);
	if (it != mAllowedPeers.end())
	{
		return true;
	}
	return false;
}

bool RsGxsCircleCache::addAllowedPeer(const RsPgpId &pgpId, const RsGxsId &gxsId)
{
	/* created if doesn't exist */
	std::list<RsGxsId> &gxsList = mAllowedPeers[pgpId];
	gxsList.push_back(gxsId);
	return true;
}


bool RsGxsCircleCache::addLocalFriend(const RsPgpId &pgpId)
{
	/* empty list as no GxsID associated */
	std::list<RsGxsId> &gxsList = mAllowedPeers[pgpId];
	return true;
}


/************************************************************************************/
/************************************************************************************/

bool p3GxsCircles::request_CircleIdList()
{
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
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
	return 1;
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
			RsTickEvent::schedule_in(CIRCLE_EVENT_CACHELOAD, 
						MIN_CIRCLE_LOAD_GAP - age);
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
	std::cerr << "p3GxsCircles::cache_load_for_token() : " << token;
	std::cerr << std::endl;
#endif // DEBUG_CIRCLES

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsCircleGroupItem *item = dynamic_cast<RsGxsCircleGroupItem*>(*vit);
			if (!item)
			{
				std::cerr << "Not a RsGxsCircleGroupItem Item, deleting!" << std::endl;
				delete(*vit);
				continue;
			}
			RsGxsCircleGroup group;
			item->convertTo(group);

#ifdef DEBUG_CIRCLES
			std::cerr << "p3GxsCircles::cache_load_for_token() Loaded Id with Meta: ";
			std::cerr << item->meta;
			std::cerr << std::endl;
#endif // DEBUG_CIRCLES


			RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

			/* should already have a LoadingCache entry */
			RsGxsCircleId id = RsGxsCircleId(item->meta.mGroupId.toStdString());
			std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
			it = mLoadingCache.find(id);
			if (it == mLoadingCache.end())
			{
				std::cerr << "p3GxsCircles::cache_load_for_token() Load ERROR: ";
				std::cerr << item->meta;
				std::cerr << std::endl;
				delete(item);
				// ERROR.
				continue;
			}

			RsGxsCircleCache &cache = it->second;
			cache.loadBaseCircle(group);
			delete item;


			bool isComplete = true;
			bool isUnprocessedPeers = false;


			if (cache.mIsExternal)
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::cache_load_for_token() Loading External Circle";
				std::cerr << std::endl;
#endif

                std::set<RsGxsId> &peers = group.mInvitedMembers;
                std::set<RsGxsId>::const_iterator pit;
	
				// need to trigger the searches.
				for(pit = peers.begin(); pit != peers.end(); ++pit)
				{
#ifdef DEBUG_CIRCLES
					std::cerr << "p3GxsCircles::cache_load_for_token() Invited Member: " << *pit;
					std::cerr << std::endl;
#endif

					/* check cache */
					if (mIdentities->haveKey(*pit))
					{
						/* we can process now! */
						RsIdentityDetails details;
						if (mIdentities->getIdDetails(*pit, details))
						{
							if (details.mPgpLinked && details.mPgpKnown)
							{
#ifdef DEBUG_CIRCLES
								std::cerr << "p3GxsCircles::cache_load_for_token() Is Known -> AllowedPeer: " << *pit;
								std::cerr << std::endl;
#endif
								cache.addAllowedPeer(details.mPgpId, *pit);
							}
							else
							{
#ifdef DEBUG_CIRCLES
								std::cerr << "p3GxsCircles::cache_load_for_token() Is Unknown -> UnknownPeer: " << *pit;
								std::cerr << std::endl;
#endif
								cache.mUnknownPeers.insert(*pit);
							}
						}
						else
						{
							std::cerr << "p3GxsCircles::cache_load_for_token() ERROR no details: " << *pit;
							std::cerr << std::endl;
							// ERROR.
						}
					}
					else
					{
#ifdef DEBUG_CIRCLES

						std::cerr << "p3GxsCircles::cache_load_for_token() Requesting UnprocessedPeer: " << *pit;
						std::cerr << std::endl;
#endif

						std::list<PeerId> peers;
						mIdentities->requestKey(*pit, peers);

						/* store in to_process queue. */
						cache.mUnprocessedPeers.insert(*pit);
	
						isComplete = false;
						isUnprocessedPeers = true;
					}
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
			else
			{
#ifdef DEBUG_CIRCLES
				std::cerr << "p3GxsCircles::cache_load_for_token() Loading Personal Circle";
				std::cerr << std::endl;
#endif

				// LOCAL Load.
                std::set<RsPgpId> &peers = group.mLocalFriends;
                std::set<RsPgpId>::const_iterator pit;
	
				// need to trigger the searches.
				for(pit = peers.begin(); pit != peers.end(); ++pit)
				{
#ifdef DEBUG_CIRCLES
					std::cerr << "p3GxsCircles::cache_load_for_token() Local Friend: " << *pit;
					std::cerr << std::endl;
#endif

					cache.addLocalFriend(*pit);
				}
				isComplete = true;
				isUnprocessedPeers = false;
			}



			if (isComplete)
			{
				checkCircleCacheForAutoSubscribe(cache);

				/* move straight into the cache */
				mCircleCache.store(id, cache);
				mCircleCache.resize();

				/* remove from loading queue */
				mLoadingCache.erase(it);
			}

			if (isUnprocessedPeers)
			{
				/* schedule event to try reload gxsIds */
				RsTickEvent::schedule_in(CIRCLE_EVENT_RELOADIDS, GXSID_LOAD_CYCLE, id.toStdString());
			}
		}
	}
	else
	{
		std::cerr << "p3GxsCircles::cache_load_for_token() ERROR no data";
		std::cerr << std::endl;

		return false;
	}

#ifdef HANDLE_SUBCIRCLES
#if 0
	if (!subCirclesToLoad.empty())
	{
		/* request load of subcircles */


	}
#endif
#endif
	return true;
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

	RsGxsCircleCache &cache = it->second;

	/* try reload Ids */
	std::set<RsGxsId>::const_iterator pit;
	for(pit = cache.mUnprocessedPeers.begin();
		pit != cache.mUnprocessedPeers.end(); ++pit)
	{
		/* check cache */
		if (mIdentities->haveKey(*pit))
		{
			/* we can process now! */
			RsIdentityDetails details;
			if (mIdentities->getIdDetails(*pit, details))
			{
				if (details.mPgpLinked && details.mPgpKnown)
				{
					cache.addAllowedPeer(details.mPgpId, *pit);

#ifdef DEBUG_CIRCLES
					std::cerr << "p3GxsCircles::cache_reloadids() AllowedPeer: ";
					std::cerr << *pit;
					std::cerr << std::endl;
#endif // DEBUG_CIRCLES
				}
				else
				{
					cache.mUnknownPeers.insert(*pit);

#ifdef DEBUG_CIRCLES
					std::cerr << "p3GxsCircles::cache_reloadids() UnknownPeer: ";
					std::cerr << *pit;
					std::cerr << std::endl;
#endif // DEBUG_CIRCLES
				}
			}
			else
			{
				// ERROR.
				std::cerr << "p3GxsCircles::cache_reloadids() ERROR ";
				std::cerr << " Should haveKey for Id: " << *pit;
				std::cerr << std::endl;
			}
		}
		else
		{
			// UNKNOWN ID.
			std::cerr << "p3GxsCircles::cache_reloadids() UNKNOWN Id: ";
			std::cerr << *pit;
			std::cerr << std::endl;
		}
	}

	// clear unprocessed List.
	cache.mUnprocessedPeers.clear();

	// If sub-circles are complete too.
	if (cache.mUnprocessedCircles.empty())
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::cache_reloadids() Adding to cache Id: ";
		std::cerr << circleId;
		std::cerr << std::endl;
#endif // DEBUG_CIRCLES

		checkCircleCacheForAutoSubscribe(cache);

		// Push to Cache.
		mCircleCache.store(circleId, cache);
		mCircleCache.resize();

		/* remove from loading queue */
		mLoadingCache.erase(it);
	}
	else
	{
		std::cerr << "p3GxsCircles::cache_reloadids() WARNING Incomplete Cache Loading: ";
		std::cerr << circleId;
		std::cerr << std::endl;
	}

	return true;
}


/* We need to AutoSubscribe if the Circle is relevent to us */
bool p3GxsCircles::checkCircleCacheForAutoSubscribe(RsGxsCircleCache &cache)
{
#ifdef DEBUG_CIRCLES
	std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() : ";
	std::cerr << cache.mCircleId;
	std::cerr << std::endl;
#endif

	/* if processed already - ignore */
	if (!(cache.mGroupStatus & GXS_SERV::GXS_GRP_STATUS_UNPROCESSED))
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() : Already Processed";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* if personal - we created ... is subscribed already */
	if (!cache.mIsExternal)
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() : Personal Circle";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* if we appear in the group - then autosubscribe, and mark as processed */
	const RsPgpId& ownId = mPgpUtils->getPGPOwnId();
	std::map<RsPgpId, std::list<RsGxsId> >::iterator it = cache.mAllowedPeers.find(ownId);
	if (it != cache.mAllowedPeers.end())
	{
#ifdef DEBUG_CIRCLES
		/* we are part of this group - subscribe, clear unprocessed flag */
		std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() Found OwnId -> AutoSubscribe!";
		std::cerr << std::endl;
#endif

	
		uint32_t token, token2;	
		RsGenExchange::subscribeToGroup(token, RsGxsGroupId(cache.mCircleId.toStdString()), true);
		RsGenExchange::setGroupStatusFlags(token2, RsGxsGroupId(cache.mCircleId.toStdString()), 0, GXS_SERV::GXS_GRP_STATUS_UNPROCESSED);
		cache.mGroupStatus ^= GXS_SERV::GXS_GRP_STATUS_UNPROCESSED;
		return true;
	}
	else if (cache.mUnknownPeers.empty())
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() Know All Peers -> Processed";
		std::cerr << std::endl;
#endif

		/* we know all the peers - we are not part - we can flag as PROCESSED. */
		uint32_t token;	
		RsGenExchange::setGroupStatusFlags(token, RsGxsGroupId(cache.mCircleId.toStdString()), 0, GXS_SERV::GXS_GRP_STATUS_UNPROCESSED);
		cache.mGroupStatus ^= GXS_SERV::GXS_GRP_STATUS_UNPROCESSED;
	}
	else
	{
#ifdef DEBUG_CIRCLES
		std::cerr << "p3GxsCircles::checkCircleCacheForAutoSubscribe() Leaving Unprocessed";
		std::cerr << std::endl;
#endif

		// Don't clear UNPROCESSED - as we might not know all the peers.
		// TODO - work out when we flag as PROCESSED.
	}
	return false;
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
        if ( (RsTokenService::GXS_REQUEST_V2_STATUS_FAILED == status) ||
                         (RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE == status) )
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
                        if (it->mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
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


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


std::ostream &operator<<(std::ostream &out, const RsGxsCircleGroup &grp)
{
	out << "RsGxsCircleGroup: Meta: " << grp.mMeta;
	out << "InvitedMembers: ";
	out << std::endl;

        std::set<RsGxsId>::const_iterator it;
        std::set<RsGxsCircleId>::const_iterator sit;
	for(it = grp.mInvitedMembers.begin();
		it != grp.mInvitedMembers.begin(); ++it)
	{
		out << "\t" << *it;
		out << std::endl;
	}

	for(sit = grp.mSubCircles.begin();
		sit != grp.mSubCircles.begin(); ++sit)
	{
		out << "\t" << *it;
		out << std::endl;
	}
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsGxsCircleMsg &msg)
{
	out << "RsGxsCircleMsg: Meta: " << msg.mMeta;
	out << std::endl;
	
	return out;
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

		case CIRCLEREQ_CACHELOAD:
			cache_load_for_token(token);
			break;

#if 0
		case CIRCLEREQ_CACHETEST:
			cachetest_handlerequest(token);
			break;
#endif

		default:
			/* error */
			std::cerr << "p3GxsCircles::handleResponse() Unknown Request Type: " << req_type;
			std::cerr << std::endl;
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

#if 0
		case CIRCLE_EVENT_CACHETEST:
			cachetest_getlist();
			break;
#endif


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
			/* error */
			std::cerr << "p3GxsCircles::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}



