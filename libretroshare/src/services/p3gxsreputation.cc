/*
 * libretroshare/src/services p3gxsreputation.cc
 *
 * Gxs Reputation Service  for RetroShare.
 *
 * Copyright 2011-2014 by Robert Fernie.
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

#include "pqi/p3linkmgr.h"

#include "retroshare/rspeers.h"

#include "services/p3gxsreputation.h"
#include "serialiser/rsgxsreputationitems.h"

#include <sys/time.h>

#include <set>

/****
 * #define DEBUG_REPUTATION		1
 ****/


/* DEFINE INTERFACE POINTER! */
//RsGxsReputation *rsGxsReputation = NULL;

const int kMaximumPeerAge =	180; // half a year.
const int kMaximumSetSize = 	100;

/************ IMPLEMENTATION NOTES *********************************
 * 
 * p3GxsReputation shares opinions / reputations with peers.
 * This is closely linked to p3IdService, receiving info, updating
 * reputations as needed.
 *
 * It is designed as separate service as the exchange of peer opinions
 * is not well suited to Gxs Groups / Messages...
 *
 * Instead we can broadcast opinions to all peers.
 *
 * To avoid too much traffic, changes are transmitted rather than whole lists.
 * Peer A		   Peer B
 *  last update ----------->
 *	<----------- modified opinions.
 *
 * This service will have to store a huge amount of data.
 * need to workout how to reduce it.
 *
 * std::map<RsGxsId, Reputation> mReputations.
 * std::multimap<time_t, RsGxsId> mUpdated.
 *
 * std::map<RsPeerId, ReputationConfig> mConfig;
 *
 * Updates from p3GxsReputation -> p3IdService.
 * Updates from p3IdService -> p3GxsReputation.
 *
 * Each peer locally stores reputations for all GXS ids. If not stored, a default value
 * is used, corresponding to a neutral opinion. Peers also share their reputation level
 * with their neighbor nodes. 
 * 
 * The calculation method is the following:
 * 
 * 	Local values:
 * 		Good:      2
 * 		Neutral:   1
 * 		Bad:       0
 *
 * 	Overall reputation score:
 * 		
 * 		if(own_opinion == 0)	// means we dont' care
 * 			r = average_of_friends_opinions
 * 		else
 * 			r = own_opinion
 * 
 * 	Decisions based on reputation score:
 * 
 *              0               x1                1                    x2                   2
 *              | <-----------------------------------------------------------------------> |
 *     ---------+
 *     Lobbies  |  Msgs dropped
 *     Forums   |  Msgs dropped
 *     Messages |  Msgs dropped
 *     ---------+----------------------------------------------------------------------------
 * 
 * 	We select x1=0.5
 * 
 * 	=> to kill an identity, either you, or at least 50% of your friends need to flag it
 * 		as bad.
 * 	Rules:
 * 		* a single peer cannot drastically change the behavior of a given GXS id
 * 		* it should be easy for many peers to globally kill a GXS id
 * 
 * 	Typical examples:
 * 
 *            Friends   |  Friend average     |  Own     |  alpha     | Score
 *           -----------+---------------------+----------+------------+--------------
 *            10        |  0.5                |  1       |  0.25      | 0.375          
 *            10        |  1.0                |  1       |  0.25      | 1.0            
 *            10        |  1.0                |  0       |  0.25      | 1.0            
 * 
 * 	To check:
 *	[ ]  Opinions are saved/loaded accross restart
 *	[ ]  Opinions are transmitted to friends
 *	[ ]  Opinions are transmitted to friends when updated
 * 
 * 	To do:
 * 	[ ]  Test the whole thing
 * 	[ ]  Implement a system to allow not storing info when we don't have it
 */

const uint32_t LOWER_LIMIT		           = 0;
const uint32_t UPPER_LIMIT		           = 2;
const float    REPUTATION_ASSESSMENT_THRESHOLD_X1 = 0.5f ;

p3GxsReputation::p3GxsReputation(p3LinkMgr *lm)
	:p3Service(), p3Config(),
	mReputationMtx("p3GxsReputation"), mLinkMgr(lm) 
{
	addSerialType(new RsGxsReputationSerialiser());

	mRequestTime = 0;
	mStoreTime = 0;
	mReputationsUpdated = false;
}

