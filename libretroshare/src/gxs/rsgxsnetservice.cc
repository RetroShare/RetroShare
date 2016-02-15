/*
 * libretroshare/src/gxs: rsgxnetservice.cc
 *
 * Access to rs network and synchronisation service implementation
 *
 * Copyright 2012-2012 by Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

// 
// RsNxsItem
//    |
//    +-- RsNxsSyncGrp                    send req for group list, with time stamp of what we have
//    +-- RsNxsSyncMsg
//    +-- RsNxsGroupPublishKeyItem
//    +-- RsNxsSyncGrpItem                send individual grp info with time stamps, authors, etc.
// 
// 
// tick()
//    |
//    +----------- sharePublishKeys()
//    |
//    +----------- syncWithPeers()
//    |              |
//    |              +--if AutoSync--- send global UpdateTS of each peer to itself => the peer knows the last 
//    |              |                 time current peer has received an updated from himself
//    |              |                    type=RsNxsSyncGrp
//    |              |                    role: advise to request grp list for mServType
//    |              |
//    |              +--Retrive all grp Id + meta
//    |
//    |                 For each peer
//    |                    For each grp to request
//    |                       send RsNxsSyncMsg(ServiceType,  grpId,  updateTS)
//    |                                                                    |
//    |                       (Only send if rand() < sendingProb())        +---comes from mClientMsgUpdateMap
//    |                       
//    +----------- recvNxsItemQueue()
//                   |
//                   +------ handleRecvPublishKeys(auto*)
//                   |
//                   |
//                   |
//                   +------ handleRecvSyncGroup( RsNxsSyncGrp*)
//                   |            - parse all subscribed groups. For each, send a RsNxsSyncGrpItem with publish TS
//                   |            - pack into a single RsNxsTransac item
//                   |                     |
//                   |                     +---- canSendGrpId(peer, grpMeta, toVet)   // determines if put in vetting list 
//                   |                     |        |                                 // or sent right away
//                   |                     |        +--CIRCLES_TYPE_LOCAL------- false
//                   |                     |        +--CIRCLES_TYPE_PUBLIC------ true
//                   |                     |        +--CIRCLES_TYPE_EXTERNAL---- mCircles->canSend(circleId, getPgpId(peerId))
//                   |                     |        +--CIRCLES_TYPE_YOUR_EYES--- internal circle stuff
//                   |                     |
//                   |                     +---- store in mPendingCircleVet ou directement locked_pushGrpRespFromList()
//                   |
//                   +------ handleRecvSyncMessage( RsNxsSyncMsg*)
//                                - parse msgs from group
//                                - send all msg IDs for this group
//                                         |
//                                         +---- canSendMsgIds(msgMeta, grpMeta, peer)
//                                         |        |                                
//                                         |        +--CIRCLES_TYPE_LOCAL------- false
//                                         |        +--CIRCLES_TYPE_PUBLIC------ true
//                                         |        +--CIRCLES_TYPE_EXTERNAL---- mCircles->canSend(circleId, getPgpId(peerId))
//                                         |        +--CIRCLES_TYPE_YOUR_EYES--- internal circle stuff
//                                         |
//                                         +---- store in mPendingCircleVet ou directement locked_pushGrpRespFromList()
// data_tick()
//    |
//    +----------- updateServerSyncTS()
//    |                    - retrieve all group meta data
//    |                    - updates mServerMsgUpdateMap[grpId]=grp->mLastPostTS for all grps
//    |                    - updates mGrpServerUpdateItem to max of all received TS
//    |
//    +----------- processTransactions()
//    |
//    +----------- processCompletedTransactions()
//    |                    |
//    |                    +------ locked_processCompletedIncomingTrans()
//    |                    |              |
//    |                    |              +-------- locked_genReqMsgTransaction()   // request messages based on list
//    |                    |              |
//    |                    |              +-------- locked_genReqGrpTransaction()   // request groups based on list
//    |                    |              |
//    |                    |              +-------- locked_genSendMsgsTransaction() // send msg list
//    |                    |              |
//    |                    |              +-------- locked_genSendGrpsTransaction() // send group list
//    |                    |
//    |                    +------ locked_processCompletedOutgoingTrans()
//    |
//    +----------- processExplicitGroupRequests()
//    |                    - parse mExplicitRequest and for each element (containing a grpId list), 
//    |                       send the group ID (?!?!)
//    |
//    +----------- runVetting()
//                      |
//                      +--------- sort items from mPendingResp
//                      |                      |
//                      |                      +------ locked_createTransactionFromPending(GrpRespPending / MsgRespPending)  
//                      |                      |            // takes accepted transaction and adds them to the list of active trans
//                      |
//                      +--------- sort items from mPendingCircleVetting
//                                             |
//                                             +------ locked_createTransactionFromPending(GrpCircleIdsRequestVetting / MsgCircleIdsRequestVetting)   
//                                                          // takes accepted transaction and adds them to the list of active trans
// 
// Objects for time stamps
// =======================
//
//     mClientGrpUpdateMap: map< RsPeerId, TimeStamp >    Time stamp of last modification of group data for that peer (in peer's clock time!)
//                                                        (Set at server side to be mGrpServerUpdateItem->grpUpdateTS)
//
//                                                        Only updated in processCompletedIncomingTransaction() from Grp list transaction.
//                                                        Used in syncWithPeers() sending in RsNxsSyncGrp once to all peers: peer will send data if
//                                                        has something new. All time comparisons are in the friends' clock time.
//     
//     mClientMsgUpdateMap: map< RsPeerId, map<grpId,TimeStamp > >    
//     
//                                                        Last msg list modification time sent by that peer Id 
//                                                        Updated in processCompletedIncomingTransaction() from Grp list trans.
//                                                        Used in syncWithPeers() sending in RsNxsSyncGrp once to all peers.
//                                                        Set at server to be mServerMsgUpdateMap[grpId]->msgUpdateTS
//     
//     mGrpServerUpdateItem:  TimeStamp                   Last group local modification timestamp over all groups
//     
//     mServerMsgUpdateMap: map< GrpId, TimeStamp >       Timestamp local modification for each group (i.e. time of most recent msg / metadata update)
// 
// 
// Group update algorithm
// ======================
// 
//          CLient                                                                                                   Server
//          ======                                                                                                   ======
// 
//    tick()                                                                                                    tick()
//      |                                                                                                         |
//      +---- SyncWithPeers                                                                                       +-- recvNxsItemQueue()
//                 |                                                                                                   |
//                 +---------------- Send global UpdateTS of each peer to itself => the peer knows        +--------->  +------ handleRecvSyncGroup( RsNxsSyncGrp*)
//                 |                 the last msg sent (stored in mClientGrpUpdateMap[peer_id]),          |            |            - parse all subscribed groups. For each, send a RsNxsSyncGrpItem with publish TS
//                 |                    type=RsNxsSyncGrp                                                 |            |            - pack into a single RsNxsTransac item
//                 |                    role: advise to request grp list for mServType -------------------+            |
//                 |                                                                                             +-->  +------ handleRecvSyncMessage( RsNxsSyncMsg*)
//                 +---------------- Retrieve all grp Id + meta                                                  |                  - parse msgs from group
//                 |                                                                                             |                  - send all msg IDs for this group
//                 +-- For each peer                                                                             |     
//                       For each grp to request                                                                 |     
//                         send RsNxsSyncMsg(ServiceType,  grpId,  updateTS)                                     |     
//                                                                       |                                       |     
//                          (Only send if rand() < sendingProb())        +---comes from mClientMsgUpdateMap -----+
//
// 
//        
// Suggestions
// ===========
//    * handleRecvSyncGroup should use mit->second.mLastPost to limit the sending of already known data
// X  * apparently mServerMsgUpdateMap is initially empty -> by default clients will always want to receive the data.
//       => new peers will always send data for each group until they get an update for that group.
// X  * check that there is a timestamp for unsubscribed items, otherwise we always send TS=0 and we always get them!! (in 346)
// 
//       -> there is not. mClientMsgUpdateMap is updated when msgs are received.
//       -> 1842: leaves before asking for msg content.
// 
//      Proposed changes:
//       - for unsubsribed groups, mClientMsgUpdateMap[peerid][grpId]=now when the group list is received => wont be asked again
//       - when we subscribe, we reset the time stamp.
// 
//      Better change:
//       - each peer sends last 
// 
//    * the last TS method is not perfect: do new peers always receive old messages?
// 
//    * there's double information between mServerMsgUpdateMap first element (groupId) and second->grpId
//    * processExplicitGroupRequests() seems to send the group list that it was asked for without further information. How is that useful???
// 
//    * grps without messages will never be stamped because stamp happens in genReqMsgTransaction, after checking msgListL.empty()
//       Problem: without msg, we cannot know the grpId!!
// 
//    * mClientMsgUpdateMap[peerid][grpId] is only updated when new msgs are received. Up to date groups will keep asking for lists!
                            
                            
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sstream>

#include "rsgxsnetservice.h"
#include "gxssecurity.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rsreputations.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxscircles.h"
#include "pgp/pgpauxutils.h"
#include "util/rsmemory.h"

/***
 * Use the following defines to debug:
 	NXS_NET_DEBUG_0		shows group update high level information
 	NXS_NET_DEBUG_1		shows group update low level info (including transaction details)
 	NXS_NET_DEBUG_2		bandwidth information
 	NXS_NET_DEBUG_3		publish key exchange
 	NXS_NET_DEBUG_4		vetting
 	NXS_NET_DEBUG_5		summary of transactions (useful to just know what comes in/out)
 	NXS_NET_DEBUG_6		
 	NXS_NET_DEBUG_7		encryption/decryption of transactions

 ***/
#define NXS_NET_DEBUG_0 	1
#define NXS_NET_DEBUG_1 	1
//#define NXS_NET_DEBUG_2 	1
//#define NXS_NET_DEBUG_3 	1
#define NXS_NET_DEBUG_4 	1
#define NXS_NET_DEBUG_5 	1
//#define NXS_NET_DEBUG_6 	1
#define NXS_NET_DEBUG_7 	1

#define GIXS_CUT_OFF 0
//#define NXS_FRAG

// The constant below have a direct influence on how fast forums/channels/posted/identity groups propagate and on the overloading of queues:
//
// Channels/forums will update at a rate of SYNC_PERIOD*MAX_REQLIST_SIZE/60 messages per minute.
// A large TRANSAC_TIMEOUT helps large transactions to finish before anything happens (e.g. disconnexion) or when the server has low upload bandwidth,
// but also uses more memory.
// A small value for MAX_REQLIST_SIZE is likely to help messages to propagate in a chaotic network, but will also slow them down.
// A small SYNC_PERIOD fasten message propagation, but is likely to overload the server side of transactions (e.g. overload outqueues).
//
#define SYNC_PERIOD                                         60
#define MAX_REQLIST_SIZE                                    20  // No more than 20 items per msg request list => creates smaller transactions that are less likely to be cancelled.
#define TRANSAC_TIMEOUT                                   2000  // In seconds. Has been increased to avoid epidemic transaction cancelling due to overloaded outqueues.
#define SECURITY_DELAY_TO_FORCE_CLIENT_REUPDATE           3600  // force re-update if there happens to be a large delay between our server side TS and the client side TS of friends
#define REJECTED_MESSAGE_RETRY_DELAY                   24*3600  // re-try rejected messages every 24hrs. Most of the time this is because the peer's reputation has changed.
#define GROUP_STATS_UPDATE_DELAY                           240  // update unsubscribed group statistics every 3 mins
#define GROUP_STATS_UPDATE_NB_PEERS                          2  // number of peers to which the group stats are asked
#define MAX_ALLOWED_GXS_MESSAGE_SIZE                    199000  // 200,000 bytes including signature and headers

// Debug system to allow to print only for some IDs (group, Peer, etc)

#if defined(NXS_NET_DEBUG_0) || defined(NXS_NET_DEBUG_1) || defined(NXS_NET_DEBUG_2)  || defined(NXS_NET_DEBUG_3) \
 || defined(NXS_NET_DEBUG_4) || defined(NXS_NET_DEBUG_5) || defined(NXS_NET_DEBUG_6)  || defined(NXS_NET_DEBUG_7)

static const RsPeerId     peer_to_print     = RsPeerId(std::string(""))   ;
static const RsGxsGroupId group_id_to_print = RsGxsGroupId(std::string("2cd04bbf91e29b1f1b87aa9aa4e358f3" )) ;	// use this to allow to this group id only, or "" for all IDs
static const uint32_t     service_to_print  = 0x215 ;                       	// use this to allow to this service id only, or 0 for all services
										// warning. Numbers should be SERVICE IDS (see serialiser/rsserviceids.h. E.g. 0x0215 for forums)

class nullstream: public std::ostream {};
        
#if defined(NXS_NET_DEBUG_0) || defined(NXS_NET_DEBUG_1) || defined(NXS_NET_DEBUG_2)  || defined(NXS_NET_DEBUG_3) \
 || defined(NXS_NET_DEBUG_4) || defined(NXS_NET_DEBUG_5) || defined(NXS_NET_DEBUG_6)  || defined(NXS_NET_DEBUG_7)

static std::string nice_time_stamp(time_t now,time_t TS)
{
    if(TS == 0)
        return "Never" ;
    else
    {
        std::ostringstream s;
        s << now - TS << " secs ago" ;
        return s.str() ;
    }
}
#endif

static std::ostream& gxsnetdebug(const RsPeerId& peer_id,const RsGxsGroupId& grp_id,uint32_t service_type) 
{
    static nullstream null ;
    
    if((peer_to_print.isNull() || peer_id.isNull() || peer_id == peer_to_print) 
    && (group_id_to_print.isNull() || grp_id.isNull() || grp_id == group_id_to_print)
    && (service_to_print==0 || service_type == 0 || ((service_type >> 8)&0xffff) == service_to_print))
        return std::cerr << time(NULL) << ": " ;
    else
        return null ;
}

#define GXSNETDEBUG___                   gxsnetdebug(RsPeerId(),RsGxsGroupId(),mServiceInfo.mServiceType) 
#define GXSNETDEBUG_P_(peer_id         ) gxsnetdebug(peer_id   ,RsGxsGroupId(),mServiceInfo.mServiceType) 
#define GXSNETDEBUG__G(        group_id) gxsnetdebug(RsPeerId(),group_id      ,mServiceInfo.mServiceType) 
#define GXSNETDEBUG_PG(peer_id,group_id) gxsnetdebug(peer_id   ,group_id      ,mServiceInfo.mServiceType) 

#endif

const uint32_t RsGxsNetService::FRAGMENT_SIZE = 150000;

RsGxsNetService::RsGxsNetService(uint16_t servType, RsGeneralDataService *gds,
                                 RsNxsNetMgr *netMgr, RsNxsObserver *nxsObs, 
                                 const RsServiceInfo serviceInfo,
                                 RsGixsReputation* reputations, RsGcxs* circles, RsGixs *gixs,
                                 PgpAuxUtils *pgpUtils, bool grpAutoSync,bool msgAutoSync)
                                     : p3ThreadedService(), p3Config(), mTransactionN(0),
                                       mObserver(nxsObs),mGixs(gixs), mDataStore(gds), mServType(servType),
                                       mTransactionTimeOut(TRANSAC_TIMEOUT), mNetMgr(netMgr), mNxsMutex("RsGxsNetService"),
                                       mSyncTs(0), mLastKeyPublishTs(0),mLastCleanRejectedMessages(0), mSYNC_PERIOD(SYNC_PERIOD), mCircles(circles), mReputations(reputations),
					mPgpUtils(pgpUtils),
                    mGrpAutoSync(grpAutoSync),mAllowMsgSync(msgAutoSync), mGrpServerUpdateItem(NULL),
					mServiceInfo(serviceInfo)

{
	addSerialType(new RsNxsSerialiser(mServType));
	mOwnId = mNetMgr->getOwnId();
    mUpdateCounter = 0;
}

RsGxsNetService::~RsGxsNetService()
{
    RS_STACK_MUTEX(mNxsMutex) ;

    for(TransactionsPeerMap::iterator it = mTransactions.begin();it!=mTransactions.end();++it)
    {
        for(TransactionIdMap::iterator it2 = it->second.begin();it2!=it->second.end();++it2)
            delete it2->second ;

        it->second.clear() ;
    }
    mTransactions.clear() ;
    
    delete mGrpServerUpdateItem ;
    
    for(ClientGrpMap::iterator it = mClientGrpUpdateMap.begin();it!=mClientGrpUpdateMap.end();++it)
        delete it->second ;
    
    mClientGrpUpdateMap.clear() ;
        
    for(std::map<RsGxsGroupId, RsGxsServerMsgUpdateItem*>::iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();)
        delete it->second ;
    
    mServerMsgUpdateMap.clear() ;
}


int RsGxsNetService::tick()
{
	// always check for new items arriving
	// from peers
    if(receivedItems())
        recvNxsItemQueue();

    bool should_notify = false;
    
    {
	    RS_STACK_MUTEX(mNxsMutex) ;

	    should_notify = should_notify || !mNewGroupsToNotify.empty() ;
	    should_notify = should_notify || !mNewMessagesToNotify.empty() ;
    }
    
    if(should_notify)
        processObserverNotifications() ;
    
    time_t now = time(NULL);
    time_t elapsed = mSYNC_PERIOD + mSyncTs;

    if((elapsed) < now)
    {
        syncWithPeers();
        syncGrpStatistics();
        
    	mSyncTs = now;
    }

    if(now > 10 + mLastKeyPublishTs)
    {
        sharePublishKeysPending() ;
        
        mLastKeyPublishTs = now ;
    }
    
    if(now > 3600 + mLastCleanRejectedMessages)
    {
        mLastCleanRejectedMessages = now ;
        cleanRejectedMessages() ;
    }
    return 1;
}

void RsGxsNetService::processObserverNotifications()
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "Processing observer notification." << std::endl;
#endif
    
    std::vector<RsNxsGrp*> grps_copy ;
    std::vector<RsNxsMsg*> msgs_copy ;

    {
	    RS_STACK_MUTEX(mNxsMutex) ;

	    grps_copy = mNewGroupsToNotify ;
	    msgs_copy = mNewMessagesToNotify ;

	    mNewGroupsToNotify.clear() ;
	    mNewMessagesToNotify.clear() ;
    }

    mObserver->notifyNewGroups(grps_copy);
    mObserver->notifyNewMessages(msgs_copy);
}
            
void RsGxsNetService::rejectMessage(const RsGxsMessageId& msg_id)
{
    RS_STACK_MUTEX(mNxsMutex) ;
    
    mRejectedMessages[msg_id] = time(NULL) ;
}
void RsGxsNetService::cleanRejectedMessages()
{
    RS_STACK_MUTEX(mNxsMutex) ;
    time_t now = time(NULL) ;
    
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "Cleaning rejected messages." << std::endl;
#endif
    
    for(std::map<RsGxsMessageId,time_t>::iterator it(mRejectedMessages.begin());it!=mRejectedMessages.end();)
        if(it->second + REJECTED_MESSAGE_RETRY_DELAY < now)
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG___ << "  message id " << it->first << " should be re-tried. removing from list..." << std::endl;
#endif
        
		std::map<RsGxsMessageId,time_t>::iterator tmp = it ;
		++tmp ;
		mRejectedMessages.erase(it) ;
		it=tmp ;
	}
	else
            ++it ;
}

// This class collects outgoing items due to the broadcast of Nxs messages. It computes
// a probability that can be used to temper the broadcast of items so as to match the
// residual bandwidth (difference between max allowed bandwidth and current outgoing rate.

class NxsBandwidthRecorder
{
public:
	static const int OUTQUEUE_CUTOFF_VALUE = 500 ;
	static const int BANDWIDTH_ESTIMATE_DELAY = 20 ;

	static void recordEvent(uint16_t service_type, RsItem *item)
	{
		RS_STACK_MUTEX(mtx) ;

		uint32_t bw = RsNxsSerialiser(service_type).size(item) ;	// this is used to estimate bandwidth.
		timeval tv ;
		gettimeofday(&tv,NULL) ;

		// compute time(NULL) in msecs, for a more accurate bw estimate.

		uint64_t now = (uint64_t) tv.tv_sec * 1000 + tv.tv_usec/1000 ;

		total_record += bw ;
		++total_events ;

#ifdef NXS_NET_DEBUG_2
		std::cerr << "bandwidthRecorder::recordEvent() Recording event time=" << now << ". bw=" << bw << std::endl;
#endif

		// Every 20 seconds at min, compute a new estimate of the required bandwidth.

		if(now > last_event_record + BANDWIDTH_ESTIMATE_DELAY*1000)
		{
			// Compute the bandwidth using recorded times, in msecs
			float speed = total_record/1024.0f/(now - last_event_record)*1000.0f ;

			// Apply a small temporal convolution.
			estimated_required_bandwidth = 0.75*estimated_required_bandwidth + 0.25 * speed ;

#ifdef NXS_NET_DEBUG_2
			std::cerr << std::dec << "  " << total_record << " Bytes (" << total_events << " items)"
			          << " received in " << now - last_event_record << " seconds. Speed: " << speed << " KBytes/sec" << std::endl;
			std::cerr << "  instantaneous speed = " << speed << " KB/s" << std::endl;
			std::cerr << "  cumulated estimated = " << estimated_required_bandwidth << " KB/s" << std::endl;
#endif

			last_event_record = now ;
			total_record = 0 ;
			total_events = 0 ;
		}
	}

