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

#include "serialiser/rsgxsreputationitems.h"

#include "retroshare/rsidentity.h"
#include "services/p3service.h"

//typedef std::string RsPeerId;

class p3LinkMgr;

class ReputationConfig
{
public:
	ReputationConfig()
	:mPeerId(), mLatestUpdate(0) { return; }

	ReputationConfig(const RsPeerId& peerId)
	:mPeerId(peerId), mLatestUpdate(0) { return; }

	RsPeerId mPeerId;
	time_t mLatestUpdate;
	time_t mLastQuery;
};

class Reputation
{
public:
	Reputation()
	:mOwnOpinion(0) { return; }

	Reputation(const RsGxsId& about)
	:mGxsId(about), mOwnOpinion(0) { return; }

int32_t CalculateReputation();

	RsGxsId mGxsId;
	std::map<RsPeerId, int32_t> mOpinions;
	int32_t mOwnOpinion;
	time_t  mOwnOpinionTs;

	int32_t mReputation;
};


//!The p3GxsReputation service.
 /**
  *
  * 
  */

class p3GxsReputation: public p3Service, public p3Config /* , public pqiMonitor */
{
	public:
		p3GxsReputation(p3LinkMgr *lm);

		/***** Interface for p3idservice *****/

		virtual bool updateOpinion(const RsGxsId& gxsid, int opinion);

		/***** overloaded from p3Service *****/
		/*!
		 * This retrieves all chat msg items and also (important!)
		 * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
		 * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
		 * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
		 * @see NotifyBase

		 */
		virtual int   tick();
		virtual int   status();


		/*!
		 * Interface stuff.
		 */

		/*************** pqiMonitor callback ***********************/
		//virtual void statusChange(const std::list<pqipeer> &plist);


		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;
		virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		virtual void saveDone();
		virtual bool loadList(std::list<RsItem*>& load) ;

	private:

		bool 	processIncoming();

		bool SendReputations(RsGxsReputationRequestItem *request);
		bool RecvReputations(RsGxsReputationUpdateItem *item);
		bool updateLatestUpdate(RsPeerId peerid, time_t ts);

		bool loadReputationSet(RsGxsReputationSetItem *item, 
				const std::set<RsPeerId> &peerSet);

		int     sendPackets();
		void sendReputationRequests();
		int sendReputationRequest(RsPeerId peerid);


	private:
		RsMutex mReputationMtx;

		time_t mRequestTime;
		time_t mStoreTime;
		bool   mReputationsUpdated;

		p3LinkMgr *mLinkMgr;

		// Data for Reputation.
		std::map<RsPeerId, ReputationConfig> mConfig;
		std::map<RsGxsId, Reputation> mReputations;
		std::multimap<time_t, RsGxsId> mUpdated;

		// set of Reputations to send to p3IdService.
		std::set<RsGxsId> mUpdatedReputations;
};

#endif //SERVICE_RSGXSREPUTATION_HEADER

