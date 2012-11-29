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
#include "gxs/rsgxsflags.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"

#include "pqi/authgpg.h"

#include <retroshare/rspeers.h>

#include <sstream>
#include <stdio.h>

/****
 * #define ID_DEBUG 1
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
#define CIRCLEREQ_CACHEOWNIDS	0x0002

#define CIRCLEREQ_PGPHASH 	0x0010
#define CIRCLEREQ_REPUTATION 	0x0020

#define CIRCLEREQ_CACHETEST 	0x1000

// Events.
#define CIRCLE_EVENT_CACHEOWNIDS	0x0001
#define CIRCLE_EVENT_CACHELOAD 		0x0002
#define CIRCLE_EVENT_RELOADIDS 		0x0003


#define CIRCLE_EVENT_CACHETEST 		0x1000
#define CACHETEST_PERIOD	60
#define OWNID_RELOAD_DELAY		10

#define GXSID_LOAD_CYCLE		10	// GXSID completes a load in this period.

#define MIN_CIRCLE_LOAD_GAP		5

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsCircles::p3GxsCircles(RsGeneralDataService *gds, RsNetworkExchangeService *nes, p3IdService *identities)
	: RsGxsCircleExchange(gds, nes, new RsGxsCircleSerialiser(), 
			RS_SERVICE_GXSV1_TYPE_GXSCIRCLE, identities, circleAuthenPolicy()), 
	RsGxsCircles(this), GxsTokenQueue(this), RsTickEvent(), mIdentities(identities), 
	mCircleMtx("p3GxsCircles"),
        mCircleCache(DEFAULT_MEM_CACHE_SIZE, "GxsCircleCache")

{
	// Kick off Cache Testing, + Others.
	RsTickEvent::schedule_in(CIRCLE_EVENT_CACHETEST, CACHETEST_PERIOD);
	//RsTickEvent::schedule_now(CIRCLE_EVENT_CACHEOWNIDS);
}


uint32_t p3GxsCircles::circleAuthenPolicy()
{
	return 0;
}

void	p3GxsCircles::service_tick()
{
	RsTickEvent::tick_events();
	GxsTokenQueue::checkRequests(); // GxsTokenQueue handles all requests.
	return;
}

void p3GxsCircles::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	std::cerr << "p3GxsCircles::notifyChanges()";
	std::cerr << std::endl;

	receiveChanges(changes);
}

/********************************************************************************/
/******************* RsCircles Interface  ***************************************/
/********************************************************************************/


#if 0
bool p3GxsCircles:: getNickname(const RsGxsId &id, std::string &nickname)
{
	return false;
}

bool p3GxsCircles:: getIdDetails(const RsGxsId &id, RsIdentityDetails &details)
{
	std::cerr << "p3GxsCircles::getIdDetails(" << id << ")";
	std::cerr << std::endl;

	{
		RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
		RsGxsIdCache data;
		if (mPublicKeyCache.fetch(id, data))
		{
			details = data.details;
			return true;
		}
	
		/* try private cache too */
		if (mPrivateKeyCache.fetch(id, data))
		{
			details = data.details;
			return true;
		}
	}

	/* it isn't there - add to public requests */
	cache_request_load(id);

	return false;
}


bool p3GxsCircles:: getOwnIds(std::list<RsGxsId> &ownIds)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/
	ownIds = mOwnIds;

	return true;
}


//
bool p3GxsCircles::submitOpinion(uint32_t& token, RsIdOpinion &opinion)
{
	return false;
}

bool p3GxsCircles::createIdentity(uint32_t& token, RsIdentityParameters &params)
{

	RsGxsIdGroup id;

	id.mMeta.mGroupName = params.nickname;
	if (params.isPgpLinked)
	{
		id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
	}
	else
	{
		id.mMeta.mGroupFlags = 0;
	}

	createGroup(token, id);

	return true;
}
#endif


/********************************************************************************/
/******************* RsGixs Interface     ***************************************/
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

		for(; vit != grpData.end(); vit++)
		{
			RsGxsCircleGroupItem* item = dynamic_cast<RsGxsCircleGroupItem*>(*vit);
			RsGxsCircleGroup group = item->group;
			group.mMeta = item->meta;

			// If its cached - add that info (TODO).
			groups.push_back(group);
		}
	}

	return ok;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