const std::string GXS_REPUTATION_APP_NAME = "gxsreputation";
const uint16_t GXS_REPUTATION_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_REPUTATION_APP_MINOR_VERSION  =       0;
const uint16_t GXS_REPUTATION_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_REPUTATION_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3GxsReputation::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_REPUTATION,
                GXS_REPUTATION_APP_NAME,
                GXS_REPUTATION_APP_MAJOR_VERSION,
                GXS_REPUTATION_APP_MINOR_VERSION,
                GXS_REPUTATION_MIN_MAJOR_VERSION,
                GXS_REPUTATION_MIN_MINOR_VERSION);
}



int	p3GxsReputation::tick()
{
	processIncoming();
	sendPackets();

	return 0;
}

int	p3GxsReputation::status()
{
	return 1;
}


/***** Implementation ******/

bool p3GxsReputation::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	while(NULL != (item = recvItem()))
	{
#ifdef DEBUG_REPUTATION
		std::cerr << "p3GxsReputation::processingIncoming() Received Item:";
		std::cerr << std::endl;
		item->print(std::cerr);
		std::cerr << std::endl;
#endif
		bool itemOk = true;
		switch(item->PacketSubType())
		{
			default:
			case RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM:
			case RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM:
				std::cerr << "p3GxsReputation::processingIncoming() Unknown Item";
				std::cerr << std::endl;
				itemOk = false;
				break;

			case RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM:
			{
				RsGxsReputationRequestItem *requestItem = 
					dynamic_cast<RsGxsReputationRequestItem *>(item);
				if (requestItem)
				{
					SendReputations(requestItem);
				}
				else
				{
					itemOk = false;
				}
			}
				break;

			case RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM:
			{
				RsGxsReputationUpdateItem *updateItem = 
					dynamic_cast<RsGxsReputationUpdateItem *>(item);
				if (updateItem)
				{
					RecvReputations(updateItem);
				}
				else
				{
					itemOk = false;
				}
			}
				break;
		}

		if (!itemOk)
		{
			std::cerr << "p3GxsReputation::processingIncoming() Error with Item";
			std::cerr << std::endl;
		}

		/* clean up */
		delete item;
	}
	return true ;
} 
	

bool p3GxsReputation::SendReputations(RsGxsReputationRequestItem *request)
{
	std::cerr << "p3GxsReputation::SendReputations()";
	std::cerr << std::endl;

	RsPeerId peerId = request->PeerId();
	time_t last_update = request->mLastUpdate;
	time_t now = time(NULL);

	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

	std::multimap<time_t, RsGxsId>::iterator tit;
	tit = mUpdated.upper_bound(last_update); // could skip some - (fixed below).

	int count = 0;
	int totalcount = 0;
	RsGxsReputationUpdateItem *pkt = new RsGxsReputationUpdateItem();
	pkt->PeerId(peerId);
	for(;tit != mUpdated.end(); ++tit)
	{
		/* find */
		std::map<RsGxsId, Reputation>::iterator rit;
		rit = mReputations.find(tit->second);
		if (rit == mReputations.end())
		{
			std::cerr << "p3GxsReputation::SendReputations() ERROR Missing Reputation";
			std::cerr << std::endl;
			// error.
			continue;
		}

		if (rit->second.mOwnOpinionTs == 0)
		{
			std::cerr << "p3GxsReputation::SendReputations() ERROR OwnOpinionTS = 0";
			std::cerr << std::endl;
			// error.
			continue;
		}

		RsGxsId gxsId = rit->first;
		pkt->mOpinions[gxsId] = rit->second.mOwnOpinion;
		pkt->mLatestUpdate = rit->second.mOwnOpinionTs;
		if (pkt->mLatestUpdate == (uint32_t) now)
		{
			// if we could possibly get another Update at this point (same second).
			// then set Update back one second to ensure there are none missed.
			pkt->mLatestUpdate--;
		}
		
		count++;
		totalcount++;

		if (count > kMaximumSetSize)
		{
			std::cerr << "p3GxsReputation::SendReputations() Sending Full Packet";
			std::cerr << std::endl;

			sendItem(pkt);
			pkt = new RsGxsReputationUpdateItem();
			pkt->PeerId(peerId);
			count = 0;
		}
	}

	if (!pkt->mOpinions.empty())
	{
		std::cerr << "p3GxsReputation::SendReputations() Sending Final Packet";
		std::cerr << std::endl;

		sendItem(pkt);
	}
	else
	{
		delete pkt;
	}

	std::cerr << "p3GxsReputation::SendReputations() Total Count: " << totalcount;
	std::cerr << std::endl;

	return true;
}


