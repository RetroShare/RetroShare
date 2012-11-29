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

class RsGxsCircleCache
{
	public:

	RsGxsCircleCache();
	bool loadBaseCircle(const RsGxsCircleGroup &circle);
	bool loadSubCircle(const RsGxsCircleCache &subcircle);

	bool getAllowedPeersList(std::list<RsPgpId> &friendlist);
	bool isAllowedPeer(const RsPgpId &id);
	bool addAllowedPeer(const RsPgpId &pgpid, const RsGxsId &gxsId);

	RsGxsCircleId mCircleId;

	time_t mUpdateTime;
	std::set<RsGxsCircleId> mUnprocessedCircles;
	std::set<RsGxsId> mUnprocessedPeers;

	std::set<RsGxsCircleId> mProcessedCircles;
	std::set<RsGxsId> mUnknownPeers;
	std::map<RsPgpId, std::list<RsGxsId> > mAllowedPeers;
};



class RsCircles
{
	/* Functions to handle Local / Internal Circles == Same as for file permissions. */
public:
	virtual void createLocalCircle() = 0;
	virtual void addToLocalCircle() = 0;
	virtual void removeFromLocalCircle() = 0;
	virtual void getLocalCirclePeers() = 0;
	virtual void getListOfLocalCircles() = 0;

	/* similar functions for External Groups */
	virtual bool createGroup(uint32_t& token, RsGxsCircleGroup &group) = 0;
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups) = 0;

};


class p3GxsCircles: public RsGxsCircleExchange, public RsGxsCircles,
		public GxsTokenQueue, public RsTickEvent
{
	public:
	p3GxsCircles(RsGeneralDataService* gds, RsNetworkExchangeService* nes, 
		p3IdService *identities);

	virtual void service_tick(); // needed for background processing.

	protected:

	static uint32_t circleAuthenPolicy();

	/** Notifications **/
	virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

	/** Overloaded to add PgpIdHash to Group Definition **/
	virtual void service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

	// Overloaded from GxsTokenQueue for Request callbacks.
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	// Overloaded from RsTickEvent.
	virtual void handle_event(uint32_t event_type, const std::string &elabel);

	public:


	virtual bool isLoaded(const RsGxsCircleId &circleId);
	virtual bool loadCircle(const RsGxsCircleId &circleId);

	virtual int canSend(const RsGxsCircleId &circleId, const RsPgpId &id);
	virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist);

	/*** External Interface */

	virtual void createLocalCircle();
	virtual void addToLocalCircle();
	virtual void removeFromLocalCircle();
	virtual void getLocalCirclePeers();
	virtual void getListOfLocalCircles();

	/* similar functions for External Groups */
	virtual bool createGroup(uint32_t& token, RsGxsCircleGroup &group);
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups);

	private:

	// Need some crazy arsed cache to store the circle info.
	// so we don't have to keep loading groups.

	int  cache_tick();

	bool cache_request_load(const RsGxsCircleId &id);
	bool cache_start_load();
	bool cache_load_for_token(uint32_t token);
	bool cache_reloadids(const std::string &circleId);


	p3IdService *mIdentities; // Needed for constructing Circle Info,

	RsMutex mCircleMtx; /* Locked Below Here */

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

virtual void generateDummyData();
std::string genRandomId();

};

#endif // P3_CIRCLES_SERVICE_HEADER
