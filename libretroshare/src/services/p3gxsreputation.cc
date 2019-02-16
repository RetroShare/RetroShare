/*******************************************************************************
 * libretroshare/src/services: p3gxsreputation.cc                              *
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
#include <math.h>
#include "pqi/p3linkmgr.h"

#include "retroshare/rspeers.h"

#include "services/p3gxsreputation.h"

#include "rsitems/rsgxsreputationitems.h"
#include "rsitems/rsconfigitems.h"

#include <sys/time.h>

#include <set>

/****
 * #define DEBUG_REPUTATION		1
 ****/

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
 * If not clever enough, this service will have to store a huge amount of data.
 * To make things tractable we do this:
 * 	- do not store reputations when no data is present, or when all friends are neutral
 * 	- only send a neutral opinion when they are a true change over someone's opinion
 * 	- only send a neutral opinion when it is a true change over someone's opinion
 * 	- auto-clean reputations for default values
 *
 * std::map<RsGxsId, Reputation> mReputations.
 * std::multimap<rstime_t, RsGxsId> mUpdated.
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
 *	[X]  Opinions are saved/loaded accross restart
 *	[X]  Opinions are transmitted to friends
 *	[X]  Opinions are transmitted to friends when updated
 * 
 * 	To do:
 * 	[X]  Add debug info
 * 	[X]  Test the whole thing
 * 	[X]  Implement a system to allow not storing info when we don't have it
 */

//static const uint32_t LOWER_LIMIT                         = 0;        // used to filter valid Opinion values from serialized data
static const uint32_t UPPER_LIMIT                         = 2;        // used to filter valid Opinion values from serialized data
//static const int      kMaximumPeerAge                     = 180;      // half a year.
static const int      kMaximumSetSize                     = 100;      // max set of updates to send at once.
static const int      CLEANUP_PERIOD        = 600 ;     // 10 minutes
//static const int      ACTIVE_FRIENDS_ONLINE_DELAY         = 86400*7 ; // 1 week.
static const int      kReputationRequestPeriod            = 600;      // 10 mins
static const int      kReputationStoreWait                = 180;      // 3 minutes.
//static const float    REPUTATION_ASSESSMENT_THRESHOLD_X1  = 0.5f ;    // reputation under which the peer gets killed. Warning there's a 1 shift with what's shown in GUI. Be careful.
//static const uint32_t PGP_AUTO_BAN_THRESHOLD_DEFAULT      = 2 ;       // above this, auto ban any GXS id signed by this node
static const uint32_t IDENTITY_FLAGS_UPDATE_DELAY         = 100 ;     // 
static const uint32_t BANNED_NODES_UPDATE_DELAY           = 313 ;     // update approx every 5 mins. Chosen to not be a multiple of IDENTITY_FLAGS_UPDATE_DELAY
static const uint32_t REPUTATION_INFO_KEEP_DELAY_DEFAULT          = 86400*35; // remove old reputation info 5 days after last usage limit, in case the ID would come back..
static const uint32_t BANNED_NODES_INACTIVITY_KEEP_DEFAULT        = 86400*60; // remove all info about banned nodes after 2 months of inactivity

static const uint32_t REPUTATION_DEFAULT_MIN_VOTES_FOR_REMOTELY_POSITIVE = 1;	// min difference in votes that makes friends opinion globally positive
static const uint32_t REPUTATION_DEFAULT_MIN_VOTES_FOR_REMOTELY_NEGATIVE = 1;	// min difference in votes that makes friends opinion globally negative
static const uint32_t MIN_DELAY_BETWEEN_REPUTATION_CONFIG_SAVE = 61 ; // never save more often than once a minute.

p3GxsReputation::p3GxsReputation(p3LinkMgr *lm)
	:p3Service(), p3Config(),
	mReputationMtx("p3GxsReputation"), mLinkMgr(lm) 
{
    addSerialType(new RsGxsReputationSerialiser());

    //mPgpAutoBanThreshold = PGP_AUTO_BAN_THRESHOLD_DEFAULT ;
    mRequestTime = 0;
    mStoreTime = 0;
    mReputationsUpdated = false;
        mLastIdentityFlagsUpdate = time(NULL) - 3;
    mLastBannedNodesUpdate = 0 ;
    mBannedNodesProxyNeedsUpdate = false;

    mAutoSetPositiveOptionToContacts = true;	// default
    mMinVotesForRemotelyPositive = REPUTATION_DEFAULT_MIN_VOTES_FOR_REMOTELY_POSITIVE;
    mMinVotesForRemotelyNegative = REPUTATION_DEFAULT_MIN_VOTES_FOR_REMOTELY_NEGATIVE;

    mLastReputationConfigSaved = 0;
    mChanged = false ;
    mMaxPreventReloadBannedIds = 0 ; // default is "never"
	mLastCleanUp = time(NULL) ;
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

	rstime_t now = time(NULL);

	if(mLastCleanUp + CLEANUP_PERIOD < now)
	{
		cleanup() ;

		mLastCleanUp = now ;
	}

	// no more than once per 5 second chunk.

	if(now > IDENTITY_FLAGS_UPDATE_DELAY+mLastIdentityFlagsUpdate)
	{
		updateStaticIdentityFlags() ;
		mLastIdentityFlagsUpdate = now ;
	}
	if(now > BANNED_NODES_UPDATE_DELAY+mLastBannedNodesUpdate)	// 613 is not a multiple of 100, to avoid piling up work
	{
		updateStaticIdentityFlags() ;	// needed before updateBannedNodesList!
		updateBannedNodesProxy();
		mLastBannedNodesUpdate = now ;
	}

	if(mBannedNodesProxyNeedsUpdate)
	{
		updateBannedNodesProxy();
		mBannedNodesProxyNeedsUpdate = false ;
	}

#ifdef DEBUG_REPUTATION
	static rstime_t last_debug_print = time(NULL) ;

	if(now > 10+last_debug_print)
	{
		last_debug_print = now ;
		debug_print() ;
	}
#endif

    if(mChanged && now > mLastReputationConfigSaved + MIN_DELAY_BETWEEN_REPUTATION_CONFIG_SAVE)
    {
        IndicateConfigChanged() ;
        mLastReputationConfigSaved = now ;
        mChanged = false ;
    }

	return 0;
}