bool p3GxsReputation::RecvReputations(RsGxsReputationUpdateItem *item)
{
	std::cerr << "p3GxsReputation::RecvReputations()";
	std::cerr << std::endl;

	RsPeerId peerid = item->PeerId();

	std::map<RsGxsId, uint32_t>::iterator it;
	for(it = item->mOpinions.begin(); it != item->mOpinions.end(); ++it)
	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

		/* find matching Reputation */
		std::map<RsGxsId, Reputation>::iterator rit;
		RsGxsId gxsId(it->first);

		rit = mReputations.find(gxsId);
		if (rit == mReputations.end())
		{
			mReputations[gxsId] = Reputation(gxsId);
			rit = mReputations.find(gxsId);
		}

		Reputation& reputation = rit->second;
		reputation.mOpinions[peerid] = std::min(it->second,UPPER_LIMIT);	// filters potentially tweaked reputation score sent by friend

		int previous = reputation.mReputation;
		if (previous != reputation.CalculateReputation())
		{
			// updated from the network.
			mUpdatedReputations.insert(gxsId);
		}
	}
	updateLatestUpdate(peerid, item->mLatestUpdate);
	return true;
}


bool p3GxsReputation::updateLatestUpdate(RsPeerId peerid, time_t ts)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    std::map<RsPeerId, ReputationConfig>::iterator it = mConfig.find(peerid);

    if (it == mConfig.end())
	{
        mConfig[peerid] = ReputationConfig(peerid);
        it = mConfig.find(peerid) ;
	}
	it->second.mLatestUpdate = ts;

	mReputationsUpdated = true;	
	// Switched to periodic save due to scale of data.
	//IndicateConfigChanged();		

	return true;
}

/********************************************************************
 * Opinion
 ****/

bool p3GxsReputation::getReputationInfo(const RsGxsId& gxsid, RsReputations::ReputationInfo& info)
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    std::map<RsGxsId,Reputation>::const_iterator it = mReputations.find(gxsid);

    if (it == mReputations.end())
    {
	    info.mOwnOpinion = RsReputations::OPINION_NEUTRAL ;
	    info.mOverallReputationScore = float(RsReputations::OPINION_NEUTRAL) ;
	    info.mAssessment = RsReputations::ASSESSMENT_OK ;
    }
    else
    {
	    info.mOwnOpinion = RsReputations::Opinion(it->second.mOwnOpinion) ;
	    info.mOverallReputationScore = float(RsReputations::OPINION_NEUTRAL) ;

	    if(info.mOverallReputationScore > REPUTATION_ASSESSMENT_THRESHOLD_X1)
		    info.mAssessment = RsReputations::ASSESSMENT_OK ;
	    else
		    info.mAssessment = RsReputations::ASSESSMENT_BAD ;
    }

    return true ;
}

bool p3GxsReputation::isIdentityOk(const RsGxsId &id)
{
    RsReputations::ReputationInfo info ;
    
    getReputationInfo(id,info) ;
    
    return info.mAssessment == RsReputations::ASSESSMENT_OK ;
}

