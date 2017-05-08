/*
 * libretroshare/src/services/p3gxsreputation.h
 *
 * Exchange list of Peers Reputations.
 *
 * Copyright 2014 by Robert Fernie.
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
	time_t mLatestUpdate;
	time_t mLastQuery;
};

struct BannedNodeInfo
{
    time_t last_activity_TS ;			// updated everytime a node or one of its former identities is required
    std::set<RsGxsId> known_identities ;	// list of known identities from this node. This is kept for a while, and useful in order to avoid re-asking these keys.
};

class Reputation
{
public:
	Reputation()
        	:mOwnOpinion(RsReputations::OPINION_NEUTRAL), mOwnOpinionTs(0),mFriendAverage(1.0f), mReputationScore(RsReputations::OPINION_NEUTRAL),mIdentityFlags(0){ }
                                                                                            
	Reputation(const RsGxsId& /*about*/)
        	:mOwnOpinion(RsReputations::OPINION_NEUTRAL), mOwnOpinionTs(0),mFriendAverage(1.0f), mReputationScore(RsReputations::OPINION_NEUTRAL),mIdentityFlags(0){ }

	void updateReputation();

	std::map<RsPeerId, RsReputations::Opinion> mOpinions;
	int32_t mOwnOpinion;
	time_t  mOwnOpinionTs;

	float mFriendAverage ;
    uint32_t mFriendsPositive ;		// number of positive vites from friends
    uint32_t mFriendsNegative ;		// number of negative vites from friends

	float mReputationScore;

	RsPgpId mOwnerNode;
    
	uint32_t mIdentityFlags;

    time_t mLastUsedTS ;			// last time the reputation was asked. Used to keep track of activity and clean up some reputation data.
};


//!The p3GxsReputation service.
 /**
  *
  * 
  */

class p3GxsReputation: public p3Service, public p3Config, public RsGixsReputation, public RsReputations /* , public pqiMonitor */
{
public:
    p3GxsReputation(p3LinkMgr *lm);
    virtual RsServiceInfo getServiceInfo();

    /***** Interface for RsReputations *****/
    virtual bool setOwnOpinion(const RsGxsId& key_id, const Opinion& op) ;
    virtual bool getOwnOpinion(const RsGxsId& key_id, Opinion& op) ;
    virtual bool getReputationInfo(const RsGxsId& id, const RsPgpId &ownerNode, ReputationInfo& info,bool stamp=true) ;
    virtual bool isIdentityBanned(const RsGxsId& id) ;

    virtual bool isNodeBanned(const RsPgpId& id);
    virtual void banNode(const RsPgpId& id,bool b) ;
    virtual ReputationLevel overallReputationLevel(const RsGxsId& id,uint32_t *identity_flags=NULL);

    virtual void setNodeAutoPositiveOpinionForContacts(bool b) ;
    virtual bool nodeAutoPositiveOpinionForContacts() ;

    virtual void setRememberDeletedNodesThreshold(uint32_t days) ;
    virtual uint32_t rememberDeletedNodesThreshold() ;

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
    bool updateLatestUpdate(RsPeerId peerid, time_t latest_update);

    void updateBannedNodesProxy();

    // internal update of data. Takes care of cleaning empty boxes.
    void locked_updateOpinion(const RsPeerId &from, const RsGxsId &about, RsReputations::Opinion op);
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

    time_t mLastCleanUp;
    time_t mRequestTime;
    time_t mStoreTime;
    time_t mLastBannedNodesUpdate ;
        time_t mLastIdentityFlagsUpdate ;
    bool   mReputationsUpdated;

    //float mAutoBanIdentitiesLimit ;
    bool mAutoSetPositiveOptionToContacts;

    p3LinkMgr *mLinkMgr;

    // Data for Reputation.
    std::map<RsPeerId, ReputationConfig> mConfig;
    std::map<RsGxsId, Reputation> mReputations;
    std::multimap<time_t, RsGxsId> mUpdated;

    // PGP Ids auto-banned. This is updated regularly.
    std::map<RsPgpId,BannedNodeInfo> mBannedPgpIds ;
    std::set<RsGxsId> mPerNodeBannedIdsProxy ;
    bool mBannedNodesProxyNeedsUpdate ;

    uint32_t mMinVotesForRemotelyPositive ;
    uint32_t mMinVotesForRemotelyNegative ;
    uint32_t mMaxPreventReloadBannedIds ;

    bool mChanged ; // slow version of IndicateConfigChanged();
    time_t mLastReputationConfigSaved ;
};

#endif //SERVICE_RSGXSREPUTATION_HEADER