void p3GxsReputation::setAutoPositiveOpinionForContacts(bool b)
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    if(b != mAutoSetPositiveOptionToContacts)
    {
        mLastIdentityFlagsUpdate = 0 ;
        mLastCleanUp = 0 ;
        mAutoSetPositiveOptionToContacts = b ;

        IndicateConfigChanged() ;
    }
}
bool p3GxsReputation::autoPositiveOpinionForContacts()
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
    return mAutoSetPositiveOptionToContacts ;
}

void p3GxsReputation::setRememberBannedIdThreshold(uint32_t days)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    if(mMaxPreventReloadBannedIds != days*86400)
    {
        mMaxPreventReloadBannedIds = days*86400 ;
        IndicateConfigChanged();
    }
}
uint32_t p3GxsReputation::rememberBannedIdThreshold()
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    return mMaxPreventReloadBannedIds/86400;
}

int	p3GxsReputation::status()
{
	return 1;
}
class ZeroInitCnt
{
	public:
		ZeroInitCnt(): cnt(0) {}
		uint32_t cnt ;
        
        	operator uint32_t& () { return cnt ; }
        	operator uint32_t() const { return cnt ; }
};

void p3GxsReputation::updateBannedNodesProxy()
{
    // This function keeps the Banned GXS id proxy up to date.
    //

    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    mPerNodeBannedIdsProxy.clear();

    for( std::map<RsPgpId, BannedNodeInfo>::iterator rit = mBannedPgpIds.begin();rit!=mBannedPgpIds.end();++rit)
        for(std::set<RsGxsId>::const_iterator it(rit->second.known_identities.begin());it!=rit->second.known_identities.end();++it)
            mPerNodeBannedIdsProxy.insert(*it) ;
}

void p3GxsReputation::updateStaticIdentityFlags()
{
    // This function is the *only* place where rsIdentity is called. Normally the cross calls between p3IdService and p3GxsReputations should only
    // happen one way: from rsIdentity to rsReputations. Still, reputations need to keep track of some identity flags. It's very important to make sure that
    // rsIdentity is not called inside a mutex-protected zone, because normally calls happen in the other way.

    std::list<RsGxsId> to_update ;

    // we need to gather the list to be used in a non locked frame
    {
	    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_REPUTATION
	    std::cerr << "Updating reputation identity flags" << std::endl;
#endif

	    for( std::map<RsGxsId, Reputation>::iterator rit = mReputations.begin();rit!=mReputations.end();++rit)
        {
            if( (!(rit->second.mIdentityFlags & REPUTATION_IDENTITY_FLAG_UP_TO_DATE)) && (mPerNodeBannedIdsProxy.find(rit->first) == mPerNodeBannedIdsProxy.end()))
			    to_update.push_back(rit->first) ;
        }
    }

    for(std::list<RsGxsId>::const_iterator rit(to_update.begin());rit!=to_update.end();++rit)
    {
	    RsIdentityDetails details;

	    if(!rsIdentity->getIdDetails(*rit,details))
	    {
#ifdef DEBUG_REPUTATION
		    std::cerr << "  cannot obtain info for " << *rit << ". Will do it later." << std::endl;
#endif
		    continue ;
	    }
        {
            RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
            std::map<RsGxsId,Reputation>::iterator it = mReputations.find(*rit) ;

            if(it == mReputations.end())
            {
                std::cerr << "  Weird situation: item " << *rit << " has been deleted from the list??" << std::endl;
                continue ;
            }
            it->second.mIdentityFlags = REPUTATION_IDENTITY_FLAG_UP_TO_DATE ;		// resets the NEEDS_UPDATE flag. All other flags set later on.

            if(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)
            {
                it->second.mIdentityFlags |= REPUTATION_IDENTITY_FLAG_PGP_LINKED ;
                it->second.mOwnerNode = details.mPgpId ;
            }
            if(details.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN ) it->second.mIdentityFlags |= REPUTATION_IDENTITY_FLAG_PGP_KNOWN ;

#ifdef DEBUG_REPUTATION
            std::cerr << "  updated flags for " << *rit << " to " << std::hex << it->second.mIdentityFlags << std::dec << std::endl;
#endif

            it->second.updateReputation() ;
            mChanged = true ;
        }
    }
}