	// Estimate the probability of sending an item so that the expected bandwidth matches the residual bandwidth

	static float computeCurrentSendingProbability()
	{
		int maxIn=50,maxOut=50;
		float currIn=0,currOut=0 ;

		rsConfig->GetMaxDataRates(maxIn,maxOut) ;
		rsConfig->GetCurrentDataRates(currIn,currOut) ;

		RsConfigDataRates rates ;
		rsConfig->getTotalBandwidthRates(rates) ;

#ifdef NXS_NET_DEBUG_2
		std::cerr << std::dec << std::endl;
#endif

		float outqueue_factor     = 1.0f/pow( std::max(0.02f,rates.mQueueOut / (float)OUTQUEUE_CUTOFF_VALUE),5.0f) ;
		float accepted_bandwidth  = std::max( 0.0f, maxOut - currOut) ;
		float max_bandwidth_factor = std::min( accepted_bandwidth / estimated_required_bandwidth,1.0f ) ;

		// We account for two things here:
		//   1 - the required max bandwidth
		//   2 - the current network overload, measured from the size of the outqueues.
		//
		// Only the later can limit the traffic if the internet connexion speed is responsible for outqueue overloading.

		float sending_probability = std::min(outqueue_factor,max_bandwidth_factor) ;

#ifdef NXS_NET_DEBUG_2
		std::cerr << "bandwidthRecorder::computeCurrentSendingProbability()" << std::endl;
		std::cerr << "  current required bandwidth : " << estimated_required_bandwidth << " KB/s" << std::endl;
		std::cerr << "  max_bandwidth_factor       : " << max_bandwidth_factor << std::endl;
		std::cerr << "  outqueue size              : " << rates.mQueueOut << ", factor=" << outqueue_factor << std::endl;
		std::cerr << "  max out                    : " << maxOut << ", currOut=" << currOut << std::endl;
		std::cerr << "  computed probability       : " << sending_probability << std::endl;
#endif

		return sending_probability ;
	}

private:
	static RsMutex mtx;
	static uint64_t last_event_record ;
	static float estimated_required_bandwidth ;
	static uint32_t total_events ;
	static uint64_t total_record ;
};

uint32_t NxsBandwidthRecorder::total_events =0 ;		     // total number of events. Not used.
uint64_t NxsBandwidthRecorder::last_event_record = time(NULL) * 1000;// starting time of bw estimate period (in msec)
uint64_t NxsBandwidthRecorder::total_record =0 ;		     // total bytes recorded in the current time frame
float    NxsBandwidthRecorder::estimated_required_bandwidth = 10.0f ;// Estimated BW for sending sync data. Set to 10KB/s, to avoid 0.
RsMutex  NxsBandwidthRecorder::mtx("Bandwidth recorder") ; 	     // Protects the recorder since bw events are collected from multiple GXS Net services

// temporary holds a map of pointers to class T, and destroys all pointers on delete.

template<class T>
class RsGxsMetaDataTemporaryMap: public std::map<RsGxsGroupId,T*>
{
public:
    virtual ~RsGxsMetaDataTemporaryMap()
    {
        clear() ;
    }
    
    virtual void clear()
    {
        for(typename RsGxsMetaDataTemporaryMap<T>::iterator it = this->begin();it!=this->end();++it)
            if(it->second != NULL)
		    delete it->second ;
        
        std::map<RsGxsGroupId,T*>::clear() ;
    }
};
        
void RsGxsNetService::syncWithPeers()
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "RsGxsNetService::syncWithPeers() this=" << (void*)this << ". serviceInfo=" << mServiceInfo << std::endl;
#endif

    static RsNxsSerialiser ser(mServType) ;	// this is used to estimate bandwidth.

    RS_STACK_MUTEX(mNxsMutex) ;

    std::set<RsPeerId> peers;
    mNetMgr->getOnlineList(mServiceInfo.mServiceType, peers);
    if (peers.empty()) {
        // nothing to do
        return;
    }

    std::set<RsPeerId>::iterator sit = peers.begin();

    // for now just grps
    for(; sit != peers.end(); ++sit)
    {

        const RsPeerId peerId = *sit;

        ClientGrpMap::const_iterator cit = mClientGrpUpdateMap.find(peerId);
        uint32_t updateTS = 0;
        if(cit != mClientGrpUpdateMap.end())
        {
            const RsGxsGrpUpdateItem *gui = cit->second;
            updateTS = gui->grpUpdateTS;
        }
        RsNxsSyncGrpReqItem *grp = new RsNxsSyncGrpReqItem(mServType);
        grp->clear();
        grp->PeerId(*sit);
        grp->updateTS = updateTS;

        //NxsBandwidthRecorder::recordEvent(mServType,grp) ;

#ifdef NXS_NET_DEBUG_5
	GXSNETDEBUG_P_(*sit) << "Service "<< std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << "  sending global group TS of peer id: " << *sit << " ts=" << nice_time_stamp(time(NULL),updateTS) << " (secs ago) to himself" << std::endl;
#endif
        sendItem(grp);
    }

    if(!mAllowMsgSync)
        return ;

#ifndef GXS_DISABLE_SYNC_MSGS

    typedef RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> GrpMetaMap;
    GrpMetaMap grpMeta;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    GrpMetaMap toRequest;

    for(GrpMetaMap::iterator mit = grpMeta.begin(); mit != grpMeta.end(); ++mit)
    {
	    RsGxsGrpMetaData* meta = mit->second;

	    // This was commented out because we want to know how many messages are available for unsubscribed groups.

	    if(meta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED )
	    {
		    toRequest.insert(std::make_pair(mit->first, meta));
		    mit->second = NULL ;	// avoids destruction ;-)
	    }
    }

    sit = peers.begin();

    // Jan. 26, 2016. This has been disabled, since GXS has been fixed, groups will not re-ask for data. So even if outqueues are filled up by multiple
    // attempts of the same request, the transfer will eventually end up. The code for NxsBandwidthRecorder should be kept for a while, 
    // just in case.
    
    // float sending_probability = NxsBandwidthRecorder::computeCurrentSendingProbability() ;
#ifdef NXS_NET_DEBUG_2
    std::cerr << "  syncWithPeers(): Sending probability = " << sending_probability << std::endl;
#endif

    // Synchronise group msg for groups which we're subscribed to
    // For each peer and each group, we send to the peer the time stamp of the most
    // recent modification the peer has sent. If the peer has more recent messages he will send them, because its latest 
    // modifications will be more recent. This ensures that we always compare timestamps all taken in the same 
    // computer (the peer's computer in this case)

    for(; sit != peers.end(); ++sit)
    {
        const RsPeerId& peerId = *sit;


        // now see if you have an updateTS so optimise whether you need
        // to get a new list of peer data
        RsGxsMsgUpdateItem* mui = NULL;

        ClientMsgMap::const_iterator cit = mClientMsgUpdateMap.find(peerId);

        if(cit != mClientMsgUpdateMap.end())
            mui = cit->second;

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peerId) << "  syncing messages with peer " << peerId << std::endl;
#endif

        GrpMetaMap::const_iterator mmit = toRequest.begin();
        for(; mmit != toRequest.end(); ++mmit)
        {
            const RsGxsGrpMetaData* meta = mmit->second;
            const RsGxsGroupId& grpId = mmit->first;

            if(!checkCanRecvMsgFromPeer(peerId, *meta))
                continue;

            // On default, the info has never been received so the TS is 0, meaning the peer has sent that it had no information.
            
            uint32_t updateTS = 0;

            if(mui)
            {
                std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator cit2 = mui->msgUpdateInfos.find(grpId);

                if(cit2 != mui->msgUpdateInfos.end())
                    updateTS = cit2->second.time_stamp;
            }

            RsNxsSyncMsgReqItem* msg = new RsNxsSyncMsgReqItem(mServType);
            msg->clear();
            msg->PeerId(peerId);
            msg->grpId = grpId;
            msg->updateTS = updateTS;

            //NxsBandwidthRecorder::recordEvent(mServType,msg) ;

            //if(RSRandom::random_f32() < sending_probability)
            //{
            
            sendItem(msg);
            
#ifdef NXS_NET_DEBUG_5
		GXSNETDEBUG_PG(*sit,grpId) << "Service "<< std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << "  sending global message TS of peer id: " << *sit << " ts=" << nice_time_stamp(time(NULL),updateTS) << " (secs ago) for group " << grpId << " to himself" << std::endl;
#endif
            //}
            //else
            //{
            //    delete msg ;
            //#ifdef NXS_NET_DEBUG_0
            //    GXSNETDEBUG_PG(*sit,grpId) << "    cancel RsNxsSyncMsg req (last local update TS for group+peer) for grpId=" << grpId << " to peer " << *sit << ": not enough bandwidth." << std::endl;
            //#endif
            //}
        }
    }

#endif
}

void RsGxsNetService::syncGrpStatistics()
{
    RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_6
    GXSNETDEBUG___<< "Sync-ing group statistics." << std::endl;
#endif
    RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMeta;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    std::set<RsPeerId> online_peers;
    mNetMgr->getOnlineList(mServiceInfo.mServiceType, online_peers);

    // Go through group statistics and groups without information are re-requested to random peers selected
    // among the ones who provided the group info.

    time_t now = time(NULL) ;
    
    for(std::map<RsGxsGroupId,RsGxsGrpMetaData*>::const_iterator it(grpMeta.begin());it!=grpMeta.end();++it)
    {
	    RsGroupNetworkStatsRecord& rec(mGroupNetworkStats[it->first]) ;
#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG__G(it->first) << "    group " << it->first ;
#endif

	    if(rec.update_TS + GROUP_STATS_UPDATE_DELAY < now && rec.suppliers.size() > 0)
	    {
#ifdef NXS_NET_DEBUG_6
		    GXSNETDEBUG__G(it->first) << " needs update. Randomly asking to some friends" << std::endl;
#endif
            // randomly select GROUP_STATS_UPDATE_NB_PEERS friends among the suppliers of this group

		    uint32_t n = RSRandom::random_u32() % rec.suppliers.size() ;

		    std::set<RsPeerId>::const_iterator rit = rec.suppliers.begin();
		    for(uint32_t i=0;i<n;++i)
			    ++rit ;

		    for(uint32_t i=0;i<std::min(rec.suppliers.size(),(size_t)GROUP_STATS_UPDATE_NB_PEERS);++i)
                    {
                // we started at a random position in the set, wrap around if the end is reached
                if(rit == rec.suppliers.end())
                    rit = rec.suppliers.begin() ;

			RsPeerId peer_id = *rit ;
			++rit ;
                        
                	if(online_peers.find(peer_id) != online_peers.end())	// check that the peer is online
			{
#ifdef NXS_NET_DEBUG_6
				GXSNETDEBUG_PG(peer_id,it->first) << "  asking friend " << peer_id << " for an update of stats for group " << it->first << std::endl;
#endif

				RsNxsSyncGrpStatsItem *grs = new RsNxsSyncGrpStatsItem(mServType) ;

				grs->request_type = RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_REQUEST ;

				grs->grpId = it->first ;
				grs->PeerId(peer_id) ;

				sendItem(grs) ;
			}
                    }
	    }
#ifdef NXS_NET_DEBUG_6
	    else
		    GXSNETDEBUG__G(it->first) << " up to date." << std::endl;
#endif
    }
}

void RsGxsNetService::handleRecvSyncGrpStatistics(RsNxsSyncGrpStatsItem *grs)
{
    if(grs->request_type == RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_REQUEST)
    {
#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "Received Grp update stats Request for group " << grs->grpId << " from friend " << grs->PeerId() << std::endl;
#endif
	RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetas;
	    grpMetas[grs->grpId] = NULL;

	    mDataStore->retrieveGxsGrpMetaData(grpMetas);

	    RsGxsGrpMetaData* grpMeta = grpMetas[grs->grpId];

	if(grpMeta == NULL)
            {
#ifdef NXS_NET_DEBUG_6
		GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  Group is unknown. Not reponding." << std::endl;
#endif
	    return ;
            }
            
	    // check if we're subscribed or not

	    if(! (grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
	    {
#ifdef NXS_NET_DEBUG_6
		    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  Group is not subscribed. Not reponding." << std::endl;
#endif
		    return ;
	    }

	    // now count available messages

	    GxsMsgReq reqIds;
	    reqIds[grs->grpId] = std::vector<RsGxsMessageId>();
	    GxsMsgMetaResult result;

#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  retrieving message information." << std::endl;
#endif
	    mDataStore->retrieveGxsMsgMetaData(reqIds, result);

	    const std::vector<RsGxsMsgMetaData*>& vec(result[grs->grpId]) ;

	    if(vec.empty())	// that means we don't have any, or there isn't any, but since the default is always 0, no need to send.
		    return ;

	    RsNxsSyncGrpStatsItem *grs_resp = new RsNxsSyncGrpStatsItem(mServType) ;
	    grs_resp->request_type = RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_RESPONSE ;
	    grs_resp->number_of_posts = vec.size();
	    grs_resp->grpId = grs->grpId;
	    grs_resp->PeerId(grs->PeerId()) ;

	    grs_resp->last_post_TS = 0 ;

	    for(uint32_t i=0;i<vec.size();++i)
	    {
		    if(grs_resp->last_post_TS < vec[i]->mPublishTs)
			    grs_resp->last_post_TS = vec[i]->mPublishTs;

		    delete vec[i] ;
	    }
#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  sending back statistics item with " << vec.size() << " elements." << std::endl;
#endif

	    sendItem(grs_resp) ;
    }
    else if(grs->request_type == RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_RESPONSE)
    {
#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "Received Grp update stats item from peer " << grs->PeerId() << " for group " << grs->grpId << ", reporting " << grs->number_of_posts << " posts." << std::endl;
#endif
	    RS_STACK_MUTEX(mNxsMutex) ;
	    RsGroupNetworkStatsRecord& rec(mGroupNetworkStats[grs->grpId]) ;

	    int32_t old_count = rec.max_visible_count ;
	    int32_t old_suppliers_count = rec.suppliers.size() ;
        
	    rec.suppliers.insert(grs->PeerId()) ;
	    rec.max_visible_count = std::max(rec.max_visible_count,grs->number_of_posts) ;
	    rec.update_TS = time(NULL) ;
        
	    if (old_count != rec.max_visible_count || old_suppliers_count != rec.suppliers.size())
		    mObserver->notifyChangedGroupStats(grs->grpId);
    }
    else
        std::cerr << "(EE) RsGxsNetService::handleRecvSyncGrpStatistics(): unknown item type " << grs->request_type << " found. This is a bug." << std::endl;
}

void RsGxsNetService::subscribeStatusChanged(const RsGxsGroupId& grpId,bool subscribed)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    if(!subscribed)
        return ;

    // When we subscribe, we reset the time stamps, so that the entire group list
    // gets requested once again, for a proper update.

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG__G(grpId) << "Changing subscribe status for grp " << grpId << " to " << subscribed << ": reseting all server msg time stamps for this group, and server global TS." << std::endl;
#endif
    std::map<RsGxsGroupId,RsGxsServerMsgUpdateItem*>::iterator it = mServerMsgUpdateMap.find(grpId) ;
    
    if(mServerMsgUpdateMap.end() == it)
    {
        RsGxsServerMsgUpdateItem *item = new RsGxsServerMsgUpdateItem(mServType) ;
        item->grpId = grpId ;
        item->msgUpdateTS = 0 ;
    }
    else
        it->second->msgUpdateTS = 0 ; // reset!
    
    // We also update mGrpServerUpdateItem so as to trigger a new grp list exchange with friends (friends will send their known ClientTS which
    // will be lower than our own grpUpdateTS, triggering our sending of the new subscribed grp list.
    
    if(mGrpServerUpdateItem == NULL)
	    mGrpServerUpdateItem = new RsGxsServerGrpUpdateItem(mServType);
    
    mGrpServerUpdateItem->grpUpdateTS = time(NULL) ;
}

bool RsGxsNetService::fragmentMsg(RsNxsMsg& msg, MsgFragments& msgFragments) const
{
	// first determine how many fragments
	uint32_t msgSize = msg.msg.TlvSize();
	uint32_t dataLeft = msgSize;
	uint8_t nFragments = ceil(float(msgSize)/FRAGMENT_SIZE);
    
    	RsTemporaryMemory buffer(FRAGMENT_SIZE);
	int currPos = 0;


	for(uint8_t i=0; i < nFragments; ++i)
	{
		RsNxsMsg* msgFrag = new RsNxsMsg(mServType);
		msgFrag->grpId = msg.grpId;
		msgFrag->msgId = msg.msgId;
		msgFrag->meta = msg.meta;
		msgFrag->transactionNumber = msg.transactionNumber;
		msgFrag->pos = i;
		msgFrag->PeerId(msg.PeerId());
		msgFrag->count = nFragments;
		uint32_t fragSize = std::min(dataLeft, FRAGMENT_SIZE);

		memcpy(buffer, ((char*)msg.msg.bin_data) + currPos, fragSize);
		msgFrag->msg.setBinData(buffer, fragSize);

		currPos += fragSize;
		dataLeft -= fragSize;
		msgFragments.push_back(msgFrag);
	}

	return true;
}

bool RsGxsNetService::fragmentGrp(RsNxsGrp& grp, GrpFragments& grpFragments) const
{
	// first determine how many fragments
	uint32_t grpSize = grp.grp.TlvSize();
	uint32_t dataLeft = grpSize;
	uint8_t nFragments = ceil(float(grpSize)/FRAGMENT_SIZE);
	char buffer[FRAGMENT_SIZE];
	int currPos = 0;


	for(uint8_t i=0; i < nFragments; ++i)
	{
		RsNxsGrp* grpFrag = new RsNxsGrp(mServType);
		grpFrag->grpId = grp.grpId;
		grpFrag->meta = grp.meta;
		grpFrag->pos = i;
		grpFrag->count = nFragments;
		uint32_t fragSize = std::min(dataLeft, FRAGMENT_SIZE);

		memcpy(buffer, ((char*)grp.grp.bin_data) + currPos, fragSize);
		grpFrag->grp.setBinData(buffer, fragSize);

		currPos += fragSize;
		dataLeft -= fragSize;
		grpFragments.push_back(grpFrag);
	}

	return true;
}

RsNxsMsg* RsGxsNetService::deFragmentMsg(MsgFragments& msgFragments) const
{
	if(msgFragments.empty()) return NULL;

	// if there is only one fragment with a count 1 or less then
	// the fragment is the msg
	if(msgFragments.size() == 1)
	{
		RsNxsMsg* m  = msgFragments.front();
        
		if(m->count > 1)	// normally mcount should be exactly 1, but if not initialised (old versions) it's going to be 0
            	{
            		// delete everything
            		std::cerr << "(WW) Cannot deFragment message set. m->count=" << m->count << ", but msgFragments.size()=" << msgFragments.size() << ". Incomplete? Dropping all." << std::endl;
            	
            		for(uint32_t i=0;i<msgFragments.size();++i)
                        	delete msgFragments[i] ;
                    
                    	msgFragments.clear();
			return NULL;
            	}
		else
            	{
            		// single piece. No need to say anything. Just return it.
            
                    	msgFragments.clear();
			return m;
            	}
	}

	// first determine total size for binary data
	MsgFragments::iterator mit = msgFragments.begin();
	uint32_t datSize = 0;

	for(; mit != msgFragments.end(); ++mit)
		datSize += (*mit)->msg.bin_len;

    	RsTemporaryMemory data(datSize) ;
        
        if(!data)
        {
	    for(uint32_t i=0;i<msgFragments.size();++i)
		    delete msgFragments[i] ;
                    
	    msgFragments.clear();
            return NULL ;
        }
    
	uint32_t currPos = 0;

        std::cerr << "(II) deFragmenting long message of size " << datSize << ", from " << msgFragments.size() << " pieces." << std::endl;
        
	for(mit = msgFragments.begin(); mit != msgFragments.end(); ++mit)
	{
		RsNxsMsg* msg = *mit;
		memcpy(data + (currPos), msg->msg.bin_data, msg->msg.bin_len);
		currPos += msg->msg.bin_len;
	}

	RsNxsMsg* msg = new RsNxsMsg(mServType);
	const RsNxsMsg& m = *(*(msgFragments.begin()));
	msg->msg.setBinData(data, datSize);
	msg->msgId = m.msgId;
	msg->grpId = m.grpId;
	msg->transactionNumber = m.transactionNumber;
	msg->meta = m.meta;

        // now clean!
	for(uint32_t i=0;i<msgFragments.size();++i)
		delete msgFragments[i] ;
                    
	msgFragments.clear();
    
	return msg;
}

// This is unused apparently, since groups are never large. Anyway, we keep it in case we need it.

RsNxsGrp* RsGxsNetService::deFragmentGrp(GrpFragments& grpFragments) const
{
	if(grpFragments.empty()) return NULL;

	// first determine total size for binary data
	GrpFragments::iterator mit = grpFragments.begin();
	uint32_t datSize = 0;

	for(; mit != grpFragments.end(); ++mit)
		datSize += (*mit)->grp.bin_len;

	char* data = new char[datSize];
	uint32_t currPos = 0;

	for(mit = grpFragments.begin(); mit != grpFragments.end(); ++mit)
	{
		RsNxsGrp* grp = *mit;
		memcpy(data + (currPos), grp->grp.bin_data, grp->grp.bin_len);
		currPos += grp->grp.bin_len;
	}

	RsNxsGrp* grp = new RsNxsGrp(mServType);
	const RsNxsGrp& g = *(*(grpFragments.begin()));
	grp->grp.setBinData(data, datSize);
	grp->grpId = g.grpId;
	grp->transactionNumber = g.transactionNumber;
	grp->meta = g.meta;

	delete[] data;

	return grp;
}

struct GrpFragCollate
{
    RsGxsGroupId mGrpId;
	GrpFragCollate(const RsGxsGroupId& grpId) : mGrpId(grpId){ }
	bool operator()(RsNxsGrp* grp) { return grp->grpId == mGrpId;}
};

void RsGxsNetService::locked_createTransactionFromPending( MsgRespPending* msgPend)
{
#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(msgPend->mPeerId) << "locked_createTransactionFromPending()" << std::endl;
#endif
	MsgAuthorV::const_iterator cit = msgPend->mMsgAuthV.begin();
	std::list<RsNxsItem*> reqList;
	uint32_t transN = locked_getTransactionId();
	for(; cit != msgPend->mMsgAuthV.end(); ++cit)
	{
		const MsgAuthEntry& entry = *cit;

		if(entry.mPassedVetting)
		{
			RsNxsSyncMsgItem* msgItem = new RsNxsSyncMsgItem(mServType);
			msgItem->grpId = entry.mGrpId;
			msgItem->msgId = entry.mMsgId;
			msgItem->authorId = entry.mAuthorId;
			msgItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
			msgItem->transactionNumber = transN;
			msgItem->PeerId(msgPend->mPeerId);
			reqList.push_back(msgItem);
		}
#ifdef NXS_NET_DEBUG_1
		else
			GXSNETDEBUG_PG(msgPend->mPeerId,entry.mGrpId) << "  entry failed vetting: grpId=" << entry.mGrpId << ", msgId=" << entry.mMsgId << ", peerId=" << msgPend->mPeerId << std::endl;
#endif
	}

	if(!reqList.empty())
		locked_pushMsgTransactionFromList(reqList, msgPend->mPeerId, transN) ;
#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(msgPend->mPeerId) << "  added " << reqList.size() << " items to transaction." << std::endl;
#endif
}

void RsGxsNetService::locked_createTransactionFromPending(GrpRespPending* grpPend)
{
#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(grpPend->mPeerId) << "locked_createTransactionFromPending() from peer " << grpPend->mPeerId << std::endl;
#endif
	GrpAuthorV::const_iterator cit = grpPend->mGrpAuthV.begin();
    std::list<RsNxsItem*> reqList;
	uint32_t transN = locked_getTransactionId();
	for(; cit != grpPend->mGrpAuthV.end(); ++cit)
	{
		const GrpAuthEntry& entry = *cit;

		if(entry.mPassedVetting)
		{
#ifdef NXS_NET_DEBUG_1
			GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGrpId) << "  entry Group Id: " << entry.mGrpId << " PASSED" << std::endl;
#endif
			RsNxsSyncGrpItem* msgItem = new RsNxsSyncGrpItem(mServType);
			msgItem->grpId = entry.mGrpId;
			msgItem->authorId = entry.mAuthorId;
			msgItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
			msgItem->transactionNumber = transN;
			msgItem->PeerId(grpPend->mPeerId);
			reqList.push_back(msgItem);
		}
#ifdef NXS_NET_DEBUG_1
        else
            GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGrpId) << "  entry failed vetting: grpId=" << entry.mGrpId << ", peerId=" << grpPend->mPeerId << std::endl;
#endif
	}

	if(!reqList.empty())
		locked_pushGrpTransactionFromList(reqList, grpPend->mPeerId, transN);
}


