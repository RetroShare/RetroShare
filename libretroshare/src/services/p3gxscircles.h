/*
 * libretroshare/src/services: p3circles.h
 *
 * Identity interface for RetroShare.
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

#ifndef P3_CIRCLES_SERVICE_HEADER
#define P3_CIRCLES_SERVICE_HEADER


#include "retroshare/rsgxscircles.h"	// External Interfaces.
#include "gxs/rsgenexchange.h"		// GXS service.
#include "gxs/rsgixs.h"			// Internal Interfaces.

#include "services/p3idservice.h"	// For constructing Caches

#include "gxs/gxstokenqueue.h"
#include "util/rstickevent.h"
#include "util/rsmemcache.h"

#include <map>
#include <string>

// TODO:
// can now edit circles. this leads to the following situation:
//    if someone gets removed from a self.retricted circle, he won't notice
//    because he can't receive the updated circle group if he is not part of the group anymore
//
// idea 1: refresh circle groups for example every week
//   if a circle was not refreshed since two weeks we can assume we where deleted
//   pro: does not leak info, simple to implement, does work even if the network topology changed (unfriending)
//   con: delay until we know about the deletion
//
// idea 2: add a field with deleted members to the circle group
//   then circle members can tell deleted circle members that they where deleted
//   pro: faster notification about deletion
//   con: more complicated, leaks info because the deleted member learns who is still a member
//   question: how to authenticate the deletion message?
//
// idea 3: make a two phase deletion process
//   first add members to a to-delete list
//   then wait a week to let the changes propagate
//   then remove from allowed peers list
//   pro: easy to implement
//   con: deletion process is slow
//   improvement idea: let only circle groups sync when the member is on he to-delete list
//     but don't allow sync of data from other services
//     this requires that the netservice knows that he is dealing with a circle group
//
// fact: have to use a timeout mechanism.
//   a timeout is the only thing which works even with a two months old backup

/* 
 * Circles Identity Service
 *
 * A collection of notes:
 *
 * We want to be able to express the following types of Circles.
 *
 *   - Public 
 *       - Groups & Messages can be passed onto anyone. ( No Restrictions. )
 *       - GXS Notes:
 * 		- This is what we have currently.
 *
 *   - External Circle
 *       - List of Identities that can receive the Group / Messages.
 * 	 - This list will be defined via a set of RsIdentities - which have PGPHashes set.
 * 	 	- We need the PGPHashes to be able to identify which peers can receive msgs.
 *  		- Messages are passed to the Intersection of (Identified PGPHashes & Friends)
 *       - Distribution of Circle Definitions can be also be restricted via circles.
 * 		- You can have Public External Groups, or Groups that only the Members know about.
 *	 - Control of these External Groups is determined by Admin / Publish Keys.
 *       - The Danger with External Groups, is your ID wll be associated with other people...
 * 		- Leaking information!!!
 *       - GXS Notes:
 * 		- p3Circles will provide a distrib list for a given Circle Group.
 *
 *   - Personal Circle or "Your Eyes Only".
 *       - Same as an Internal Circle Definition. (What will be used for File Sharing initially)
 *       - Each peer will have a bunch of these, Friends, Family, etc.
 *
 *       - The list is not publically shared, only the originator of the message will distribute.
 *       - You can communicate back to the originator, who will share with the other members.
 *	 but you mustn't discuss / share content with anyone else.
 *       - This is quite a Weak / Fragile Group, as there is only one distributor.
 *       - GXS NOTES:
 *	   - TO make this work, we need GXS or RsCircles to maintain extra info:
 *	   - GXS stores the original source, so communications can go back there.
 * 	     - If Originator, GXS store a REFERENCE, Circles turn this into a distrib list of peers.
 *
 *
 *
 * Like RsIdentities are used to validation messages, 
 * RsCircles will be used to determine if a peer can receive a group / messages.
 *
 *	bool RsCircles::canSend(RsGxsCircleId, RsPeerId)
 *	bool RsCircles::canSend(RsCircleInternalId, RsPeerId)
 *
 * or maybe just:
 *
 *	bool RsCircles::recipients(GxsPermission &perms, std::list<RsPeerId> friendlist);
 *
 */

/* Permissions is part of GroupMetaData 
 */

class RsGxsCircleMembershipStatus
{
public:
    RsGxsCircleMembershipStatus() : last_subscription_TS(0), subscription_flags(0) {}
    
    time_t   last_subscription_TS ;
    uint32_t subscription_flags ;	// combination of  GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST and  GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED   
};

class RsGxsCircleCache
{
	public:

	RsGxsCircleCache();
	bool loadBaseCircle(const RsGxsCircleGroup &circle);
	bool loadSubCircle(const RsGxsCircleCache &subcircle);

	bool getAllowedPeersList(std::list<RsPgpId> &friendlist) const;
	bool isAllowedPeer(const RsPgpId &id) const;
	bool isAllowedPeer(const RsGxsId &id, const RsGxsGroupId &destination_group) const;
	bool addAllowedPeer(const RsPgpId &pgpid);
	bool addLocalFriend(const RsPgpId &pgpid);

	RsGxsCircleId mCircleId;
	std::string mCircleName;

	uint32_t      mCircleType;
	bool	      mIsExternal;
    	RsGxsCircleId mRestrictedCircleId ;	// circle ID that circle is restricted to.

	uint32_t      mGroupStatus;
	uint32_t      mGroupSubscribeFlags;