void p3GxsReputation::cleanup()
{
	// remove opinions from friends that havn't been seen online for more than the specified delay

#ifdef DEBUG_REPUTATION
	std::cerr << "p3GxsReputation::cleanup() " << std::endl;
#endif
	rstime_t now = time(NULL) ;

    // We should keep opinions about identities that do not exist anymore, but only rely on the usage TS. That will in particular avoid asking p3idservice about deleted
    // identities, which would cause an excess of hits to the database. We do it in two steps to avoid a deadlock when calling rsIdentity from here.
    // Also, neutral opinions for banned PGP linked nodes are kept, so as to be able to not request them again.

	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

		for(std::map<RsGxsId,Reputation>::iterator it(mReputations.begin());it!=mReputations.end();)
        {
            bool should_delete = false ;

			if( it->second.mOwnOpinion ==
			        static_cast<int32_t>(RsOpinion::NEGATIVE) &&
			        mMaxPreventReloadBannedIds != 0 &&
			        it->second.mOwnOpinionTs + mMaxPreventReloadBannedIds < now )
            {
#ifdef DEBUG_REPUTATION
                std::cerr << "  ID " << it->first << ": own is negative for more than " << mMaxPreventReloadBannedIds/86400 << " days. Reseting it!" << std::endl;
#endif
                mChanged = true ;
            }

            // Delete slots with basically no information

			if( it->second.mOpinions.empty() &&
			        it->second.mOwnOpinion ==
			            static_cast<int32_t>(RsOpinion::NEUTRAL) &&
			        it->second.mOwnerNode.isNull() )
            {
#ifdef DEBUG_REPUTATION
                std::cerr << "  ID " << it->first << ": own is neutral and no opinions from friends => remove entry" << std::endl;
#endif
                should_delete = true ;
            }

            // Delete slots that havn't been used for a while. The else below is here for debug display purposes, and not harmful since both conditions lead the same effect.

            else if(it->second.mLastUsedTS + REPUTATION_INFO_KEEP_DELAY_DEFAULT < now)
            {
#ifdef DEBUG_REPUTATION
                std::cerr << "  ID " << it->first << ": no request for reputation for more than " << REPUTATION_INFO_KEEP_DELAY_DEFAULT/86400 << " days => deleting." << std::endl;
#endif
                should_delete = true ;
            }
#ifdef DEBUG_REPUTATION
            else
				std::cerr << "  ID " << it->first << ": flags=" << std::hex << it->second.mIdentityFlags << std::dec << ". Last used: " << (now - it->second.mLastUsedTS)/86400 << " days ago: kept." << std::endl;
#endif

			if(should_delete)
			{
                std::map<RsGxsId,Reputation>::iterator tmp(it) ;
				++tmp ;
				mReputations.erase(it) ;
				it = tmp ;
                mChanged = true ;
			}
			else
				++it;
        }
	}

    // Clean up of the banned PGP ids.

    {
        RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

        for(std::map<RsPgpId,BannedNodeInfo>::iterator it(mBannedPgpIds.begin());it!=mBannedPgpIds.end();)
            if(it->second.last_activity_TS + BANNED_NODES_INACTIVITY_KEEP_DEFAULT < now)
            {
#ifdef DEBUG_REPUTATION
                std::cerr << "  Removing all info about banned node " << it->first << " by lack of activity." << std::endl;
#endif
                std::map<RsPgpId,BannedNodeInfo>::iterator tmp(it   ) ;
                ++tmp ;
                mBannedPgpIds.erase(it) ;
                it = tmp ;

                mChanged = true ;
            }
            else
                ++it ;
    }

    // Update opinions based on flags and contact information.
	// Note: the call to rsIdentity->isARegularContact() is done off-mutex, in order to avoid a cross-deadlock, as
	// normally, p3GxsReputation gets called by p3dentity and not te reverse. That explains the weird implementation
	// of these two loops.
	{
        std::list<RsGxsId> should_set_to_positive_candidates ;

        if(mAutoSetPositiveOptionToContacts)
		{
			RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

			for(std::map<RsGxsId,Reputation>::iterator it(mReputations.begin());it!=mReputations.end();++it)
				if( it->second.mOwnOpinion ==
				        static_cast<int32_t>(RsOpinion::NEUTRAL) )
					should_set_to_positive_candidates.push_back(it->first) ;
		}

		for(std::list<RsGxsId>::const_iterator it(should_set_to_positive_candidates.begin());it!=should_set_to_positive_candidates.end();++it)
			if(rsIdentity->isARegularContact(*it))
				    setOwnOpinion(*it, RsOpinion::POSITIVE);
	}
}

static RsOpinion safe_convert_uint32t_to_opinion(uint32_t op)
{
	return RsOpinion(std::min( static_cast<uint32_t>(op), UPPER_LIMIT ));
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
				RsGxsReputationRequestItem *requestItem =  dynamic_cast<RsGxsReputationRequestItem *>(item);
				if (requestItem)
					SendReputations(requestItem);
				else
					itemOk = false;
			}
				break;

			case RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM:
			{
				RsGxsReputationUpdateItem *updateItem =  dynamic_cast<RsGxsReputationUpdateItem *>(item);
                
				if (updateItem)
					RecvReputations(updateItem);
				else
					itemOk = false;
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
#ifdef DEBUG_REPUTATION
	std::cerr << "p3GxsReputation::SendReputations()" << std::endl;
#endif

	RsPeerId peerId = request->PeerId();
	rstime_t last_update = request->mLastUpdate;
	rstime_t now = time(NULL);

	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

	std::multimap<rstime_t, RsGxsId>::iterator tit;
	tit = mUpdated.upper_bound(last_update); // could skip some - (fixed below).

	int count = 0;
	int totalcount = 0;
	RsGxsReputationUpdateItem *pkt = new RsGxsReputationUpdateItem();
    
	pkt->PeerId(peerId);
	for(;tit != mUpdated.end(); ++tit)
	{
		/* find */
		std::map<RsGxsId, Reputation>::iterator rit = mReputations.find(tit->second);
        
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
#ifdef DEBUG_REPUTATION
			std::cerr << "p3GxsReputation::SendReputations() Sending Full Packet";
			std::cerr << std::endl;
#endif

			sendItem(pkt);
            
			pkt = new RsGxsReputationUpdateItem();
			pkt->PeerId(peerId);
			count = 0;
		}
	}

	if (!pkt->mOpinions.empty())
	{
#ifdef DEBUG_REPUTATION
		std::cerr << "p3GxsReputation::SendReputations() Sending Final Packet";
		std::cerr << std::endl;
#endif

		sendItem(pkt);
	}
	else
	{
		delete pkt;
	}

#ifdef DEBUG_REPUTATION
	std::cerr << "p3GxsReputation::SendReputations() Total Count: " << totalcount;
	std::cerr << std::endl;
#endif

	return true;
}