bool 	p3GxsCircles::createGroup(uint32_t& token, RsGxsCircleGroup &group)
{
    RsGxsCircleGroupItem* item = new RsGxsCircleGroupItem();
    item->group = group;
    item->meta = group.mMeta;
    RsGenExchange::publishGroup(token, item);
    return true;
}

void p3GxsCircles::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/)
{
	RsGxsCircleGroupItem *item = dynamic_cast<RsGxsCircleGroupItem *>(grpItem);
	if (!item)
	{
		std::cerr << "p3GxsCircles::service_CreateGroup() ERROR invalid cast";
		std::cerr << std::endl;
		return;
	}

	// Now copy the GroupId into the mCircleId, and set the mode.
	// TODO.
#ifdef HAVE_CIRCLE_META_DATA
	grpItem->meta.mCircleType = EXTERNAL;
	grpItem->meta.mCircleId = grpItem->meta.mGroupId;

	grpItem->group.mMeta.mCircleType = grpItem->meta.mCircleType;
	grpItem->group.mMeta.mCircleId = grpItem->meta.mCircleId;
#endif

}



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* 
 * Cache of recently used circles.
 */

RsGxsCircleCache::RsGxsCircleCache() 
{ 
	return; 
}


bool RsGxsCircleCache::loadBaseCircle(const RsGxsCircleGroup &circle)
{

	mCircleId = circle.mMeta.mGroupId;
	mUpdateTime = time(NULL);
	mProcessedCircles.insert(mCircleId);

	std::cerr << "RsGxsCircleCache::loadBaseCircle(" << mCircleId << ")";
	std::cerr << std::endl;

	return true;
}

bool RsGxsCircleCache::loadSubCircle(const RsGxsCircleCache &subcircle)
{
	/* copy across all the lists */

	/* should not be any unprocessed circles or peers */
	std::cerr << "RsGxsCircleCache::loadSubCircle(" << subcircle.mCircleId << ") TODO";
	std::cerr << std::endl;

	return true;
}

bool RsGxsCircleCache::getAllowedPeersList(std::list<RsPgpId> &friendlist)
{
	std::map<RsPgpId, std::list<RsGxsId> >::iterator it;
	for(mAllowedPeers.begin(); it != mAllowedPeers.end(); it++)
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




/****************************************************************************/
// ID STUFF. \/ \/ \/ \/ \/ \/ \/     :)
/****************************************************************************/
#if 0

/************************************************************************************/
/************************************************************************************/

bool p3GxsCircles::cache_request_ownids()
{
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
	std::cerr << "p3GxsCircles::cache_request_ownids()";
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	//opts.mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;

	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, CIRCLEREQ_CACHEOWNIDS);	
	return 1;
}