	time_t mUpdateTime;
#ifdef SUBSCIRCLES
	std::set<RsGxsCircleId> mUnprocessedCircles;
	std::set<RsGxsCircleId> mProcessedCircles;
#endif
	std::map<RsGxsId,RsGxsCircleMembershipStatus> mMembershipStatus;
    	time_t mLastUpdatedMembershipTS ;	// last time the subscribe messages have been requested. Should be reset when new messages arrive.
    
	std::set<RsGxsId> mAllowedGxsIds;	// IDs that are allowed in the circle and have requested membership. This is the official members list.
	std::set<RsPgpId> mAllowedNodes;
    
    	RsPeerId mOriginator ; // peer who sent the data, in case we need to ask for ids
};


class PgpAuxUtils;

class p3GxsCircles: public RsGxsCircleExchange, public RsGxsCircles, public GxsTokenQueue, public RsTickEvent
{
	public:
	p3GxsCircles(RsGeneralDataService* gds, RsNetworkExchangeService* nes,  p3IdService *identities, PgpAuxUtils *pgpUtils);

virtual RsServiceInfo getServiceInfo();

	/*********** External Interface ***************/

	virtual bool getCircleDetails(const RsGxsCircleId &id, RsGxsCircleDetails &details);
	virtual bool getCircleExternalIdList(std::list<RsGxsCircleId> &circleIds);
	virtual bool getCirclePersonalIdList(std::list<RsGxsCircleId> &circleIds);

	virtual bool isLoaded(const RsGxsCircleId &circleId);
	virtual bool loadCircle(const RsGxsCircleId &circleId);

	virtual int canSend(const RsGxsCircleId &circleId, const RsPgpId &id, bool &should_encrypt);
	virtual int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id);
    
	virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist) ;
	virtual bool recipients(const RsGxsCircleId &circleId, const RsGxsGroupId& dest_group, std::list<RsGxsId> &gxs_ids) ;
        virtual bool isRecipient(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, const RsGxsId& id) ;


	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups);
    virtual void createGroup(uint32_t& token, RsGxsCircleGroup &group);
    virtual void updateGroup(uint32_t &token, RsGxsCircleGroup &group);


    	/* membership management for external circles */
    
    	virtual bool requestCircleMembership(const RsGxsId &own_gxsid, const RsGxsCircleId& circle_id) ;
    	virtual bool cancelCircleMembership(const RsGxsId &own_gxsid, const RsGxsCircleId& circle_id) ;
    
	/**********************************************/

	// needed for background processing.
	virtual void service_tick(); 

	protected:

	bool pushCircleMembershipRequest(const RsGxsId& own_gxsid,const RsGxsCircleId& circle_id,uint32_t request_type) ;
	static uint32_t circleAuthenPolicy();

	/** Notifications **/
	virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

	/** Overloaded to add PgpIdHash to Group Definition **/
	virtual ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

	// Overloaded from GxsTokenQueue for Request callbacks.
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	// Overloaded from RsTickEvent.
	virtual void handle_event(uint32_t event_type, const std::string &elabel);

	private:

	// Load data.
	bool request_CircleIdList();
	bool load_CircleIdList(uint32_t token);
	bool processMembershipRequests(uint32_t token);

	// Need some crazy arsed cache to store the circle info.
	// so we don't have to keep loading groups.

	int  cache_tick();

	bool cache_request_load(const RsGxsCircleId &id);
	bool cache_start_load();
	bool force_cache_reload(const RsGxsCircleId& id);
	bool cache_load_for_token(uint32_t token);
	bool cache_reloadids(const RsGxsCircleId &circleId);

	bool checkCircleCache();
    
	bool locked_checkCircleCacheForAutoSubscribe(RsGxsCircleCache &cache);
	bool locked_processLoadingCacheEntry(RsGxsCircleCache &cache);
	bool locked_checkCircleCacheForMembershipUpdate(RsGxsCircleCache &cache);

	p3IdService *mIdentities; // Needed for constructing Circle Info,
	PgpAuxUtils *mPgpUtils;

    // put a circle id into the external or personal circle id list
    // this function locks the mutex
    // if the id is already in the list, it will not be added again
    void addCircleIdToList(const RsGxsCircleId& circleId, uint32_t circleType);

	RsMutex mCircleMtx; /* Locked Below Here */

	std::list<RsGxsCircleId> mCircleExternalIdList;
	std::list<RsGxsCircleId> mCirclePersonalIdList;

	/***** Caching Circle Info, *****/
	// initial load queue
	std::list<RsGxsCircleId> mCacheLoad_ToCache;   

	// waiting for subcircle to load. (first is part of each of the second list)
	// TODO.
	//std::map<RsGxsCircleId, std::list<RsGxsCircleId> > mCacheLoad_SubCircle; 

	// Circles that are being loaded.
	std::map<RsGxsCircleId, RsGxsCircleCache> mLoadingCache;

	// actual cache.
	RsMemCache<RsGxsCircleId, RsGxsCircleCache> mCircleCache;

	private:

	std::string genRandomId();

	void generateDummyData();
	void checkDummyIdData();
	void generateDummyCircle();

    time_t mLastCacheMembershipUpdateTS ;

	uint32_t mDummyIdToken;
	std::list<RsGxsId> mDummyPgpLinkedIds;
	std::list<RsGxsId> mDummyOwnIds;
};

#endif // P3_CIRCLES_SERVICE_HEADER