void p3GxsReputation::locked_updateOpinion(
        const RsPeerId& from, const RsGxsId& about, RsOpinion op )
{
    /* find matching Reputation */
    std::map<RsGxsId, Reputation>::iterator rit = mReputations.find(about);

	RsOpinion new_opinion = op;
	RsOpinion old_opinion = RsOpinion::NEUTRAL ;	// default if not set

    bool updated = false ;

#ifdef DEBUG_REPUTATION
    std::cerr << "p3GxsReputation::update opinion of " << about << " from " << from << " to " << op << std::endl;
#endif
    // now 4 cases;
    //    Opinion already stored          
    //        New opinion is same:         nothing to do
    //        New opinion is different:    if neutral, remove entry
    //    Nothing stored
    //        New opinion is neutral:      nothing to do
    //        New opinion is != 1:         create entry and store

    if (rit == mReputations.end())	
    {
#ifdef DEBUG_REPUTATION
	    std::cerr << "  no preview record"<< std::endl;
#endif

		if(new_opinion != RsOpinion::NEUTRAL)
	    {
			mReputations[about] = Reputation();
		    rit = mReputations.find(about);
	    }
	    else
	    {
#ifdef DEBUG_REPUTATION
		    std::cerr << "  no changes!"<< std::endl;
#endif
		    return ;	// nothing to do
	    }
    }

    Reputation& reputation = rit->second;

	std::map<RsPeerId,RsOpinion>::iterator it2 = reputation.mOpinions.find(from) ;

    if(it2 == reputation.mOpinions.end())
    {
		if(new_opinion != RsOpinion::NEUTRAL)
	    {
		    reputation.mOpinions[from] = new_opinion;	// filters potentially tweaked reputation score sent by friend
		    updated = true ;
	    }
    }
    else
    {
	    old_opinion = it2->second ;

		if(new_opinion == RsOpinion::NEUTRAL)
	    {
		    reputation.mOpinions.erase(it2) ;		// don't store when the opinion is neutral
		    updated = true ;
	    }
	    else if(new_opinion != old_opinion)
	    {
		    it2->second = new_opinion ;
		    updated = true ;
	    }
    }

	if( reputation.mOpinions.empty() &&
	        reputation.mOwnOpinion == static_cast<int32_t>(RsOpinion::NEUTRAL) )
    {
	    mReputations.erase(rit) ;
#ifdef DEBUG_REPUTATION
	    std::cerr << "  own is neutral and no opinions from friends => remove entry" << std::endl;
#endif
        updated = true ;
    }
    else if(updated)
    {
#ifdef DEBUG_REPUTATION
	    std::cerr << "  reputation changed. re-calculating." << std::endl;
#endif
	    reputation.updateReputation() ;
    }
    
    if(updated)
	    IndicateConfigChanged() ;
}

bool p3GxsReputation::RecvReputations(RsGxsReputationUpdateItem *item)
{
#ifdef DEBUG_REPUTATION
	std::cerr << "p3GxsReputation::RecvReputations() from " << item->PeerId() << std::endl;
#endif

	RsPeerId peerid = item->PeerId();

	for( std::map<RsGxsId, uint32_t>::iterator it = item->mOpinions.begin(); it != item->mOpinions.end(); ++it)
	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

		locked_updateOpinion(peerid,it->first,safe_convert_uint32t_to_opinion(it->second));
	}

	updateLatestUpdate(peerid,item->mLatestUpdate);

	return true;
}


bool p3GxsReputation::updateLatestUpdate(RsPeerId peerid,rstime_t latest_update)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    std::map<RsPeerId, ReputationConfig>::iterator it = mConfig.find(peerid);

    if (it == mConfig.end())
	{
        mConfig[peerid] = ReputationConfig(peerid);
        it = mConfig.find(peerid) ;
	}
	it->second.mLatestUpdate = latest_update ;

	mReputationsUpdated = true;	
	// Switched to periodic save due to scale of data.
    
	IndicateConfigChanged();		

	return true;
}

/********************************************************************
 * Opinion
 ****/

RsReputationLevel p3GxsReputation::overallReputationLevel(
        const RsGxsId& id, uint32_t* identity_flags )
{
	RsReputationInfo info ;
    getReputationInfo(id,RsPgpId(),info) ;

    RsPgpId owner_id ;

    if(identity_flags)
        getIdentityFlagsAndOwnerId(id,*identity_flags,owner_id);

    return info.mOverallReputationLevel ;
}

bool p3GxsReputation::getIdentityFlagsAndOwnerId(const RsGxsId& gxsid, uint32_t& identity_flags,RsPgpId& owner_id)
{
    if(gxsid.isNull())
        return false ;

   RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

   std::map<RsGxsId,Reputation>::iterator it = mReputations.find(gxsid) ;

   if(it == mReputations.end())
       return false ;

   if(!(it->second.mIdentityFlags & REPUTATION_IDENTITY_FLAG_UP_TO_DATE))
       return false ;

   if(it->second.mIdentityFlags & REPUTATION_IDENTITY_FLAG_PGP_LINKED)
       identity_flags |= RS_IDENTITY_FLAGS_PGP_LINKED ;

   if(it->second.mIdentityFlags & REPUTATION_IDENTITY_FLAG_PGP_KNOWN)
       identity_flags |= RS_IDENTITY_FLAGS_PGP_KNOWN ;

   owner_id = it->second.mOwnerNode ;

   return true ;
}