bool p3GxsReputation::setOwnOpinion(const RsGxsId& gxsid, const RsReputations::Opinion& opinion)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

	std::map<RsGxsId, Reputation>::iterator rit;

	/* find matching Reputation */
	rit = mReputations.find(gxsid);
	if (rit == mReputations.end())
	{
		mReputations[gxsid] = Reputation(gxsid);
		rit = mReputations.find(gxsid);
	}

	// we should remove previous entries from Updates...
	Reputation &reputation = rit->second;
	if (reputation.mOwnOpinionTs != 0)
	{
		if (reputation.mOwnOpinion == opinion)
		{
			// if opinion is accurate, don't update.
			return false;
		}

		std::multimap<time_t, RsGxsId>::iterator uit, euit;
		uit = mUpdated.lower_bound(reputation.mOwnOpinionTs);
		euit = mUpdated.upper_bound(reputation.mOwnOpinionTs);
		for(; uit != euit; ++uit)
		{
			if (uit->second == gxsid)
			{
				mUpdated.erase(uit);
				break;
			}
		}
	}

	time_t now = time(NULL);
	reputation.mOwnOpinion = opinion;
	reputation.mOwnOpinionTs = now;
	reputation.CalculateReputation();

	mUpdated.insert(std::make_pair(now, gxsid));
	mUpdatedReputations.insert(gxsid);

	mReputationsUpdated = true;	
	// Switched to periodic save due to scale of data.
	//IndicateConfigChanged();		
	return true;
}


/********************************************************************
 * Configuration.
 ****/

RsSerialiser *p3GxsReputation::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsGxsReputationSerialiser());
        return rss ;
}

bool p3GxsReputation::saveList(bool& cleanup, std::list<RsItem*> &savelist)
{
	cleanup = true;
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

	/* save */
	std::map<RsPeerId, ReputationConfig>::iterator it;
	for(it = mConfig.begin(); it != mConfig.end(); ++it)
	{
		if (!rsPeers->isFriend(it->first))
		{
			// discard info from non-friends.
			continue;
		}

		RsGxsReputationConfigItem *item = new RsGxsReputationConfigItem();
		item->mPeerId = it->first;
		item->mLatestUpdate = it->second.mLatestUpdate;
		item->mLastQuery = it->second.mLastQuery;
		savelist.push_back(item);
	}

	int count = 0;
 	std::map<RsGxsId, Reputation>::iterator rit;
	for(rit = mReputations.begin(); rit != mReputations.end(); ++rit, count++)
	{
		RsGxsReputationSetItem *item = new RsGxsReputationSetItem();
		item->mGxsId = rit->first;
		item->mOwnOpinion = rit->second.mOwnOpinion;
		item->mOwnOpinionTS = rit->second.mOwnOpinionTs;
		item->mReputation = rit->second.mReputation;

		std::map<RsPeerId, uint32_t>::iterator oit;
		for(oit = rit->second.mOpinions.begin(); oit != rit->second.mOpinions.end(); ++oit)
		{
			// should be already limited.
			item->mOpinions[oit->first] = oit->second;
		}

		savelist.push_back(item);
		count++;
	}
	return true;
}

void p3GxsReputation::saveDone()
{
	return;
}

bool p3GxsReputation::loadList(std::list<RsItem *>& loadList)
{
	std::list<RsItem *>::iterator it;
	std::set<RsPeerId> peerSet;

	for(it = loadList.begin(); it != loadList.end(); ++it)
	{
		RsGxsReputationConfigItem *item = dynamic_cast<RsGxsReputationConfigItem *>(*it);
		// Configurations are loaded first. (to establish peerSet).
		if (item)
		{
			RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
			RsPeerId peerId(item->mPeerId);
			ReputationConfig &config = mConfig[peerId];
			config.mPeerId = peerId;
			config.mLatestUpdate = item->mLatestUpdate;
			config.mLastQuery = 0;
			
			peerSet.insert(peerId);
		}
		RsGxsReputationSetItem *set = dynamic_cast<RsGxsReputationSetItem *>(*it);
		if (set)
		{
			loadReputationSet(set, peerSet);
		}
		delete (*it);
	}

	return true;
}