void RsGxsNetService::locked_createTransactionFromPending(GrpCircleIdRequestVetting* grpPend)
{
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(grpPend->mPeerId) << "locked_createTransactionFromPending(GrpCircleIdReq)" << std::endl;
#endif
    std::vector<GrpIdCircleVet>::iterator cit = grpPend->mGrpCircleV.begin();
	uint32_t transN = locked_getTransactionId();
	std::list<RsNxsItem*> itemL;
	for(; cit != grpPend->mGrpCircleV.end(); ++cit)
	{
		const GrpIdCircleVet& entry = *cit;
		if(entry.mCleared)
		{
#ifdef NXS_NET_DEBUG_1
			GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGroupId)        << "  Group Id: " << entry.mGroupId << " PASSED" << std::endl;
#endif
			RsNxsSyncGrpItem* gItem = new RsNxsSyncGrpItem(mServType);
			gItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
			gItem->grpId = entry.mGroupId;
			gItem->publishTs = 0;
			gItem->PeerId(grpPend->mPeerId);
			gItem->transactionNumber = transN;
			gItem->authorId = entry.mAuthorId;
			// why it authorId not set here???
			itemL.push_back(gItem);
		}
#ifdef NXS_NET_DEBUG_1
        else
            GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGroupId) << "  Group Id: " << entry.mGroupId << " FAILED" << std::endl;
#endif
    }

	if(!itemL.empty())
		locked_pushGrpRespFromList(itemL, grpPend->mPeerId, transN);
}

void RsGxsNetService::locked_createTransactionFromPending(MsgCircleIdsRequestVetting* msgPend)
{
	std::vector<MsgIdCircleVet>::iterator vit = msgPend->mMsgs.begin();
	std::list<RsNxsItem*> itemL;

	uint32_t transN = locked_getTransactionId();
    	RsGxsGroupId grp_id ;

	for(; vit != msgPend->mMsgs.end(); ++vit)
	{
		MsgIdCircleVet& mic = *vit;
		RsNxsSyncMsgItem* mItem = new RsNxsSyncMsgItem(mServType);
		mItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
		mItem->grpId = msgPend->mGrpId;
		mItem->msgId = mic.mMsgId;
		mItem->authorId = mic.mAuthorId;
		mItem->PeerId(msgPend->mPeerId);
		mItem->transactionNumber =  transN;
		itemL.push_back(mItem);
       
        	grp_id = msgPend->mGrpId ;
	}

	if(!itemL.empty())
		locked_pushMsgRespFromList(itemL, msgPend->mPeerId,grp_id, transN,msgPend->mCircleId);
}

/*bool RsGxsNetService::locked_canReceive(const RsGxsGrpMetaData * const grpMeta
                                      , const RsPeerId& peerId )
{

	double timeDelta = 0.2;

	if(grpMeta->mCircleType == GXS_CIRCLE_TYPE_EXTERNAL) {
		int i=0;
		mCircles->loadCircle(grpMeta->mCircleId);

		// check 5 times at most
		// spin for 1 second at most
		while(i < 5) {

			if(mCircles->isLoaded(grpMeta->mCircleId)) {
				const RsPgpId& pgpId = mPgpUtils->getPGPId(peerId);
				return mCircles->canSend(grpMeta->mCircleId, pgpId);
			}//if(mCircles->isLoaded(grpMeta->mCircleId))

			usleep((int) (timeDelta * 1000 * 1000));// timeDelta sec
			i++;
		}//while(i < 5)

	} else {//if(grpMeta->mCircleType == GXS_CIRCLE_TYPE_EXTERNAL)
		return true;
	}//else (grpMeta->mCircleType == GXS_CIRCLE_TYPE_EXTERNAL)

	return false;
}*/

void RsGxsNetService::collateGrpFragments(GrpFragments fragments,
		std::map<RsGxsGroupId, GrpFragments>& partFragments) const
{
	// get all unique grpIds;
	GrpFragments::iterator vit = fragments.begin();
	std::set<RsGxsGroupId> grpIds;

	for(; vit != fragments.end(); ++vit)
		grpIds.insert( (*vit)->grpId );

	std::set<RsGxsGroupId>::iterator sit = grpIds.begin();

	for(; sit != grpIds.end(); ++sit)
	{
		const RsGxsGroupId& grpId = *sit;
		GrpFragments::iterator bound = std::partition(
					fragments.begin(), fragments.end(),
					GrpFragCollate(grpId));

		// something will always be found for a group id
		for(vit = fragments.begin(); vit != bound; )
		{
			partFragments[grpId].push_back(*vit);
			vit = fragments.erase(vit);
		}

		GrpFragments& f = partFragments[grpId];
		RsNxsGrp* grp = *(f.begin());

		// if counts of fragments is incorrect remove
		// from coalescion
		if(grp->count != f.size())
		{
			GrpFragments::iterator vit2 = f.begin();

			for(; vit2 != f.end(); ++vit2)
				delete *vit2;

			partFragments.erase(grpId);
		}
	}

	fragments.clear();
}

struct MsgFragCollate
{
	RsGxsMessageId mMsgId;
	MsgFragCollate(const RsGxsMessageId& msgId) : mMsgId(msgId){ }
	bool operator()(RsNxsMsg* msg) { return msg->msgId == mMsgId;}
};

void RsGxsNetService::collateMsgFragments(MsgFragments& fragments, std::map<RsGxsMessageId, MsgFragments>& partFragments) const
{
	// get all unique message Ids;
	MsgFragments::iterator vit = fragments.begin();
	std::set<RsGxsMessageId> msgIds;

	for(; vit != fragments.end(); ++vit)
		msgIds.insert( (*vit)->msgId );


	std::set<RsGxsMessageId>::iterator sit = msgIds.begin();

	for(; sit != msgIds.end(); ++sit)
	{
		const RsGxsMessageId& msgId = *sit;
		MsgFragments::iterator bound = std::partition(
					fragments.begin(), fragments.end(),
					MsgFragCollate(msgId));

		// something will always be found for a group id
		for(vit = fragments.begin(); vit != bound; ++vit )
		{
			partFragments[msgId].push_back(*vit);
		}

		fragments.erase(fragments.begin(), bound);
		MsgFragments& f = partFragments[msgId];
		RsNxsMsg* msg = *(f.begin());

		// if counts of fragments is incorrect remove
		// from coalescion
		if(msg->count != f.size())
		{
			MsgFragments::iterator vit2 = f.begin();

			for(; vit2 != f.end(); ++vit2)
				delete *vit2;

			partFragments.erase(msgId);
		}
	}

	fragments.clear();
}

class StoreHere
{
public:

    StoreHere(RsGxsNetService::ClientGrpMap& cgm, RsGxsNetService::ClientMsgMap& cmm, RsGxsNetService::ServerMsgMap& smm, RsGxsServerGrpUpdateItem*& sgm)
            : mClientGrpMap(cgm), mClientMsgMap(cmm), mServerMsgMap(smm), mServerGrpUpdateItem(sgm)
    {}

    void operator() (RsItem* item)
    {
        RsGxsMsgUpdateItem* mui;
        RsGxsGrpUpdateItem* gui;
        RsGxsServerGrpUpdateItem* gsui;
        RsGxsServerMsgUpdateItem* msui;

        if((mui = dynamic_cast<RsGxsMsgUpdateItem*>(item)) != NULL)
            mClientMsgMap.insert(std::make_pair(mui->peerId, mui));
        else if((gui = dynamic_cast<RsGxsGrpUpdateItem*>(item)) != NULL)
            mClientGrpMap.insert(std::make_pair(gui->peerId, gui));
        else if((msui = dynamic_cast<RsGxsServerMsgUpdateItem*>(item)) != NULL)
            mServerMsgMap.insert(std::make_pair(msui->grpId, msui));
        else if((gsui = dynamic_cast<RsGxsServerGrpUpdateItem*>(item)) != NULL)
        {
            if(mServerGrpUpdateItem == NULL)
                mServerGrpUpdateItem = gsui;
            else
            {
                std::cerr << "Error! More than one server group update item exists!" << std::endl;
                delete gsui;
            }
        }
        else
        {
            std::cerr << "Type not expected!" << std::endl;
            delete item ;
        }
    }

private:

    RsGxsNetService::ClientGrpMap& mClientGrpMap;
    RsGxsNetService::ClientMsgMap& mClientMsgMap;
    RsGxsNetService::ServerMsgMap& mServerMsgMap;
    RsGxsServerGrpUpdateItem*& mServerGrpUpdateItem;

};

bool RsGxsNetService::loadList(std::list<RsItem *> &load)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    // The delete is done in StoreHere, if necessary

    std::for_each(load.begin(), load.end(), StoreHere(mClientGrpUpdateMap, mClientMsgUpdateMap, mServerMsgUpdateMap, mGrpServerUpdateItem));

    // We reset group statistics here. This is the best place since we know at this point which are all unsubscribed groups.
    
    time_t now = time(NULL);
    
    for(std::map<RsGxsGroupId,RsGroupNetworkStatsRecord>::iterator it(mGroupNetworkStats.begin());it!=mGroupNetworkStats.end();++it)
    {
		    // At each reload, we reset the count of visible messages. It will be rapidely restored to its real value from friends.

		    it->second.max_visible_count = 0; // std::max(it2->second.message_count,gnsr.max_visible_count) ;
            
            	    // the update time stamp is randomised so as not to ask all friends at once about group statistics.
            
		    it->second.update_TS = now - GROUP_STATS_UPDATE_DELAY + (RSRandom::random_u32()%(GROUP_STATS_UPDATE_DELAY/10)) ;

		    // Similarly, we remove all suppliers. 
		    // Actual suppliers will come back automatically.  

		    it->second.suppliers.clear() ;
    }

    return true;
}

#include <algorithm>

template <typename UpdateMap>
struct get_second : public std::unary_function<typename UpdateMap::value_type, RsItem*>
{
    RsItem* operator()(const typename UpdateMap::value_type& value) const
    {
        return value.second;
    }
};

bool RsGxsNetService::saveList(bool& cleanup, std::list<RsItem*>& save)
{
	RS_STACK_MUTEX(mNxsMutex) ;

    // hardcore templates
    std::transform(mClientGrpUpdateMap.begin(), mClientGrpUpdateMap.end(), std::back_inserter(save), get_second<ClientGrpMap>());
    std::transform(mClientMsgUpdateMap.begin(), mClientMsgUpdateMap.end(), std::back_inserter(save), get_second<ClientMsgMap>());
    std::transform(mServerMsgUpdateMap.begin(), mServerMsgUpdateMap.end(), std::back_inserter(save), get_second<ServerMsgMap>());

    save.push_back(mGrpServerUpdateItem);

    cleanup = false;
    return true;
}

RsSerialiser *RsGxsNetService::setupSerialiser()
{

    RsSerialiser *rss = new RsSerialiser;
    rss->addSerialType(new RsGxsUpdateSerialiser(mServType));

    return rss;
}

void RsGxsNetService::recvNxsItemQueue()
{
    RsItem *item ;

    while(NULL != (item=recvItem()))
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_P_(item->PeerId()) << "Received RsGxsNetService Item:" << (void*)item << " type=" << std::hex << item->PacketId() << std::dec << std::endl ;
#endif
        // RsNxsItem needs dynamic_cast, since they have derived siblings.
        //
        RsNxsItem *ni = dynamic_cast<RsNxsItem*>(item) ;
        if(ni != NULL)
        {
            // a live transaction has a non zero value
            if(ni->transactionNumber != 0)
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_P_(item->PeerId()) << "  recvNxsItemQueue() handlingTransaction, transN " << ni->transactionNumber << std::endl;
#endif

                if(!handleTransaction(ni))
                    delete ni;

                continue;
            }


            switch(ni->PacketSubType())
            {
            case RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM: handleRecvSyncGrpStatistics   (dynamic_cast<RsNxsSyncGrpStatsItem*>(ni)) ; break ;
            case RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM:   handleRecvSyncGroup           (dynamic_cast<RsNxsSyncGrpReqItem*>(ni)) ; break ;
            case RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM:   handleRecvSyncMessage         (dynamic_cast<RsNxsSyncMsgReqItem*>(ni)) ; break ;
            case RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM:handleRecvPublishKeys         (dynamic_cast<RsNxsGroupPublishKeyItem*>(ni)) ; break ;

            default:
                std::cerr << "Unhandled item subtype " << (uint32_t) ni->PacketSubType() << " in RsGxsNetService: " << std::endl; break;
            }
            delete item ;
        }
        else
        {
            std::cerr << "Not a RsNxsItem, deleting!" << std::endl;
            delete(item);
        }
    }
}


bool RsGxsNetService::handleTransaction(RsNxsItem* item)
{
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(item->PeerId()) << "handleTransaction(RsNxsItem) number=" << item->transactionNumber << std::endl;
#endif

    /*!
     * This attempts to handle a transaction
     * It first checks if this transaction id already exists
     * If it does then check this not a initiating transactions
     */

    RS_STACK_MUTEX(mNxsMutex) ;

    const RsPeerId& peer = item->PeerId();

    RsNxsTransacItem* transItem = dynamic_cast<RsNxsTransacItem*>(item);

    // if this is a RsNxsTransac item process
    if(transItem)
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_P_(item->PeerId()) << "  this is a RsNxsTransac item. callign process." << std::endl;
#endif
        return locked_processTransac(transItem);
    }


    // then this must be transaction content to be consumed
    // first check peer exist for transaction
    bool peerTransExists = mTransactions.find(peer) != mTransactions.end();

    // then check transaction exists

    NxsTransaction* tr = NULL;
    uint32_t transN = item->transactionNumber;

    if(peerTransExists)
    {
        TransactionIdMap& transMap = mTransactions[peer];

        if(transMap.find(transN) != transMap.end())
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_P_(item->PeerId()) << "  Consuming Transaction content, transN: " << item->transactionNumber << std::endl;
            GXSNETDEBUG_P_(item->PeerId()) << "  Consuming Transaction content, from Peer: " << item->PeerId() << std::endl;
#endif

            tr = transMap[transN];
            tr->mItems.push_back(item);

            return true;
        }
    }

    return false;
}

bool RsGxsNetService::locked_processTransac(RsNxsTransacItem *item)
{

	/*!
	 * To process the transaction item
	 * It can either be initiating a transaction
	 * or ending one that already exists
	 *
	 * For initiating an incoming transaction the peer
	 * and transaction item need not exists
	 * as the peer will be added and transaction number
	 * added thereafter
	 *
	 * For commencing/starting an outgoing transaction
	 * the transaction must exist already
	 *
	 * For ending a transaction the
	 */

	RsPeerId peer;

	// for outgoing transaction use own id
	if(item->transactFlag & (RsNxsTransacItem::FLAG_BEGIN_P2 | RsNxsTransacItem::FLAG_END_SUCCESS))
		peer = mOwnId;
	else
		peer = item->PeerId();

	uint32_t transN = item->transactionNumber;
	item->timestamp = time(NULL); // register time received
	NxsTransaction* tr = NULL;

#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(peer) << "locked_processTransac() " << std::endl;
	GXSNETDEBUG_P_(peer) << "  Received transaction item: " << transN << std::endl;
	GXSNETDEBUG_P_(peer) << "  With peer: " << item->PeerId() << std::endl;
	GXSNETDEBUG_P_(peer) << "  trans type: " << item->transactFlag << std::endl;
#endif

	bool peerTrExists = mTransactions.find(peer) != mTransactions.end();
	bool transExists = false;

    if(peerTrExists)
    {
		TransactionIdMap& transMap = mTransactions[peer];
		// record whether transaction exists already
		transExists = transMap.find(transN) != transMap.end();
	}

	// initiating an incoming transaction
    if(item->transactFlag & RsNxsTransacItem::FLAG_BEGIN_P1)
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_P_(peer) << "  initiating Incoming transaction." << std::endl;
#endif

        if(transExists)
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_P_(peer) << "  transaction already exist! ERROR" << std::endl;
#endif
            return false; // should not happen!
    }

        // create a transaction if the peer does not exist
        if(!peerTrExists)
            mTransactions[peer] = TransactionIdMap();

        TransactionIdMap& transMap = mTransactions[peer];


        // create new transaction
        tr = new NxsTransaction();
        transMap[transN] = tr;
        tr->mTransaction = item;
        tr->mTimeOut = item->timestamp + mTransactionTimeOut;
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "  Setting timeout of " << mTransactionTimeOut << " secs, which is " << tr->mTimeOut - time(NULL) << " secs from now." << std::endl;
#endif

        // note state as receiving, commencement item
        // is sent on next run() loop
        tr->mFlag = NxsTransaction::FLAG_STATE_STARTING;
        return true;
        // commencement item for outgoing transaction
    }
    else if(item->transactFlag & RsNxsTransacItem::FLAG_BEGIN_P2)
    {
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "  initiating outgoign transaction." << std::endl;
#endif
        // transaction must exist
        if(!peerTrExists || !transExists)
        {
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "  transaction does not exist. Cancelling!" << std::endl;
#endif

            return false;
        }


		// alter state so transaction content is sent on
		// next run() loop
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
		tr->mFlag = NxsTransaction::FLAG_STATE_SENDING;
        delete item;
        return true;
		// end transac item for outgoing transaction
    }
    else if(item->transactFlag & RsNxsTransacItem::FLAG_END_SUCCESS)
    {

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "  marking this transaction succeed" << std::endl;
#endif
        // transaction does not exist
        if(!peerTrExists || !transExists)
    {
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "  transaction does not exist. Cancelling!" << std::endl;
#endif
            return false;
    }

		// alter state so that transaction is removed
		// on next run() loop
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
        tr->mFlag = NxsTransaction::FLAG_STATE_COMPLETED;
        delete item;
        return true;
    }
    else
        return false;
}