bool p3GxsReputation::getReputationInfo(
        const RsGxsId& gxsid, const RsPgpId& ownerNode, RsReputationInfo& info,
        bool stamp )
{
    if(gxsid.isNull())
        return false ;
        
	rstime_t now = time(nullptr);

    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_REPUTATION2
    std::cerr << "getReputationInfo() for " << gxsid << ", stamp = " << stamp << std::endl;
#endif
    std::map<RsGxsId,Reputation>::iterator it = mReputations.find(gxsid) ;
    RsPgpId owner_id ;

    if(it == mReputations.end())
    {
		info.mOwnOpinion = RsOpinion::NEUTRAL ;
        info.mFriendAverageScore = RS_REPUTATION_THRESHOLD_DEFAULT ;
        info.mFriendsNegativeVotes = 0 ;
        info.mFriendsPositiveVotes = 0 ;

        owner_id = ownerNode ;
    }
    else
    {
        Reputation& rep(it->second) ;

		info.mOwnOpinion =
		        safe_convert_uint32t_to_opinion(
		            static_cast<uint32_t>(rep.mOwnOpinion) );
        info.mFriendAverageScore = rep.mFriendAverage ;
        info.mFriendsNegativeVotes = rep.mFriendsNegative ;
        info.mFriendsPositiveVotes = rep.mFriendsPositive ;

        if(rep.mOwnerNode.isNull() && !ownerNode.isNull())
            rep.mOwnerNode = ownerNode ;

        owner_id = rep.mOwnerNode ;

        if(stamp)
			rep.mLastUsedTS = now ;

		mChanged = true ;
    }

    // now compute overall score and reputation

    // 0 - check for own opinion. If positive or negative, it decides on the result

	if(info.mOwnOpinion == RsOpinion::NEGATIVE)
    {
    	// own opinion is always read in priority

		info.mOverallReputationLevel = RsReputationLevel::LOCALLY_NEGATIVE;
        return true ;
    }
	 if(info.mOwnOpinion == RsOpinion::POSITIVE)
    {
    	// own opinion is always read in priority

		info.mOverallReputationLevel = RsReputationLevel::LOCALLY_POSITIVE;
        return true ;
    }

    // 1 - check for banned PGP ids.

    std::map<RsPgpId,BannedNodeInfo>::iterator it2 ;

    if(!owner_id.isNull() && (it2 = mBannedPgpIds.find(owner_id))!=mBannedPgpIds.end())
    {
        // Check if current identity is present in the list of known identities for this banned node.

        if(it2->second.known_identities.find(gxsid) == it2->second.known_identities.end())
        {
            it2->second.known_identities.insert(gxsid) ;
            it2->second.last_activity_TS = now ;

            // if so, update

            mBannedNodesProxyNeedsUpdate = true ;
        }

#ifdef DEBUG_REPUTATION2
        std::cerr << "p3GxsReputations: identity " << gxsid << " is banned because owner node ID " << owner_id << " is banned (found in banned nodes list)." << std::endl;
#endif
		info.mOverallReputationLevel = RsReputationLevel::LOCALLY_NEGATIVE;
        return true ;
    }
    // also check the proxy

	if(mPerNodeBannedIdsProxy.find(gxsid) != mPerNodeBannedIdsProxy.end())
    {
#ifdef DEBUG_REPUTATION2
        std::cerr << "p3GxsReputations: identity " << gxsid << " is banned because owner node ID " << owner_id << " is banned (found in proxy)." << std::endl;
#endif
		info.mOverallReputationLevel = RsReputationLevel::LOCALLY_NEGATIVE;
        return true;
    }
    // 2 - now, our own opinion is neutral, which means we rely on what our friends tell

    if(info.mFriendsPositiveVotes >= info.mFriendsNegativeVotes + mMinVotesForRemotelyPositive)
		info.mOverallReputationLevel = RsReputationLevel::REMOTELY_POSITIVE;
    else if(info.mFriendsPositiveVotes + mMinVotesForRemotelyNegative <= info.mFriendsNegativeVotes)
		info.mOverallReputationLevel = RsReputationLevel::REMOTELY_NEGATIVE;
    else
		info.mOverallReputationLevel = RsReputationLevel::NEUTRAL;

#ifdef DEBUG_REPUTATION2
        std::cerr << "  information present. OwnOp = " << info.mOwnOpinion << ", owner node=" << owner_id << ", overall score=" << info.mAssessment << std::endl;
#endif

    return true ;
}

uint32_t p3GxsReputation::thresholdForRemotelyNegativeReputation()
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
    return mMinVotesForRemotelyNegative ;
}
uint32_t p3GxsReputation::thresholdForRemotelyPositiveReputation()
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
    return mMinVotesForRemotelyPositive ;
}
void p3GxsReputation::setThresholdForRemotelyPositiveReputation(uint32_t thresh)
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
    if(mMinVotesForRemotelyPositive == thresh || thresh==0)
        return ;

    mMinVotesForRemotelyPositive = thresh ;
    IndicateConfigChanged();
}

void p3GxsReputation::setThresholdForRemotelyNegativeReputation(uint32_t thresh)
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
    if(mMinVotesForRemotelyNegative == thresh || thresh==0)
        return ;

    mMinVotesForRemotelyNegative = thresh ;
    IndicateConfigChanged();
}

void p3GxsReputation::banNode(const RsPgpId& id,bool b)
{
    RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    if(b)
    {
        if(mBannedPgpIds.find(id) == mBannedPgpIds.end())
        {
            mBannedPgpIds[id] = BannedNodeInfo() ;
            IndicateConfigChanged();
        }
    }
    else
    {
        if(mBannedPgpIds.find(id) != mBannedPgpIds.end())
        {
            mBannedPgpIds.erase(id) ;
            IndicateConfigChanged();
        }
    }
}

RsReputationLevel p3GxsReputation::overallReputationLevel(const RsGxsId& id)
{ return overallReputationLevel(id, nullptr); }

bool p3GxsReputation::isNodeBanned(const RsPgpId& id)
{
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

    return mBannedPgpIds.find(id) != mBannedPgpIds.end();
}

bool p3GxsReputation::isIdentityBanned(const RsGxsId &id)
{
	RsReputationInfo info;
    
    if(!getReputationInfo(id,RsPgpId(),info))
        return false ;

#ifdef DEBUG_REPUTATION
    std::cerr << "isIdentityBanned(): returning " << (info.mOverallReputationLevel == RsReputations::REPUTATION_LOCALLY_NEGATIVE) << " for GXS id " << id << std::endl;
#endif
	return info.mOverallReputationLevel == RsReputationLevel::LOCALLY_NEGATIVE;
}

bool p3GxsReputation::getOwnOpinion(
        const RsGxsId& gxsid, RsOpinion& opinion )
{
#ifdef DEBUG_REPUTATION
    std::cerr << "setOwnOpinion(): for GXS id " << gxsid << " to " << opinion << std::endl;
#endif
    if(gxsid.isNull())
    {
        std::cerr << "  ID " << gxsid << " is rejected. Look for a bug in calling method." << std::endl;
        return false ;
    }

	RS_STACK_MUTEX(mReputationMtx);

	std::map<RsGxsId, Reputation>::iterator rit = mReputations.find(gxsid);

	if(rit != mReputations.end())
		opinion = safe_convert_uint32t_to_opinion(
		            static_cast<uint32_t>(rit->second.mOwnOpinion) );
	else
		opinion = RsOpinion::NEUTRAL;

    return true;
}