bool p3GxsReputation::loadReputationSet(RsGxsReputationSetItem *item, const std::set<RsPeerId> &peerSet)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

	std::map<RsGxsId, Reputation>::iterator rit;

	/* find matching Reputation */
	RsGxsId gxsId(item->mGxsId);
	rit = mReputations.find(gxsId);
	if (rit != mReputations.end())
	{
		std::cerr << "ERROR";
		std::cerr << std::endl;
	}

	Reputation &reputation = mReputations[gxsId];

	// install opinions.
	std::map<RsPeerId, uint32_t>::const_iterator oit;
	for(oit = item->mOpinions.begin(); oit != item->mOpinions.end(); ++oit)
	{
		// expensive ... but necessary.
		RsPeerId peerId(oit->first);
		if (peerSet.end() != peerSet.find(peerId))
			reputation.mOpinions[peerId] = oit->second;
	}

	reputation.mOwnOpinion = item->mOwnOpinion;
	reputation.mOwnOpinionTs = item->mOwnOpinionTS;

	// if dropping entries has changed the score -> must update.
	int previous = item->mReputation;
	if (previous != reputation.CalculateReputation())
	{
		mUpdatedReputations.insert(gxsId);
	}

	mUpdated.insert(std::make_pair(reputation.mOwnOpinionTs, gxsId));
	return true;
}


/********************************************************************
 * Send Requests.
 ****/

const int kReputationRequestPeriod	= 	3600;
const int kReputationStoreWait		= 	180; // 3 minutes.

int	p3GxsReputation::sendPackets()
{
	time_t now = time(NULL);
	time_t requestTime, storeTime;
	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
		requestTime = mRequestTime;
		storeTime = mStoreTime;
	}

	if (now - requestTime > kReputationRequestPeriod)
	{
		sendReputationRequests();

		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_REPUTATION
		std::cerr << "p3GxsReputation::sendPackets() Regular Broadcast";
		std::cerr << std::endl;
#endif
		mRequestTime = now;
		mStoreTime = now + kReputationStoreWait;
	}

	if (now > storeTime)
	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_REPUTATION
		std::cerr << "p3GxsReputation::sendPackets() Regular Broadcast";
		std::cerr << std::endl;
#endif
		// push it into the future.
		// store time will be reset when requests are send.
		mStoreTime = now + kReputationRequestPeriod;

		if (mReputationsUpdated)
		{
			IndicateConfigChanged();
			mReputationsUpdated = false;
		}
	}

	return true ;
}

void p3GxsReputation::sendReputationRequests()
{
	/* we ping our peers */
	/* who is online? */
	std::list<RsPeerId> idList;

	mLinkMgr->getOnlineList(idList);

#ifdef DEBUG_REPUTATION
	std::cerr << "p3GxsReputation::sendReputationRequests()";
	std::cerr << std::endl;
#endif

	/* prepare packets */
	std::list<RsPeerId>::iterator it;
	for(it = idList.begin(); it != idList.end(); ++it)
	{
#ifdef DEBUG_REPUTATION
		std::cerr << "p3GxsReputation::sendReputationRequest() To: " << *it;
		std::cerr << std::endl;
#endif
		sendReputationRequest(*it);
	}
}



int p3GxsReputation::sendReputationRequest(RsPeerId peerid)
{
	std::cerr << "p3GxsReputation::sendReputationRequest(" << peerid << ")";
	std::cerr << std::endl;

	/* */
	RsGxsReputationRequestItem *requestItem = 
		new RsGxsReputationRequestItem();

	requestItem->PeerId(peerid);

	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

		/* find the last timestamp we have */
		std::map<RsPeerId, ReputationConfig>::iterator it;
		it = mConfig.find(peerid);
		if (it != mConfig.end())
		{
			requestItem->mLastUpdate = it->second.mLatestUpdate;
		}
		else
		{
			// get whole list.
			requestItem->mLastUpdate = 0;
		}
	}

	sendItem(requestItem);
	return 1;
}

float Reputation::CalculateReputation()
{
 	// the calculation of reputation makes the whole thing   
    
    	if(mOwnOpinion == RsReputations::OPINION_NEUTRAL)
        {
            uint32_t friend_total = 0;
            
            for(std::map<RsPeerId,uint32_t>::const_iterator it(mOpinions.begin());it!=mOpinions.end();++it)
                friend_total += it->second ;
            
            if(friend_total == 0)	// includes the case of no friends!
                return 0.0f ;
            else
                return friend_total / float(mOpinions.size()) ;
        }
        else
		return float(mOwnOpinion) ;
}