void RsGxsNetService::data_tick()
{
    static const double timeDelta = 0.5;

        //Start waiting as nothing to do in runup
        usleep((int) (timeDelta * 1000 * 1000)); // timeDelta sec

        if(mUpdateCounter >= 120) // 60 seconds
        {
            updateServerSyncTS();
#ifdef TO_REMOVE
            updateClientSyncTS();
#endif
            mUpdateCounter = 1;
        }
        else
            mUpdateCounter++;

        if(mUpdateCounter % 20 == 0)	// dump the full shit every 20 secs
            debugDump() ;
            
        // process active transactions
        processTransactions();

        // process completed transactions
        processCompletedTransactions();

        // vetting of id and circle info
        runVetting();

        processExplicitGroupRequests();
}

void RsGxsNetService::debugDump()
{
#ifdef NXS_NET_DEBUG_0
	RS_STACK_MUTEX(mNxsMutex) ;
    time_t now = time(NULL) ;

    GXSNETDEBUG___<< "RsGxsNetService::debugDump():" << std::endl;

    if(mGrpServerUpdateItem != NULL)
	    GXSNETDEBUG___<< "  mGrpServerUpdateItem time stamp: " << nice_time_stamp(time(NULL) , mGrpServerUpdateItem->grpUpdateTS) << " (is the last local modification time over all groups of this service)" << std::endl;
    else
	    GXSNETDEBUG___<< "  mGrpServerUpdateItem time stamp: not inited yet (is the last local modification time over all groups of this service)" << std::endl;
    
    GXSNETDEBUG___<< "  mServerMsgUpdateMap: (is for each subscribed group, the last local modification time)" << std::endl;

    for(std::map<RsGxsGroupId,RsGxsServerMsgUpdateItem*>::const_iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();++it)
	    GXSNETDEBUG__G(it->first) << "    Grp:" << it->first << " last local modification (secs ago): " << nice_time_stamp(time(NULL),it->second->msgUpdateTS) << std::endl;

    GXSNETDEBUG___<< "  mClientGrpUpdateMap: (is for each friend, last modif time of group meta data at that friend, all groups included, sent by the friend himself)" << std::endl;

    for(std::map<RsPeerId,RsGxsGrpUpdateItem*>::const_iterator it(mClientGrpUpdateMap.begin());it!=mClientGrpUpdateMap.end();++it)
	    GXSNETDEBUG_P_(it->first) << "    From peer: " << it->first << " - last updated at peer (secs ago): " << nice_time_stamp(time(NULL),it->second->grpUpdateTS) << std::endl;

    GXSNETDEBUG___<< "  mClientMsgUpdateMap: (is for each friend, the modif time for each group (e.g. last message received), sent by the friend himself)" << std::endl;

    for(std::map<RsPeerId,RsGxsMsgUpdateItem*>::const_iterator it(mClientMsgUpdateMap.begin());it!=mClientMsgUpdateMap.end();++it)
    {
	    GXSNETDEBUG_P_(it->first) << "    From peer: " << it->first << std::endl;

	    for(std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator it2(it->second->msgUpdateInfos.begin());it2!=it->second->msgUpdateInfos.end();++it2)
		    GXSNETDEBUG_PG(it->first,it2->first) << "      group " << it2->first << " - last updated at peer (secs ago): " << nice_time_stamp(time(NULL),it2->second.time_stamp) << ". Message count=" << it2->second.message_count << std::endl;
    }            
    
    GXSNETDEBUG___<< "  List of rejected message ids: " << mRejectedMessages.size() << std::endl;
#endif
}

#ifdef TO_REMOVE
// This method is normally not needed, but we use it to correct possible inconsistencies in the updte time stamps
// on the client side.

void RsGxsNetService::updateClientSyncTS()
{
    RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___<< "updateClientSyncTS(): checking last modification time stamps of local data w.r.t. client's modification times" << std::endl;
#endif

    if(mGrpServerUpdateItem == NULL)
	    mGrpServerUpdateItem = new RsGxsServerGrpUpdateItem(mServType);

    for(ClientGrpMap::iterator it = mClientGrpUpdateMap.begin();it!=mClientGrpUpdateMap.end();++it)
	    if(it->second->grpUpdateTS > SECURITY_DELAY_TO_FORCE_CLIENT_REUPDATE + mGrpServerUpdateItem->grpUpdateTS)
	    {
#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(it->first) << "  last global client GRP modification time known for peer (" << nice_time_stamp(time(NULL),it->second->grpUpdateTS) << " is quite more recent than our own server modification time (" << nice_time_stamp(time(NULL),mGrpServerUpdateItem->grpUpdateTS) << ". Forcing update! " << std::endl;
#endif
		    it->second->grpUpdateTS = 0 ;
	    }
    
    for(ClientMsgMap::iterator it = mClientMsgUpdateMap.begin();it!=mClientMsgUpdateMap.end();++it)
    	for(std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::iterator it2 = it->second->msgUpdateInfos.begin();it2!=it->second->msgUpdateInfos.end();++it2)
	{
		std::map<RsGxsGroupId,RsGxsServerMsgUpdateItem*>::const_iterator mmit = mServerMsgUpdateMap.find(it2->first) ;

		if(mmit != mServerMsgUpdateMap.end() && it2->second.time_stamp > SECURITY_DELAY_TO_FORCE_CLIENT_REUPDATE + mmit->second->msgUpdateTS)
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG_PG(it->first,it2->first) << "  last group msg modification time known for peer (" << nice_time_stamp(time(NULL),it2->second.time_stamp) << " and group " << it2->first << " is quite more recent than our own server modification time (" << nice_time_stamp(time(NULL),mmit->second->msgUpdateTS) << ". Forcing update! " << std::endl;
#endif
            		it2->second.time_stamp = 0 ;
		}
	}
}
#endif

void RsGxsNetService::updateServerSyncTS()
{
	RS_STACK_MUTEX(mNxsMutex) ;

	RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> gxsMap;

#ifdef NXS_NET_DEBUG_0
    	GXSNETDEBUG___<< "updateServerSyncTS(): updating last modification time stamp of local data." << std::endl;
#endif
    	
	// retrieve all grps and update TS
	mDataStore->retrieveGxsGrpMetaData(gxsMap);

#ifdef TO_REMOVE
   	// (cyril) This code is removed because it is inconsistent: the list of grps does not need to be updated when 
    	// new posts arrive. The two (grp list and msg list) are handled independently.
    
	// as a grp list server also note this is the latest item you have
	if(mGrpServerUpdateItem == NULL)
		mGrpServerUpdateItem = new RsGxsServerGrpUpdateItem(mServType);

    	// First reset it. That's important because it will re-compute correct TS in case
    	// we have unsubscribed a group.
    
	mGrpServerUpdateItem->grpUpdateTS = 0 ;
#endif
	bool change = false;

    	// then remove from mServerMsgUpdateMap, all items that are not in the group list!
    
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "  cleaning server map of groups with no data:" << std::endl;
#endif
                                  
    for(std::map<RsGxsGroupId, RsGxsServerMsgUpdateItem*>::iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();)
	    if(gxsMap.find(it->first) == gxsMap.end())
	    {
		    // not found! Removing server update info for this group

#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG__G(it->first) << "    removing server update info for group " << it->first << std::endl;
#endif
		    std::map<RsGxsGroupId, RsGxsServerMsgUpdateItem*>::iterator tmp(it) ;
		    ++tmp ;
		    mServerMsgUpdateMap.erase(it) ;
		    it = tmp ;
	    }
	    else
		    ++it;

#ifdef NXS_NET_DEBUG_0
    	if(gxsMap.empty())
            GXSNETDEBUG___<< "  database seems to be empty. The modification timestamp will be reset." << std::endl;
#endif
    	// finally, update timestamps.
        
	for(std::map<RsGxsGroupId, RsGxsGrpMetaData*>::const_iterator mit = gxsMap.begin();mit != gxsMap.end(); ++mit)
	{
		const RsGxsGroupId& grpId = mit->first;
		const RsGxsGrpMetaData* grpMeta = mit->second;
		ServerMsgMap::iterator mapIT = mServerMsgUpdateMap.find(grpId);
		RsGxsServerMsgUpdateItem* msui = NULL;

#ifdef TO_REMOVE
		// That accounts for modification of the meta data.

		if(mGrpServerUpdateItem->grpUpdateTS < grpMeta->mPublishTs)
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG__G(grpId) << "  publish time stamp of group " << grpId << " has changed to " << time(NULL)-grpMeta->mPublishTs << " secs ago. updating!" << std::endl;
#endif
			mGrpServerUpdateItem->grpUpdateTS = grpMeta->mPublishTs;
		}
#endif

		if(mapIT == mServerMsgUpdateMap.end())
		{
			msui = new RsGxsServerMsgUpdateItem(mServType);
			msui->grpId = grpMeta->mGroupId;
            
			mServerMsgUpdateMap.insert(std::make_pair(msui->grpId, msui));
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG__G(grpId) << "  created new entry for group " << grpId << std::endl;
#endif
		}
	else
			msui = mapIT->second;

		if(grpMeta->mLastPost > msui->msgUpdateTS )
		{
			change = true;
			msui->msgUpdateTS = grpMeta->mLastPost;
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG__G(grpId) << "  updated msgUpdateTS to last post = " << time(NULL) - grpMeta->mLastPost << " secs ago for group "<< grpId << std::endl;
#endif
		}

#ifdef TO_REMOVE
		// This might be very inefficient with time. This is needed because an old message might have been received, so the last modification time
        	// needs to account for this so that a friend who hasn't 
        
		if(mGrpServerUpdateItem->grpUpdateTS < grpMeta->mRecvTS)
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG__G(grpId) << "  updated msgUpdateTS to last RecvTS = " << time(NULL) - grpMeta->mRecvTS << " secs ago for group "<< grpId << std::endl;
#endif
			mGrpServerUpdateItem->grpUpdateTS = grpMeta->mRecvTS;
			change = true;
		}
#endif
	}
    
	// actual change in config settings, then save configuration
	if(change)
		IndicateConfigChanged();
}
bool RsGxsNetService::locked_checkTransacTimedOut(NxsTransaction* tr)
{
   return tr->mTimeOut < ((uint32_t) time(NULL));
}

void RsGxsNetService::processTransactions()
{
#ifdef NXS_NET_DEBUG_1
    if(!mTransactions.empty())
        GXSNETDEBUG___ << "processTransactions()" << std::endl;
#endif
    RS_STACK_MUTEX(mNxsMutex) ;

    TransactionsPeerMap::iterator mit = mTransactions.begin();

    for(; mit != mTransactions.end(); ++mit)
    {
        TransactionIdMap& transMap = mit->second;
        TransactionIdMap::iterator mmit = transMap.begin(),  mmit_end = transMap.end();

	if(mmit == mmit_end)	// no waiting transactions for this peer
	    continue ;
    
#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(mit->first) << "  peerId=" << mit->first << std::endl;
#endif
	// transaction to be removed
        std::list<uint32_t> toRemove;

        /*!
         * Transactions owned by peer
         */
        if(mit->first == mOwnId)
        {
            for(; mmit != mmit_end; ++mmit)
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_P_(mit->first) << "    type: outgoing " << std::endl;
                GXSNETDEBUG_P_(mit->first) << "    transN = " << mmit->second->mTransaction->transactionNumber << std::endl;
#endif
                NxsTransaction* tr = mmit->second;
                uint16_t flag = tr->mFlag;
                std::list<RsNxsItem*>::iterator lit, lit_end;
                uint32_t transN = tr->mTransaction->transactionNumber;

                // first check transaction has not expired
                if(locked_checkTransacTimedOut(tr))
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    timeout! " << std::endl;
                    GXSNETDEBUG_P_(mit->first) << std::dec ;
                    int total_transaction_time = (int)time(NULL) - (tr->mTimeOut - mTransactionTimeOut) ;
                    GXSNETDEBUG_P_(mit->first) << "    Outgoing Transaction has failed, tranN: " << transN << ", Peer: " << mit->first ;
                    GXSNETDEBUG_P_(mit->first) << ", age: " << total_transaction_time << ", nItems=" << tr->mTransaction->nItems << ". tr->mTimeOut = " << tr->mTimeOut << ", now = " << (uint32_t) time(NULL) << std::endl;
#endif

                    tr->mFlag = NxsTransaction::FLAG_STATE_FAILED;
                    toRemove.push_back(transN);
                    mComplTransactions.push_back(tr);
                    continue;
                }
#ifdef NXS_NET_DEBUG_1
                else
                    GXSNETDEBUG_P_(mit->first) << "    still on time." << std::endl;
#endif

                // send items requested
                if(flag & NxsTransaction::FLAG_STATE_SENDING)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first)<< "     Sending Transaction content, transN: " << transN << " with peer: " << tr->mTransaction->PeerId() << std::endl;
#endif
                    lit = tr->mItems.begin();
                    lit_end = tr->mItems.end();

                    for(; lit != lit_end; ++lit){
                        sendItem(*lit);
                    }

                    tr->mItems.clear(); // clear so they don't get deleted in trans cleaning
                    tr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

                }
                else if(flag & NxsTransaction::FLAG_STATE_WAITING_CONFIRM)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first)<< "     Waiting confirm! returning." << std::endl;
#endif
                    continue;

                }
                else if(flag & NxsTransaction::FLAG_STATE_COMPLETED)
                {

#ifdef NXS_NET_DEBUG_1
                    int total_transaction_time = (int)time(NULL) - (tr->mTimeOut - mTransactionTimeOut) ;

                    GXSNETDEBUG_P_(mit->first)<< "    Outgoing completed " << tr->mTransaction->nItems << " items transaction in " << total_transaction_time << " seconds." << std::endl;
#endif
                    // move to completed transactions
                    toRemove.push_back(transN);
                    mComplTransactions.push_back(tr);
                }else{

#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first)<< "    Unknown flag for active transaction, transN: " << transN << ", Peer: " << mit->first<< std::endl;
#endif

                    toRemove.push_back(transN);
                    tr->mFlag = NxsTransaction::FLAG_STATE_FAILED;
                    mComplTransactions.push_back(tr);
                }
            }

        }else{

            /*!
             * Essentially these are incoming transactions
             * Several states are dealth with
             * Receiving: waiting to receive items from peer's transaction
             * and checking if all have been received
             * Completed: remove transaction from active and tell peer
             * involved in transaction
             * Starting: this is a new transaction and need to teell peer
             * involved in transaction
             */

            for(; mmit != mmit_end; ++mmit){

                NxsTransaction* tr = mmit->second;
                uint16_t flag = tr->mFlag;
                uint32_t transN = tr->mTransaction->transactionNumber;

#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_P_(mit->first) << "    type: incoming " << std::endl;
                GXSNETDEBUG_P_(mit->first) << "    transN = " << mmit->second->mTransaction->transactionNumber << std::endl;
#endif
                // first check transaction has not expired
                if(locked_checkTransacTimedOut(tr))
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    timeout!" << std::endl;
                    GXSNETDEBUG_P_(mit->first) << std::dec ;
                    int total_transaction_time = (int)time(NULL) - (tr->mTimeOut - mTransactionTimeOut) ;
                    GXSNETDEBUG_P_(mit->first) << "    Incoming Transaction has failed, tranN: " << transN << ", Peer: " << mit->first ;
                    GXSNETDEBUG_P_(mit->first) << ", age: " << total_transaction_time << ", nItems=" << tr->mTransaction->nItems << ". tr->mTimeOut = " << tr->mTimeOut << ", now = " << (uint32_t) time(NULL) << std::endl;
#endif

                    tr->mFlag = NxsTransaction::FLAG_STATE_FAILED;
                    toRemove.push_back(transN);
                    mComplTransactions.push_back(tr);
                    continue;
                }

                if(flag & NxsTransaction::FLAG_STATE_RECEIVING)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    received " << tr->mItems.size() << " item over a total of " << tr->mTransaction->nItems << std::endl;
#endif

                    // if the number it item received equal that indicated
                    // then transaction is marked as completed
                    // to be moved to complete transations
                    // check if done
                    if(tr->mItems.size() == tr->mTransaction->nItems)
                    {
                        tr->mFlag = NxsTransaction::FLAG_STATE_COMPLETED;
#ifdef NXS_NET_DEBUG_1
                        GXSNETDEBUG_P_(mit->first) << "    completed!" << std::endl;
#endif
                    }

                }else if(flag & NxsTransaction::FLAG_STATE_COMPLETED)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    transaction is completed!" << std::endl;
                    GXSNETDEBUG_P_(mit->first) << "    sending success!" << std::endl;
#endif

                    // send completion msg
                    RsNxsTransacItem* trans = new RsNxsTransacItem(mServType);
                    trans->clear();
                    trans->transactFlag = RsNxsTransacItem::FLAG_END_SUCCESS;
                    trans->transactionNumber = transN;
                    trans->PeerId(tr->mTransaction->PeerId());
                    sendItem(trans);

                    // move to completed transactions
                    mComplTransactions.push_back(tr);
#ifdef NXS_NET_DEBUG_1
                    int total_transaction_time = (int)time(NULL) - (tr->mTimeOut - mTransactionTimeOut) ;
                    GXSNETDEBUG_P_(mit->first) << "    incoming completed " << tr->mTransaction->nItems << " items transaction in " << total_transaction_time << " seconds." << std::endl;
#endif

                    // transaction processing done
                    // for this id, add to removal list
                    toRemove.push_back(mmit->first);
                }
                else if(flag & NxsTransaction::FLAG_STATE_STARTING)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    transaction is starting!" << std::endl;
                    GXSNETDEBUG_P_(mit->first) << "    setting state to Receiving" << std::endl;
#endif
                    // send item to tell peer your are ready to start
                    RsNxsTransacItem* trans = new RsNxsTransacItem(mServType);
                    trans->clear();
                    trans->transactFlag = RsNxsTransacItem::FLAG_BEGIN_P2 |
                                    (tr->mTransaction->transactFlag & RsNxsTransacItem::FLAG_TYPE_MASK);
                    trans->transactionNumber = transN;
                    trans->PeerId(tr->mTransaction->PeerId());
                    sendItem(trans);
                    tr->mFlag = NxsTransaction::FLAG_STATE_RECEIVING;

                }
                else{
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_P_(mit->first) << "    transaction is in unknown state. ERROR!" << std::endl;
                    GXSNETDEBUG_P_(mit->first) << "    transaction FAILS!" << std::endl;
#endif

                    std::cerr << "  Unknown flag for active transaction, transN: " << transN << ", Peer: " << mit->first << std::endl;
                    toRemove.push_back(mmit->first);
                    mComplTransactions.push_back(tr);
                    tr->mFlag = NxsTransaction::FLAG_STATE_FAILED; // flag as a failed transaction
                }
            }
        }

        std::list<uint32_t>::iterator lit = toRemove.begin();

        for(; lit != toRemove.end(); ++lit)
        {
            transMap.erase(*lit);
        }

    }
}

bool RsGxsNetService::getGroupNetworkStats(const RsGxsGroupId& gid,RsGroupNetworkStats& stats)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    std::map<RsGxsGroupId,RsGroupNetworkStatsRecord>::const_iterator it = mGroupNetworkStats.find(gid) ;

    if(it == mGroupNetworkStats.end())
        return false ;

    stats.mSuppliers = it->second.suppliers.size();
    stats.mMaxVisibleCount = it->second.max_visible_count ;

    return true ;
}