bool p3GxsReputation::setOwnOpinion(
        const RsGxsId& gxsid, RsOpinion opinion )
{
#ifdef DEBUG_REPUTATION
    std::cerr << "setOwnOpinion(): for GXS id " << gxsid << " to " << opinion << std::endl;
#endif
    if(gxsid.isNull())
    {
        std::cerr << "  ID " << gxsid << " is rejected. Look for a bug in calling method." << std::endl;
        return false ;
    }

	RS_STACK_MUTEX(mReputationMtx);

	std::map<RsGxsId, Reputation>::iterator rit;

	/* find matching Reputation */
	rit = mReputations.find(gxsid);
    
	if (rit == mReputations.end())
	{
#warning csoler 2017-01-05: We should set the owner node id here.
		mReputations[gxsid] = Reputation();
		rit = mReputations.find(gxsid);
	}

	// we should remove previous entries from Updates...
	Reputation &reputation = rit->second;
	if (reputation.mOwnOpinionTs != 0)
	{
		if (reputation.mOwnOpinion == static_cast<int32_t>(opinion))
		{
			// if opinion is accurate, don't update.
			return false;
		}

		std::multimap<rstime_t, RsGxsId>::iterator uit, euit;
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

	rstime_t now = time(nullptr);
	reputation.mOwnOpinion = static_cast<int32_t>(opinion);
	reputation.mOwnOpinionTs = now;
	reputation.updateReputation();

	mUpdated.insert(std::make_pair(now, gxsid));
	mReputationsUpdated = true;	
	mLastBannedNodesUpdate = 0 ;	// for update of banned nodes
    
	// Switched to periodic save due to scale of data.
	IndicateConfigChanged();		
    
	return true;
}


/********************************************************************
 * Configuration.
 ****/

RsSerialiser *p3GxsReputation::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsGxsReputationSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());
        return rss ;
}

bool p3GxsReputation::saveList(bool& cleanup, std::list<RsItem*> &savelist)
{
	cleanup = true;
	RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_REPUTATION
    std::cerr << "p3GxsReputation::saveList()" << std::endl;
#endif

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
		item->mIdentityFlags = rit->second.mIdentityFlags;
        item->mOwnerNodeId = rit->second.mOwnerNode;
        item->mLastUsedTS = rit->second.mLastUsedTS;

		std::map<RsPeerId, RsOpinion>::iterator oit;
		for(oit = rit->second.mOpinions.begin(); oit != rit->second.mOpinions.end(); ++oit)
		{
			// should be already limited.
			item->mOpinions[oit->first] = (uint32_t)oit->second;
		}

		savelist.push_back(item);
		count++;
	}

    for(std::map<RsPgpId,BannedNodeInfo>::const_iterator it(mBannedPgpIds.begin());it!=mBannedPgpIds.end();++it)
    {
        RsGxsReputationBannedNodeSetItem *item = new RsGxsReputationBannedNodeSetItem();

        item->mPgpId = it->first ;
        item->mLastActivityTS = it->second.last_activity_TS;
        item->mKnownIdentities.ids = it->second.known_identities;

        savelist.push_back(item) ;
    }
    
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;

	kv.key = "AUTO_REMOTELY_POSITIVE_THRESHOLD" ;
	rs_sprintf(kv.value, "%d", mMinVotesForRemotelyPositive);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = "AUTO_REMOTELY_NEGATIVE_THRESHOLD" ;
	rs_sprintf(kv.value, "%d", mMinVotesForRemotelyNegative);
	vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "AUTO_POSITIVE_CONTACTS" ;
    kv.value = mAutoSetPositiveOptionToContacts?"YES":"NO";
    vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "MAX_PREVENT_RELOAD_BANNED_IDS" ;
	rs_sprintf(kv.value, "%d", mMaxPreventReloadBannedIds) ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    savelist.push_back(vitem) ;

	return true;
}

void p3GxsReputation::saveDone()
{
	return;
}

bool p3GxsReputation::loadList(std::list<RsItem *>& loadList)
{
#ifdef DEBUG_REPUTATION
    std::cerr << "p3GxsReputation::loadList()" << std::endl;
#endif
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
		    loadReputationSet(set, peerSet);

#ifdef TO_REMOVE
	    RsGxsReputationSetItem_deprecated3 *set2 = dynamic_cast<RsGxsReputationSetItem_deprecated3 *>(*it);

        if(set2)
        {
            std::cerr << "(II) reading and converting old format ReputationSetItem." << std::endl;
		    loadReputationSet_deprecated3(set2, peerSet);
        }
#endif

        RsGxsReputationBannedNodeSetItem *itm2 = dynamic_cast<RsGxsReputationBannedNodeSetItem*>(*it) ;

        if(itm2 != NULL)
        {
            BannedNodeInfo& info(mBannedPgpIds[itm2->mPgpId]) ;
            info.last_activity_TS = itm2->mLastActivityTS ;
            info.known_identities = itm2->mKnownIdentities.ids ;
        }

	    RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

	    if(vitem)
		    for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
		    {
			    if(kit->key == "AUTO_REMOTELY_POSITIVE_THRESHOLD")
			    {
				    int val ;
				    if (sscanf(kit->value.c_str(), "%d", &val) == 1)
				    {
					    mMinVotesForRemotelyPositive = val ;
					    std::cerr << "Setting mMinVotesForRemotelyPositive threshold to " << val << std::endl ;
				    }
			    };
				if(kit->key == "AUTO_REMOTELY_NEGATIVE_THRESHOLD")
			    {
				    int val ;
				    if (sscanf(kit->value.c_str(), "%d", &val) == 1)
				    {
					    mMinVotesForRemotelyNegative = val ;
					    std::cerr << "Setting mMinVotesForRemotelyNegative threshold to " << val << std::endl ;
				    }
			    };
                if(kit->key == "AUTO_POSITIVE_CONTACTS")
                {
                    mAutoSetPositiveOptionToContacts = (kit->value == "YES");
                    std::cerr << "Setting AutoPositiveContacts to " << kit->value << std::endl ;
                    mLastBannedNodesUpdate = 0 ;	// force update
                }
                if(kit->key == "MAX_PREVENT_RELOAD_BANNED_IDS" )
                {
				    int val ;
				    if (sscanf(kit->value.c_str(), "%d", &val) == 1)
				    {
						mMaxPreventReloadBannedIds = val ;
					    std::cerr << "Setting mMaxPreventReloadBannedIds threshold to " << val << std::endl ;
				    }
				}
            }

	    delete (*it);
    }

    updateBannedNodesProxy();
    loadList.clear() ;
    return true;
}
#ifdef TO_REMOVE
bool p3GxsReputation::loadReputationSet_deprecated3(RsGxsReputationSetItem_deprecated3 *item, const std::set<RsPeerId> &peerSet)
{
    {
        RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

        std::map<RsGxsId, Reputation>::iterator rit;

        if(item->mGxsId.isNull())	// just a protection against potential errors having put 00000 into ids.
            return false ;

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
                reputation.mOpinions[peerId] = safe_convert_uint32t_to_opinion(oit->second);
        }

        reputation.mOwnOpinion = item->mOwnOpinion;
        reputation.mOwnOpinionTs = item->mOwnOpinionTS;
        reputation.mOwnerNode = item->mOwnerNodeId;
        reputation.mIdentityFlags = item->mIdentityFlags & (~REPUTATION_IDENTITY_FLAG_UP_TO_DATE);
        reputation.mLastUsedTS = time(NULL);

        // if dropping entries has changed the score -> must update.

        reputation.updateReputation() ;

        mUpdated.insert(std::make_pair(reputation.mOwnOpinionTs, gxsId));
    }
