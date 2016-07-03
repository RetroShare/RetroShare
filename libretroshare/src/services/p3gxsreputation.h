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

#define REPUTATION_IDENTITY_FLAG_NEEDS_UPDATE  0x0100
#define REPUTATION_IDENTITY_FLAG_PGP_LINKED    0x0001
#define REPUTATION_IDENTITY_FLAG_PGP_KNOWN     0x0002

#include "serialiser/rsgxsreputationitems.h"

#include "retroshare/rsidentity.h"
#include "retroshare/rsreputations.h"
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

class Reputation
{
public:
	Reputation()
        	:mOwnOpinion(RsReputations::OPINION_NEUTRAL), mOwnOpinionTs(0),mFriendAverage(1.0f), mReputation(RsReputations::OPINION_NEUTRAL),mIdentityFlags(REPUTATION_IDENTITY_FLAG_NEEDS_UPDATE){ }
                                                                                            
	Reputation(const RsGxsId& /*about*/)
        	:mOwnOpinion(RsReputations::OPINION_NEUTRAL), mOwnOpinionTs(0),mFriendAverage(1.0f), mReputation(RsReputations::OPINION_NEUTRAL),mIdentityFlags(REPUTATION_IDENTITY_FLAG_NEEDS_UPDATE){ }

	void updateReputation();

	std::map<RsPeerId, RsReputations::Opinion> mOpinions;
	int32_t mOwnOpinion;
	time_t  mOwnOpinionTs;

    	float mFriendAverage ;
	float mReputation;
        
        RsPgpId mOwnerNode;
    
    	uint32_t mIdentityFlags;
};


//!The p3GxsReputation service.
 /**
  *
  * 
  */

class p3GxsReputation: public p3Service, public p3Config, public RsReputations /* , public pqiMonitor */
{
	public:
		p3GxsReputation(p3LinkMgr *lm);
		virtual RsServiceInfo getServiceInfo();

		/***** Interface for RsReputations *****/
		virtual bool setOwnOpinion(const RsGxsId& key_id, const Opinion& op) ;
		virtual bool getReputationInfo(const RsGxsId& id, const RsPgpId &owner_id, ReputationInfo& info) ;
		virtual bool isIdentityBanned(const RsGxsId& id, const RsPgpId &owner_node) ;
        
        	virtual void setNodeAutoBanThreshold(uint32_t n) ;
        	virtual uint32_t nodeAutoBanThreshold() ;
                
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

		bool 	processIncoming();

		bool SendReputations(RsGxsReputationRequestItem *request);
		bool RecvReputations(RsGxsReputationUpdateItem *item);
		bool updateLatestUpdate(RsPeerId peerid, time_t latest_update);
        	void updateActiveFriends() ;
		void updateBannedNodesList();
        
        	// internal update of data. Takes care of cleaning empty boxes.
        	void locked_updateOpinion(const RsPeerId &from, const RsGxsId &about, RsReputations::Opinion op);
		bool loadReputationSet(RsGxsReputationSetItem *item,  const std::set<RsPeerId> &peerSet);

		int  sendPackets();
        	void cleanup();
		void sendReputationRequests();
		int  sendReputationRequest(RsPeerId peerid);
        	void debug_print() ;
        	void updateIdentityFlags();

	private:
		RsMutex mReputationMtx;

		time_t mLastActiveFriendsUpdate;
		time_t mRequestTime;
		time_t mStoreTime;
            	time_t mLastBannedNodesUpdate ;
		bool   mReputationsUpdated;
        	uint32_t mAverageActiveFriends ;

		p3LinkMgr *mLinkMgr;

		// Data for Reputation.
		std::map<RsPeerId, ReputationConfig> mConfig;
		std::map<RsGxsId, Reputation> mReputations;
		std::multimap<time_t, RsGxsId> mUpdated;

		// set of Reputations to send to p3IdService.
		std::set<RsGxsId> mUpdatedReputations;
        
        	// PGP Ids auto-banned. This is updated regularly.
        	std::set<RsPgpId> mBannedPgpIds ; 
            	uint32_t mPgpAutoBanThreshold ;
};

#endif //SERVICE_RSGXSREPUTATION_HEADER