void RsGxsNetService::processCompletedTransactions()
{
	RS_STACK_MUTEX(mNxsMutex) ;
	/*!
	 * Depending on transaction we may have to respond to peer
	 * responsible for transaction
	 */
	while(mComplTransactions.size()>0)
	{

		NxsTransaction* tr = mComplTransactions.front();

		bool outgoing = tr->mTransaction->PeerId() == mOwnId;

		if(outgoing){
			locked_processCompletedOutgoingTrans(tr);
		}else{
			locked_processCompletedIncomingTrans(tr);
		}


		delete tr;
		mComplTransactions.pop_front();
	}
}

void RsGxsNetService::locked_processCompletedIncomingTrans(NxsTransaction* tr)
{
	uint16_t flag = tr->mTransaction->transactFlag;

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "Processing complete Incoming transaction with " << tr->mTransaction->nItems << " items." << std::endl;
    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  flags = " << flag << std::endl;
    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  peerId= " << tr->mTransaction->PeerId() << std::endl;
#endif
    if(tr->mFlag & NxsTransaction::FLAG_STATE_COMPLETED)
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  transaction has completed." << std::endl;
#endif
        // for a completed list response transaction
        // one needs generate requests from this
        if(flag & RsNxsTransacItem::FLAG_TYPE_MSG_LIST_RESP)
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = msg list response." << std::endl;
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  => generate msg request based on it." << std::endl;
#endif
            // generate request based on a peers response
            locked_genReqMsgTransaction(tr);

        }else if(flag & RsNxsTransacItem::FLAG_TYPE_GRP_LIST_RESP)
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = grp list response." << std::endl;
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  => generate group transaction request based on it." << std::endl;
#endif
            locked_genReqGrpTransaction(tr);
        }
        // you've finished receiving request information now gen
        else if(flag & RsNxsTransacItem::FLAG_TYPE_MSG_LIST_REQ)
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = msg list request." << std::endl;
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  => generate msg list based on it." << std::endl;
#endif
            locked_genSendMsgsTransaction(tr);
        }
        else if(flag & RsNxsTransacItem::FLAG_TYPE_GRP_LIST_REQ)
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = grp list request." << std::endl;
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  => generate grp list based on it." << std::endl;
#endif
            locked_genSendGrpsTransaction(tr);
        }
        else if(flag & RsNxsTransacItem::FLAG_TYPE_GRPS)
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = groups." << std::endl;
#endif
            std::vector<RsNxsGrp*> grps;

            while(tr->mItems.size() != 0)
            {
                RsNxsGrp* grp = dynamic_cast<RsNxsGrp*>(tr->mItems.front());

                if(grp)
                {
                    tr->mItems.pop_front();
                    grps.push_back(grp);
#ifdef NXS_NET_DEBUG_0
                    GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grp->grpId) << "    adding new group " << grp->grpId << " to incoming list!" << std::endl;
#endif
                }
                else
                    std::cerr << "    /!\\ item did not caste to grp" << std::endl;
            }

#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    ...and notifying observer " << std::endl;
#endif
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (tr->mTransaction->PeerId()) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - Received new groups meta data from peer " << tr->mTransaction->PeerId() << std::endl;
            for(uint32_t i=0;i<grps.size();++i)
                GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grps[i]->grpId) ;
#endif
            // notify listener of grps
            for(uint32_t i=0;i<grps.size();++i)
                mNewGroupsToNotify.push_back(grps[i]) ;

            // now note this as the latest you've received from this peer
            RsPeerId peerFrom = tr->mTransaction->PeerId();
            uint32_t updateTS = tr->mTransaction->updateTS;

            ClientGrpMap::iterator it = mClientGrpUpdateMap.find(peerFrom);

            RsGxsGrpUpdateItem* item = NULL;

            if(it != mClientGrpUpdateMap.end())
            {
                item = it->second;
            }else
            {
                item = new RsGxsGrpUpdateItem(mServType);
                mClientGrpUpdateMap.insert(std::make_pair(peerFrom, item));
            }
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    and updating mClientGrpUpdateMap for peer " << peerFrom << " of new time stamp " << nice_time_stamp(time(NULL),updateTS) << std::endl;
#endif

            item->grpUpdateTS = updateTS;
            item->peerId = peerFrom;

            IndicateConfigChanged();


        }
        else if(flag & RsNxsTransacItem::FLAG_TYPE_MSGS)
        {

            std::vector<RsNxsMsg*> msgs;
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = msgs." << std::endl;
#endif
            RsGxsGroupId grpId;
            while(tr->mItems.size() > 0)
            {
                RsNxsMsg* msg = dynamic_cast<RsNxsMsg*>(tr->mItems.front());
                if(msg)
                {
                    if(grpId.isNull())
                        grpId = msg->grpId;

                    tr->mItems.pop_front();
                    msgs.push_back(msg);
#ifdef NXS_NET_DEBUG_0
                    GXSNETDEBUG_PG(tr->mTransaction->PeerId(),msg->grpId) << "    pushing grpId="<< msg->grpId << ", msgsId=" << msg->msgId << " to list of incoming messages" << std::endl;
#endif
                }
                else
                    std::cerr << "RsGxsNetService::processCompletedTransactions(): item did not caste to msg" << std::endl;
            }

#ifdef NXS_FRAG
            // (cyril) This code does not work. Since we do not really need message fragmenting, I won't fix it.
            
            std::map<RsGxsMessageId, MsgFragments > collatedMsgs;
            collateMsgFragments(msgs, collatedMsgs);			// this destroys msgs whatsoever and recovers memory when needed

            msgs.clear();

            std::map<RsGxsMessageId, MsgFragments >::iterator mit = collatedMsgs.begin();
            for(; mit != collatedMsgs.end(); ++mit)
            {
                MsgFragments& f = mit->second;
                RsNxsMsg* msg = deFragmentMsg(f);

                if(msg)
                    msgs.push_back(msg);
            }
            collatedMsgs.clear();
#endif
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "  ...and notifying observer of " << msgs.size() << " new messages." << std::endl;
#endif
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_PG (tr->mTransaction->PeerId(),grpId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - Received new messages from peer " << tr->mTransaction->PeerId() << " for group " << grpId << std::endl;
            for(uint32_t i=0;i<msgs.size();++i)
                GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "   " << msgs[i]->msgId << std::endl ;
#endif
            // notify listener of msgs
            for(uint32_t i=0;i<msgs.size();++i)
                mNewMessagesToNotify.push_back(msgs[i]) ;

            // now note that this is the latest you've received from this peer
            // for the grp id
            locked_doMsgUpdateWork(tr->mTransaction, grpId);

        }
    }
    else if(tr->mFlag == NxsTransaction::FLAG_STATE_FAILED)
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  transaction has failed. Wasting it." << std::endl;
#endif
        // don't do anything transaction will simply be cleaned
    }
	return;
}

void RsGxsNetService::locked_doMsgUpdateWork(const RsNxsTransacItem *nxsTrans, const RsGxsGroupId &grpId)
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(nxsTrans->PeerId(),grpId) << "updating MsgUpdate time stamps for peerId=" << nxsTrans->PeerId() << ", grpId=" << grpId << std::endl;
#endif
    // firts check if peer exists
    const RsPeerId& peerFrom = nxsTrans->PeerId();

    ClientMsgMap::iterator it = mClientMsgUpdateMap.find(peerFrom);

    RsGxsMsgUpdateItem* mui = NULL;

    // now update the peer's entry for this grp id
    if(it != mClientMsgUpdateMap.end())
    {
        mui = it->second;
    }
    else
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(nxsTrans->PeerId(),grpId) << "  created new entry." << std::endl;
#endif
        mui = new RsGxsMsgUpdateItem(mServType);
        mClientMsgUpdateMap.insert(std::make_pair(peerFrom, mui));
    }

    mui->peerId = peerFrom;

    if(mPartialMsgUpdates[peerFrom].find(grpId) != mPartialMsgUpdates[peerFrom].end())
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(nxsTrans->PeerId(),grpId) << "  this is a partial update. Not using new time stamp." << std::endl;
#endif
    }
    else
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(nxsTrans->PeerId(),grpId) << "  this is a full update. Updating time stamp." << std::endl;
#endif
        mui->msgUpdateInfos[grpId].time_stamp = nxsTrans->updateTS;
        IndicateConfigChanged();
    }
}

void RsGxsNetService::locked_processCompletedOutgoingTrans(NxsTransaction* tr)
{
    uint16_t flag = tr->mTransaction->transactFlag;

#ifdef NXS_NET_DEBUG_0
    RsNxsTransacItem *nxsTrans = tr->mTransaction;
    GXSNETDEBUG_P_(nxsTrans->PeerId()) << "locked_processCompletedOutgoingTrans(): tr->flags = " << flag << std::endl;
#endif

    if(tr->mFlag & NxsTransaction::FLAG_STATE_COMPLETED)
    {
	    // for a completed list response transaction
	    // one needs generate requests from this
	    if(flag & RsNxsTransacItem::FLAG_TYPE_MSG_LIST_RESP)
	    {
#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  complete Sending Msg List Response, transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif
	    }else if(flag & RsNxsTransacItem::FLAG_TYPE_GRP_LIST_RESP)
	    {
#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  complete Sending Grp Response, transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif
	    }
	    // you've finished sending a request so don't do anything
	    else if( (flag & RsNxsTransacItem::FLAG_TYPE_MSG_LIST_REQ) ||
	             (flag & RsNxsTransacItem::FLAG_TYPE_GRP_LIST_REQ) )
	    {
#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  complete Sending Msg/Grp Request, transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif

	    }else if(flag & RsNxsTransacItem::FLAG_TYPE_GRPS)
	    {

#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  complete Sending Grp Data, transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif

	    }else if(flag & RsNxsTransacItem::FLAG_TYPE_MSGS)
	    {
#ifdef NXS_NET_DEBUG_0
		    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  complete Sending Msg Data, transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif
	    }
    }else if(tr->mFlag == NxsTransaction::FLAG_STATE_FAILED){
#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  Failed transaction! transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif
    }else{

#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_P_(nxsTrans->PeerId())<< "  Serious error unrecognised trans Flag! transN: " << tr->mTransaction->transactionNumber << std::endl;
#endif
    }
}


void RsGxsNetService::locked_pushMsgTransactionFromList(std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN)
{
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peerId) << "locked_pushMsgTransactionFromList()" << std::endl;
    GXSNETDEBUG_P_(peerId) << "   nelems = " << reqList.size() << std::endl;
    GXSNETDEBUG_P_(peerId) << "   peerId = " << peerId << std::endl;
    GXSNETDEBUG_P_(peerId) << "   transN = " << transN << std::endl;
#endif
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (peerId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending message request to peer " 
                                    << peerId << " for " << reqList.size() << " messages" << std::endl;
#endif
    RsNxsTransacItem* transac = new RsNxsTransacItem(mServType);
    transac->transactFlag = RsNxsTransacItem::FLAG_TYPE_MSG_LIST_REQ
                    | RsNxsTransacItem::FLAG_BEGIN_P1;
    transac->timestamp = 0;
    transac->nItems = reqList.size();
    transac->PeerId(peerId);
    transac->transactionNumber = transN;
    NxsTransaction* newTrans = new NxsTransaction();
    newTrans->mItems = reqList;
    newTrans->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;
    newTrans->mTimeOut = time(NULL) + mTransactionTimeOut;
    // create transaction copy with your id to indicate
    // its an outgoing transaction
    newTrans->mTransaction = new RsNxsTransacItem(*transac);
    newTrans->mTransaction->PeerId(mOwnId);

    if (locked_addTransaction(newTrans))
    	sendItem(transac);
    else
    {
        delete newTrans;
        delete transac;
    }

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peerId) << "  Requested new transaction for " << reqList.size() << " items." << std::endl;
#endif
}

void RsGxsNetService::locked_genReqMsgTransaction(NxsTransaction* tr)
{

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG___ << "RsGxsNetService::genReqMsgTransaction()" << std::endl;
#endif

    // to create a transaction you need to know who you are transacting with
    // then what msgs to request
    // then add an active Transaction for request

    std::list<RsNxsSyncMsgItem*> msgItemL;
    std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

    // first get item list sent from transaction
    for(; lit != tr->mItems.end(); ++lit)
    {
        RsNxsSyncMsgItem* item = dynamic_cast<RsNxsSyncMsgItem*>(*lit);
        if(item)
        {
            msgItemL.push_back(item);
        }else
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_P_(item->PeerId()) << "RsGxsNetService::genReqMsgTransaction(): item failed cast to RsNxsSyncMsgItem* " << std::endl;
#endif
        }
    }
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  found " << msgItemL.size()<< " messages in this transaction." << std::endl;
#endif

    if(msgItemL.empty())
        return;

    // get grp id for this transaction
    RsNxsSyncMsgItem* item = msgItemL.front();
    const RsGxsGroupId& grpId = item->grpId;

    // store the count for the peer who sent the message list
    uint32_t mcount = msgItemL.size() ;
    RsPeerId pid = msgItemL.front()->PeerId() ;

    RsGroupNetworkStatsRecord& gnsr = mGroupNetworkStats[grpId];

    std::set<RsPeerId>::size_type oldSuppliersCount = gnsr.suppliers.size();
    uint32_t oldVisibleCount = gnsr.max_visible_count;

    gnsr.suppliers.insert(pid) ;
    gnsr.max_visible_count = std::max(gnsr.max_visible_count, mcount) ;

    if (oldVisibleCount != gnsr.max_visible_count || oldSuppliersCount != gnsr.suppliers.size())
        mObserver->notifyChangedGroupStats(grpId);

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  grpId = " << grpId << std::endl;
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  retrieving grp mesta data..." << std::endl;
#endif
    RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetaMap;
    grpMetaMap[grpId] = NULL;
    
    mDataStore->retrieveGxsGrpMetaData(grpMetaMap);
    RsGxsGrpMetaData* grpMeta = grpMetaMap[grpId];

    if(grpMeta == NULL) // this should not happen, but just in case...
    {
        std::cerr << "(EE) grpMeta is NULL in " << __PRETTY_FUNCTION__ << " line " << __LINE__ << ". This is very unexpected." << std::endl;
        return ;
    }
    
    if(! (grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
    {
        // For unsubscribed groups, we update the timestamp something more recent, so that the group content will not be asked to the same
        // peer again, unless the peer has new info about it. It's important to use the same clock (this is peer's clock) so that
        // we never compare times from different (and potentially badly sync-ed clocks)

        std::cerr << "(EE) stepping in part of the code (" << __PRETTY_FUNCTION__ << ") where we shouldn't. This is a bug." << std::endl;
        
#ifdef TO_REMOVE 
        locked_stampPeerGroupUpdateTime(pid,grpId,tr->mTransaction->updateTS,msgItemL.size()) ;
#endif
        return ;
    }

    int cutoff = 0;
    if(grpMeta != NULL)
        cutoff = grpMeta->mReputationCutOff;

    GxsMsgReq reqIds;
    reqIds[grpId] = std::vector<RsGxsMessageId>();
    GxsMsgMetaResult result;
    mDataStore->retrieveGxsMsgMetaData(reqIds, result);
    std::vector<RsGxsMsgMetaData*> &msgMetaV = result[grpId];

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  retrieving grp message list..." << std::endl;
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  grp locally contains " << msgMetaV.size() << " messsages." << std::endl;
#endif
    std::vector<RsGxsMsgMetaData*>::const_iterator vit = msgMetaV.begin();
    std::set<RsGxsMessageId> msgIdSet;

    // put ids in set for each searching
    for(; vit != msgMetaV.end(); ++vit)
    {
        msgIdSet.insert((*vit)->mMsgId);
        delete(*vit);
    }
    msgMetaV.clear();

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  grp locally contains " << msgIdSet.size() << " unique messsages." << std::endl;
#endif
    // get unique id for this transaction
    uint32_t transN = locked_getTransactionId();

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  new transaction ID: " << transN << std::endl;
#endif
    // add msgs that you don't have to request list
    std::list<RsNxsSyncMsgItem*>::iterator llit = msgItemL.begin();
    std::list<RsNxsItem*> reqList;
    int reqListSize = 0 ;

    const RsPeerId peerFrom = tr->mTransaction->PeerId();

    MsgAuthorV toVet;

    std::list<RsPeerId> peers;
    peers.push_back(tr->mTransaction->PeerId());
    bool reqListSizeExceeded = false ;

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  sorting items..." << std::endl;
#endif
    for(; llit != msgItemL.end(); ++llit)
    {
        RsNxsSyncMsgItem*& syncItem = *llit;
        const RsGxsMessageId& msgId = syncItem->msgId;

#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_PG(item->PeerId(),grpId) << "  msg ID = " << msgId ;
#endif
        if(reqListSize >= MAX_REQLIST_SIZE)
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_PG(item->PeerId(),grpId) << ". reqlist too big. Pruning out this item for now." << std::endl;
#endif
            reqListSizeExceeded = true ;
            continue ;	// we should actually break, but we need to print some debug info.
        }

        if(reqListSize < MAX_REQLIST_SIZE && msgIdSet.find(msgId) == msgIdSet.end())
        {

            // if reputation is in reputations cache then proceed
            // or if there isn't an author (note as author requirement is
            // enforced at service level, if no author is needed then reputation
            // filtering is optional)
            bool noAuthor = syncItem->authorId.isNull();

#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_PG(item->PeerId(),grpId) << ", reqlist size=" << reqListSize << ", message not present." ;
#endif
            // grp meta must be present if author present
            if(!noAuthor && grpMeta == NULL)
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_PG(item->PeerId(),grpId) << ", no group meta found. Givign up." << std::endl;
#endif
                continue;
            }
            
            if(rsReputations->isIdentityBanned(syncItem->authorId))
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_PG(item->PeerId(),grpId) << ", Identity " << syncItem->authorId << " is banned. Not requesting message!" << std::endl;
#endif
                continue ;
            }
            
            if(mRejectedMessages.find(msgId) != mRejectedMessages.end())
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_PG(item->PeerId(),grpId) << ", message has been recently rejected. Not requesting message!" << std::endl;
#endif
                continue ;
            }
                
            
            if(mReputations->haveReputation(syncItem->authorId) || noAuthor)
            {
                GixsReputation rep;

#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_PG(item->PeerId(),grpId) << ", author Id=" << syncItem->authorId << ". Reputation: " ;
#endif
                if(!noAuthor)
                    mReputations->getReputation(syncItem->authorId, rep);

                // if author is required for this message, it will simply get dropped
                // at genexchange side of things
                if(rep.score >= (int)grpMeta->mReputationCutOff || noAuthor)
                {
#ifdef NXS_NET_DEBUG_1
                    GXSNETDEBUG_PG(item->PeerId(),grpId) << ", passed! Adding message to req list." << std::endl;
#endif
                    RsNxsSyncMsgItem* msgItem = new RsNxsSyncMsgItem(mServType);
                    msgItem->grpId = grpId;
                    msgItem->msgId = msgId;
                    msgItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
                    msgItem->transactionNumber = transN;
                    msgItem->PeerId(peerFrom);
                    reqList.push_back(msgItem);
                    ++reqListSize ;
                }
#ifdef NXS_NET_DEBUG_1
                else
                    GXSNETDEBUG_PG(item->PeerId(),grpId) << ", failed!" << std::endl;
#endif
            }
            else
            {
#ifdef NXS_NET_DEBUG_1
                GXSNETDEBUG_PG(item->PeerId(),grpId) << ", no author/no reputation. Pushed to Vetting list." << std::endl;
#endif
                // preload for speed
                mReputations->loadReputation(syncItem->authorId, peers);
                MsgAuthEntry entry;
                entry.mAuthorId = syncItem->authorId;
                entry.mGrpId = syncItem->grpId;
                entry.mMsgId = syncItem->msgId;
                toVet.push_back(entry);
            }
        }
#ifdef NXS_NET_DEBUG_1
        else
            GXSNETDEBUG_PG(item->PeerId(),grpId) << ". already here." << std::endl;
#endif
    }

    if(!toVet.empty())
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_PG(item->PeerId(),grpId) << "  Vetting list: " << toVet.size() << " elements." << std::endl;
#endif
        MsgRespPending* mrp = new MsgRespPending(mReputations, tr->mTransaction->PeerId(), toVet, cutoff);
        mPendingResp.push_back(mrp);
    }

    if(!reqList.empty())
    {
#ifdef NXS_NET_DEBUG_1
        GXSNETDEBUG_PG(item->PeerId(),grpId) << "  Request list: " << reqList.size() << " elements." << std::endl;
#endif
        locked_pushMsgTransactionFromList(reqList, tr->mTransaction->PeerId(), transN);

        if(reqListSizeExceeded)
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_PG(item->PeerId(),grpId) << "  Marking update operation as unfinished." << std::endl;
#endif
            mPartialMsgUpdates[tr->mTransaction->PeerId()].insert(item->grpId) ;
        }
        else
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_PG(item->PeerId(),grpId) << "  Marking update operation as terminal." << std::endl;
#endif
            mPartialMsgUpdates[tr->mTransaction->PeerId()].erase(item->grpId) ;
        }
    }
    else
    {
#ifdef NXS_NET_DEBUG_1
	    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  Request list is empty. Not doing anything. " << std::endl;
#endif
	    // The list to req is empty. That means we already have all messages that this peer can
	    // provide. So we can stamp the group from this peer to be up to date.
        
            // Part of this is already achieved in two other places:
            // - the GroupStats exchange system, which counts the messages at each peer. It could also supply TS for the messages, but it does not for the time being
            // - client TS are updated when receiving messages
        
	    locked_stampPeerGroupUpdateTime(pid,grpId,tr->mTransaction->updateTS,msgItemL.size()) ;
    }
}