#ifdef DEBUG_REPUTATION
    RsReputations::ReputationInfo info ;
    getReputationInfo(item->mGxsId,item->mOwnerNodeId,info) ;
    std::cerr << item->mGxsId << " : own: " << info.mOwnOpinion << ", owner node: " << item->mOwnerNodeId << ", overall level: " << info.mOverallReputationLevel << std::endl;
#endif
    return true;
}
#endif

bool p3GxsReputation::loadReputationSet(RsGxsReputationSetItem *item, const std::set<RsPeerId> &peerSet)
{
    {
        RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

        std::map<RsGxsId, Reputation>::iterator rit;

        if(item->mGxsId.isNull())	// just a protection against potential errors having put 00000 into ids.
            return false ;

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
                reputation.mOpinions[peerId] = safe_convert_uint32t_to_opinion(oit->second);
        }

        reputation.mOwnOpinion = item->mOwnOpinion;
        reputation.mOwnOpinionTs = item->mOwnOpinionTS;
        reputation.mOwnerNode = item->mOwnerNodeId;
        reputation.mIdentityFlags = item->mIdentityFlags;
        reputation.mLastUsedTS = item->mLastUsedTS;

        // if dropping entries has changed the score -> must update.

        reputation.updateReputation() ;

        mUpdated.insert(std::make_pair(reputation.mOwnOpinionTs, gxsId));
    }
#ifdef DEBUG_REPUTATION
    RsReputations::ReputationInfo info ;
    getReputationInfo(item->mGxsId,item->mOwnerNodeId,info,false) ;
    std::cerr << item->mGxsId << " : own: " << info.mOwnOpinion << ", owner node: " << item->mOwnerNodeId << ", level: " << info.mOverallReputationLevel << std::endl;
#endif
    return true;
}


/********************************************************************
 * Send Requests.
 ****/

int	p3GxsReputation::sendPackets()
{
	rstime_t now = time(NULL);
	rstime_t requestTime, storeTime;
	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/
		requestTime = mRequestTime;
		storeTime = mStoreTime;
	}

	if (now > requestTime + kReputationRequestPeriod)
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

	/* prepare packets */
	std::list<RsPeerId>::iterator it;
	for(it = idList.begin(); it != idList.end(); ++it)
		sendReputationRequest(*it);
}

int p3GxsReputation::sendReputationRequest(RsPeerId peerid)
{
#ifdef DEBUG_REPUTATION
    rstime_t now = time(NULL) ;
	std::cerr << "  p3GxsReputation::sendReputationRequest(" << peerid << ") " ;
#endif

	/* */
	RsGxsReputationRequestItem *requestItem =  new RsGxsReputationRequestItem();
	requestItem->PeerId(peerid);

	{
		RsStackMutex stack(mReputationMtx); /****** LOCKED MUTEX *******/

		/* find the last timestamp we have */
		std::map<RsPeerId, ReputationConfig>::iterator it = mConfig.find(peerid);
        
		if (it != mConfig.end())
		{
#ifdef DEBUG_REPUTATION
            		std::cerr << "  lastUpdate = " << now - it->second.mLatestUpdate << " secs ago. Requesting only more recent." << std::endl;
#endif
			requestItem->mLastUpdate = it->second.mLatestUpdate;
		}
		else
		{
#ifdef DEBUG_REPUTATION
            		std::cerr << "  lastUpdate = never. Requesting all!" << std::endl;
#endif
			// get whole list.
			requestItem->mLastUpdate = 0;
		}
	}

	sendItem(requestItem);
	return 1;
}

