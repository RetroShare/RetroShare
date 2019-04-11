/*******************************************************************************
 * libretroshare/src/services: p3gxsreputation.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014-2014 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef SERVICE_RSGXSREPUTATION_HEADER
#define SERVICE_RSGXSREPUTATION_HEADER

#include <string>
#include <list>
#include <map>
#include <set>

static const uint32_t  REPUTATION_IDENTITY_FLAG_UP_TO_DATE    = 0x0100;	// This flag means that the static info has been initialised from p3IdService. Normally such a call should happen once.
static const uint32_t  REPUTATION_IDENTITY_FLAG_PGP_LINKED    = 0x0001;
static const uint32_t  REPUTATION_IDENTITY_FLAG_PGP_KNOWN     = 0x0002;

#include "rsitems/rsgxsreputationitems.h"

#include "retroshare/rsidentity.h"
#include "retroshare/rsreputations.h"
#include "gxs/rsgixs.h"
#include "services/p3service.h"


class p3LinkMgr;

class ReputationConfig
{
public:
	ReputationConfig()
    :mPeerId(), mLatestUpdate(0), mLastQuery(0) { return; }

	ReputationConfig(const RsPeerId& peerId)
    :mPeerId(peerId), mLatestUpdate(0), mLastQuery(0) { return; }

	RsPeerId mPeerId;
	rstime_t mLatestUpdate;
	rstime_t mLastQuery;
};

struct BannedNodeInfo
{
    rstime_t last_activity_TS ;			// updated everytime a node or one of its former identities is required
    std::set<RsGxsId> known_identities ;	// list of known identities from this node. This is kept for a while, and useful in order to avoid re-asking these keys.
};

class Reputation
{
public:
	Reputation() :
	    mOwnOpinion(static_cast<int32_t>(RsOpinion::NEUTRAL)), mOwnOpinionTs(0),
	    mFriendAverage(1.0f),
	    /* G10h4ck: TODO shouln't this be initialized with
		 * RsReputation::NEUTRAL or UNKOWN? */
	    mReputationScore(static_cast<float>(RsOpinion::NEUTRAL)),
	    mIdentityFlags(0) {}

	void updateReputation();

	std::map<RsPeerId, RsOpinion> mOpinions;
	int32_t mOwnOpinion;
	rstime_t  mOwnOpinionTs;

	float mFriendAverage ;
    uint32_t mFriendsPositive ;		// number of positive vites from friends
    uint32_t mFriendsNegative ;		// number of negative vites from friends

	float mReputationScore;

	RsPgpId mOwnerNode;
    
	uint32_t mIdentityFlags;

    rstime_t mLastUsedTS ;			// last time the reputation was asked. Used to keep track of activity and clean up some reputation data.
};


//!The p3GxsReputation service.
class p3GxsReputation: public p3Service, public p3Config, public RsGixsReputation, public RsReputations /* , public pqiMonitor */
{
public:
    p3GxsReputation(p3LinkMgr *lm);
    virtual RsServiceInfo getServiceInfo();

    /***** Interface for RsReputations *****/
	virtual bool setOwnOpinion(const RsGxsId& key_id, RsOpinion op);
	virtual bool getOwnOpinion(const RsGxsId& key_id, RsOpinion& op) ;
	virtual bool getReputationInfo(
	        const RsGxsId& id, const RsPgpId& ownerNode, RsReputationInfo& info,
	        bool stamp = true );
    virtual bool isIdentityBanned(const RsGxsId& id) ;

    virtual bool isNodeBanned(const RsPgpId& id);
    virtual void banNode(const RsPgpId& id,bool b) ;

	RsReputationLevel overallReputationLevel(const RsGxsId& id) override;

	virtual RsReputationLevel overallReputationLevel(
	        const RsGxsId& id, uint32_t* identity_flags );

	virtual void setAutoPositiveOpinionForContacts(bool b) ;
	virtual bool autoPositiveOpinionForContacts() ;

	virtual void setRememberBannedIdThreshold(uint32_t days) ;
	virtual uint32_t rememberBannedIdThreshold() ;

	uint32_t thresholdForRemotelyNegativeReputation();
	uint32_t thresholdForRemotelyPositiveReputation();
	void setThresholdForRemotelyNegativeReputation(uint32_t thresh);
	void setThresholdForRemotelyPositiveReputation(uint32_t thresh);

    /***** overloaded from p3Service *****/
    virtual int   tick();
    virtual int   status();

    /*!
         * Interface stuff.
         */

    /************* from p3Config *******************/
    virtual RsSerialiser *setupSerialiser() ;
    virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
    virtual void saveDone();
    virtual bool loadList(std::list<RsItem*>& load) ;

private:
	bool getIdentityFlagsAndOwnerId(const RsGxsId& gxsid, uint32_t& identity_flags, RsPgpId &owner_id);

    bool 	processIncoming();

    bool SendReputations(RsGxsReputationRequestItem *request);
    bool RecvReputations(RsGxsReputationUpdateItem *item);
    bool updateLatestUpdate(RsPeerId peerid, rstime_t latest_update);

    void updateBannedNodesProxy();

    // internal update of data. Takes care of cleaning empty boxes.
	void locked_updateOpinion(
	        const RsPeerId& from, const RsGxsId& about, RsOpinion op);
    bool loadReputationSet(RsGxsReputationSetItem *item,  const std::set<RsPeerId> &peerSet);
#ifdef TO_REMOVE
	bool loadReputationSet_deprecated3(RsGxsReputationSetItem_deprecated3 *item, const std::set<RsPeerId> &peerSet);
#endif
    int  sendPackets();
    void cleanup();
    void sendReputationRequests();
    int  sendReputationRequest(RsPeerId peerid);
    void debug_print() ;
    void updateStaticIdentityFlags();

private:
    RsMutex mReputationMtx;

    rstime_t mLastCleanUp;
    rstime_t mRequestTime;
    rstime_t mStoreTime;
    rstime_t mLastBannedNodesUpdate ;
        rstime_t mLastIdentityFlagsUpdate ;
    bool   mReputationsUpdated;

    //float mAutoBanIdentitiesLimit ;
    bool mAutoSetPositiveOptionToContacts;

    p3LinkMgr *mLinkMgr;

    // Data for Reputation.
    std::map<RsPeerId, ReputationConfig> mConfig;
    std::map<RsGxsId, Reputation> mReputations;
    std::multimap<rstime_t, RsGxsId> mUpdated;

    // PGP Ids auto-banned. This is updated regularly.
    std::map<RsPgpId,BannedNodeInfo> mBannedPgpIds ;
    std::set<RsGxsId> mPerNodeBannedIdsProxy ;
    bool mBannedNodesProxyNeedsUpdate ;

    uint32_t mMinVotesForRemotelyPositive ;
    uint32_t mMinVotesForRemotelyNegative ;
    uint32_t mMaxPreventReloadBannedIds ;

    bool mChanged ; // slow version of IndicateConfigChanged();
    rstime_t mLastReputationConfigSaved ;
};

#endif //SERVICE_RSGXSREPUTATION_HEADER