void RsGxsNetService::locked_stampPeerGroupUpdateTime(const RsPeerId& pid,const RsGxsGroupId& grpId,time_t tm,uint32_t n_messages)
{
    std::map<RsPeerId,RsGxsMsgUpdateItem*>::iterator it = mClientMsgUpdateMap.find(pid) ;
    
    RsGxsMsgUpdateItem *pitem; 
    
    if(it == mClientMsgUpdateMap.end())
    {
        pitem = new RsGxsMsgUpdateItem(mServType) ;
        pitem->peerId = pid ;

		  mClientMsgUpdateMap[pid] = pitem ;
    }
    else
        pitem = it->second ;
    
    pitem->msgUpdateInfos[grpId].time_stamp = tm;
    pitem->msgUpdateInfos[grpId].message_count = std::max(n_messages, pitem->msgUpdateInfos[grpId].message_count) ;

    IndicateConfigChanged();
}

void RsGxsNetService::locked_pushGrpTransactionFromList( std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN)
{
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peerId) << "locked_pushGrpTransactionFromList()" << std::endl;
    GXSNETDEBUG_P_(peerId) << "   nelems = " << reqList.size() << std::endl;
    GXSNETDEBUG_P_(peerId) << "   peerId = " << peerId << std::endl;
    GXSNETDEBUG_P_(peerId) << "   transN = " << transN << std::endl;
#endif
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (peerId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending group request to peer " 
                                    << peerId << " for " << reqList.size() << " groups" << std::endl;
#endif
    RsNxsTransacItem* transac = new RsNxsTransacItem(mServType);
	transac->transactFlag = RsNxsTransacItem::FLAG_TYPE_GRP_LIST_REQ
			| RsNxsTransacItem::FLAG_BEGIN_P1;
	transac->timestamp = 0;
	transac->nItems = reqList.size();
	transac->PeerId(peerId);
	transac->transactionNumber = transN;
	NxsTransaction* newTrans = new NxsTransaction();
	newTrans->mItems = reqList;
	newTrans->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;
    newTrans->mTimeOut = time(NULL) + mTransactionTimeOut;
	newTrans->mTransaction = new RsNxsTransacItem(*transac);
	newTrans->mTransaction->PeerId(mOwnId);
    
    if (locked_addTransaction(newTrans))
	    sendItem(transac);
    else
    {
	    delete newTrans;
	    delete transac;
    }
}
void RsGxsNetService::addGroupItemToList(NxsTransaction*& tr, const RsGxsGroupId& grpId, uint32_t& transN, std::list<RsNxsItem*>& reqList)
{
#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "RsGxsNetService::addGroupItemToList() Added GroupID: << grpId" << std::endl;
#endif

	RsNxsSyncGrpItem* grpItem = new RsNxsSyncGrpItem(mServType);
	grpItem->PeerId(tr->mTransaction->PeerId());
	grpItem->grpId = grpId;
	grpItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
	grpItem->transactionNumber = transN;
	reqList.push_back(grpItem);
}

void RsGxsNetService::locked_genReqGrpTransaction(NxsTransaction* tr)
{
    // to create a transaction you need to know who you are transacting with
    // then what grps to request
    // then add an active Transaction for request

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "locked_genReqGrpTransaction(): " << std::endl;
#endif

    RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetaMap;

    std::list<RsNxsSyncGrpItem*> grpItemL;

    for(std::list<RsNxsItem*>::iterator lit = tr->mItems.begin(); lit != tr->mItems.end(); ++lit)
    {
        RsNxsSyncGrpItem* item = dynamic_cast<RsNxsSyncGrpItem*>(*lit);
        if(item)
        {
            grpItemL.push_back(item);
            grpMetaMap[item->grpId] = NULL;
        }else
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_PG(tr->mTransaction->PeerId(),item->grpId) << "RsGxsNetService::genReqGrpTransaction(): item failed to caste to RsNxsSyncMsgItem* " << std::endl;
#endif
        }
    }

    if (grpItemL.empty())
        return;

    mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

    // now do compare and add loop
    std::list<RsNxsSyncGrpItem*>::iterator llit = grpItemL.begin();
    std::list<RsNxsItem*> reqList;

    uint32_t transN = locked_getTransactionId();

    GrpAuthorV toVet;
    std::list<RsPeerId> peers;
    peers.push_back(tr->mTransaction->PeerId());

    for(; llit != grpItemL.end(); ++llit)
    {
        RsNxsSyncGrpItem*& grpSyncItem = *llit;
        const RsGxsGroupId& grpId = grpSyncItem->grpId;
        
	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::const_iterator metaIter = grpMetaMap.find(grpId);
        bool haveItem = false;
        bool latestVersion = false;
        
        if (metaIter != grpMetaMap.end() && metaIter->second)
        {
            haveItem = true;
            latestVersion = grpSyncItem->publishTs > metaIter->second->mPublishTs;
        }
        
	if(!grpSyncItem->authorId.isNull() && rsReputations->isIdentityBanned(grpSyncItem->authorId))
	{
#ifdef NXS_NET_DEBUG_0
                GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "  Identity " << grpSyncItem->authorId << " is banned. Not syncing group." << std::endl;
#endif
    		continue ;            
	}
        
        if( (mGrpAutoSync && !haveItem) || latestVersion)
        {
            // determine if you need to check reputation
            bool checkRep = !grpSyncItem->authorId.isNull();

            // check if you have reputation, if you don't then
            // place in holding pen
            if(checkRep)
            {
                if(mReputations->haveReputation(grpSyncItem->authorId))
                {
                    GixsReputation rep;
                    mReputations->getReputation(grpSyncItem->authorId, rep);

                    if(rep.score >= GIXS_CUT_OFF)
                    {
                        addGroupItemToList(tr, grpId, transN, reqList);
#ifdef NXS_NET_DEBUG_0
                        GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId)<< "  reputation cut off: limit=" << GIXS_CUT_OFF << " value=" << rep.score << ": allowed." << std::endl;
#endif
                    }
#ifdef NXS_NET_DEBUG_0
                    else
                        GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId)<< "  reputation cut off: limit=" << GIXS_CUT_OFF << " value=" << rep.score << ": you shall not pass." << std::endl;
#endif
                }
                else
                {
                    // preload reputation for later
                    mReputations->loadReputation(grpSyncItem->authorId, peers);
                    GrpAuthEntry entry;
                    entry.mAuthorId = grpSyncItem->authorId;
                    entry.mGrpId = grpSyncItem->grpId;
                    toVet.push_back(entry);
                }
            }
            else
            {
                addGroupItemToList(tr, grpId, transN, reqList);
            }
        }
    }

    if(!toVet.empty())
    {
        RsPeerId peerId = tr->mTransaction->PeerId();
        GrpRespPending* grp = new GrpRespPending(mReputations, peerId, toVet);
        mPendingResp.push_back(grp);
    }


    if(!reqList.empty())
        locked_pushGrpTransactionFromList(reqList, tr->mTransaction->PeerId(), transN);
    else
    {
        ClientGrpMap::iterator it = mClientGrpUpdateMap.find(tr->mTransaction->PeerId());
        RsGxsGrpUpdateItem* item = NULL;
        if(it != mClientGrpUpdateMap.end())
            item = it->second;
        else
        {
            item = new RsGxsGrpUpdateItem(mServType);
            mClientGrpUpdateMap.insert(std::make_pair(tr->mTransaction->PeerId(), item));
        }
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    reqList is empty, updating anyway ClientGrpUpdate TS for peer " << tr->mTransaction->PeerId() << " to: " << tr->mTransaction->updateTS << std::endl;
#endif
        item->grpUpdateTS = tr->mTransaction->updateTS;
        item->peerId = tr->mTransaction->PeerId();
        IndicateConfigChanged();
    }
}

void RsGxsNetService::locked_genSendGrpsTransaction(NxsTransaction* tr)
{

#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "locked_genSendGrpsTransaction() Generating Grp data send fron TransN: " << tr->mTransaction->transactionNumber << std::endl;
#endif

	// go groups requested in transaction tr

	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

    	RsGxsMetaDataTemporaryMap<RsNxsGrp> grps ;

	for(;lit != tr->mItems.end(); ++lit)
	{
		RsNxsSyncGrpItem* item = dynamic_cast<RsNxsSyncGrpItem*>(*lit);
		if (item)
			grps[item->grpId] = NULL;
		else
		{
#ifdef NXS_NET_DEBUG_1
			GXSNETDEBUG_PG(tr->mTransaction->PeerId(),item->grpId) << "RsGxsNetService::locked_genSendGrpsTransaction(): item failed to caste to RsNxsSyncGrpItem* " << std::endl;
#endif
		}
	}

	if(!grps.empty())
		mDataStore->retrieveNxsGrps(grps, false, false);
    else
        return;

	NxsTransaction* newTr = new NxsTransaction();
	newTr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

	uint32_t transN = locked_getTransactionId();

	// store grp items to send in transaction
	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grps.begin();
	RsPeerId peerId = tr->mTransaction->PeerId();
	for(;mit != grps.end(); ++mit)
	{
		mit->second->PeerId(peerId); // set so it gets sent to right peer
		mit->second->transactionNumber = transN;
		newTr->mItems.push_back(mit->second);
        	mit->second = NULL ; // avoids deletion
	}

	if(newTr->mItems.empty()){
		delete newTr;
		return;
	}

	uint32_t updateTS = 0;
	if(mGrpServerUpdateItem)
		updateTS = mGrpServerUpdateItem->grpUpdateTS;

#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (tr->mTransaction->PeerId()) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending global group TS " 
                                                        << updateTS << " to peer " << tr->mTransaction->PeerId() << std::endl;
#endif
	RsNxsTransacItem* ntr = new RsNxsTransacItem(mServType);
	ntr->transactionNumber = transN;
	ntr->transactFlag = RsNxsTransacItem::FLAG_BEGIN_P1 | RsNxsTransacItem::FLAG_TYPE_GRPS;
	ntr->updateTS = updateTS;
	ntr->nItems = grps.size();
	ntr->PeerId(tr->mTransaction->PeerId());

	newTr->mTransaction = new RsNxsTransacItem(*ntr);
	newTr->mTransaction->PeerId(mOwnId);
	newTr->mTimeOut = time(NULL) + mTransactionTimeOut;

	ntr->PeerId(tr->mTransaction->PeerId());

	if(locked_addTransaction(newTr))
		sendItem(ntr);
        else
        {
            delete ntr ;
            delete newTr;
        }

    return;
}

void RsGxsNetService::runVetting()
{
    // The vetting operation consists in transforming pending group/msg Id requests and grp/msg content requests
    // into real transactions, based on the authorisations of the Peer Id these transactions are targeted to using the
    // reputation system.
    //
    
	RS_STACK_MUTEX(mNxsMutex) ;

	std::vector<AuthorPending*>::iterator vit = mPendingResp.begin();

	for(; vit != mPendingResp.end(); )
	{
		AuthorPending* ap = *vit;

		if(ap->accepted() || ap->expired())
		{
			// add to transactions
			if(AuthorPending::MSG_PEND == ap->getType())
			{
				MsgRespPending* mrp = static_cast<MsgRespPending*>(ap);
				locked_createTransactionFromPending(mrp);
			}
			else if(AuthorPending::GRP_PEND == ap->getType())
			{
				GrpRespPending* grp = static_cast<GrpRespPending*>(ap);
				locked_createTransactionFromPending(grp);
			}else
				std::cerr << "RsGxsNetService::runVetting(): Unknown pending type! Type: " << ap->getType() << std::endl;

			delete ap;
			vit = mPendingResp.erase(vit);
		}
		else
		{
			++vit;
		}

	}


	// now lets do circle vetting
	std::vector<GrpCircleVetting*>::iterator vit2 = mPendingCircleVets.begin();
	for(; vit2 != mPendingCircleVets.end(); )
	{
		GrpCircleVetting*& gcv = *vit2;
		if(gcv->cleared() || gcv->expired())
		{
			if(gcv->getType() == GrpCircleVetting::GRP_ID_PEND)
			{
				GrpCircleIdRequestVetting* gcirv = static_cast<GrpCircleIdRequestVetting*>(gcv);

				locked_createTransactionFromPending(gcirv);
			}
			else if(gcv->getType() == GrpCircleVetting::MSG_ID_SEND_PEND)
			{
				MsgCircleIdsRequestVetting* mcirv = static_cast<MsgCircleIdsRequestVetting*>(gcv);

				if(mcirv->cleared())
					locked_createTransactionFromPending(mcirv);
			}
			else
			{
#ifdef NXS_NET_DEBUG_4
				std::cerr << "RsGxsNetService::runVetting(): Unknown Circle pending type! Type: " << gcv->getType() << std::endl;
#endif
			}

			delete gcv;
			vit2 = mPendingCircleVets.erase(vit2);
		}
		else
		{
			++vit2;
		}
	}
}