void Reputation::updateReputation() 
{
    // the calculation of reputation makes the whole thing   

    int friend_total = 0;

    mFriendsNegative = 0 ;
    mFriendsPositive = 0 ;

    // accounts for all friends. Neutral opinions count for 1-1=0
    // because the average is performed over only accessible peers (not the total number) we need to shift to 1

	for( std::map<RsPeerId,RsOpinion>::const_iterator it(mOpinions.begin());
	     it != mOpinions.end(); ++it )
    {
		if( it->second == RsOpinion::NEGATIVE)
            ++mFriendsNegative ;

		if( it->second == RsOpinion::POSITIVE)
            ++mFriendsPositive ;

		friend_total += static_cast<int>(it->second) - 1;
    }

    if(mOpinions.empty())	// includes the case of no friends!
	    mFriendAverage = 1.0f ;
    else
    {
	    static const float REPUTATION_FRIEND_FACTOR_ANON       =  2.0f ;
	    static const float REPUTATION_FRIEND_FACTOR_PGP_LINKED =  5.0f ;
	    static const float REPUTATION_FRIEND_FACTOR_PGP_KNOWN  = 10.0f ;

	    // For positive votes, start from 1 and slowly tend to 2
	    // for negative votes, start from 1 and slowly tend to 0
	    // depending on signature state, the ID is harder (signed ids) or easier (anon ids) to ban or to promote.
	    //
	    // when REPUTATION_FRIEND_VARIANCE = 3, that gives the following values:
	    //
	    // total votes  |  mFriendAverage anon |  mFriendAverage PgpLinked | mFriendAverage PgpKnown  |
	    //              |        F=2.0         |        F=5.0              |      F=10.0              |
	    // -------------+----------------------+---------------------------+--------------------------+
	    // -10          |  0.00  Banned        |  0.13  Banned             | 0.36 Banned              |
	    // -5           |  0.08  Banned        |  0.36  Banned             | 0.60                     |
	    // -4           |  0.13  Banned        |  0.44  Banned             | 0.67                     |
	    // -3           |  0.22  Banned        |  0.54                     | 0.74                     |
	    // -2           |  0.36  Banned        |  0.67                     | 0.81                     |
	    // -1           |  0.60                |  0.81                     | 0.90                     |
	    //  0           |  1.0                 |  1.0                      | 1.00                     |
	    //  1           |  1.39                |  1.18                     | 1.09                     |
	    //  2           |  1.63                |  1.32                     | 1.18                     |
	    //  3           |  1.77                |  1.45                     | 1.25                     |
	    //  4           |  1.86                |  1.55                     | 1.32                     |
	    //  5           |  1.91                |  1.63                     | 1.39                     |
	    //
	    // Banning info is provided by the reputation system, and does not depend on PGP-sign state.
	    //
	    // However, each service might have its own rules for the different cases. For instance
	    // PGP-favoring forums might want a score > 1.4 for anon ids, and >= 1.0 for PGP-signed.

	    float reputation_bias ;

	    if(mIdentityFlags & REPUTATION_IDENTITY_FLAG_PGP_KNOWN)
		    reputation_bias = REPUTATION_FRIEND_FACTOR_PGP_KNOWN ;
	    else if(mIdentityFlags & REPUTATION_IDENTITY_FLAG_PGP_LINKED)
		    reputation_bias = REPUTATION_FRIEND_FACTOR_PGP_LINKED ;
	    else
		    reputation_bias = REPUTATION_FRIEND_FACTOR_ANON ;

	    if(friend_total > 0)
		    mFriendAverage = 2.0f-exp(-friend_total / reputation_bias) ;
	    else
		    mFriendAverage =      exp( friend_total / reputation_bias) ;
    }

    // now compute a bias for PGP-signed ids.

	if(mOwnOpinion == static_cast<int32_t>(RsOpinion::NEUTRAL))
		mReputationScore = mFriendAverage;
	else mReputationScore = static_cast<float>(mOwnOpinion);
}

void p3GxsReputation::debug_print()
{
    std::cerr << "Reputations database: " << std::endl;
    std::cerr << "  GXS ID data: " << std::endl;
    std::cerr << std::dec ;

	std::map<RsGxsId,Reputation> rep_copy;

	{
		RS_STACK_MUTEX(mReputationMtx);
		rep_copy = mReputations;
	}

	rstime_t now = time(nullptr);


	for( std::map<RsGxsId,Reputation>::const_iterator it(rep_copy.begin());
	     it != rep_copy.end(); ++it )
    {
		RsReputationInfo info;
		getReputationInfo(it->first, RsPgpId(), info, false);
		uint32_t lev = static_cast<uint32_t>(info.mOverallReputationLevel);

        std::cerr << "    " << it->first << ": own: " << it->second.mOwnOpinion
                  << ", PGP id=" << it->second.mOwnerNode
                  << ", flags=" << std::setfill('0') << std::setw(4) << std::hex << it->second.mIdentityFlags << std::dec
                  << ", Friend pos/neg: " << it->second.mFriendsPositive << "/" << it->second.mFriendsNegative
                  << ", reputation lev: [" << lev
                  << "], last own update: " << std::setfill(' ') << std::setw(10) << now - it->second.mOwnOpinionTs << " secs ago"
                  << ", last needed: " << std::setfill(' ') << std::setw(10) << now - it->second.mLastUsedTS << " secs ago, "
		          << std::endl;

#ifdef DEBUG_REPUTATION2
        for(std::map<RsPeerId,RsReputations::Opinion>::const_iterator it2(it->second.mOpinions.begin());it2!=it->second.mOpinions.end();++it2)
            std::cerr << "    " << it2->first << ": " << it2->second << std::endl;
#endif
    }

	RS_STACK_MUTEX(mReputationMtx);
    std::cerr << "  Banned RS nodes by ID: " << std::endl;

    for(std::map<RsPgpId,BannedNodeInfo>::const_iterator it(mBannedPgpIds.begin());it!=mBannedPgpIds.end();++it)
    {
        std::cerr << "    Node " << it->first << ", last activity: " << now - it->second.last_activity_TS << " secs ago." << std::endl;

        for(std::set<RsGxsId>::const_iterator it2(it->second.known_identities.begin());it2!=it->second.known_identities.end();++it2)
            std::cerr << "      " << *it2 << std::endl;
    }

    std::cerr << "  Per node Banned GXSIds proxy: " << std::endl;

    for(std::set<RsGxsId>::const_iterator it(mPerNodeBannedIdsProxy.begin());it!=mPerNodeBannedIdsProxy.end();++it)
        std::cerr << "    " << *it << std::endl;
}