bool p3GxsCircles::cache_load_ownids(uint32_t token)
{
	std::cerr << "p3GxsCircles::cache_load_ownids() : " << token;
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		// Save List 
		{
			RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

			mOwnIds.clear();
			for(vit = grpData.begin(); vit != grpData.end(); vit++)
			{
				RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);


				if (item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
				{
					mOwnIds.push_back(item->meta.mGroupId);	
				}
			}
		}

		// Cache Items too.
		for(vit = grpData.begin(); vit != grpData.end(); vit++)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
			if (item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			{
	
				std::cerr << "p3GxsCircles::cache_load_ownids() Loaded Id with Meta: ";
				std::cerr << item->meta;
				std::cerr << std::endl;
	
				/* cache the data */
				cache_store(item);
			}
			delete item;
		}

	}
	else
	{
		std::cerr << "p3GxsCircles::cache_load_ownids() ERROR no data";
		std::cerr << std::endl;

		return false;
	}
	return true;
}


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
		for(; vit != grpIds.end(); vit++)
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
	std::cerr << "p3GxsCircles::cache_request_load(" << id << ")";
	std::cerr << std::endl;

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
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
	{
		RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		std::list<RsGxsId>::iterator it;
		for(it = mCacheLoad_ToCache.begin(); it != mCacheLoad_ToCache.end(); it++)
		{
			std::cerr << "p3GxsCircles::cache_start_load() GroupId: " << *it;
			std::cerr << std::endl;
			groupIds.push_back(*it); // might need conversion?
		}

		mCacheLoad_ToCache.clear();
	}

	if (groupIds.size() > 0)
	{
		std::cerr << "p3GxsCircles::cache_start_load() #Groups: " << groupIds.size();
		std::cerr << std::endl;

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
	std::cerr << "p3GxsCircles::cache_load_for_token() : " << token;
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); vit++)
		{
			RsGxsCircleGroupItem *item = dynamic_cast<RsGxsCircleGroupItem*>(*vit);
			RsGxsCircleGroup group = item->group;
			group.mMeta = item->meta;

			std::cerr << "p3GxsCircles::cache_load_for_token() Loaded Id with Meta: ";
			std::cerr << item->meta;
			std::cerr << std::endl;


			RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

			/* should already have a LoadingCache entry */
			RsGxsCircleId id = item->meta.mGroupId;
			std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
			it = mLoadingCache.find(id);
			if (it == mLoadingCache.end())
			{
				// ERROR.
				continue;
			}

			RsGxsCircleCache &cache = it->second;
			cache.loadBaseCircle(group);
			delete item;

			bool isComplete = true;
			bool isUnprocessedPeers = false;

			std::list<RsGxsId> &peers = group.mInvitedMembers;
			std::list<RsGxsId>::const_iterator pit;

			// need to trigger the searches.
			for(pit = peers.begin(); pit != peers.end(); pit++)
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
						}
						else
						{
							cache.mUnknownPeers.insert(*pit);
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
					cache.mUnprocessedPeers.insert(*pit);

					isComplete = false;
					isUnprocessedPeers = true;
				}
			}

#ifdef HANDLE_SUBCIRCLES
#if 0
			std::list<RsGxsCircleId> &circles = group.mSubCircles;
			std::list<RsGxsCircleId>::const_iterator cit;
			for(cit = circles.begin(); cit != circles.end(); cit++)
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

			if (isComplete)
			{
				/* move straight into the cache */
				mCircleCache.store(id, cache);
				mCircleCache.resize();

				/* remove from loading queue */
				mLoadingCache.erase(it);
			}

			if (isUnprocessedPeers)
			{
				/* schedule event to try reload gxsIds */
				RsTickEvent::schedule_in(CIRCLE_EVENT_RELOADIDS, GXSID_LOAD_CYCLE, id);
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


bool p3GxsCircles::cache_reloadids(const std::string &circleId)
{
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

	/* fetch from loadMap */
	std::map<RsGxsCircleId, RsGxsCircleCache>::iterator it;
	it = mLoadingCache.find(circleId);
	if (it == mLoadingCache.end())
	{
		// ERROR
		return false;
	}

	RsGxsCircleCache &cache = it->second;

	/* try reload Ids */
	std::set<RsGxsId>::const_iterator pit;
	for(pit = cache.mUnprocessedPeers.begin();
		pit != cache.mUnprocessedPeers.end(); pit++)
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
				}
				else
				{
					cache.mUnknownPeers.insert(*pit);
				}
			}
			else
			{
				// ERROR.
			}
		}
		else
		{
			// UNKNOWN ID.
		}
	}

	// clear unprocessed List.
	cache.mUnprocessedPeers.clear();

	// If sub-circles are complete too.
	if (cache.mUnprocessedCircles.empty())
	{

		// Push to Cache.
		mCircleCache.store(circleId, cache);
		mCircleCache.resize();

		/* remove from loading queue */
		mLoadingCache.erase(it);
	}
	return true;
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

		for(; vit != grpData.end(); vit++)
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
			for(pit = peers.begin(); pit != peers.end(); pit++)
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

							for(uit = updateCaches.begin(); uit != updateCaches.end(); uit++)
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
			for(cit = circles.begin(); cit != circles.end(); cit++)
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
	RsStackMutex stack(mCircleMtx); /********** STACK LOCKED MTX ******/