void RsGxsNetService::locked_genSendMsgsTransaction(NxsTransaction* tr)
{
#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "locked_genSendMsgsTransaction() Generating Msg data send fron TransN: " << tr->mTransaction->transactionNumber << std::endl;
#endif

    // go groups requested in transaction tr

    std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

    GxsMsgReq msgIds;
    GxsMsgResult msgs;

    if(tr->mItems.empty()){
	    return;
    }

    // hacky assumes a transaction only consist of a single grpId
    RsGxsGroupId grpId;

    for(;lit != tr->mItems.end(); ++lit)
	 {
		 RsNxsSyncMsgItem* item = dynamic_cast<RsNxsSyncMsgItem*>(*lit);
		 if (item)
		 {
			 msgIds[item->grpId].push_back(item->msgId);

			 if(grpId.isNull())
				 grpId = item->grpId;
			 else if(grpId != item->grpId)
			 {
				 std::cerr << "RsGxsNetService::locked_genSendMsgsTransaction(): transaction on two different groups! ERROR!" << std::endl;
				 return ;
			 }
		 }
		 else
		 {
#ifdef NXS_NET_DEBUG_0
			 GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "RsGxsNetService::locked_genSendMsgsTransaction(): item failed to caste to RsNxsSyncMsgItem* " << std::endl;
#endif
		 }
	 }

    mDataStore->retrieveNxsMsgs(msgIds, msgs, false, false);

    NxsTransaction* newTr = new NxsTransaction();
    newTr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

    uint32_t transN = locked_getTransactionId();

    // store msg items to send in transaction
    GxsMsgResult::iterator mit = msgs.begin();
    RsPeerId peerId = tr->mTransaction->PeerId();
    uint32_t msgSize = 0;

    for(;mit != msgs.end(); ++mit)
	 {
		 std::vector<RsNxsMsg*>& msgV = mit->second;
		 std::vector<RsNxsMsg*>::iterator vit = msgV.begin();

		 for(; vit != msgV.end(); ++vit)
		 {
			 RsNxsMsg* msg = *vit;
			 msg->PeerId(peerId);
			 msg->transactionNumber = transN;

			 // Quick trick to clamp messages with an exceptionnally large size. Signature will fail on client side, and the message
			 // will be rejected.

			 if(msg->msg.bin_len > MAX_ALLOWED_GXS_MESSAGE_SIZE)
			 {
				 std::cerr << "(WW) message with ID " << msg->msgId << " in group " << msg->grpId << " exceeds size limit of " << MAX_ALLOWED_GXS_MESSAGE_SIZE << " bytes. Actual size is " << msg->msg.bin_len << " bytes. Message will be truncated and rejected at client." << std::endl;
				 msg->msg.bin_len = 1 ;	// arbitrary small size, but not 0. No need to send the data since it's going to be rejected.
			 }
#ifdef 	NXS_FRAG		
			 MsgFragments fragments;
			 fragmentMsg(*msg, fragments);

			 delete msg ;

			 MsgFragments::iterator mit = fragments.begin();

			 for(; mit != fragments.end(); ++mit)
			 {
				 newTr->mItems.push_back(*mit);
				 msgSize++;
			 }
#else			
			 msg->count = 1;	// only one piece. This is to keep compatibility if we ever implement fragmenting in the future.
			 msg->pos = 0;

			 newTr->mItems.push_back(msg);
			 msgSize++;
#endif			

		 }
	 }

	 if(newTr->mItems.empty()){
		 delete newTr;
		 return;
	 }

    // now if transaction is limited to an external group, encrypt it for members of the group.

    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grp;
    grp[grpId] = NULL ;
    
    mDataStore->retrieveGxsGrpMetaData(grp);
    
    RsGxsGrpMetaData *grpMeta = grp[grpId] ;
    
    if(grpMeta != NULL)
    {
        newTr->destination_circle = grpMeta->mCircleId ;
        delete grpMeta ;
        grp.clear() ;
    }

    // now send a transaction item and store the transaction data
    
    uint32_t updateTS = 0;
    ServerMsgMap::const_iterator cit = mServerMsgUpdateMap.find(grpId);

    if(cit != mServerMsgUpdateMap.end())
	    updateTS = cit->second->msgUpdateTS;

    RsNxsTransacItem* ntr = new RsNxsTransacItem(mServType);
    ntr->transactionNumber = transN;
    ntr->transactFlag = RsNxsTransacItem::FLAG_BEGIN_P1 |
                    RsNxsTransacItem::FLAG_TYPE_MSGS;
    ntr->updateTS = updateTS;
    ntr->nItems = msgSize;
    ntr->PeerId(peerId);

    newTr->mTransaction = new RsNxsTransacItem(*ntr);
    newTr->mTransaction->PeerId(mOwnId);
    newTr->mTimeOut = time(NULL) + mTransactionTimeOut;

#ifdef NXS_NET_DEBUG_5
    GXSNETDEBUG_PG (peerId,grpId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending message update to peer " << peerId << " for group " << grpId << " with TS=" << nice_time_stamp(time(NULL),updateTS) <<" (secs ago)" << std::endl;

#endif
	ntr->PeerId(tr->mTransaction->PeerId());

    // Encryption will happen here if needed.
    
    if(locked_addTransaction(newTr))
	sendItem(ntr);
    else
    {
        delete ntr ;
        delete newTr;
    }

    return;
}
uint32_t RsGxsNetService::locked_getTransactionId()
{
	return ++mTransactionN;
}
bool RsGxsNetService::locked_addTransaction(NxsTransaction* tr)
{
    const RsPeerId& peer = tr->mTransaction->PeerId();
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_P_(peer) << "locked_addTransaction() " << std::endl;
#endif
    uint32_t transN = tr->mTransaction->transactionNumber;
    TransactionIdMap& transMap = mTransactions[peer];
    bool transNumExist = transMap.find(transN) != transMap.end();

    if(transNumExist)
    {
#ifdef NXS_NET_DEBUG_1
	    GXSNETDEBUG_P_(peer) << "  Transaction number exist already, transN: " << transN << std::endl;
#endif
	    return false;
    }

    if(!tr->destination_circle.isNull())
    {
#ifdef NXS_NET_DEBUG_7
	    GXSNETDEBUG_P_(peer) << "  Calling transaction encryption on transaction " << transN << " for circle " << tr->destination_circle << std::endl;
#endif
	    if(!encryptTransaction(tr))
		return false ;
    }
#ifdef NXS_NET_DEBUG_7
    else
	    GXSNETDEBUG_P_(peer) << "  Transaction " << transN << " sent in clear since no circle restriction found!" << std::endl;
#endif

    transMap[transN] = tr;

    return true;
}

bool RsGxsNetService::encryptTransaction(NxsTransaction *tr)
{
#ifdef NXS_NET_DEBUG_7
    RsPeerId peerId = tr->mTransaction->PeerId() ;
    GXSNETDEBUG_P_ (peerId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - Encrypting transaction for peer " << peerId << ", for circle ID " << tr->destination_circle  << std::endl;
#endif
    std::cerr << "RsGxsNetService::encryptTransaction()" << std::endl;
         
    // 1 - Find out the list of GXS ids to encrypt for
    //     We could do smarter things (like see if the peer_id owns one of the circle's identities
    //     but for now we aim at the simplest solution: encrypt for all identities in the circle.
    
    std::list<RsGxsId> recipients ;
    
    if(!mCircles->recipients(tr->destination_circle,recipients))
    {
        std::cerr << "  (EE) Cannot encrypt transaction: recipients list not available." << std::endl;
        return false ;
    }
    
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_ (peerId) << "  Dest  Ids: " << std::endl;
#endif
    std::vector<RsTlvSecurityKey> recipient_keys ;
    
    for(std::list<RsGxsId>::const_iterator it(recipients.begin());it!=recipients.end();++it)
    {
        RsTlvSecurityKey pkey ;
        
       	if(!mGixs->getKey(*it,pkey))
        {
            std::cerr  << "(EE) Cannot retrieve public key " << *it << " for circle encryption." << std::endl;
            // we should probably request the key.
            continue ;
        }
#ifdef NXS_NET_DEBUG_7
        GXSNETDEBUG_P_ (peerId) << "    added key " << *it << std::endl;
#endif
        
        recipient_keys.push_back(pkey) ;
    }
    
    // 2 - call GXSSecurity to make a header item that encrypts for the given list of peers.
    
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_ (peerId) << "  Encrypting..." << std::endl;
#endif
    GxsSecurity::MultiEncryptionContext muctx ; 
    GxsSecurity::initEncryption(muctx,recipient_keys);
            
    // 3 - serialise and encrypt each message, converting it into a NxsEncryptedDataItem
    
    std::list<RsNxsItem*> encrypted_items ;
    
    for(std::list<RsNxsItem*>::const_iterator it(tr->mItems.begin());it!=tr->mItems.end();++it)
    {
        uint32_t size = (*it)->serial_size() ;
        RsTemporaryMemory tempmem( size ) ;
        
        if(!(*it)->serialise(tempmem,size))
        {
            std::cerr << "  (EE) Cannot serialise item. Something went wrong." << std::endl;
            continue ;
        }
        
        unsigned char *encrypted_data = NULL ;
        uint32_t encrypted_len  = 0 ;
        
        if(!GxsSecurity::encrypt(encrypted_data, encrypted_len,tempmem,size,muctx))
        {
            std::cerr << "  (EE) Cannot multi-encrypt item. Something went wrong." << std::endl;
            continue ;
        }
        
        RsNxsEncryptedDataItem *enc_item = new RsNxsEncryptedDataItem(mServType) ;
        
        enc_item->aes_encrypted_data.bin_len  = encrypted_len ;
        enc_item->aes_encrypted_data.bin_data = encrypted_data ;
        enc_item->aes_encrypted_data.tlvtype  = TLV_TYPE_BIN_ENCRYPTED ;
        
        encrypted_items.push_back(enc_item) ;
#ifdef NXS_NET_DEBUG_7
        GXSNETDEBUG_P_(peerId) << "    encrypted item of size " << encrypted_len << std::endl;
#endif
    }
        
    // 4 - put back in transaction.
    
    for(std::list<RsNxsItem*>::const_iterator it(tr->mItems.begin());it!=tr->mItems.end();++it)
        delete *it ;
    
    tr->mItems = encrypted_items ;
    
    // 5 - make session key item and push it front.
    
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_(peerId) << "  Creating session key" << std::endl;
#endif
    RsNxsSessionKeyItem *session_key_item = new RsNxsSessionKeyItem(mServType) ;
    
    memcpy(session_key_item->iv,muctx.initialisation_vector(),EVP_MAX_IV_LENGTH) ;
    
    for(int i=0;i<muctx.n_encrypted_keys();++i)
    {
#ifdef NXS_NET_DEBUG_7
        GXSNETDEBUG_P_(peerId) << "     addign session key for ID " << muctx.encrypted_key_id(i) << std::endl;
#endif
        RsTlvBinaryData data ;
        
        data.setBinData(muctx.encrypted_key_data(i), muctx.encrypted_key_size(i)) ;
        
        session_key_item->encrypted_session_keys[muctx.encrypted_key_id(i)] = data ;
    }
    
    tr->mItems.push_front(session_key_item) ;
    
    return true ;
}

bool RsGxsNetService::decryptTransaction(NxsTransaction *tr)
{
#ifdef NXS_NET_DEBUG_7
    RsPeerId peerId = tr->mTransaction->PeerId() ;
    GXSNETDEBUG_P_(peerId) << "RsGxsNetService::decryptTransaction()" << std::endl;
    GXSNETDEBUG_P_(peerId) << "  Circle Id: " << tr->destination_circle << std::endl;   
#endif
         
    // 1 - Checks that the transaction is encrypted. It should contain
    // 	   one packet with an encrypted session key for the group,
    //     and as many encrypted data items as necessary.
    
    RsNxsSessionKeyItem *esk = NULL;
    
    for(std::list<RsNxsItem*>::const_iterator it(tr->mItems.begin());it!=tr->mItems.end();++it)
        if(NULL != (esk = dynamic_cast<RsNxsSessionKeyItem*>(*it)))
            break ;
        
    if(esk == NULL)
    {
#ifdef NXS_NET_DEBUG_7
        GXSNETDEBUG_P_(peerId) << "  (II) nothing to decrypt. No session key packet in this transaction." << std::endl;
#endif
        return false ;
    }
    // 2 - Try to decrypt the session key. If not, return false. That probably means
    //     we don't own that identity.
    
    GxsSecurity::MultiEncryptionContext muctx ;
    RsGxsId private_key_id ;
    RsTlvBinaryData ek ;
    RsTlvSecurityKey private_key;
    bool found = false ;
    
    for(std::map<RsGxsId,RsTlvBinaryData>::const_iterator it(esk->encrypted_session_keys.begin());it!=esk->encrypted_session_keys.end();++it)
	    if(mGixs->havePrivateKey(it->first))
	    {
		    found = true ;
                    private_key_id = it->first ;
                    ek = it->second ;
                    
                    if(!mGixs->getPrivateKey(private_key_id,private_key))
                    {
                        std::cerr << "(EE) Cannot find private key to decrypt incoming transaction, for ID " << it->first << ". This is a bug since the key is supposed ot be here." << std::endl;
                        return false;
                    }
                    
#ifdef NXS_NET_DEBUG_7
                    GXSNETDEBUG_P_(peerId) << "  found appropriate private key to decrypt session key: " << it->first << std::endl;
#endif
		    break ;
	    }
    
    if(!found)
    {
        std::cerr << "  (EE) no private key for this encrypted transaction. Cannot decrypt!" << std::endl;
        return false ;
    }
    
    if(!GxsSecurity::initDecryption(muctx,private_key,esk->iv,EVP_MAX_IV_LENGTH,(unsigned char*)ek.bin_data,ek.bin_len))
    {
        std::cerr << "  (EE) cannot decrypt transaction. initDecryption() failed." << std::endl;
        return false ;
    }
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_(peerId) << "  Session key successfully decrypted, with length " << ek.bin_len << std::endl;
    GXSNETDEBUG_P_(peerId) << "  Now, decrypting transaction items..." << std::endl;
#endif
    
    // 3 - Using session key, decrypt all packets, by calling GXSSecurity.
    
    std::list<RsNxsItem*> decrypted_items ;
    RsNxsEncryptedDataItem *encrypted_item ;
    RsNxsSerialiser serial(mServType) ;
    
    for(std::list<RsNxsItem*>::const_iterator it(tr->mItems.begin());it!=tr->mItems.end();++it)
        if(NULL != (encrypted_item = dynamic_cast<RsNxsEncryptedDataItem*>(*it)))
	{
		unsigned char *tempmem;
		uint32_t tempmemsize ;

		if(!GxsSecurity::decrypt(tempmem,tempmemsize,(uint8_t*)encrypted_item->aes_encrypted_data.bin_data, encrypted_item->aes_encrypted_data.bin_len,muctx))
		{
			std::cerr << "  (EE) Cannot decrypt item. Something went wrong. Skipping this item." << std::endl;
			continue ;
		}

		RsItem *ditem = serial.deserialise(tempmem,&tempmemsize) ;

#ifdef NXS_NET_DEBUG_7
		GXSNETDEBUG_P_(peerId) << "    Decrypted an item of type " << std::hex << ditem->PacketId() << std::dec << std::endl;
#endif

		RsNxsItem *nxsi = dynamic_cast<RsNxsItem*>(ditem) ;

		if(nxsi != NULL)
			decrypted_items.push_back(nxsi) ;
		else
		{
			std::cerr << "  (EE) decrypted an item of unknown type!" << std::endl;
		}

		free(tempmem) ;
	}
        
    // 4 - put back in transaction.
    
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_(peerId) << "  replacing items with clear items" << std::endl;
#endif
    
    for(std::list<RsNxsItem*>::const_iterator it(tr->mItems.begin());it!=tr->mItems.end();++it)
        delete *it ;
    
    tr->mItems = decrypted_items ;
    
    return true ;

}

void RsGxsNetService::cleanTransactionItems(NxsTransaction* tr) const
{
	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

	for(; lit != tr->mItems.end(); ++lit)
	{
		delete *lit;
	}

	tr->mItems.clear();
}

void RsGxsNetService::locked_pushGrpRespFromList(std::list<RsNxsItem*>& respList, const RsPeerId& peer, const uint32_t& transN)
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(peer) << "locked_pushGrpResponseFromList()" << std::endl;
    GXSNETDEBUG_P_(peer) << "   nelems = " << respList.size() << std::endl;
    GXSNETDEBUG_P_(peer) << "   peerId = " << peer << std::endl;
    GXSNETDEBUG_P_(peer) << "   transN = " << transN << std::endl;
#endif
    NxsTransaction* tr = new NxsTransaction();
	tr->mItems = respList;

	tr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;
	RsNxsTransacItem* trItem = new RsNxsTransacItem(mServType);
	trItem->transactFlag = RsNxsTransacItem::FLAG_BEGIN_P1
			| RsNxsTransacItem::FLAG_TYPE_GRP_LIST_RESP;
	trItem->nItems = respList.size();
	trItem->timestamp = 0;
	trItem->PeerId(peer);
	trItem->transactionNumber = transN;
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_ (peer) << "Setting tr->mTransaction->updateTS to " << mGrpServerUpdateItem->grpUpdateTS << std::endl;
#endif
        trItem->updateTS = mGrpServerUpdateItem->grpUpdateTS;
	// also make a copy for the resident transaction
	tr->mTransaction = new RsNxsTransacItem(*trItem);
	tr->mTransaction->PeerId(mOwnId);
    tr->mTimeOut = time(NULL) + mTransactionTimeOut;
	// signal peer to prepare for transaction
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (peer) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending group response to peer " 
                                    << peer << " with " << respList.size() << " groups " << std::endl;
#endif
	if(locked_addTransaction(tr))
	    sendItem(trItem);
    else
    {
        delete tr ;
        delete trItem ;
    }
}

bool RsGxsNetService::locked_CanReceiveUpdate(const RsNxsSyncGrpReqItem *item)
{
    // Do we have new updates for this peer?

    // This is one of the few places where we compare a local time stamp (mGrpServerUpdateItem->grpUpdateTS) to a peer's time stamp.
    // Because this is the global modification time for groups, async-ed computers will eventually figure out that their data needs
    // to be synced.
    
    if(mGrpServerUpdateItem)
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_(item->PeerId()) << "  local modification time stamp: " << std::dec<< time(NULL) - mGrpServerUpdateItem->grpUpdateTS << " secs ago. Update sent: " <<
                     ((item->updateTS < mGrpServerUpdateItem->grpUpdateTS)?"YES":"NO")  << std::endl;
#endif
        return item->updateTS < mGrpServerUpdateItem->grpUpdateTS;
    }
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(item->PeerId()) << "  no local time stamp. This will be fixed after updateServerSyncTS(). Not sending for now. " << std::endl;
#endif

    return false;
}

void RsGxsNetService::handleRecvSyncGroup(RsNxsSyncGrpReqItem *item)
{
	if (!item)
		return;

	RS_STACK_MUTEX(mNxsMutex) ;

	RsPeerId peer = item->PeerId();
#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peer) << "HandleRecvSyncGroup(): Service: " << mServType << " from " << peer << ", Last update TS (from myself) sent from peer is T = " << std::dec<< time(NULL) - item->updateTS << " secs ago" << std::endl;
#endif

	if(!locked_CanReceiveUpdate(item))
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_P_(peer) << "  RsGxsNetService::handleRecvSyncGroup() update will not be sent." << std::endl;
#endif
		return;
	}

	RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grp;
	mDataStore->retrieveGxsGrpMetaData(grp);

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peer) << "  RsGxsNetService::handleRecvSyncGroup() retrieving local list of groups..." << std::endl;
#endif
	if(grp.empty())
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_P_(peer) << "  RsGxsNetService::handleRecvSyncGroup() Grp Empty" << std::endl;
#endif
		return;
	}

	std::list<RsNxsItem*> itemL;

	uint32_t transN = locked_getTransactionId();

	std::vector<GrpIdCircleVet> toVet;
#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peer) << "  Group list beings being sent: " << std::endl;
#endif

	for(std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grp.begin(); mit != grp.end(); ++mit)
	{
		RsGxsGrpMetaData* grpMeta = mit->second;

		// Only send info about subscribed groups.

		if(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
		{

			// check if you can send this id to peer
			// or if you need to add to the holding
			// pen for peer to be vetted
            
			if(canSendGrpId(peer, *grpMeta, toVet))
			{
				RsNxsSyncGrpItem* gItem = new RsNxsSyncGrpItem(mServType);
				gItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
				gItem->grpId = mit->first;
				gItem->publishTs = mit->second->mPublishTs;
				gItem->authorId = grpMeta->mAuthorId;
				gItem->PeerId(peer);
				gItem->transactionNumber = transN;
				itemL.push_back(gItem);
#ifdef NXS_NET_DEBUG_0
				GXSNETDEBUG_PG(peer,mit->first) << "    sending item for Grp " << mit->first << " name=" << grpMeta->mGroupName << ", publishTS=" << std::dec<< time(NULL) - mit->second->mPublishTs << " secs ago to peer ID " << peer << std::endl;
#endif
			}
		}
	}

	if(!toVet.empty())
	{
		mPendingCircleVets.push_back(new GrpCircleIdRequestVetting(mCircles, mPgpUtils, toVet, peer));
	}

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peer) << "  final list sent (after vetting): " << itemL.size() << " elements." << std::endl;
#endif
	locked_pushGrpRespFromList(itemL, peer, transN);

	return;
}



bool RsGxsNetService::canSendGrpId(const RsPeerId& sslId, RsGxsGrpMetaData& grpMeta, std::vector<GrpIdCircleVet>& toVet)
{
#ifdef NXS_NET_DEBUG_4
	GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::canSendGrpId()"<< std::endl;
#endif
	// first do the simple checks
	uint8_t circleType = grpMeta.mCircleType;

	if(circleType == GXS_CIRCLE_TYPE_LOCAL)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  LOCAL_CIRCLE, cannot send"<< std::endl;
#endif
		return false;
	}

	if(circleType == GXS_CIRCLE_TYPE_PUBLIC)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  PUBLIC_CIRCLE, can send"<< std::endl;
#endif
		return true;
	}

	if(circleType == GXS_CIRCLE_TYPE_EXTERNAL)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  EXTERNAL_CIRCLE, will be sent encrypted."<< std::endl;
#endif
        	return true ;
            
#ifdef TO_BE_REMOVED_OLD_VETTING_FOR_EXTERNAL_CIRCLES
		const RsGxsCircleId& circleId = grpMeta.mCircleId;
		if(circleId.isNull())
		{
			std::cerr << "  EXTERNAL_CIRCLE missing NULL CircleId: " << grpMeta.mGroupId<< std::endl;

			// ERROR, will never be shared.
			return false;
		}

		if(mCircles->isLoaded(circleId))
		{
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  EXTERNAL_CIRCLE, checking mCircles->canSend"<< std::endl;
#endif
			// the sending authorisation is based on:
			//		getPgpId(peer_id)  being a signer of one GxsId in the Circle
			//
			const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
            
			bool res = mCircles->canSend(circleId, pgpId);
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  answer is: " << res << std::endl;
#endif
            		return res ;
		}
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  grp not ready. Adding to vetting list." << std::endl;
#endif

		toVet.push_back(GrpIdCircleVet(grpMeta.mGroupId, circleId, grpMeta.mAuthorId));
		return false;
#endif
	}

	if(circleType == GXS_CIRCLE_TYPE_YOUREYESONLY)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  YOUREYESONLY, checking further"<< std::endl;
#endif
		// a non empty internal circle id means this
		// is the personal circle owner
		if(!grpMeta.mInternalCircle.isNull())
		{
			const RsGxsCircleId& internalCircleId = grpMeta.mInternalCircle;
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  have mInternalCircle - we are Group creator" << std::endl;
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  mCircleId: " << grpMeta.mCircleId << std::endl;
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  mInternalCircle: " << grpMeta.mInternalCircle << std::endl;
#endif

			if(mCircles->isLoaded(internalCircleId))
			{
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  circle Loaded - checking mCircles->canSend" << std::endl;
#endif
				const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
				bool res = mCircles->canSend(internalCircleId, pgpId);
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  answer is: " << res << std::endl;
#endif
                		return res ;
			}

#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Circle Not Loaded - add to vetting"<< std::endl;
#endif
			toVet.push_back(GrpIdCircleVet(grpMeta.mGroupId, internalCircleId, grpMeta.mAuthorId));
			return false;
		}
		else
		{
			// an empty internal circle id means this peer can only
			// send circle related info from peer he received it
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  mInternalCircle not set, someone else's personal circle"<< std::endl;
#endif
			if(grpMeta.mOriginator == sslId)
			{
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Originator matches -> can send"<< std::endl;
#endif
				return true;
			}
			else
			{
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Originator doesn't match -> cannot send"<< std::endl;
#endif
				return false;
			}
		}
	}

	return true;
}

bool RsGxsNetService::checkCanRecvMsgFromPeer(const RsPeerId& sslId, const RsGxsGrpMetaData& grpMeta)
{

	#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::checkCanRecvMsgFromPeer()";
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  peer Id = " << sslId << ", grpId=" << grpMeta.mGroupId <<std::endl;
	#endif
		// first do the simple checks
		uint8_t circleType = grpMeta.mCircleType;

		if(circleType == GXS_CIRCLE_TYPE_LOCAL)
		{
	#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  LOCAL_CIRCLE, cannot request sync from peer" << std::endl;
	#endif
			return false;
		}

		if(circleType == GXS_CIRCLE_TYPE_PUBLIC)
		{
	#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  PUBLIC_CIRCLE, can request msg sync" << std::endl;
	#endif
			return true;
		}

		if(circleType == GXS_CIRCLE_TYPE_EXTERNAL)
		{
#warning Here we can keep a test based on whether we are in the circle or not. Returning true also works but the transaction might turn out to be useless
#ifdef NXS_NET_DEBUG_4
	    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: EXTERNAL => returning true. Msgs will be encrypted." << std::endl;
#endif
        	return true ;
#ifdef TO_BE_REMOVED_OLD_VETTING_FOR_EXTERNAL_CIRCLES
			const RsGxsCircleId& circleId = grpMeta.mCircleId;
			if(circleId.isNull())
			{
#ifdef NXS_NET_DEBUG_0
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  ERROR; EXTERNAL_CIRCLE missing NULL CircleId";
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << grpMeta.mGroupId;
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << std::endl;
#endif

				// should just be shared. ? no - this happens for
				// Circle Groups which lose their CircleIds.
				// return true;
			}

			if(mCircles->isLoaded(circleId))
			{
	#ifdef NXS_NET_DEBUG_4
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  EXTERNAL_CIRCLE, checking mCircles->canSend" << std::endl;
	#endif
				const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
				return mCircles->canSend(circleId, pgpId);
			}
			else
				mCircles->loadCircle(circleId); // simply request for next pass

			return false;
#endif
		}

		if(circleType == GXS_CIRCLE_TYPE_YOUREYESONLY) // do not attempt to sync msg unless to originator or those permitted
		{
	#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  YOUREYESONLY, checking further" << std::endl;
	#endif
			// a non empty internal circle id means this
			// is the personal circle owner
			if(!grpMeta.mInternalCircle.isNull())
			{
				const RsGxsCircleId& internalCircleId = grpMeta.mInternalCircle;
	#ifdef NXS_NET_DEBUG_4
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    have mInternalCircle - we are Group creator" << std::endl;
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    mCircleId: " << grpMeta.mCircleId << std::endl;
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    mInternalCircle: " << grpMeta.mInternalCircle << std::endl;
	#endif

				if(mCircles->isLoaded(internalCircleId))
				{
	#ifdef NXS_NET_DEBUG_4
                    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    circle Loaded - checking mCircles->canSend" << std::endl;
	#endif
					const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
					return mCircles->canSend(internalCircleId, pgpId);
				}
				else
					mCircles->loadCircle(internalCircleId); // request for next pass

				return false;
			}
			else
			{
				// an empty internal circle id means this peer can only
				// send circle related info from peer he received it
	#ifdef NXS_NET_DEBUG_4
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    mInternalCircle not set, someone else's personal circle" << std::endl;
	#endif
				if(grpMeta.mOriginator == sslId)
				{
	#ifdef NXS_NET_DEBUG_4
                    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    Originator matches -> can send" << std::endl;
	#endif
					return true;
				}
				else
				{
	#ifdef NXS_NET_DEBUG_4
                    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "    Originator doesn't match -> cannot send"<< std::endl;
	#endif
					return false;
				}
			}
		}

		return true;
}