#if 0
	/* grab all the gpg ids... and make some ids */

	std::list<std::string> gpgids;
	std::list<std::string>::iterator it;
	
	rsPeers->getGPGAllList(gpgids);

	std::string ownId = rsPeers->getGPGOwnId();
	gpgids.push_back(ownId);

	int genCount = 0;
	int i;
	for(it = gpgids.begin(); it != gpgids.end(); it++)
	{
		/* create one or two for each one */
		int nIds = 1 + (RSRandom::random_u32() % 2);
		for(i = 0; i < nIds; i++)
		{
			RsGxsIdGroup id;

			RsPeerDetails details;

			//id.mKeyId = genRandomId();
			id.mMeta.mGroupId = genRandomId();
			id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
			id.mPgpIdHash = genRandomId();
			id.mPgpIdSign = genRandomId();

			if (rsPeers->getPeerDetails(*it, details))
			{
				std::ostringstream out;
				out << details.name << "_" << i + 1;

				//id.mNickname = out.str();
				id.mMeta.mGroupName = out.str();
			
	
			}
			else
			{
				std::cerr << "p3GxsCircles::generateDummyData() missing" << std::endl;
				std::cerr << std::endl;

				//id.mNickname = genRandomId();
				id.mMeta.mGroupName = genRandomId();
			}

			uint32_t dummyToken = 0;
			createGroup(dummyToken, id);

// LIMIT - AS GENERATION IS BROKEN.
#define MAX_TEST_GEN 5
			if (++genCount > MAX_TEST_GEN)
			{
				return;
			}
		}
	}
	return;

#define MAX_RANDOM_GPGIDS	10 //1000
#define MAX_RANDOM_PSEUDOIDS	50 //5000

	int nFakeGPGs = (RSRandom::random_u32() % MAX_RANDOM_GPGIDS);
	int nFakePseudoIds = (RSRandom::random_u32() % MAX_RANDOM_PSEUDOIDS);

	/* make some fake gpg ids */
	for(i = 0; i < nFakeGPGs; i++)
	{
		RsGxsCircleGroup id;

		RsPeerDetails details;

		id.mMeta.mGroupName = genRandomId();

		id.mMeta.mGroupId = genRandomId();
		id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
		id.mPgpIdHash = genRandomId();
		id.mPgpIdSign = genRandomId();

		uint32_t dummyToken = 0;
		createGroup(dummyToken, id);
	}

	/* make lots of pseudo ids */
	for(i = 0; i < nFakePseudoIds; i++)
	{
		RsGxsIdGroup id;

		RsPeerDetails details;

		id.mMeta.mGroupName = genRandomId();

		id.mMeta.mGroupId = genRandomId();
		id.mMeta.mGroupFlags = 0;
		id.mPgpIdHash = "";
		id.mPgpIdSign = "";


		uint32_t dummyToken = 0;
		createGroup(dummyToken, id);
	}

	//mUpdated = true;
#endif
	return;
}



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


std::ostream &operator<<(std::ostream &out, const RsGxsIdGroup &grp)
{
	out << "RsGxsIdGroup: Meta: " << grp.mMeta;
	out << " PgpIdHash: " << grp.mPgpIdHash;
	out << " PgpIdSign: [binary]"; // << grp.mPgpIdSign;
	out << std::endl;
	
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsGxsIdOpinion &opinion)
{
	out << "RsGxsIdOpinion: Meta: " << opinion.mMeta;
	out << std::endl;
	
	return out;
}










	// Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsCircles::handleResponse(uint32_t token, uint32_t req_type)
{
	std::cerr << "p3GxsCircles::handleResponse(" << token << "," << req_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(req_type)
	{
#if 0
		case CIRCLEREQ_CACHEOWNIDS:
			cache_load_ownids(token);
			break;
#endif
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
	std::cerr << "p3GxsCircles::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
#if 0
		case CIRCLE_EVENT_CACHEOWNIDS:
			cache_request_ownids();
			break;
#endif

		case CIRCLE_EVENT_CACHELOAD:
			cache_start_load();
			break;

		case CIRCLE_EVENT_RELOADIDS:
			cache_reloadids(elabel);
			break;

#if 0
		case CIRCLE_EVENT_CACHETEST:
			cachetest_getlist();
			break;
#endif

		default:
			/* error */
			std::cerr << "p3GxsCircles::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}