bool RsGxsNetService::locked_CanReceiveUpdate(const RsNxsSyncMsgReqItem *item)
{
    // Do we have new updates for this peer?
    // Here we compare times in the same clock: the friend's clock, so it should be fine.

    ServerMsgMap::const_iterator cit = mServerMsgUpdateMap.find(item->grpId);

    if(cit != mServerMsgUpdateMap.end())
    {
        const RsGxsServerMsgUpdateItem *msui = cit->second;

#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  local time stamp: " << std::dec<< time(NULL) - msui->msgUpdateTS << " secs ago. Update sent: " << (item->updateTS < msui->msgUpdateTS)  ;
#endif
        return item->updateTS < msui->msgUpdateTS ;
    }
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  no local time stamp for this grp. " ;
#endif
    
    return false;
}
void RsGxsNetService::handleRecvSyncMessage(RsNxsSyncMsgReqItem *item)
{
    if (!item)
	    return;

    RS_STACK_MUTEX(mNxsMutex) ;

    const RsPeerId& peer = item->PeerId();

    // Insert the PeerId in suppliers list for this grpId
#ifdef NXS_NET_DEBUG_6
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "RsGxsNetService::handleRecvSyncMessage(): Inserting PeerId " << item->PeerId() << " in suppliers list for group " << item->grpId << std::endl;
#endif
    RsGroupNetworkStatsRecord& rec(mGroupNetworkStats[item->grpId]) ;	// this creates it if needed
    rec.suppliers.insert(peer) ;

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "handleRecvSyncMsg(): Received last update TS of group " << item->grpId << ", for peer " << peer << ", TS = " << time(NULL) - item->updateTS << " secs ago." ;
#endif

        if(!locked_CanReceiveUpdate(item))
    {
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  no update will be sent." << std::endl;
#endif
            return;
    }

	GxsMsgMetaResult metaResult;
	GxsMsgReq req;

	RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetas;
	grpMetas[item->grpId] = NULL;
    
	mDataStore->retrieveGxsGrpMetaData(grpMetas);
	RsGxsGrpMetaData* grpMeta = grpMetas[item->grpId];

	if(grpMeta == NULL)
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_PG(item->PeerId(),item->grpId) << " Grp is unknown." << std::endl;
#endif
		return;
	}
	if(!(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_PG(item->PeerId(),item->grpId) << " Grp is not subscribed." << std::endl;
#endif
		return ;
	}

	req[item->grpId] = std::vector<RsGxsMessageId>();
	mDataStore->retrieveGxsMsgMetaData(req, metaResult);
	std::vector<RsGxsMsgMetaData*>& msgMetas = metaResult[item->grpId];

#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "   retrieving message meta data." << std::endl;
#endif
	if(req.empty())
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  No msg meta data.." << std::endl;
#endif
	}
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  Sending MSG meta data!" << std::endl;
#endif

	std::list<RsNxsItem*> itemL;

	uint32_t transN = locked_getTransactionId();
    	RsGxsCircleId should_encrypt_to_this_circle_id ;

	if(canSendMsgIds(msgMetas, *grpMeta, peer, should_encrypt_to_this_circle_id))
	{
		std::vector<RsGxsMsgMetaData*>::iterator vit = msgMetas.begin();

		for(; vit != msgMetas.end(); ++vit)
		{
			RsGxsMsgMetaData* m = *vit;

			RsNxsSyncMsgItem* mItem = new RsNxsSyncMsgItem(mServType);
			mItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
			mItem->grpId = m->mGroupId;
			mItem->msgId = m->mMsgId;
			mItem->authorId = m->mAuthorId;
			mItem->PeerId(peer);
			mItem->transactionNumber = transN;
			itemL.push_back(mItem);
#ifdef NXS_NET_DEBUG_0
                if(!should_encrypt_to_this_circle_id.isNull())
			GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "    sending info item for msg id " << mItem->msgId << ". Transaction will be encrypted for group " << should_encrypt_to_this_circle_id << std::endl;
                else
			GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "    sending info item for msg id " << mItem->msgId ;
#endif
		}

		if(!itemL.empty())
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  sending final msg info list of " << itemL.size() << " items." << std::endl;
#endif
			locked_pushMsgRespFromList(itemL, peer, item->grpId,transN,should_encrypt_to_this_circle_id);
		}
#ifdef NXS_NET_DEBUG_0
        	else
			GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  list is empty! Not sending anything." << std::endl;
#endif
	}
#ifdef NXS_NET_DEBUG_0
	else
		GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  vetting forbids sending. Nothing will be sent." << itemL.size() << " items." << std::endl;
#endif

	std::vector<RsGxsMsgMetaData*>::iterator vit = msgMetas.begin();
	// release meta resource
	for(vit = msgMetas.begin(); vit != msgMetas.end(); ++vit)
		delete *vit;
}

void RsGxsNetService::locked_pushMsgRespFromList(std::list<RsNxsItem*>& itemL, const RsPeerId& sslId, const RsGxsGroupId& grp_id,const uint32_t& transN,const RsGxsCircleId& destination_circle)
{
#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(sslId,grp_id) << "locked_pushMsgResponseFromList()" << std::endl;
    GXSNETDEBUG_PG(sslId,grp_id) << "   nelems = " << itemL.size() << std::endl;
    GXSNETDEBUG_PG(sslId,grp_id) << "   peerId = " << sslId << std::endl;
    GXSNETDEBUG_PG(sslId,grp_id) << "   transN = " << transN << std::endl;
#endif
    NxsTransaction* tr = new NxsTransaction();
	tr->mItems = itemL;
	tr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

	RsNxsTransacItem* trItem = new RsNxsTransacItem(mServType);
    trItem->transactFlag = RsNxsTransacItem::FLAG_BEGIN_P1 | RsNxsTransacItem::FLAG_TYPE_MSG_LIST_RESP;
    trItem->nItems = itemL.size();
	trItem->timestamp = 0;
	trItem->PeerId(sslId);
	trItem->transactionNumber = transN;

	// also make a copy for the resident transaction
	tr->mTransaction = new RsNxsTransacItem(*trItem);
	tr->mTransaction->PeerId(mOwnId);
    tr->mTimeOut = time(NULL) + mTransactionTimeOut;
    tr->destination_circle = destination_circle;

	ServerMsgMap::const_iterator cit = mServerMsgUpdateMap.find(grp_id);

    	// This time stamp is not supposed to be used on the other side. We just set it to avoid sending an uninitialiszed value.
    
	if(cit != mServerMsgUpdateMap.end())
	    trItem->updateTS = cit->second->msgUpdateTS;
    	else
    	{
	    std::cerr << "(EE) cannot find a server TS for message of group " << grp_id << " in locked_pushMsgRespFromList. This is weird." << std::endl;
	    trItem->updateTS = 0 ;
    	}
    
#ifdef NXS_NET_DEBUG_5
            GXSNETDEBUG_P_ (sslId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending messages response to peer " 
                                    << sslId << " with " << itemL.size() << " messages " << std::endl;
#endif
	// signal peer to prepare for transaction
	if(locked_addTransaction(tr))
		sendItem(trItem);
        else
        {
            delete tr ;
            delete trItem ;
        }
            
}

bool RsGxsNetService::canSendMsgIds(const std::vector<RsGxsMsgMetaData*>& msgMetas, const RsGxsGrpMetaData& grpMeta, const RsPeerId& sslId,RsGxsCircleId& should_encrypt_id)
{
#ifdef NXS_NET_DEBUG_4
	GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::canSendMsgIds() CIRCLE VETTING" << std::endl;
#endif

	// first do the simple checks
	uint8_t circleType = grpMeta.mCircleType;

	if(circleType == GXS_CIRCLE_TYPE_LOCAL)
    	{
#ifdef NXS_NET_DEBUG_4
	    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: LOCAL => returning false" << std::endl;
#endif
	    return false;
    	}

    	if(circleType == GXS_CIRCLE_TYPE_PUBLIC)
    	{
#ifdef NXS_NET_DEBUG_4
	    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: PUBLIC => returning true" << std::endl;
#endif
	    return true;
    	}

	const RsGxsCircleId& circleId = grpMeta.mCircleId;

	if(circleType == GXS_CIRCLE_TYPE_EXTERNAL)
	{
#ifdef NXS_NET_DEBUG_4
	    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: EXTERNAL => returning true. Msgs ids list will be encrypted." << std::endl;
#endif
	should_encrypt_id = circleId ;
        	return true ;
            
#ifdef TO_BE_REMOVED_OLD_VETTING_FOR_EXTERNAL_CIRCLES
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: EXTERNAL. Circle Id: " << circleId << std::endl;
#endif
		if(mCircles->isLoaded(circleId))
		{
			const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
			bool res = mCircles->canSend(circleId, pgpId);
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Answer from circle::canSend(): " << res << std::endl;
#endif
            		return res ;
		}

#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle info not loaded. Putting in vetting list and returning false." << std::endl;
#endif
		std::vector<MsgIdCircleVet> toVet;
		std::vector<RsGxsMsgMetaData*>::const_iterator vit = msgMetas.begin();

		for(; vit != msgMetas.end(); ++vit)
		{
			const RsGxsMsgMetaData* const& meta = *vit;

			MsgIdCircleVet mic(meta->mMsgId, meta->mAuthorId);
			toVet.push_back(mic);
		}

		if(!toVet.empty())
			mPendingCircleVets.push_back(new MsgCircleIdsRequestVetting(mCircles, mPgpUtils, toVet, grpMeta.mGroupId,
					sslId, grpMeta.mCircleId));

		return false;
#endif
	}

	if(circleType == GXS_CIRCLE_TYPE_YOUREYESONLY)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: YOUR EYES ONLY" << std::endl;
#endif
		// a non empty internal circle id means this
		// is the personal circle owner
		if(!grpMeta.mInternalCircle.isNull())
		{
			const RsGxsCircleId& internalCircleId = grpMeta.mInternalCircle;
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Group internal circle: " << internalCircleId << std::endl;
#endif
			if(mCircles->isLoaded(internalCircleId))
			{
				const RsPgpId& pgpId = mPgpUtils->getPGPId(sslId);
				bool res= mCircles->canSend(internalCircleId, pgpId);
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Answer from circle::canSend(): " << res << std::endl;
#endif
                		return res ;
			}
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Not loaded. Putting in vetting list and returning false." << std::endl;
#endif

			std::vector<MsgIdCircleVet> toVet;
			std::vector<RsGxsMsgMetaData*>::const_iterator vit = msgMetas.begin();

			for(; vit != msgMetas.end(); ++vit)
			{
				const RsGxsMsgMetaData* const& meta = *vit;

				MsgIdCircleVet mic(meta->mMsgId, meta->mAuthorId);
				toVet.push_back(mic);
			}

			if(!toVet.empty())
				mPendingCircleVets.push_back(new MsgCircleIdsRequestVetting(mCircles, mPgpUtils, 
						toVet, grpMeta.mGroupId,
						sslId, grpMeta.mCircleId));

			return false;
		}
		else
		{
			// an empty internal circle id means this peer can only
			// send circle related info from peer he received it
                    
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Empty internal circle: cannot only send info from Peer we received it (grpMeta.mOriginator=" << grpMeta.mOriginator << " answer is: " << (grpMeta.mOriginator == sslId) << std::endl;
#endif
			if(grpMeta.mOriginator == sslId)
				return true;
			else
				return false;
		}
	}

	return true;
}

/** inherited methods **/

void RsGxsNetService::pauseSynchronisation(bool /* enabled */)
{

}

void RsGxsNetService::setSyncAge(uint32_t /* age */)
{

}

int RsGxsNetService::requestGrp(const std::list<RsGxsGroupId>& grpId, const RsPeerId& peerId)
{
	RS_STACK_MUTEX(mNxsMutex) ;
	mExplicitRequest[peerId].assign(grpId.begin(), grpId.end());
	return 1;
}

void RsGxsNetService::processExplicitGroupRequests()
{
	RS_STACK_MUTEX(mNxsMutex) ;

	std::map<RsPeerId, std::list<RsGxsGroupId> >::const_iterator cit = mExplicitRequest.begin();

	for(; cit != mExplicitRequest.end(); ++cit)
	{
		const RsPeerId& peerId = cit->first;
		const std::list<RsGxsGroupId>& groupIdList = cit->second;

		std::list<RsNxsItem*> grpSyncItems;
		std::list<RsGxsGroupId>::const_iterator git = groupIdList.begin();
		uint32_t transN = locked_getTransactionId();
		for(; git != groupIdList.end(); ++git)
		{
			RsNxsSyncGrpItem* item = new RsNxsSyncGrpItem(mServType);
			item->grpId = *git;
			item->PeerId(peerId);
			item->flag = RsNxsSyncGrpItem::FLAG_REQUEST;
			item->transactionNumber = transN;
			grpSyncItems.push_back(item);
		}

		if(!grpSyncItems.empty())
			locked_pushGrpTransactionFromList(grpSyncItems, peerId, transN);
	}

	mExplicitRequest.clear();
}

int RsGxsNetService::sharePublishKey(const RsGxsGroupId& grpId,const std::set<RsPeerId>& peers)
{
	RS_STACK_MUTEX(mNxsMutex) ;

	mPendingPublishKeyRecipients[grpId] = peers ;
#ifdef NXS_NET_DEBUG_3
    GXSNETDEBUG__G(grpId) << "RsGxsNetService::sharePublishKeys() " << (void*)this << " adding publish keys for grp " << grpId << " to sending list" << std::endl;
#endif

    return true ;
}

void RsGxsNetService::sharePublishKeysPending()
{
	RS_STACK_MUTEX(mNxsMutex) ;

    if(mPendingPublishKeyRecipients.empty())
        return ;

#ifdef NXS_NET_DEBUG_3
    GXSNETDEBUG___ << "RsGxsNetService::sharePublishKeys()  " << (void*)this << std::endl;
#endif
    // get list of peers that are online

    std::set<RsPeerId> peersOnline;
    std::list<RsGxsGroupId> toDelete;
    std::map<RsGxsGroupId,std::set<RsPeerId> >::iterator mit ;

    mNetMgr->getOnlineList(mServiceInfo.mServiceType, peersOnline);

#ifdef NXS_NET_DEBUG_3
    GXSNETDEBUG___ << "  " << peersOnline.size() << " peers online." << std::endl;
#endif
    /* send public key to peers online */

    for(mit = mPendingPublishKeyRecipients.begin();  mit != mPendingPublishKeyRecipients.end(); ++mit)
    {
        // Compute the set of peers to send to. We start with this, to avoid retrieving the data for nothing.

        std::list<RsPeerId> recipients ;
        std::set<RsPeerId> offline_recipients ;

        for(std::set<RsPeerId>::const_iterator it(mit->second.begin());it!=mit->second.end();++it)
            if(peersOnline.find(*it) != peersOnline.end())
            {
#ifdef NXS_NET_DEBUG_3
                GXSNETDEBUG_P_(*it) << "    " << *it << ": online. Adding." << std::endl;
#endif
                recipients.push_back(*it) ;
            }
            else
            {
#ifdef NXS_NET_DEBUG_3
                GXSNETDEBUG_P_(*it) << "    " << *it << ": offline. Keeping for next try." << std::endl;
#endif
                offline_recipients.insert(*it) ;
            }

        // If empty, skip

        if(recipients.empty())
        {
#ifdef NXS_NET_DEBUG_3
            GXSNETDEBUG___ << "  No recipients online. Skipping." << std::endl;
#endif
            continue ;
        }

        // Get the meta data for this group Id
        //
        RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetaMap;
        grpMetaMap[mit->first] = NULL;
        mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

        // Find the publish keys in the retrieved info

        RsGxsGrpMetaData *grpMeta = grpMetaMap[mit->first] ;

        if(grpMeta == NULL)
        {
            std::cerr << "(EE) RsGxsNetService::sharePublishKeys() Publish keys cannot be found for group " << mit->first << std::endl;
            continue ;
        }

        const RsTlvSecurityKeySet& keys = grpMeta->keys;

        std::map<RsGxsId, RsTlvSecurityKey>::const_iterator kit = keys.keys.begin(), kit_end = keys.keys.end();
        bool publish_key_found = false;
        RsTlvSecurityKey publishKey ;

        for(; kit != kit_end && !publish_key_found; ++kit)
        {
            publish_key_found = (kit->second.keyFlags == (RSTLV_KEY_DISTRIB_PUBLISH | RSTLV_KEY_TYPE_FULL));
            publishKey = kit->second ;
        }

        if(!publish_key_found)
        {
            std::cerr << "(EE) no publish key in group " << mit->first << ". Cannot share!" << std::endl;
            continue ;
        }


#ifdef NXS_NET_DEBUG_3
        GXSNETDEBUG__G(grpMeta->mGroupId) << "  using publish key ID=" << publishKey.keyId << ", flags=" << publishKey.keyFlags << std::endl;
#endif
        for(std::list<RsPeerId>::const_iterator it(recipients.begin());it!=recipients.end();++it)
        {
            /* Create publish key sharing item */
            RsNxsGroupPublishKeyItem *publishKeyItem = new RsNxsGroupPublishKeyItem(mServType);

            publishKeyItem->clear();
            publishKeyItem->grpId = mit->first;

            publishKeyItem->key = publishKey ;
            publishKeyItem->PeerId(*it);

            sendItem(publishKeyItem);
#ifdef NXS_NET_DEBUG_3
            GXSNETDEBUG_PG(*it,grpMeta->mGroupId) << "  sent key item to " << *it << std::endl;
#endif
        }

        mit->second = offline_recipients ;

        // If given peers have all received key(s) then stop sending for group
        if(offline_recipients.empty())
            toDelete.push_back(mit->first);
    }

    // delete pending peer list which are done with
    for(std::list<RsGxsGroupId>::const_iterator lit = toDelete.begin(); lit != toDelete.end(); ++lit)
        mPendingPublishKeyRecipients.erase(*lit);
}

void RsGxsNetService::handleRecvPublishKeys(RsNxsGroupPublishKeyItem *item)
{
#ifdef NXS_NET_DEBUG_3
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "RsGxsNetService::sharePublishKeys() " << std::endl;
#endif

	if (!item)
		return;

	RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_3
	 GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  PeerId : " << item->PeerId() << std::endl;
	 GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  GrpId: " << item->grpId << std::endl;
	 GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  Got key Item: " << item->key.keyId << std::endl;
#endif

	 // Get the meta data for this group Id
	 //
	 RsGxsMetaDataTemporaryMap<RsGxsGrpMetaData> grpMetaMap;
	 grpMetaMap[item->grpId] = NULL;
     
	 mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

	 // update the publish keys in this group meta info

	 RsGxsGrpMetaData *grpMeta = grpMetaMap[item->grpId] ;

	 // Check that the keys correspond, and that FULL keys are supplied, etc.

#ifdef NXS_NET_DEBUG_3
	 GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "  Key received: " << std::endl;
#endif

	 bool admin = (item->key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)   && (item->key.keyFlags & RSTLV_KEY_TYPE_FULL) ;
     bool publi = (item->key.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH) && (item->key.keyFlags & RSTLV_KEY_TYPE_FULL) ;

#ifdef NXS_NET_DEBUG_3
     GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "    Key id = " << item->key.keyId << "  admin=" << admin << ", publish=" << publi << " ts=" << item->key.endTS << std::endl;
#endif

     if(!(!admin && publi))
     {
         std::cerr << "  Key is not a publish private key. Discarding!" << std::endl;
         return ;
     }
	 // Also check that we don't already have full keys for that group.
	 
     std::map<RsGxsId,RsTlvSecurityKey>::iterator it = grpMeta->keys.keys.find(item->key.keyId) ;

     if(it == grpMeta->keys.keys.end())
     {
         std::cerr << "   (EE) Key not found in known group keys. This is an inconsistency." << std::endl;
         return ;
     }

     if((it->second.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH) && (it->second.keyFlags & RSTLV_KEY_TYPE_FULL))
     {
#ifdef NXS_NET_DEBUG_3
         GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "   (EE) Publish key already present in database. Discarding message." << std::endl;
#endif
			return ;
     }

	  // Store/update the info.

     it->second = item->key ;
     bool ret = mDataStore->updateGroupKeys(item->grpId,grpMeta->keys, grpMeta->mSubscribeFlags | GXS_SERV::GROUP_SUBSCRIBE_PUBLISH) ;

	if(ret)
	{
#ifdef NXS_NET_DEBUG
		GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "  updated database with new publish keys." << std::endl;
#endif
		mObserver->notifyReceivePublishKey(item->grpId);
	}
	else
	{
		std::cerr << "(EE) could not update database. Something went wrong." << std::endl;
	}
}
