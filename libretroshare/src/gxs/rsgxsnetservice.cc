/*******************************************************************************
 * libretroshare/src/gxs: rsgxsnetservice.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Christopher Evi-Parker                               *
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
//
// Distant sync
// ============
//
// Distant sync uses tunnels to sync subscribed GXS groups that are not supplied by friends. Peers can subscribe to a GXS group using a RS link
// which GXS uses to request updates through tunnels.
//   * The whole exchange should be kept private and anonymous between the two distant peers, so we use the same trick than for FT: encrypt the data using the group ID.
//   * The same node shouldn't be known as a common server for different GXS groups
//
//   GXS net service:
//   	* talks to virtual peers, treated like normal peers
//   	* virtual peers only depend on the server ID, not on tunnel ID, and be kept constant accross time so that ClientGroupUpdateMap is kept consistent
//      * does not use tunnels if friends can already supply the data (??) This causes issues with "islands".
//
//   Tunnels:
//	    * a specific service named GxsSyncTunnelService handles the creation/management of sync tunnels:
//	    * tunnel data need to be encrypted.
//
//              bool manageTunnels(const RsGxsGroupId&) ;													// start managing tunnels for this group
//              bool releaseTunnels(const RsGxsGroupId&) ;													// stop managing tunnels for this group
//              bool sendData(const unsigned char *data,uint32_t size,const RsPeerId& virtual_peer) ;		// send data to this virtual peer
//              bool getVirtualPeers(const RsGxsGroupId&, std::list<RsPeerId>& peers) ; 					// returns the virtual peers for this group
//
//   Proposed protocol:
//      * request tunnels based on H(GroupId)
//      * encrypt tunnel data using chacha20+HMAC-SHA256 using AEAD( GroupId, 96bits IV, tunnel ID ) (similar to what FT does)
//      * when tunnel is established, exchange virtual peer names: vpid = H( GroupID | Random bias )
//      * when vpid is known, notify the client (GXS net service) which can use the virtual peer to sync
//
//      * only use a single tunnel per virtual peer ID
//
//              Client  ------------------ TR(H(GroupId)) -------------->  Server
//
//              Client  <-------------------- T OK ----------------------  Server
//
//           [Encrypted traffic using H(GroupId, 96bits IV, tunnel ID)]
//
//              Client  <--------- VPID = H( Random IV | GroupId ) ------  Server
//                 |                                                         |
//                 +--------------> Mark the virtual peer active <-----------+
//
//   Unsolved problems:
//      * if we want to preserve anonymity, we cannot prevent GXS from duplicating the data from virtual/real peers that actually are the same peers.
//      * ultimately we should only use tunnels to sync GXS. The mix between tunnels and real peers is not a problem but will cause unnecessary traffic.
//
//   Notes:
//      * given that GXS only talks to peers once every 2 mins, it's likely that keep-alive packets will be needed


#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sstream>
#include <typeinfo>

#include "rsgxsnetservice.h"
#include "gxssecurity.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rspeers.h"
#include "pgp/pgpauxutils.h"
#include "crypto/rscrypto.h"
#include "util/rsdir.h"
#include "util/rstime.h"
#include "util/rsmemory.h"
#include "util/stacktrace.h"

#ifdef RS_DEEP_CHANNEL_INDEX
#	include "deep_search/channelsindex.hpp"
#endif

/***
 * Use the following defines to debug:
 	NXS_NET_DEBUG_0		shows group update high level information
 	NXS_NET_DEBUG_1		shows group update low level info (including transaction details)
 	NXS_NET_DEBUG_2		bandwidth information
 	NXS_NET_DEBUG_3		publish key exchange
 	NXS_NET_DEBUG_4		vetting
 	NXS_NET_DEBUG_5		summary of transactions (useful to just know what comes in/out)
    NXS_NET_DEBUG_6		group sync statistics (e.g. number of posts at nighbour nodes, etc)
 	NXS_NET_DEBUG_7		encryption/decryption of transactions
 	NXS_NET_DEBUG_8		gxs distant sync
 	NXS_NET_DEBUG_9		gxs distant search

 ***/
//#define NXS_NET_DEBUG_0 	1
//#define NXS_NET_DEBUG_1 	1
//#define NXS_NET_DEBUG_2 	1
//#define NXS_NET_DEBUG_3 	1
//#define NXS_NET_DEBUG_4 	1
//#define NXS_NET_DEBUG_5 	1
//#define NXS_NET_DEBUG_6 	1
//#define NXS_NET_DEBUG_7 	1
//#define NXS_NET_DEBUG_8 	1
//#define NXS_NET_DEBUG_9 	1

//#define NXS_FRAG

// The constant below have a direct influence on how fast forums/channels/posted/identity groups propagate and on the overloading of queues:
//
// Channels/forums will update at a rate of SYNC_PERIOD*MAX_REQLIST_SIZE/60 messages per minute.
// A large TRANSAC_TIMEOUT helps large transactions to finish before anything happens (e.g. disconnexion) or when the server has low upload bandwidth,
// but also uses more memory.
// A small value for MAX_REQLIST_SIZE is likely to help messages to propagate in a chaotic network, but will also slow them down.
// A small SYNC_PERIOD fasten message propagation, but is likely to overload the server side of transactions (e.g. overload outqueues).
//
//static const uint32_t GIXS_CUT_OFF                            =            0;
static const uint32_t SYNC_PERIOD                             =           60;
static const uint32_t MAX_REQLIST_SIZE                        =           20; // No more than 20 items per msg request list => creates smaller transactions that are less likely to be cancelled.
static const uint32_t TRANSAC_TIMEOUT                         =         2000; // In seconds. Has been increased to avoid epidemic transaction cancelling due to overloaded outqueues.
#ifdef TO_REMOVE
static const uint32_t SECURITY_DELAY_TO_FORCE_CLIENT_REUPDATE =         3600; // force re-update if there happens to be a large delay between our server side TS and the client side TS of friends
#endif
static const uint32_t REJECTED_MESSAGE_RETRY_DELAY            =      24*3600; // re-try rejected messages every 24hrs. Most of the time this is because the peer's reputation has changed.
static const uint32_t GROUP_STATS_UPDATE_DELAY                =          240; // update unsubscribed group statistics every 3 mins
static const uint32_t GROUP_STATS_UPDATE_NB_PEERS             =            2; // number of peers to which the group stats are asked
static const uint32_t MAX_ALLOWED_GXS_MESSAGE_SIZE            =       199000; // 200,000 bytes including signature and headers
static const uint32_t MIN_DELAY_BETWEEN_GROUP_SEARCH          =           40; // dont search same group more than every 40 secs.
static const uint32_t SAFETY_DELAY_FOR_UNSUCCESSFUL_UPDATE    =            0; // avoid re-sending the same msg list to a peer who asks twice for the same update in less than this time

static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN             = 0x00 ;
static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_NO_ERROR            = 0x01 ;
static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_CIRCLE_ERROR        = 0x02 ;
static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_ENCRYPTION_ERROR    = 0x03 ;
static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_SERIALISATION_ERROR = 0x04 ;
static const uint32_t RS_NXS_ITEM_ENCRYPTION_STATUS_GXS_KEY_MISSING     = 0x05 ;

// Debug system to allow to print only for some IDs (group, Peer, etc)

#if defined(NXS_NET_DEBUG_0) || defined(NXS_NET_DEBUG_1) || defined(NXS_NET_DEBUG_2)  || defined(NXS_NET_DEBUG_3) \
 || defined(NXS_NET_DEBUG_4) || defined(NXS_NET_DEBUG_5) || defined(NXS_NET_DEBUG_6)  || defined(NXS_NET_DEBUG_7) \
 || defined(NXS_NET_DEBUG_8) || defined(NXS_NET_DEBUG_9)

static const RsPeerId     peer_to_print     = RsPeerId();//std::string("a97fef0e2dc82ddb19200fb30f9ac575"))   ;
static const RsGxsGroupId group_id_to_print = RsGxsGroupId();//std::string("66052380f5d1d0c5992e2b55dc402ce6")) ;	// use this to allow to this group id only, or "" for all IDs
static const uint32_t     service_to_print  = RS_SERVICE_GXS_TYPE_CHANNELS;                       	// use this to allow to this service id only, or 0 for all services
										// warning. Numbers should be SERVICE IDS (see serialiser/rsserviceids.h. E.g. 0x0215 for forums)

class nullstream: public std::ostream {};

static std::string nice_time_stamp(rstime_t now,rstime_t TS)
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

static std::ostream& gxsnetdebug(const RsPeerId& peer_id,const RsGxsGroupId& grp_id,uint32_t service_type)
{
    static nullstream null ;

    if((peer_to_print.isNull() || peer_id.isNull() || peer_id == peer_to_print)
    && (group_id_to_print.isNull() || grp_id.isNull() || grp_id == group_id_to_print)
    && (service_to_print==0 || service_type == 0 || ((service_type >> 8)&0xffff) == service_to_print))
        return std::cerr << time(NULL) << ":GXSNETSERVICE: " ;
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
                                 PgpAuxUtils *pgpUtils, RsGxsNetTunnelService *mGxsNT,
                                 bool grpAutoSync, bool msgAutoSync, bool distSync, uint32_t default_store_period, uint32_t default_sync_period)
                                 : p3ThreadedService(), p3Config(), mTransactionN(0),
                                   mObserver(nxsObs), mDataStore(gds),
                                   mServType(servType), mTransactionTimeOut(TRANSAC_TIMEOUT),
                                   mNetMgr(netMgr), mNxsMutex("RsGxsNetService"),
                                   mSyncTs(0), mLastKeyPublishTs(0),
                                   mLastCleanRejectedMessages(0), mSYNC_PERIOD(SYNC_PERIOD),
                                   mCircles(circles), mGixs(gixs),
                                   mReputations(reputations), mPgpUtils(pgpUtils), mGxsNetTunnel(mGxsNT),
                                   mGrpAutoSync(grpAutoSync), mAllowMsgSync(msgAutoSync),mAllowDistSync(distSync),
                                   mServiceInfo(serviceInfo), mDefaultMsgStorePeriod(default_store_period),
                                   mDefaultMsgSyncPeriod(default_sync_period)
{
	addSerialType(new RsNxsSerialiser(mServType));
	mOwnId = mNetMgr->getOwnId();
    mUpdateCounter = 0;

	mLastCacheReloadTS = 0;

	// check the consistency

	if(mDefaultMsgStorePeriod > 0 && mDefaultMsgSyncPeriod > mDefaultMsgStorePeriod)
	{
		std::cerr << "(WW) in GXS service \"" << getServiceInfo().mServiceName << "\":  too large message sync period will be set to message store period." << std::endl;
		mDefaultMsgSyncPeriod = mDefaultMsgStorePeriod ;
	}
}

void RsGxsNetService::getItemNames(std::map<uint8_t,std::string>& names) const
{
	names.clear();

	names[RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM    ] = "Group Sync Request" ;
	names[RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM        ] = "Group Sync" ;
	names[RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM  ] = "Group Stats" ;
	names[RS_PKT_SUBTYPE_NXS_GRP_ITEM             ] = "Group Data" ;
	names[RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM  ] = "Encrypted data" ;
	names[RS_PKT_SUBTYPE_NXS_SESSION_KEY_ITEM     ] = "Session Key" ;
	names[RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM        ] = "Message Sync" ;
	names[RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM    ] = "Message Sync Request" ;
	names[RS_PKT_SUBTYPE_NXS_MSG_ITEM             ] = "Message Data" ;
	names[RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM         ] = "Transaction" ;
	names[RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM ] = "Publish key" ;
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

    mClientGrpUpdateMap.clear() ;
    mServerMsgUpdateMap.clear() ;
}


int RsGxsNetService::tick()
{
	// always check for new items arriving
	// from peers
	recvNxsItemQueue();

    bool should_notify = false;

    {
	    RS_STACK_MUTEX(mNxsMutex) ;

	    should_notify = should_notify || !mNewGroupsToNotify.empty() ;
	    should_notify = should_notify || !mNewMessagesToNotify.empty() ;
	    should_notify = should_notify || !mNewPublishKeysToNotify.empty() ;
	    should_notify = should_notify || !mNewStatsToNotify.empty() ;
        should_notify = should_notify || !mNewGrpSyncParamsToNotify.empty() ;
    }

    if(should_notify)
        processObserverNotifications() ;

    rstime_t now = time(NULL);
    rstime_t elapsed = mSYNC_PERIOD + mSyncTs;

    if((elapsed) < now)
    {
        syncWithPeers();
        syncGrpStatistics();
		checkDistantSyncState();

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

    // Observer notifycation should never be done explicitly within a Mutex-protected region, because of the risk
    // of causing a cross-deadlock between the observer (RsGxsGenExchange) and the network layer (RsGxsNetService).

    std::vector<RsNxsGrp*> grps_copy ;
    std::vector<RsNxsMsg*> msgs_copy ;
    std::set<RsGxsGroupId> stat_copy ;
    std::set<RsGxsGroupId> keys_copy,grpss_copy ;

    {
	    RS_STACK_MUTEX(mNxsMutex) ;

	    grps_copy = mNewGroupsToNotify ;
	    msgs_copy = mNewMessagesToNotify ;
	    stat_copy = mNewStatsToNotify ;
	    keys_copy = mNewPublishKeysToNotify ;
        grpss_copy = mNewGrpSyncParamsToNotify ;

	    mNewGroupsToNotify.clear() ;
	    mNewMessagesToNotify.clear() ;
	    mNewStatsToNotify.clear() ;
	    mNewPublishKeysToNotify.clear() ;
        mNewGrpSyncParamsToNotify.clear() ;
    }

    if(!grps_copy.empty()) mObserver->receiveNewGroups  (grps_copy);
    if(!msgs_copy.empty()) mObserver->receiveNewMessages(msgs_copy);

    for(std::set<RsGxsGroupId>::const_iterator it(keys_copy.begin());it!=keys_copy.end();++it)
		mObserver->notifyReceivePublishKey(*it);

    for(std::set<RsGxsGroupId>::const_iterator it(stat_copy.begin());it!=stat_copy.end();++it)
		mObserver->notifyChangedGroupStats(*it);

    for(std::set<RsGxsGroupId>::const_iterator it(grpss_copy.begin());it!=grpss_copy.end();++it)
        mObserver->notifyChangedGroupSyncParams(*it);
}

void RsGxsNetService::rejectMessage(const RsGxsMessageId& msg_id)
{
    RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "adding message " << msg_id << " to rejection list for 24hrs." << std::endl;
#endif
    mRejectedMessages[msg_id] = time(NULL) ;
}
void RsGxsNetService::cleanRejectedMessages()
{
    RS_STACK_MUTEX(mNxsMutex) ;
    rstime_t now = time(NULL) ;

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "Cleaning rejected messages." << std::endl;
#endif

    for(std::map<RsGxsMessageId,rstime_t>::iterator it(mRejectedMessages.begin());it!=mRejectedMessages.end();)
        if(it->second + REJECTED_MESSAGE_RETRY_DELAY < now)
	{
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG___ << "  message id " << it->first << " should be re-tried. removing from list..." << std::endl;
#endif

		std::map<RsGxsMessageId,rstime_t>::iterator tmp = it ;
		++tmp ;
		mRejectedMessages.erase(it) ;
		it=tmp ;
	}
	else
            ++it ;
}

RsGxsGroupId RsGxsNetService::hashGrpId(const RsGxsGroupId& gid,const RsPeerId& pid)
{
    static const uint32_t SIZE = RsGxsGroupId::SIZE_IN_BYTES + RsPeerId::SIZE_IN_BYTES ;
    unsigned char tmpmem[SIZE];
    uint32_t offset = 0 ;

    pid.serialise(tmpmem,SIZE,offset) ;
    gid.serialise(tmpmem,SIZE,offset) ;

    assert(RsGxsGroupId::SIZE_IN_BYTES <= Sha1CheckSum::SIZE_IN_BYTES) ;

    return RsGxsGroupId( RsDirUtil::sha1sum(tmpmem,SIZE).toByteArray() );
}

void RsGxsNetService::syncWithPeers()
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG___ << "RsGxsNetService::syncWithPeers() this=" << (void*)this << ". serviceInfo=" << mServiceInfo << std::endl;
#endif

    static RsNxsSerialiser ser(mServType) ;	// this is used to estimate bandwidth.

    RS_STACK_MUTEX(mNxsMutex) ;

    std::set<RsPeerId> peers;
    mNetMgr->getOnlineList(mServiceInfo.mServiceType, peers);

	if(mAllowDistSync && mGxsNetTunnel != NULL)
	{
		// Grab all online virtual peers of distant tunnels for the current service.

		std::list<RsGxsNetTunnelVirtualPeerId> vpids ;
		mGxsNetTunnel->getVirtualPeers(vpids);

		for(auto it(vpids.begin());it!=vpids.end();++it)
			peers.insert(RsPeerId(*it)) ;
	}

    if (peers.empty()) {
        // nothing to do
        return;
    }

	// for now just grps
	for(auto sit = peers.begin(); sit != peers.end(); ++sit)
	{

		const RsPeerId peerId = *sit;

		ClientGrpMap::const_iterator cit = mClientGrpUpdateMap.find(peerId);
		uint32_t updateTS = 0;

		if(cit != mClientGrpUpdateMap.end())
		{
			const RsGxsGrpUpdate *gui = &cit->second;
			updateTS = gui->grpUpdateTS;
		}
		RsNxsSyncGrpReqItem *grp = new RsNxsSyncGrpReqItem(mServType);
		grp->clear();
		grp->PeerId(*sit);
		grp->updateTS = updateTS;

#ifdef NXS_NET_DEBUG_5
		GXSNETDEBUG_P_(*sit) << "Service "<< std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << "  sending global group TS of peer id: " << *sit << " ts=" << nice_time_stamp(time(NULL),updateTS) << " (secs ago) to himself" << std::endl;
#endif
		generic_sendItem(grp);
	}

    if(!mAllowMsgSync)
        return ;

#ifndef GXS_DISABLE_SYNC_MSGS

    RsGxsGrpMetaTemporaryMap grpMeta;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    RsGxsGrpMetaTemporaryMap toRequest;

    for(RsGxsGrpMetaTemporaryMap::iterator mit = grpMeta.begin(); mit != grpMeta.end(); ++mit)
    {
	    RsGxsGrpMetaData* meta = mit->second;

	    // This was commented out because we want to know how many messages are available for unsubscribed groups.

	    if(meta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED )
	    {
		    toRequest.insert(std::make_pair(mit->first, meta));
		    mit->second = NULL ;	// avoids destruction ;-)
	    }
    }

    // Synchronise group msg for groups which we're subscribed to
    // For each peer and each group, we send to the peer the time stamp of the most
    // recent modification the peer has sent. If the peer has more recent messages he will send them, because its latest
    // modifications will be more recent. This ensures that we always compare timestamps all taken in the same
    // computer (the peer's computer in this case)

    for(auto sit = peers.begin(); sit != peers.end(); ++sit)
    {
        const RsPeerId& peerId = *sit;

        // now see if you have an updateTS so optimise whether you need
        // to get a new list of peer data
        const RsGxsMsgUpdate *mui = NULL;

        ClientMsgMap::const_iterator cit = mClientMsgUpdateMap.find(peerId);

        if(cit != mClientMsgUpdateMap.end())
            mui = &cit->second;

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(peerId) << "  syncing messages with peer " << peerId << std::endl;
#endif

        RsGxsGrpMetaTemporaryMap::const_iterator mmit = toRequest.begin();
        for(; mmit != toRequest.end(); ++mmit)
        {
            const RsGxsGrpMetaData* meta = mmit->second;
            const RsGxsGroupId& grpId = mmit->first;
            RsGxsCircleId encrypt_to_this_circle_id ;

            if(!checkCanRecvMsgFromPeer(peerId, *meta,encrypt_to_this_circle_id))
                continue;

#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_PG(peerId,grpId) << "    peer can send messages for group " << grpId ;
	    if(!encrypt_to_this_circle_id.isNull())
		    GXSNETDEBUG_PG(peerId,grpId) << " request should be encrypted for circle ID " << encrypt_to_this_circle_id << std::endl;
	    else
		    GXSNETDEBUG_PG(peerId,grpId) << " request should be sent in clear." << std::endl;

#endif
            // On default, the info has never been received so the TS is 0, meaning the peer has sent that it had no information.

            uint32_t updateTS = 0;

            if(mui)
            {
                std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator cit2 = mui->msgUpdateInfos.find(grpId);

                if(cit2 != mui->msgUpdateInfos.end())
                    updateTS = cit2->second.time_stamp;
            }

            // get sync params for this group

            RsNxsSyncMsgReqItem* msg = new RsNxsSyncMsgReqItem(mServType);

            msg->clear();
            msg->PeerId(peerId);
            msg->updateTS = updateTS;

            int req_delay  = (int)locked_getGrpConfig(grpId).msg_req_delay ;
            int keep_delay = (int)locked_getGrpConfig(grpId).msg_keep_delay ;

            // If we store for less than we request, we request less, otherwise the posts will be deleted after being obtained.

            if(keep_delay > 0 && req_delay > 0 && keep_delay < req_delay)
                req_delay = keep_delay ;

            // The last post will be set to TS 0 if the req delay is 0, which means "Indefinitly"

            if(req_delay > 0)
				msg->createdSinceTS = std::max(0,(int)time(NULL) - req_delay);
			else
				msg->createdSinceTS = 0 ;

            if(encrypt_to_this_circle_id.isNull())
                msg->grpId = grpId;
            else
            {
                msg->grpId = hashGrpId(grpId,mNetMgr->getOwnId()) ;
                msg->flag |= RsNxsSyncMsgReqItem::FLAG_USE_HASHED_GROUP_ID ;
            }

#ifdef NXS_NET_DEBUG_7
	    GXSNETDEBUG_PG(*sit,grpId) << "    Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << "  sending message TS of peer id: " << *sit << " ts=" << nice_time_stamp(time(NULL),updateTS) << " (secs ago) for group " << grpId << " to himself - in clear " << std::endl;
#endif
		generic_sendItem(msg);

#ifdef NXS_NET_DEBUG_5
		GXSNETDEBUG_PG(*sit,grpId) << "Service "<< std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << "  sending global message TS of peer id: " << *sit << " ts=" << nice_time_stamp(time(NULL),updateTS) << " (secs ago) for group " << grpId << " to himself" << std::endl;
#endif
        }
    }

#endif
}

void RsGxsNetService::generic_sendItem(RsNxsItem *si)
{
	// check if the item is to be sent to a distant peer or not

	RsGxsGroupId tmp_grpId;

	if(mAllowDistSync && mGxsNetTunnel != NULL && mGxsNetTunnel->isDistantPeer( static_cast<RsGxsNetTunnelVirtualPeerId>(si->PeerId()),tmp_grpId))
	{
		RsNxsSerialiser ser(mServType);

		uint32_t size = ser.size(si);
		unsigned char *mem = (unsigned char *)rs_malloc(size) ;

		if(!mem)
			return ;

#ifdef NXS_NET_DEBUG_8
        GXSNETDEBUG_P_(si->PeerId()) << "Sending RsGxsNetTunnelService Item:" << (void*)si << " of type: " << std::hex << si->PacketId() << std::dec
		                               << " transaction " << si->transactionNumber << " to virtual peer " << si->PeerId() << std::endl ;
#endif
		ser.serialise(si,mem,&size) ;

		mGxsNetTunnel->sendTunnelData(mServType,mem,size,static_cast<RsGxsNetTunnelVirtualPeerId>(si->PeerId()));
        delete si;
	}
	else
		sendItem(si) ;
}

void RsGxsNetService::checkDistantSyncState()
{
	if(!mAllowDistSync || mGxsNetTunnel==NULL || !mGrpAutoSync)
		return ;

	RsGxsGrpMetaTemporaryMap grpMeta;
    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    // Go through group statistics and groups without information are re-requested to random peers selected
    // among the ones who provided the group info.

#ifdef NXS_NET_DEBUG_8
    GXSNETDEBUG___<< "Checking distant sync for all groups." << std::endl;
#endif
	// get the list of online peers

    std::set<RsPeerId> online_peers;
    mNetMgr->getOnlineList(mServiceInfo.mServiceType , online_peers);

    RS_STACK_MUTEX(mNxsMutex) ;

    for(auto it(grpMeta.begin());it!=grpMeta.end();++it)
		if(it->second->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)	// we only consider subscribed groups here.
	    {
#warning (cyril) We might need to also remove peers for recently unsubscribed groups
			const RsGxsGroupId& grpId(it->first);
		    const RsGxsGrpConfig& rec = locked_getGrpConfig(grpId) ;

#ifdef NXS_NET_DEBUG_6
		    GXSNETDEBUG__G(it->first) << "    group " << grpId;
#endif
			bool at_least_one_friend_is_supplier = false ;

			for(auto it2(rec.suppliers.ids.begin());it2!=rec.suppliers.ids.end() && !at_least_one_friend_is_supplier;++it2)
				if(online_peers.find(*it2) != online_peers.end())	                // check that the peer is online
					at_least_one_friend_is_supplier = true ;

			// That strategy is likely to create islands of friends connected to each other. There's no real way
			// to decide what to do here, except maybe checking the last message TS remotely vs. locally.

			if(at_least_one_friend_is_supplier)
			{
				mGxsNetTunnel->releaseDistantPeers(mServType,grpId);
#ifdef NXS_NET_DEBUG_8
				GXSNETDEBUG___<< "  Group " << grpId << ": suppliers among friends. Releasing peers." << std::endl;
#endif
			}
			else
			{
				mGxsNetTunnel->requestDistantPeers(mServType,grpId);
#ifdef NXS_NET_DEBUG_8
				GXSNETDEBUG___<< "  Group " << grpId << ": no suppliers among friends. Requesting peers." << std::endl;
#endif
			}
		}
}

void RsGxsNetService::syncGrpStatistics()
{
    RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_6
    GXSNETDEBUG___<< "Sync-ing group statistics." << std::endl;
#endif
    RsGxsGrpMetaTemporaryMap grpMeta;

    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    std::set<RsPeerId> online_peers;
    mNetMgr->getOnlineList(mServiceInfo.mServiceType, online_peers);

	if(mAllowDistSync && mGxsNetTunnel != NULL)
	{
		// Grab all online virtual peers of distant tunnels for the current service.

		std::list<RsGxsNetTunnelVirtualPeerId> vpids ;
		mGxsNetTunnel->getVirtualPeers(vpids);

		for(auto it(vpids.begin());it!=vpids.end();++it)
			online_peers.insert(RsPeerId(*it)) ;
	}

    // Go through group statistics and groups without information are re-requested to random peers selected
    // among the ones who provided the group info.

    rstime_t now = time(NULL) ;

    for(auto it(grpMeta.begin());it!=grpMeta.end();++it)
    {
	    const RsGxsGrpConfig& rec = locked_getGrpConfig(it->first) ;

#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG__G(it->first) << "    group " << it->first ;
#endif

	    if(rec.statistics_update_TS + GROUP_STATS_UPDATE_DELAY < now && rec.suppliers.ids.size() > 0)
		{
#ifdef NXS_NET_DEBUG_6
			GXSNETDEBUG__G(it->first) << " needs update. Randomly asking to some friends" << std::endl;
#endif
			// randomly select GROUP_STATS_UPDATE_NB_PEERS friends among the suppliers of this group

			uint32_t n = RSRandom::random_u32() % rec.suppliers.ids.size() ;

			std::set<RsPeerId>::const_iterator rit = rec.suppliers.ids.begin();
			for(uint32_t i=0;i<n;++i)
				++rit ;

			for(uint32_t i=0;i<std::min(rec.suppliers.ids.size(),(size_t)GROUP_STATS_UPDATE_NB_PEERS);++i)
			{
				// we started at a random position in the set, wrap around if the end is reached
				if(rit == rec.suppliers.ids.end())
					rit = rec.suppliers.ids.begin() ;

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

					generic_sendItem(grs) ;
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
	RsGxsGrpMetaTemporaryMap grpMetas;
	    grpMetas[grs->grpId] = NULL;

	    mDataStore->retrieveGxsGrpMetaData(grpMetas);

	    const RsGxsGrpMetaData* grpMeta = grpMetas[grs->grpId];

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
	    reqIds[grs->grpId] = std::set<RsGxsMessageId>();
	    GxsMsgMetaResult result;

#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  retrieving message information." << std::endl;
#endif
	    mDataStore->retrieveGxsMsgMetaData(reqIds, result);

	    const std::vector<const RsGxsMsgMetaData*>& vec(result[grs->grpId]) ;

	    if(vec.empty())	// that means we don't have any, or there isn't any, but since the default is always 0, no need to send.
		    return ;

	    RsNxsSyncGrpStatsItem *grs_resp = new RsNxsSyncGrpStatsItem(mServType) ;
	    grs_resp->request_type = RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_RESPONSE ;
	    grs_resp->number_of_posts = vec.size();
	    grs_resp->grpId = grs->grpId;
	    grs_resp->PeerId(grs->PeerId()) ;

	    grs_resp->last_post_TS = grpMeta->mPublishTs ;	// This is not zero, and necessarily older than any message in the group up to clock precision.
														// This allows us to use 0 as "uninitialized" proof. If the group meta has been changed, this time
														// will be more recent than some messages. This shouldn't be a problem, since this value can only
														// be used to discard groups that are not used.

	    for(uint32_t i=0;i<vec.size();++i)
		    if(grs_resp->last_post_TS < vec[i]->mPublishTs)
			    grs_resp->last_post_TS = vec[i]->mPublishTs;

#ifdef NXS_NET_DEBUG_6
	    GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "  sending back statistics item with " << vec.size() << " elements." << std::endl;
#endif

	    generic_sendItem(grs_resp) ;
    }
    else if(grs->request_type == RsNxsSyncGrpStatsItem::GROUP_INFO_TYPE_RESPONSE)
	{
#ifdef NXS_NET_DEBUG_6
	   GXSNETDEBUG_PG(grs->PeerId(),grs->grpId) << "Received Grp update stats item from peer " << grs->PeerId() << " for group " << grs->grpId << ", reporting " << grs->number_of_posts << " posts." << std::endl;
#endif
	   RS_STACK_MUTEX(mNxsMutex) ;

	   RsGxsGrpConfig& rec(locked_getGrpConfig(grs->grpId)) ;

	   uint32_t old_count = rec.max_visible_count ;
	   uint32_t old_suppliers_count = rec.suppliers.ids.size() ;

	   rec.suppliers.ids.insert(grs->PeerId()) ;
	   rec.max_visible_count = std::max(rec.max_visible_count,grs->number_of_posts) ;
	   rec.statistics_update_TS = time(NULL) ;
	   rec.last_group_modification_TS = grs->last_post_TS;

	   if (old_count != rec.max_visible_count || old_suppliers_count != rec.suppliers.ids.size())
		  mNewStatsToNotify.insert(grs->grpId) ;
	}
    else
        std::cerr << "(EE) RsGxsNetService::handleRecvSyncGrpStatistics(): unknown item type " << grs->request_type << " found. This is a bug." << std::endl;
}

// This function is useful when we need to force a new sync of messages from  all friends when e.g. the delay for sync gets changed.
// Normally, when subscribing to a group (not needed when unsubscribing), we should also call this method.

void RsGxsNetService::locked_resetClientTS(const RsGxsGroupId& grpId)
{
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG__G(grpId) << "Resetting client TS for grp " << grpId << std::endl;
#endif

    for(ClientMsgMap::iterator it(mClientMsgUpdateMap.begin());it!=mClientMsgUpdateMap.end();++it)
        it->second.msgUpdateInfos.erase(grpId) ;
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
    std::map<RsGxsGroupId,RsGxsServerMsgUpdate>::iterator it = mServerMsgUpdateMap.find(grpId) ;
#endif

    RsGxsServerMsgUpdate& item(mServerMsgUpdateMap[grpId]) ;

	item.msgUpdateTS = time(NULL) ;

    // We also update mGrpServerUpdateItem so as to trigger a new grp list exchange with friends (friends will send their known ClientTS which
    // will be lower than our own grpUpdateTS, triggering our sending of the new subscribed grp list.

    mGrpServerUpdate.grpUpdateTS = time(NULL) ;

    if(subscribed)
        locked_resetClientTS(grpId) ;
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


bool RsGxsNetService::locked_createTransactionFromPending(GrpCircleIdRequestVetting* grpPend)
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

		    if(entry.mShouldEncrypt)
		    {
#ifdef NXS_NET_DEBUG_7
			    GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGroupId) << "    item for this grpId should be encrypted." << std::endl;
#endif
			    RsNxsItem *encrypted_item = NULL ;
			    uint32_t status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;

			    if(encryptSingleNxsItem(gItem, entry.mCircleId, entry.mGroupId,encrypted_item,status))
			    {
				    itemL.push_back(encrypted_item) ;
				    delete gItem ;
			    }
#ifdef NXS_NET_DEBUG_7
			    else
				    GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGroupId) << "    Could not encrypt item for grpId " << entry.mGroupId << " for circle " << entry.mCircleId << ". Will try later. Adding to vetting list." << std::endl;
#endif
		    }
		    else
			    itemL.push_back(gItem);
	    }
#ifdef NXS_NET_DEBUG_1
	    else
		    GXSNETDEBUG_PG(grpPend->mPeerId,entry.mGroupId) << "  Group Id: " << entry.mGroupId << " FAILED" << std::endl;
#endif
    }

    if(!itemL.empty())
	    locked_pushGrpRespFromList(itemL, grpPend->mPeerId, transN);

    return true ;
}

bool RsGxsNetService::locked_createTransactionFromPending(MsgCircleIdsRequestVetting* msgPend)
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

        	grp_id = msgPend->mGrpId ;

            	if(msgPend->mShouldEncrypt)
                {
                    RsNxsItem *encrypted_item = NULL ;
                    uint32_t status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;

                    if(encryptSingleNxsItem(mItem,msgPend->mCircleId,msgPend->mGrpId,encrypted_item,status))
                    {
                        itemL.push_back(encrypted_item) ;
                        delete mItem ;
                    }
                    else
                    {
                        std::cerr << "(EE) cannot encrypt Msg ids in circle-restriced response to grp " << msgPend->mGrpId << " for circle " << msgPend->mCircleId << std::endl;
                        return false ;
                    }
                }
                else
			itemL.push_back(mItem);
	}

	if(!itemL.empty())
		locked_pushMsgRespFromList(itemL, msgPend->mPeerId,grp_id, transN);

    return true ;
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

    StoreHere(RsGxsNetService::ClientGrpMap& cgm, RsGxsNetService::ClientMsgMap& cmm, RsGxsNetService::ServerMsgMap& smm,RsGxsNetService::GrpConfigMap& gcm, RsGxsServerGrpUpdate& sgm)
            : mClientGrpMap(cgm), mClientMsgMap(cmm), mServerMsgMap(smm), mGrpConfigMap(gcm), mServerGrpUpdate(sgm)
    {}

	template <typename ID_type,typename UpdateMap,class ItemClass> void check_store(ID_type id,UpdateMap& map,ItemClass& item)
    {
        if(!id.isNull())
            map.insert(std::make_pair(id,item)) ;
       	else
            std::cerr << "(EE) loaded a null ID for class type " << typeid(map).name() << std::endl;
    }

    void operator() (RsItem* item)
    {
        RsGxsMsgUpdateItem        *mui;
        RsGxsGrpUpdateItem        *gui;
        RsGxsServerGrpUpdateItem  *gsui;
        RsGxsServerMsgUpdateItem  *msui;
        RsGxsGrpConfigItem        *mgci;

        if((mui = dynamic_cast<RsGxsMsgUpdateItem*>(item)) != NULL)
            check_store(mui->peerID,mClientMsgMap,*mui);
        else if((mgci = dynamic_cast<RsGxsGrpConfigItem*>(item)) != NULL)
            check_store(mgci->grpId,mGrpConfigMap, *mgci);
        else if((gui = dynamic_cast<RsGxsGrpUpdateItem*>(item)) != NULL)
            check_store(gui->peerID,mClientGrpMap, *gui);
        else if((msui = dynamic_cast<RsGxsServerMsgUpdateItem*>(item)) != NULL)
            check_store(msui->grpId,mServerMsgMap, *msui);
        else if((gsui = dynamic_cast<RsGxsServerGrpUpdateItem*>(item)) != NULL)
            mServerGrpUpdate = *gsui;
        else
            std::cerr << "Type not expected!" << std::endl;

        delete item ;
    }

private:

    RsGxsNetService::ClientGrpMap& mClientGrpMap;
    RsGxsNetService::ClientMsgMap& mClientMsgMap;
    RsGxsNetService::ServerMsgMap& mServerMsgMap;
    RsGxsNetService::GrpConfigMap& mGrpConfigMap;

    RsGxsServerGrpUpdate& mServerGrpUpdate;
};

bool RsGxsNetService::loadList(std::list<RsItem *> &load)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    // The delete is done in StoreHere, if necessary

    std::for_each(load.begin(), load.end(), StoreHere(mClientGrpUpdateMap, mClientMsgUpdateMap, mServerMsgUpdateMap, mServerGrpConfigMap, mGrpServerUpdate));
	rstime_t now = time(NULL);

    // We reset group statistics here. This is the best place since we know at this point which are all unsubscribed groups.

    for(GrpConfigMap::iterator it(mServerGrpConfigMap.begin());it!=mServerGrpConfigMap.end();++it)
	{
		// At each reload, we reset the count of visible messages. It will be rapidely restored to its real value from friends.

		it->second.max_visible_count = 0; // std::max(it2->second.message_count,gnsr.max_visible_count) ;

		// the update time stamp is randomised so as not to ask all friends at once about group statistics.

		it->second.statistics_update_TS = now - GROUP_STATS_UPDATE_DELAY + (RSRandom::random_u32()%(GROUP_STATS_UPDATE_DELAY/10)) ;

		// Similarly, we remove all suppliers.
		// Actual suppliers will come back automatically.

		it->second.suppliers.ids.clear() ;

		// also make sure that values stored for keep and req delays correspond to the canonical values

		locked_checkDelay(it->second.msg_req_delay);
		locked_checkDelay(it->second.msg_keep_delay);
	}

    return true;
}

void RsGxsNetService::locked_checkDelay(uint32_t& time_in_secs)
{
    if(time_in_secs <    1 * 86400) { time_in_secs =   0        ; return ; }
    if(time_in_secs <=  10 * 86400) { time_in_secs =   5 * 86400; return ; }
    if(time_in_secs <=  20 * 86400) { time_in_secs =  15 * 86400; return ; }
    if(time_in_secs <=  60 * 86400) { time_in_secs =  30 * 86400; return ; }
    if(time_in_secs <= 120 * 86400) { time_in_secs =  90 * 86400; return ; }
    if(time_in_secs <= 250 * 86400) { time_in_secs = 180 * 86400; return ; }
                                      time_in_secs = 365 * 86400;
}

#include <algorithm>

template <typename UpdateMap,class ItemClass>
struct get_second : public std::unary_function<typename UpdateMap::value_type, RsItem*>
{
    get_second(uint16_t serv_type,typename UpdateMap::key_type ItemClass::*member): mServType(serv_type),ID_member(member) {}

    RsItem* operator()(const typename UpdateMap::value_type& value) const
    {
        ItemClass *item = new ItemClass(value.second,mServType);
        (*item).*ID_member = value.first ;
        return item ;
    }

    uint16_t mServType ;
    typename UpdateMap::key_type ItemClass::*ID_member ;
};


bool RsGxsNetService::saveList(bool& cleanup, std::list<RsItem*>& save)
{
	RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_0
    std::cerr << "RsGxsNetService::saveList()..." << std::endl;
#endif

    // hardcore templates
    std::transform(mClientGrpUpdateMap.begin(), mClientGrpUpdateMap.end(), std::back_inserter(save), get_second<ClientGrpMap,RsGxsGrpUpdateItem>(mServType,&RsGxsGrpUpdateItem::peerID));
    std::transform(mClientMsgUpdateMap.begin(), mClientMsgUpdateMap.end(), std::back_inserter(save), get_second<ClientMsgMap,RsGxsMsgUpdateItem>(mServType,&RsGxsMsgUpdateItem::peerID));
    std::transform(mServerMsgUpdateMap.begin(), mServerMsgUpdateMap.end(), std::back_inserter(save), get_second<ServerMsgMap,RsGxsServerMsgUpdateItem>(mServType,&RsGxsServerMsgUpdateItem::grpId));
    std::transform(mServerGrpConfigMap.begin(), mServerGrpConfigMap.end(), std::back_inserter(save), get_second<GrpConfigMap,RsGxsGrpConfigItem>(mServType,&RsGxsGrpConfigItem::grpId));

    RsGxsServerGrpUpdateItem *it = new RsGxsServerGrpUpdateItem(mGrpServerUpdate,mServType) ;

    save.push_back(it);

    cleanup = true;
    return true;
}

RsSerialiser *RsGxsNetService::setupSerialiser()
{

    RsSerialiser *rss = new RsSerialiser;
    rss->addSerialType(new RsGxsUpdateSerialiser(mServType));

    return rss;
}

RsItem *RsGxsNetService::generic_recvItem()
{
	{
		RsItem *item ;

		if(NULL != (item=recvItem()))
			return item ;
	}

	unsigned char *data = NULL ;
	uint32_t size = 0 ;
	RsGxsNetTunnelVirtualPeerId virtual_peer_id ;

	while(mAllowDistSync && mGxsNetTunnel!=NULL && mGxsNetTunnel->receiveTunnelData(mServType,data,size,virtual_peer_id))
	{
		RsNxsItem *item = dynamic_cast<RsNxsItem*>(RsNxsSerialiser(mServType).deserialise(data,&size)) ;
		item->PeerId(virtual_peer_id) ;

		free(data) ;

		if(!item)
			continue ;

#ifdef NXS_NET_DEBUG_8
        GXSNETDEBUG_P_(item->PeerId()) << "Received RsGxsNetTunnelService Item:" << (void*)item << " of type: " << std::hex << item->PacketId() << std::dec
		                               << " transaction " << item->transactionNumber << " from virtual peer " << item->PeerId() << std::endl ;
#endif
		return item ;
	}

	return NULL ;
}

void RsGxsNetService::recvNxsItemQueue()
{
    RsItem *item ;

    while(NULL != (item=generic_recvItem()))
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

            // Check whether the item is encrypted. If so, try to decrypt it, and replace ni with the decrypted item..

            bool item_was_encrypted = false ;

            if(ni->PacketSubType() == RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM)
            {
                RsNxsItem *decrypted_item ;

                if(decryptSingleNxsItem(dynamic_cast<RsNxsEncryptedDataItem*>(ni),decrypted_item))
                {
                    item = ni = decrypted_item ;
                    item_was_encrypted = true ;
#ifdef NXS_NET_DEBUG_7
		    GXSNETDEBUG_P_(item->PeerId()) << "    decrypted item "  << std::endl;
#endif
                }
#ifdef NXS_NET_DEBUG_7
                else
                    GXSNETDEBUG_P_(item->PeerId()) << "    (EE) Could not decrypt incoming encrypted NXS item.  Probably a friend subscribed to a circle-restricted group." << std::endl;
#endif
            }

            switch(ni->PacketSubType())
            {
            case RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM: handleRecvSyncGrpStatistics   (dynamic_cast<RsNxsSyncGrpStatsItem*>(ni)) ; break ;
            case RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM:   handleRecvSyncGroup           (dynamic_cast<RsNxsSyncGrpReqItem*>(ni)) ; break ;
            case RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM:   handleRecvSyncMessage         (dynamic_cast<RsNxsSyncMsgReqItem*>(ni),item_was_encrypted) ; break ;
            case RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM:handleRecvPublishKeys         (dynamic_cast<RsNxsGroupPublishKeyItem*>(ni)) ; break ;

            default:
                if(ni->PacketSubType() != RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM)
                {
                    std::cerr << "Unhandled item subtype " << (uint32_t) ni->PacketSubType() << " in RsGxsNetService: " << std::endl ; break ;
                }
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

void RsGxsNetService::threadTick()
{
    static const double timeDelta = 0.5;

        //Start waiting as nothing to do in runup
        rstime::rs_usleep((int) (timeDelta * 1000 * 1000)); // timeDelta sec

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
    //rstime_t now = time(NULL) ;

    GXSNETDEBUG___<< "RsGxsNetService::debugDump():" << std::endl;

    RsGxsGrpMetaTemporaryMap grpMetas;

    if(!group_id_to_print.isNull())
        grpMetas[group_id_to_print] = NULL ;

    mDataStore->retrieveGxsGrpMetaData(grpMetas);

	GXSNETDEBUG___<< "  mGrpServerUpdateItem time stamp: " << nice_time_stamp(time(NULL) , mGrpServerUpdate.grpUpdateTS) << " (is the last local modification time over all groups of this service)" << std::endl;

    GXSNETDEBUG___<< "  mServerMsgUpdateMap: (is for each subscribed group, the last local modification time)" << std::endl;

    for(ServerMsgMap::const_iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();++it)
    {
        RsGxsGrpMetaTemporaryMap::const_iterator it2 = grpMetas.find(it->first) ;
        RsGxsGrpMetaData *grpMeta = (it2 != grpMetas.end())? it2->second : NULL;
        std::string subscribe_string = (grpMeta==NULL)?"Unknown" :  ((grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)?" Subscribed":" NOT Subscribed") ;

	GXSNETDEBUG__G(it->first) << "    Grp:" << it->first << " last local modification (secs ago): " << nice_time_stamp(time(NULL),it->second.msgUpdateTS) << ", " << subscribe_string  << std::endl;
    }

    GXSNETDEBUG___<< "  mClientGrpUpdateMap: (is for each friend, last modif time of group meta data at that friend, all groups included, sent by the friend himself)" << std::endl;

    for(std::map<RsPeerId,RsGxsGrpUpdate>::const_iterator it(mClientGrpUpdateMap.begin());it!=mClientGrpUpdateMap.end();++it)
	    GXSNETDEBUG_P_(it->first) << "    From peer: " << it->first << " - last updated at peer (secs ago): " << nice_time_stamp(time(NULL),it->second.grpUpdateTS) << std::endl;

    GXSNETDEBUG___<< "  mClientMsgUpdateMap: (is for each friend, the modif time for each group (e.g. last message received), sent by the friend himself)" << std::endl;

    for(std::map<RsPeerId,RsGxsMsgUpdate>::const_iterator it(mClientMsgUpdateMap.begin());it!=mClientMsgUpdateMap.end();++it)
    {
	    GXSNETDEBUG_P_(it->first) << "    From peer: " << it->first << std::endl;

	    for(std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator it2(it->second.msgUpdateInfos.begin());it2!=it->second.msgUpdateInfos.end();++it2)
		    GXSNETDEBUG_PG(it->first,it2->first) << "      group " << it2->first << " - last updated at peer (secs ago): " << nice_time_stamp(time(NULL),it2->second.time_stamp) << ". Message count=" << it2->second.message_count << std::endl;
    }

    GXSNETDEBUG___<< "  List of rejected message ids: " << std::dec << mRejectedMessages.size() << std::endl;
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
	RsGxsGrpMetaTemporaryMap gxsMap;

#ifdef NXS_NET_DEBUG_0
    	GXSNETDEBUG___<< "updateServerSyncTS(): updating last modification time stamp of local data." << std::endl;
#endif

	{
		RS_STACK_MUTEX(mNxsMutex) ;
		// retrieve all grps and update TS
		mDataStore->retrieveGxsGrpMetaData(gxsMap);

		// (cyril) This code was previously removed because it sounded inconsistent: the list of grps normally does not need to be updated when
		// new posts arrive. The two (grp list and msg list) are handled independently. Still, when group meta data updates are received,
		// the server TS needs to be updated, because it is the only way to propagate the changes. So we update it to the publish time stamp,
		// if needed.

		// as a grp list server also note this is the latest item you have
		// then remove from mServerMsgUpdateMap, all items that are not in the group list!

#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG___ << "  cleaning server map of groups with no data:" << std::endl;
#endif

		for(ServerMsgMap::iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();)
			if(gxsMap.find(it->first) == gxsMap.end())
			{
				// not found! Removing server update info for this group

#ifdef NXS_NET_DEBUG_0
				GXSNETDEBUG__G(it->first) << "    removing server update info for group " << it->first << std::endl;
#endif
				ServerMsgMap::iterator tmp(it) ;
				++tmp ;
				mServerMsgUpdateMap.erase(it) ;
				it = tmp ;
			}
			else
				++it;
	}

#ifdef NXS_NET_DEBUG_0
    	if(gxsMap.empty())
            GXSNETDEBUG___<< "  database seems to be empty. The modification timestamp will be reset." << std::endl;
#endif
    	// finally, update timestamps.
	bool change = false;

	for(auto mit = gxsMap.begin();mit != gxsMap.end(); ++mit)
	{
        	// Check if the group is subscribed and restricted to a circle. If the circle has changed, update the
        	// global TS to reflect that change to clients who may be able to see/subscribe to that particular group.

        	if( (mit->second->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) && !mit->second->mCircleId.isNull())
	    	{
                	// ask to the GxsNetService of circles what the server TS is for that circle. If more recent, we update the serverTS of the
                	// local group

                        rstime_t circle_group_server_ts ;
                        rstime_t circle_msg_server_ts ;

                        // This call needs to be off-mutex, because of self-restricted circles.
                        // Normally we should update as a function of MsgServerUpdateTS and the mRecvTS of the circle, not the global grpServerTS.
                        // But grpServerTS is easier to get since it does not require a db access. So we trade the real call for this cheap and conservative call.

                        if(mCircles->getLocalCircleServerUpdateTS(mit->second->mCircleId,circle_group_server_ts,circle_msg_server_ts))
                        {
#ifdef NXS_NET_DEBUG_0
                            GXSNETDEBUG__G(mit->first) << "  Group " << mit->first << " is conditionned to circle " << mit->second->mCircleId << ". local Grp TS=" << time(NULL) - mGrpServerUpdate.grpUpdateTS << " secs ago, circle grp server update TS=" << time(NULL) - circle_group_server_ts << " secs ago";
#endif

                            if(circle_group_server_ts > mGrpServerUpdate.grpUpdateTS)
							{
#ifdef NXS_NET_DEBUG_0
								GXSNETDEBUG__G(mit->first) << " - Updating local Grp Server update TS to follow changes in circles." << std::endl;
#endif

								RS_STACK_MUTEX(mNxsMutex) ;
								mGrpServerUpdate.grpUpdateTS = circle_group_server_ts ;
							}
#ifdef NXS_NET_DEBUG_0
                            else
                                GXSNETDEBUG__G(mit->first) << " - Nothing to do." << std::endl;
#endif
                        }
                        else
                            std::cerr << "(EE) Cannot retrieve attached circle TS" << std::endl;
            	}

		RS_STACK_MUTEX(mNxsMutex) ;

		const RsGxsGrpMetaData* grpMeta = mit->second;
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

        // I keep the creation, but the data is not used yet.
#warning csoler 2016-12-12: Disabled this, but do we need it?
		// RsGxsServerMsgUpdate& msui(mServerMsgUpdateMap[grpId]) ;

        // (cyril) I'm removing this, because the msgUpdateTS is updated when new messages are received by calling locked_stampMsgServerUpdateTS().
        //       mLastPost is actually updated somewhere when loading group meta data. It's not clear yet whether it is set to the latest publish time (wrong)
        //		 or the latest receive time (right). The former would cause problems because it would need to compare times coming from different (potentially async-ed)
        //		 machines.
        //
        // if(grpMeta->mLastPost > msui->msgUpdateTS )
        // {
        // 	change = true;
        // 	msui->msgUpdateTS = grpMeta->mLastPost;
#ifdef NXS_NET_DEBUG_0
        // 	GXSNETDEBUG__G(grpId) << "  updated msgUpdateTS to last post = " << time(NULL) - grpMeta->mLastPost << " secs ago for group "<< grpId << std::endl;
#endif
        // }

		// This is needed for group metadata updates to actually propagate: only a new grpUpdateTS will trigger the exchange of groups mPublishTs which
		// will then be compared and pssibly trigger a MetaData transmission. mRecvTS is upated when creating, receiving for the first time, or receiving
		// an update, all in rsgenexchange.cc, after group/update validation. It is therefore a local TS, that can be compared to grpUpdateTS (same machine).

		if(mGrpServerUpdate.grpUpdateTS < grpMeta->mRecvTS)
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG__G(grpMeta->mGroupId) << "  updated msgUpdateTS to last RecvTS = " << time(NULL) - grpMeta->mRecvTS << " secs ago for group "<< grpMeta->mGroupId << ". This is probably because an update has been received." << std::endl;
#endif
			mGrpServerUpdate.grpUpdateTS = grpMeta->mRecvTS;
			change = true;
		}
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
    RS_STACK_MUTEX(mNxsMutex) ;

    for(TransactionsPeerMap::iterator mit = mTransactions.begin();mit != mTransactions.end(); ++mit)
    {
#ifdef NXS_NET_DEBUG_1
        if(!mit->second.empty())
		GXSNETDEBUG_P_(mit->first) << "processTransactions from/to peer " << mit->first << std::endl;
#endif

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
                        generic_sendItem(*lit);
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
                    generic_sendItem(trans);

                    // move to completed transactions

                    // Try to decrypt the items that need to be decrypted. This function returns true if the transaction is not encrypted.

                    if(processTransactionForDecryption(tr))
		    {
#ifdef NXS_NET_DEBUG_7
			    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "   successfully decrypted/processed transaction " << transN << ". Adding to completed list." << std::endl;
#endif
			    mComplTransactions.push_back(tr);

			    // transaction processing done
			    // for this id, add to removal list
			    toRemove.push_back(mmit->first);
#ifdef NXS_NET_DEBUG_1
			    int total_transaction_time = (int)time(NULL) - (tr->mTimeOut - mTransactionTimeOut) ;
			    GXSNETDEBUG_P_(mit->first) << "    incoming completed " << tr->mTransaction->nItems << " items transaction in " << total_transaction_time << " seconds." << std::endl;
#endif
		    }
		    else
		    {
#ifdef NXS_NET_DEBUG_7
			    GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "   no decryption occurred because of unloaded keys. Will retry later. TransN=" << transN << std::endl;
#endif
		    }


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
                    generic_sendItem(trans);
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

    GrpConfigMap::const_iterator it ( mServerGrpConfigMap.find(gid) );

    if(it == mServerGrpConfigMap.end())
        return false ;

    stats.mSuppliers = it->second.suppliers.ids.size();
    stats.mMaxVisibleCount = it->second.max_visible_count ;
    stats.mAllowMsgSync = mAllowMsgSync ;
    stats.mGrpAutoSync = mGrpAutoSync ;
    stats.mLastGroupModificationTS = it->second.last_group_modification_TS ;

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

#ifdef NXS_NET_DEBUG_0
            ClientGrpMap::iterator it = mClientGrpUpdateMap.find(peerFrom);
#endif

            RsGxsGrpUpdate& item(mClientGrpUpdateMap[peerFrom]) ;

#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    and updating mClientGrpUpdateMap for peer " << peerFrom << " of new time stamp " << nice_time_stamp(time(NULL),updateTS) << std::endl;
#endif

            item.grpUpdateTS = updateTS;

            IndicateConfigChanged();
        }
        else if(flag & RsNxsTransacItem::FLAG_TYPE_MSGS)
        {

            std::vector<RsNxsMsg*> msgs;
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "  type = msgs." << std::endl;
#endif
            RsGxsGroupId grpId;
            //rstime_t now = time(NULL) ;

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

//#warning We need here to queue all incoming items into a list where the vetting will be checked
//#warning in order to avoid someone without the proper rights to post in a group protected with an external circle

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

            // also update server sync TS, since we need to send the new message list to friends for comparison
            locked_stampMsgServerUpdateTS(grpId);
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

    if(peerFrom.isNull())
    {
        std::cerr << "(EE) update from null peer!" << std::endl;
        print_stacktrace() ;
    }

    RsGxsMsgUpdate& mui(mClientMsgUpdateMap[peerFrom]) ;

    // now update the peer's entry for this grp id

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
        mui.msgUpdateInfos[grpId].time_stamp = nxsTrans->updateTS;
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
    	generic_sendItem(transac);
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

    RsGxsGrpConfig& gnsr(locked_getGrpConfig(grpId));

    std::set<RsPeerId>::size_type oldSuppliersCount = gnsr.suppliers.ids.size();
    uint32_t oldVisibleCount = gnsr.max_visible_count;

    gnsr.suppliers.ids.insert(pid) ;
    gnsr.max_visible_count = std::max(gnsr.max_visible_count, mcount) ;

    if (oldVisibleCount != gnsr.max_visible_count || oldSuppliersCount != gnsr.suppliers.ids.size())
        mNewStatsToNotify.insert(grpId) ;

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  grpId = " << grpId << std::endl;
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  retrieving grp mesta data..." << std::endl;
#endif
    RsGxsGrpMetaTemporaryMap grpMetaMap;
    grpMetaMap[grpId] = NULL;

    mDataStore->retrieveGxsGrpMetaData(grpMetaMap);
    const RsGxsGrpMetaData* grpMeta = grpMetaMap[grpId];

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

#ifdef TO_REMOVE
    int cutoff = 0;
    if(grpMeta != NULL)
        cutoff = grpMeta->mReputationCutOff;
#endif

    GxsMsgReq reqIds;
    reqIds[grpId] = std::set<RsGxsMessageId>();
    GxsMsgMetaResult result;
    mDataStore->retrieveGxsMsgMetaData(reqIds, result);
    std::vector<const RsGxsMsgMetaData*> &msgMetaV = result[grpId];

#ifdef NXS_NET_DEBUG_1
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  retrieving grp message list..." << std::endl;
    GXSNETDEBUG_PG(item->PeerId(),grpId) << "  grp locally contains " << msgMetaV.size() << " messsages." << std::endl;
#endif
    std::vector<const RsGxsMsgMetaData*>::const_iterator vit = msgMetaV.begin();
    std::set<RsGxsMessageId> msgIdSet;

    // put ids in set for each searching
    for(; vit != msgMetaV.end(); ++vit)
        msgIdSet.insert((*vit)->mMsgId);

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
        if(reqListSize >= (int)MAX_REQLIST_SIZE)
        {
#ifdef NXS_NET_DEBUG_1
            GXSNETDEBUG_PG(item->PeerId(),grpId) << ". reqlist too big. Pruning out this item for now." << std::endl;
#endif
            reqListSizeExceeded = true ;
            continue ;	// we should actually break, but we need to print some debug info.
        }

        if(reqListSize < (int)MAX_REQLIST_SIZE && msgIdSet.find(msgId) == msgIdSet.end())
        {

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

            // The algorithm on request of message is:
            //
            //  - always re-check for author ban level
            //  - if author is locally banned, do not download.
            //  - if author is not locally banned, download, whatever friends' opinion might be.

			if( mReputations->overallReputationLevel(syncItem->authorId) ==
			        RsReputationLevel::LOCALLY_NEGATIVE )
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
            GXSNETDEBUG_PG(item->PeerId(),grpId) << ". already here." << std::endl;
#endif
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

void RsGxsNetService::locked_stampPeerGroupUpdateTime(const RsPeerId& pid,const RsGxsGroupId& grpId,rstime_t tm,uint32_t n_messages)
{
    RsGxsMsgUpdate& up(mClientMsgUpdateMap[pid]);

    up.msgUpdateInfos[grpId].time_stamp = tm;
    up.msgUpdateInfos[grpId].message_count = std::max(n_messages, up.msgUpdateInfos[grpId].message_count) ;

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
	    generic_sendItem(transac);
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

    std::list<RsNxsSyncGrpItem*> grpItemL;
    RsGxsGrpMetaTemporaryMap grpMetaMap;

    for(std::list<RsNxsItem*>::iterator lit = tr->mItems.begin(); lit != tr->mItems.end(); ++lit)
    {
        RsNxsSyncGrpItem* item = dynamic_cast<RsNxsSyncGrpItem*>(*lit);
        if(item)
        {
            grpItemL.push_back(item);
            grpMetaMap[item->grpId] = NULL;
        }
        else
        {
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_PG(tr->mTransaction->PeerId(),item->grpId) << "RsGxsNetService::genReqGrpTransaction(): item failed to caste to RsNxsSyncMsgItem* " << std::endl;
#endif
        }
    }

    if (grpItemL.empty())
	{
		// Normally the client grp updateTS is set after the transaction, but if no transaction is to happen, we have to set it here.
        // Possible change: always do the update of the grpClientTS here. Needs to be tested...

		RsGxsGrpUpdate& item (mClientGrpUpdateMap[tr->mTransaction->PeerId()]);

#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    reqList is empty, updating anyway ClientGrpUpdate TS for peer " << tr->mTransaction->PeerId() << " to: " << tr->mTransaction->updateTS << std::endl;
#endif

        if(item.grpUpdateTS != tr->mTransaction->updateTS)
        {
			item.grpUpdateTS = tr->mTransaction->updateTS;
			IndicateConfigChanged();
        }
		return;
	}

    mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

    // now do compare and add loop
    std::list<RsNxsSyncGrpItem*>::iterator llit = grpItemL.begin();
    std::list<RsNxsItem*> reqList;

    uint32_t transN = locked_getTransactionId();

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
        // FIXTESTS global variable rsReputations not available in unittests!

		if( mReputations->overallReputationLevel(RsGxsId(grpSyncItem->grpId)) == RsReputationLevel::LOCALLY_NEGATIVE )
		{
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "  Identity " << grpSyncItem->grpId << " is banned. Not GXS-syncing group." << std::endl;
#endif
			continue ;
		}

        if( (mGrpAutoSync && !haveItem) || latestVersion)
        {
#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "  Identity " << grpId << " will be sync-ed using GXS. mGrpAutoSync:" << mGrpAutoSync << " haveItem:" << haveItem << " latest_version: " << latestVersion << std::endl;
#endif
			addGroupItemToList(tr, grpId, transN, reqList);
        }
    }

    if(!reqList.empty())
        locked_pushGrpTransactionFromList(reqList, tr->mTransaction->PeerId(), transN);
    else
	{
		// Normally the client grp updateTS is set after the transaction, but if no transaction is to happen, we have to set it here.
		// Possible change: always do the update of the grpClientTS here. Needs to be tested...

		RsGxsGrpUpdate& item (mClientGrpUpdateMap[tr->mTransaction->PeerId()]);

#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "    reqList is empty, updating anyway ClientGrpUpdate TS for peer " << tr->mTransaction->PeerId() << " to: " << tr->mTransaction->updateTS << std::endl;
#endif

		if(item.grpUpdateTS != tr->mTransaction->updateTS)
		{
			item.grpUpdateTS = tr->mTransaction->updateTS;
			IndicateConfigChanged();
		}
	}

}

void RsGxsNetService::locked_genSendGrpsTransaction(NxsTransaction* tr)
{

#ifdef NXS_NET_DEBUG_1
	GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "locked_genSendGrpsTransaction() Generating Grp data send from TransN: " << tr->mTransaction->transactionNumber << std::endl;
#endif

	// go groups requested in transaction tr

	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

    	t_RsGxsGenericDataTemporaryMap<RsGxsGroupId,RsNxsGrp> grps ;

	for(;lit != tr->mItems.end(); ++lit)
	{
		RsNxsSyncGrpItem* item = dynamic_cast<RsNxsSyncGrpItem*>(*lit);
		if (item)
		{
#ifdef NXS_NET_DEBUG_1
			GXSNETDEBUG_PG(tr->mTransaction->PeerId(),item->grpId) << "locked_genSendGrpsTransaction() retrieving data for group \"" << item->grpId << "\"" << std::endl;
#endif
			grps[item->grpId] = NULL;
		}
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
	{
#ifdef NXS_NET_DEBUG_1
		GXSNETDEBUG_P_(tr->mTransaction->PeerId()) << "RsGxsNetService::locked_genSendGrpsTransaction(): no group to request! This is unexpected" << std::endl;
#endif
		return;
	}

	NxsTransaction* newTr = new NxsTransaction();
	newTr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

	uint32_t transN = locked_getTransactionId();

	// store grp items to send in transaction
	std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit = grps.begin();
	RsPeerId peerId = tr->mTransaction->PeerId();
	for(;mit != grps.end(); ++mit)
	{
#warning csoler: Should make sure that no private key information is sneaked in here for the grp
		mit->second->PeerId(peerId); // set so it gets sent to right peer
		mit->second->transactionNumber = transN;
		newTr->mItems.push_back(mit->second);
        	mit->second = NULL ; // avoids deletion
#ifdef NXS_NET_DEBUG_1
		GXSNETDEBUG_PG(tr->mTransaction->PeerId(),mit->first) << "RsGxsNetService::locked_genSendGrpsTransaction(): adding grp data of group \"" << mit->first << "\" to transaction" << std::endl;
#endif
	}

	if(newTr->mItems.empty()){
		delete newTr;
		return;
	}

	uint32_t updateTS = 0;
	updateTS = mGrpServerUpdate.grpUpdateTS;

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
		generic_sendItem(ntr);
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

#ifdef TO_BE_REMOVED
    // Author response vetting is disabled since not used, as the reputations are currently not async-ed anymore.

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
#endif


	// now lets do circle vetting
	std::vector<GrpCircleVetting*>::iterator vit2 = mPendingCircleVets.begin();
	for(; vit2 != mPendingCircleVets.end(); )
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG___ << "   Examining/clearing pending vetting of type " << (*vit2)->getType() << std::endl;
#endif
		GrpCircleVetting*& gcv = *vit2;
		if(gcv->cleared() || gcv->expired())
		{
			if(gcv->getType() == GrpCircleVetting::GRP_ID_PEND)
			{
				GrpCircleIdRequestVetting* gcirv = static_cast<GrpCircleIdRequestVetting*>(gcv);
#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_P_(gcirv->mPeerId) << "     vetting is a GRP ID PENDING Response" << std::endl;
#endif

				if(!locked_createTransactionFromPending(gcirv))
                    		{
#ifdef NXS_NET_DEBUG_4
		    			GXSNETDEBUG_P_(gcirv->mPeerId)  << "     Response sent!" << std::endl;
#endif
                    			++vit2 ;
                    			continue ;
                		}
			}
			else if(gcv->getType() == GrpCircleVetting::MSG_ID_SEND_PEND)
			{
				MsgCircleIdsRequestVetting* mcirv = static_cast<MsgCircleIdsRequestVetting*>(gcv);

#ifdef NXS_NET_DEBUG_4
				GXSNETDEBUG_P_(mcirv->mPeerId) << "     vetting is a MSG ID PENDING Response" << std::endl;
#endif
				if(mcirv->cleared())
                		{
#ifdef NXS_NET_DEBUG_4
		    			GXSNETDEBUG_P_(mcirv->mPeerId) << "     vetting cleared! Sending..." << std::endl;
#endif
					if(!locked_createTransactionFromPending(mcirv))
                        			continue ;					// keep it in the list for retry
                    		}
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
#ifdef NXS_NET_DEBUG_4
			GXSNETDEBUG___ << "   ... not cleared yet." << std::endl;
#endif
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
		    msgIds[item->grpId].insert(item->msgId);

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

#ifdef CODE_TO_ENCRYPT_MESSAGE_DATA
    // now if transaction is limited to an external group, encrypt it for members of the group.

    RsGxsCircleId encryption_circle ;
    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grp;
    grp[grpId] = NULL ;

    mDataStore->retrieveGxsGrpMetaData(grp);

    RsGxsGrpMetaData *grpMeta = grp[grpId] ;

    if(grpMeta == NULL)
    {
	    std::cerr << "(EE) cannot retrieve group meta data for message transaction " << tr->mTransaction->transactionNumber << std::endl;
	    return ;
    }

    encryption_circle = grpMeta->mCircleId ;
    delete grpMeta ;
    grp.clear() ;
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_PG(tr->mTransaction->PeerId(),grpId) << "  Msg transaction items will be encrypted for circle " << std::endl;
#endif
#endif

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

#ifdef NXS_NET_DEBUG_0
			GXSNETDEBUG_PG(tr->mTransaction->PeerId(),msg->grpId) << "   sending msg Id " << msg->msgId << " in Group " << msg->grpId << std::endl;
#endif

		    newTr->mItems.push_back(msg);
		    msgSize++;
#endif

#ifdef CODE_TO_ENCRYPT_MESSAGE_DATA
		    // encrypt

		    if(!encryption_circle.isNull())
		    {
			    uint32_t status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;

			    RsNxsEncryptedDataItem encrypted_msg_item = NULL ;

			    if(encryptSingleNxsItem(msg,encryption_circle,encrypted_msg_item,status))
			    {
				    newTr->mItems.push_back(msg);
				    delete msg ;
			    }
			    else
			    {
			    }
		    }
		    else
		    {
			    newTr->mItems.push_back(msg);

			    msgSize++;
		    }
#endif
	    }
    }

    if(newTr->mItems.empty()){
	    delete newTr;
	    return;
    }

    // now send a transaction item and store the transaction data

    uint32_t updateTS = mServerMsgUpdateMap[grpId].msgUpdateTS;

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

    if(locked_addTransaction(newTr))
		generic_sendItem(ntr);
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

    transMap[transN] = tr;

    return true;
}

// Turns a single RsNxsItem into an encrypted one, suitable for the supplied destination circle.
// Returns false when the keys are not loaded. Question to solve: what do we do if we miss some keys??
// We should probably send anyway.

bool RsGxsNetService::encryptSingleNxsItem(RsNxsItem *item, const RsGxsCircleId& destination_circle, const RsGxsGroupId& destination_group, RsNxsItem *&encrypted_item, uint32_t& status)
{
        encrypted_item = NULL ;
	status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;
#ifdef NXS_NET_DEBUG_7
	GXSNETDEBUG_P_ (item->PeerId()) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - Encrypting single item for peer " << item->PeerId() << ", for circle ID " << destination_circle  << std::endl;
#endif
	// 1 - Find out the list of GXS ids to encrypt for
	//     We could do smarter things (like see if the peer_id owns one of the circle's identities
	//     but for now we aim at the simplest solution: encrypt for all identities in the circle.

	std::list<RsGxsId> recipients ;

	if(!mCircles->recipients(destination_circle,destination_group,recipients))
	{
		std::cerr << "  (EE) Cannot encrypt transaction: recipients list not available. Should re-try later." << std::endl;
        	status = RS_NXS_ITEM_ENCRYPTION_STATUS_CIRCLE_ERROR ;
		return false ;
	}

    	if(recipients.empty())
        {
#ifdef NXS_NET_DEBUG_7
            GXSNETDEBUG_P_(item->PeerId()) << "  (EE) No recipients found for circle " << destination_circle << ". Circle not in cache, or empty circle?" << std::endl;
#endif
            return false ;
        }

#ifdef NXS_NET_DEBUG_7
	GXSNETDEBUG_P_ (item->PeerId()) << "  Dest  Ids: " << std::endl;
#endif
	std::vector<RsTlvPublicRSAKey> recipient_keys ;

	for(std::list<RsGxsId>::const_iterator it(recipients.begin());it!=recipients.end();++it)
	{
		RsTlvPublicRSAKey pkey ;

		if(!mGixs->getKey(*it,pkey))
		{
			std::cerr  << "  (EE) Cannot retrieve public key " << *it << " for circle encryption. Should retry later?" << std::endl;

			// we should probably request the key?
			status = RS_NXS_ITEM_ENCRYPTION_STATUS_GXS_KEY_MISSING ;
			continue ;
		}
#ifdef NXS_NET_DEBUG_7
		GXSNETDEBUG_P_ (item->PeerId()) << "  added key " << *it << std::endl;
#endif
		recipient_keys.push_back(pkey) ;
	}

	// 2 - call GXSSecurity to make a header item that encrypts for the given list of peers.

#ifdef NXS_NET_DEBUG_7
	GXSNETDEBUG_P_ (item->PeerId()) << "  Encrypting..." << std::endl;
#endif
	uint32_t size = RsNxsSerialiser(mServType).size(item) ;
	RsTemporaryMemory tempmem( size ) ;

	if(!RsNxsSerialiser(mServType).serialise(item,tempmem,&size))
	{
		std::cerr << "  (EE) Cannot serialise item. Something went wrong." << std::endl;
		status = RS_NXS_ITEM_ENCRYPTION_STATUS_SERIALISATION_ERROR ;
		return false ;
	}

	unsigned char *encrypted_data = NULL ;
	uint32_t encrypted_len  = 0 ;

	if(!GxsSecurity::encrypt(encrypted_data, encrypted_len,tempmem,size,recipient_keys))
	{
		std::cerr << "  (EE) Cannot multi-encrypt item. Something went wrong." << std::endl;
		status = RS_NXS_ITEM_ENCRYPTION_STATUS_ENCRYPTION_ERROR ;
		return false ;
	}

	RsNxsEncryptedDataItem *enc_item = new RsNxsEncryptedDataItem(mServType) ;

	enc_item->encrypted_data.bin_len  = encrypted_len ;
	enc_item->encrypted_data.bin_data = encrypted_data ;

	// also copy all the important data.

	enc_item->transactionNumber = item->transactionNumber ;
	enc_item->PeerId(item->PeerId()) ;

	encrypted_item = enc_item ;
#ifdef NXS_NET_DEBUG_7
	GXSNETDEBUG_P_(item->PeerId()) << "    encrypted item of size " << encrypted_len << std::endl;
#endif
	status = RS_NXS_ITEM_ENCRYPTION_STATUS_NO_ERROR ;

	return true ;
}

// Tries to decrypt the transaction. First load the keys and process all items.
// If keys are loaded, encrypted items that cannot be decrypted are discarded.
// Otherwise the transaction is untouched for retry later.

bool RsGxsNetService::processTransactionForDecryption(NxsTransaction *tr)
{
#ifdef NXS_NET_DEBUG_7
    RsPeerId peerId = tr->mTransaction->PeerId() ;
    GXSNETDEBUG_P_(peerId) << "RsGxsNetService::decryptTransaction()" << std::endl;
#endif

    std::list<RsNxsItem*> decrypted_items ;
    std::vector<RsTlvPrivateRSAKey> private_keys ;

    // get all private keys. Normally we should look into the circle name and only supply the keys that we have

    for(std::list<RsNxsItem*>::iterator it(tr->mItems.begin());it!=tr->mItems.end();)
    {
        RsNxsEncryptedDataItem *encrypted_item = dynamic_cast<RsNxsEncryptedDataItem*>(*it) ;

        if(encrypted_item == NULL)
        {
#ifdef NXS_NET_DEBUG_7
            GXSNETDEBUG_P_(peerId) << "  skipping unencrypted item..." << std::endl;
#endif
            ++it ;
            continue ;
        }

        // remove the encrypted item. After that it points to the next item to handle
        it = tr->mItems.erase(it) ;

        RsNxsItem *nxsitem = NULL ;

        if(decryptSingleNxsItem(encrypted_item,nxsitem,&private_keys))
	{
#ifdef NXS_NET_DEBUG_7
		GXSNETDEBUG_P_(peerId) << "    Replacing the encrypted item with the clear one." << std::endl;
#endif
		tr->mItems.insert(it,nxsitem) ;	// inserts before it, so no need to ++it
	}

        delete encrypted_item ;
    }

    return true ;
}

bool RsGxsNetService::decryptSingleNxsItem(const RsNxsEncryptedDataItem *encrypted_item, RsNxsItem *& nxsitem,std::vector<RsTlvPrivateRSAKey> *pprivate_keys)
{
    // if private_keys storage is supplied use/update them, otherwise, find which key should be used, and store them in a local std::vector.

    nxsitem = NULL ;
    std::vector<RsTlvPrivateRSAKey> local_keys ;
    std::vector<RsTlvPrivateRSAKey>& private_keys = pprivate_keys?(*pprivate_keys):local_keys ;

    // we need the private keys to decrypt the item. First load them in!
    bool key_loading_failed = false ;

    if(private_keys.empty())
    {
#ifdef NXS_NET_DEBUG_7
	    GXSNETDEBUG_P_(encrypted_item->PeerId()) << "  need to retrieve private keys..." << std::endl;
#endif

	    std::list<RsGxsId> own_keys ;
	    mGixs->getOwnIds(own_keys) ;

	    for(std::list<RsGxsId>::const_iterator it(own_keys.begin());it!=own_keys.end();++it)
	    {
		    RsTlvPrivateRSAKey private_key ;

		    if(mGixs->getPrivateKey(*it,private_key))
		    {
			    private_keys.push_back(private_key) ;
#ifdef NXS_NET_DEBUG_7
			    GXSNETDEBUG_P_(encrypted_item->PeerId())<< "    retrieved private key " << *it << std::endl;
#endif
		    }
		    else
		    {
			    std::cerr << "    (EE) Cannot retrieve private key for ID " << *it << std::endl;
			    key_loading_failed = true ;
			    break ;
		    }
	    }
    }
    if(key_loading_failed)
    {
#ifdef NXS_NET_DEBUG_7
	    GXSNETDEBUG_P_(encrypted_item->PeerId()) << "  Some keys not loaded.Returning false to retry later." << std::endl;
#endif
	    return false ;
    }

    // we do this only when something actually needs to be decrypted.

    unsigned char *decrypted_mem = NULL;
    uint32_t decrypted_len =0;

#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_(encrypted_item->PeerId())<< "    Trying to decrypt item..." ;
#endif

    if(!GxsSecurity::decrypt(decrypted_mem,decrypted_len, (uint8_t*)encrypted_item->encrypted_data.bin_data,encrypted_item->encrypted_data.bin_len,private_keys))
    {
#ifdef NXS_NET_DEBUG_7
	 GXSNETDEBUG_P_(encrypted_item->PeerId()) << "    Failed! Cannot decrypt this item." << std::endl;
#endif
	    decrypted_mem = NULL ; // for safety
	return false ;
    }
#ifdef NXS_NET_DEBUG_7
    GXSNETDEBUG_P_(encrypted_item->PeerId())<< "    Succeeded! deserialising..." << std::endl;
#endif

    // deserialise the item

    RsItem *ditem = NULL ;

    if(decrypted_mem!=NULL)
    {
	    ditem = RsNxsSerialiser(mServType).deserialise(decrypted_mem,&decrypted_len) ;
		free(decrypted_mem) ;

	    if(ditem != NULL)
	    {
		    ditem->PeerId(encrypted_item->PeerId()) ;	// This is needed because the deserialised item has no peer id
		    nxsitem = dynamic_cast<RsNxsItem*>(ditem) ;
	    }
	    else
		    std::cerr << "    Cannot deserialise. Item encoding error!" << std::endl;

	    return (nxsitem != NULL) ;
    }
    return false ;
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
        GXSNETDEBUG_P_ (peer) << "Setting tr->mTransaction->updateTS to " << mGrpServerUpdate.grpUpdateTS << std::endl;
#endif
	trItem->updateTS = mGrpServerUpdate.grpUpdateTS;

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
	    generic_sendItem(trItem);
    else
    {
        delete tr ;
        delete trItem ;
    }
}

bool RsGxsNetService::locked_CanReceiveUpdate(const RsNxsSyncGrpReqItem *item)
{
    // Do we have new updates for this peer?
    // The received TS is in our own clock, but send to us by the friend.

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG_P_(item->PeerId()) << "  local modification time stamp: " << std::dec<< time(NULL) - mGrpServerUpdate.grpUpdateTS << " secs ago. Update sent: " <<
	                                  ((item->updateTS < mGrpServerUpdate.grpUpdateTS)?"YES":"NO")  << std::endl;
#endif
	return item->updateTS < mGrpServerUpdate.grpUpdateTS && locked_checkResendingOfUpdates(item->PeerId(),RsGxsGroupId(),item->updateTS,mGrpServerUpdate.grpUpdateTsRecords[item->PeerId()]) ;
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

    RsGxsGrpMetaTemporaryMap grp;
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

    for(auto mit = grp.begin(); mit != grp.end(); ++mit)
    {
	    const RsGxsGrpMetaData* grpMeta = mit->second;

	    // Only send info about subscribed groups.

	    if(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	    {

		    // check if you can send this id to peer
		    // or if you need to add to the holding
		    // pen for peer to be vetted

		    bool should_encrypt = false ;

		    if(canSendGrpId(peer, *grpMeta, toVet,should_encrypt))
		    {
			    RsNxsSyncGrpItem* gItem = new RsNxsSyncGrpItem(mServType);
			    gItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
			    gItem->grpId = mit->first;
			    gItem->publishTs = mit->second->mPublishTs;
			    gItem->authorId = grpMeta->mAuthorId;
			    gItem->PeerId(peer);
			    gItem->transactionNumber = transN;

			    if(should_encrypt)
			    {
#ifdef NXS_NET_DEBUG_7
				    GXSNETDEBUG_PG(peer,mit->first) << "    item for this grpId should be encrypted." << std::endl;
#endif
				    RsNxsItem *encrypted_item = NULL ;
				    uint32_t status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;

				    if(encryptSingleNxsItem(gItem, grpMeta->mCircleId,mit->first, encrypted_item,status))
					    itemL.push_back(encrypted_item) ;
				    else
				    {
					    switch(status)
					    {
					    case RS_NXS_ITEM_ENCRYPTION_STATUS_CIRCLE_ERROR:
					    case RS_NXS_ITEM_ENCRYPTION_STATUS_GXS_KEY_MISSING:	toVet.push_back(GrpIdCircleVet(grpMeta->mGroupId, grpMeta->mCircleId, grpMeta->mAuthorId));
#ifdef NXS_NET_DEBUG_7
						    							GXSNETDEBUG_PG(peer,mit->first) << "    Could not encrypt item for grpId " << grpMeta->mGroupId << " for circle " << grpMeta->mCircleId << ". Will try later. Adding to vetting list." << std::endl;
#endif
						    							break ;
					    default:
						    std::cerr << "    Could not encrypt item for grpId " << grpMeta->mGroupId << " for circle " << grpMeta->mCircleId << ". Not sending it." << std::endl;
					    }
				    }
					delete gItem ;
			    }
			    else
				    itemL.push_back(gItem);

#ifdef NXS_NET_DEBUG_0
			    GXSNETDEBUG_PG(peer,mit->first) << "    sending item for Grp " << mit->first << " name=" << grpMeta->mGroupName << ", publishTS=" << std::dec<< time(NULL) - mit->second->mPublishTs << " secs ago to peer ID " << peer << std::endl;
#endif
		    }
	    }
    }

    if(!toVet.empty())
	    mPendingCircleVets.push_back(new GrpCircleIdRequestVetting(mCircles, mPgpUtils, toVet, peer));

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(peer) << "  final list sent (after vetting): " << itemL.size() << " elements." << std::endl;
#endif
    locked_pushGrpRespFromList(itemL, peer, transN);

    return;
}



bool RsGxsNetService::canSendGrpId(const RsPeerId& sslId, const RsGxsGrpMetaData& grpMeta, std::vector<GrpIdCircleVet>& /* toVet */, bool& should_encrypt)
{
#ifdef NXS_NET_DEBUG_4
	GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::canSendGrpId()"<< std::endl;
#endif
	// check if that peer is a virtual peer id, in which case we only send/recv data to/from it items for the group it's requested for

	RsGxsGroupId peer_grp ;
	if(mAllowDistSync && mGxsNetTunnel != NULL && mGxsNetTunnel->isDistantPeer(RsGxsNetTunnelVirtualPeerId(sslId),peer_grp) && peer_grp != grpMeta.mGroupId)
	{
#warning (cyril) make sure that this is not a problem for cross-service sending of items
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Distant peer designed for group " << peer_grp << ": cannot request sync for different group." << std::endl;
#endif
		return false ;
	}

	// first do the simple checks
	uint8_t circleType = grpMeta.mCircleType;

	if(circleType == GXS_CIRCLE_TYPE_LOCAL)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  LOCAL_CIRCLE, cannot send"<< std::endl;
#endif
		return false;
	}
	else if(circleType == GXS_CIRCLE_TYPE_PUBLIC || circleType == GXS_CIRCLE_TYPE_UNKNOWN)	// this complies with the fact that p3IdService does not initialise the circle type.
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  PUBLIC_CIRCLE, can send"<< std::endl;
#endif
		return true;
	}
	else if(circleType == GXS_CIRCLE_TYPE_EXTERNAL)
	{
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  EXTERNAL_CIRCLE, will be sent encrypted."<< std::endl;
#endif
        	should_encrypt = true ;
        	return true ;
	}
	else if(circleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY)
	{
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  YOUREYESONLY, checking further" << std::endl;
#endif
        bool res = checkPermissionsForFriendGroup(sslId,grpMeta) ;
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Final answer: " << res << std::endl;
#endif
        return res ;
#ifdef NXS_NET_DEBUG_4
		GXSNETDEBUG_PG(sslId,grpMeta.mGroupId)<< "  YOUREYESONLY, checking further"<< std::endl;
#endif
	}
    else
    {
        std::cerr << "(EE) unknown value found in circle type for group " << grpMeta.mGroupId << ": " << (int)circleType << ": this is probably a bug in the design of the group creation." << std::endl;
		return false;
    }
}

bool RsGxsNetService::checkCanRecvMsgFromPeer(const RsPeerId& sslId, const RsGxsGrpMetaData& grpMeta, RsGxsCircleId& should_encrypt_id)
{

#ifdef NXS_NET_DEBUG_4
    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::checkCanRecvMsgFromPeer()";
    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  peer Id = " << sslId << ", grpId=" << grpMeta.mGroupId <<std::endl;
#endif
	// check if that peer is a virtual peer id, in which case we only send/recv data to/from it items for the group it's requested for

	RsGxsGroupId peer_grp ;
	if(mAllowDistSync && mGxsNetTunnel != NULL && mGxsNetTunnel->isDistantPeer(RsGxsNetTunnelVirtualPeerId(sslId),peer_grp) && peer_grp != grpMeta.mGroupId)
	{
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Distant peer designed for group " << peer_grp << ": cannot request sync for different group." << std::endl;
#endif
		return false ;
	}

    // first do the simple checks
    uint8_t circleType = grpMeta.mCircleType;
    should_encrypt_id.clear() ;

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
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: EXTERNAL => returning true. Msgs will be encrypted." << std::endl;
#endif
        should_encrypt_id = grpMeta.mCircleId ;
        return true ;
    }

    if(circleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY) // do not attempt to sync msg unless to originator or those permitted
    {
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  YOUREYESONLY, checking further" << std::endl;
#endif
        bool res = checkPermissionsForFriendGroup(sslId,grpMeta) ;
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Final answer: " << res << std::endl;
#endif
        return res ;
    }

    return true;
}

bool RsGxsNetService::locked_checkResendingOfUpdates(const RsPeerId& pid,const RsGxsGroupId& grpId,rstime_t incoming_ts,RsPeerUpdateTsRecord& rec)
{
	rstime_t now = time(NULL);

	// Now we check if the peer is sending the same outdated TS for the same time in a short while. This would mean the peer
	// hasn't finished processing the updates we're sending and we shouldn't send new data anymore. Of course the peer might
	// have disconnected or so, which means that we need to be careful about not sending. As a compromise we still send, but
	// after waiting for a while (See

	if(rec.mLastTsReceived == incoming_ts && rec.mTs + SAFETY_DELAY_FOR_UNSUCCESSFUL_UPDATE > now)
    {
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG_PG(pid,grpId) << "(II) peer " << pid << " already sent the same TS " << (long int)now-(long int)rec.mTs << " secs ago for that group ID. Will not send msg list again for a while to prevent clogging..." << std::endl;
#endif
		return false;
    }

	rec.mLastTsReceived = incoming_ts;
	rec.mTs = now;

	return true;
}

bool RsGxsNetService::locked_CanReceiveUpdate(RsNxsSyncMsgReqItem *item,bool& grp_is_known)
{
    // Do we have new updates for this peer?
    // Here we compare times in the same clock: our own clock, so it should be fine.

    grp_is_known = false ;

    if(item->flag & RsNxsSyncMsgReqItem::FLAG_USE_HASHED_GROUP_ID)
    {
	    // Item contains the hashed group ID in order to protect is from friends who don't know it. So we de-hash it using bruteforce over known group IDs for this peer.
	    // We could save the de-hash result. But the cost is quite light, since the number of encrypted groups per service is usually low.

	    for(ServerMsgMap::iterator it(mServerMsgUpdateMap.begin());it!=mServerMsgUpdateMap.end();++it)
		    if(item->grpId == hashGrpId(it->first,item->PeerId()))
		    {
			    item->grpId = it->first ;
			    item->flag &= ~RsNxsSyncMsgReqItem::FLAG_USE_HASHED_GROUP_ID;
#ifdef NXS_NET_DEBUG_0
			    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "(II) de-hashed group ID " << it->first << " from hash " << item->grpId << " and peer id " << item->PeerId() << std::endl;
#endif
			    grp_is_known = true ;

                // The order of tests below is important because we want to only modify the map of requests records if the request actually is a valid requests instead of
                // a simple check that nothing's changed.

			    return item->updateTS < it->second.msgUpdateTS && locked_checkResendingOfUpdates(item->PeerId(),item->grpId,item->updateTS,it->second.msgUpdateTsRecords[item->PeerId()]) ;
		    }

	    return false ;
    }

    ServerMsgMap::iterator cit = mServerMsgUpdateMap.find(item->grpId);

    if(cit != mServerMsgUpdateMap.end())
    {
#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  local time stamp: " << std::dec<< time(NULL) - cit->second.msgUpdateTS << " secs ago. Update sent: " << (item->updateTS < cit->second.msgUpdateTS) << std::endl;
#endif
	    grp_is_known = true ;

		return item->updateTS < cit->second.msgUpdateTS && locked_checkResendingOfUpdates(item->PeerId(),item->grpId,item->updateTS,cit->second.msgUpdateTsRecords[item->PeerId()]) ;
    }

#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  no local time stamp for this grp. "<< std::endl;
#endif

    return false;
}

void RsGxsNetService::handleRecvSyncMessage(RsNxsSyncMsgReqItem *item,bool item_was_encrypted)
{
    if (!item)
	    return;

    RS_STACK_MUTEX(mNxsMutex) ;

    const RsPeerId& peer = item->PeerId();
    bool grp_is_known = false;
    bool was_circle_protected = item_was_encrypted || bool(item->flag & RsNxsSyncMsgReqItem::FLAG_USE_HASHED_GROUP_ID);

    // This call determines if the peer can receive updates from us, meaning that our last TS is larger than what the peer sent.
    // It also changes the items' group id into the un-hashed group ID if the group is a distant group.

    bool peer_can_receive_update = locked_CanReceiveUpdate(item, grp_is_known);

    if(item_was_encrypted)
        std::cerr << "(WW) got an encrypted msg sync req. from " << item->PeerId() << ". This will not send messages updates for group " << item->grpId << std::endl;

    // Insert the PeerId in suppliers list for this grpId
#ifdef NXS_NET_DEBUG_6
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "RsGxsNetService::handleRecvSyncMessage(): Inserting PeerId " << item->PeerId() << " in suppliers list for group " << item->grpId << std::endl;
#endif
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "handleRecvSyncMsg(): Received last update TS of group " << item->grpId << ", for peer " << peer << ", TS = " << time(NULL) - item->updateTS << " secs ago." ;
#endif

    // We update suppliers in two cases:
    // Case 1: the grp is known because it is the hash of an existing group, but it's not yet in the server config map
    // Case 2: the grp is not known, possibly because it was deleted, but there's an entry in mServerGrpConfigMap due to statistics gathering. Still, statistics are only
    // 		 gathered from known suppliers. So statistics never add new suppliers. These are only added here.

    if(grp_is_known || mServerGrpConfigMap.find(item->grpId)!=mServerGrpConfigMap.end())
    {
	    RsGxsGrpConfig& rec(locked_getGrpConfig(item->grpId)); // this creates it if needed. When the grp is unknown (and hashed) this will would create a unused entry
	    rec.suppliers.ids.insert(peer) ;
    }
    if(!peer_can_receive_update)
    {
#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  no update will be sent." << std::endl;
#endif
	    return;
    }

    RsGxsGrpMetaTemporaryMap grpMetas;
    grpMetas[item->grpId] = NULL;

    mDataStore->retrieveGxsGrpMetaData(grpMetas);
    const RsGxsGrpMetaData* grpMeta = grpMetas[item->grpId];

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

    if( (grpMeta->mCircleType == GXS_CIRCLE_TYPE_EXTERNAL) != was_circle_protected )
    {
        std::cerr << "(EE) received a sync Msg request for group " << item->grpId << " from peer " << item->PeerId() ;
        if(!was_circle_protected)
            std::cerr << ". The group is tied to an external circle (ID=" << grpMeta->mCircleId << ") but the request wasn't encrypted." << std::endl;
        else
            std::cerr << ". The group is not tied to an external circle (ID=" << grpMeta->mCircleId << ") but the request was encrypted." << std::endl;

        return ;
    }

    GxsMsgReq req;
    req[item->grpId] = std::set<RsGxsMessageId>();

    GxsMsgMetaResult metaResult;
    mDataStore->retrieveGxsMsgMetaData(req, metaResult);
    std::vector<const RsGxsMsgMetaData*>& msgMetas = metaResult[item->grpId];

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
    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  Sending MSG meta data created since TS=" << item->createdSinceTS << std::endl;
#endif

    std::list<RsNxsItem*> itemL;

    uint32_t transN = locked_getTransactionId();
    RsGxsCircleId should_encrypt_to_this_circle_id ;

    rstime_t now = time(NULL) ;

#ifndef RS_GXS_SEND_ALL
    uint32_t max_send_delay = locked_getGrpConfig(item->grpId).msg_req_delay;	// we should use "sync" but there's only one variable used in the GUI: the req one.
#endif

    if(canSendMsgIds(msgMetas, *grpMeta, peer, should_encrypt_to_this_circle_id))
    {
	    for(auto vit = msgMetas.begin();vit != msgMetas.end(); ++vit)
		{
			const RsGxsMsgMetaData* m = *vit;

            // Check reputation

            if(!m->mAuthorId.isNull())
			{
				RsIdentityDetails details ;

				if(!rsIdentity->getIdDetails(m->mAuthorId,details))
				{
#ifdef NXS_NET_DEBUG_0
					GXSNETDEBUG_PG(item->PeerId(),item->grpId) << " not sending grp message ID " << (*vit)->mMsgId << ", because the identity of the author (" << m->mAuthorId << ") is not accessible (unknown/not cached)" << std::endl;
#endif
					continue ;
				}

				if(details.mReputation.mOverallReputationLevel < minReputationForForwardingMessages(grpMeta->mSignFlags, details.mFlags))
				{
#ifdef NXS_NET_DEBUG_0
					GXSNETDEBUG_PG(item->PeerId(),item->grpId) << " not sending item ID " << (*vit)->mMsgId << ", because the author is flags " << std::hex << details.mFlags << std::dec << " and reputation level " << (int) details.mReputation.mOverallReputationLevel << std::endl;
#endif
					continue ;
				}
			}
			// Check publish TS
#ifndef RS_GXS_SEND_ALL
			if(item->createdSinceTS > (*vit)->mPublishTs || ((max_send_delay > 0) && (*vit)->mPublishTs + max_send_delay < now))
#else
			if(item->createdSinceTS > (*vit)->mPublishTs)
#endif
			{
#ifdef NXS_NET_DEBUG_0
				GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  not sending item ID " << (*vit)->mMsgId << ", because it is too old (publishTS = " << (time(NULL)-(*vit)->mPublishTs)/86400 << " days ago" << std::endl;
#endif
				continue ;
			}

			RsNxsSyncMsgItem* mItem = new RsNxsSyncMsgItem(mServType);
			mItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
			mItem->grpId = m->mGroupId;
			mItem->msgId = m->mMsgId;
			mItem->authorId = m->mAuthorId;
			mItem->PeerId(peer);
			mItem->transactionNumber = transN;

			if(!should_encrypt_to_this_circle_id.isNull())
			{
#ifdef NXS_NET_DEBUG_7
				GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "    sending info item for msg id " << mItem->msgId << ". Transaction will be encrypted for group " << should_encrypt_to_this_circle_id << std::endl;
#endif
				RsNxsItem *encrypted_item = NULL ;
				uint32_t status = RS_NXS_ITEM_ENCRYPTION_STATUS_UNKNOWN ;

				if(encryptSingleNxsItem(mItem, grpMeta->mCircleId,m->mGroupId, encrypted_item,status))
				{
					itemL.push_back(encrypted_item) ;
					delete mItem ;
				}
				else
				{
					// Something's not ready (probably the circle content. We could put on a vetting list, but actually the client will re-ask the list asap.

					std::cerr << "  (EE) Cannot encrypt msg meta data. MsgId=" << mItem->msgId << ", grpId=" << mItem->grpId << ", circleId=" << should_encrypt_to_this_circle_id << ". Dropping the whole list." << std::endl;

					for(std::list<RsNxsItem*>::const_iterator it(itemL.begin());it!=itemL.end();++it)
						delete *it ;

					itemL.clear() ;
					delete mItem ;
					break ;
				}
			}
			else
			{
#ifdef NXS_NET_DEBUG_7
				GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "    sending info item for msg id " << mItem->msgId << " in clear." << std::endl;
#endif
				itemL.push_back(mItem);
			}
		}
    }
#ifdef NXS_NET_DEBUG_0
    else
	    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  vetting forbids sending. Nothing will be sent." << itemL.size() << " items." << std::endl;
#endif

    if(!itemL.empty())
    {
#ifdef NXS_NET_DEBUG_0
	    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  sending final msg info list of " << itemL.size() << " items." << std::endl;
#endif
	    locked_pushMsgRespFromList(itemL, peer, item->grpId,transN);
    }
#ifdef NXS_NET_DEBUG_0
    else
	    GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  list is empty! Not sending anything." << std::endl;
#endif


    // release meta resource
	//   for(std::vector<RsGxsMsgMetaData*>::iterator vit = msgMetas.begin(); vit != msgMetas.end(); ++vit)
	//     delete *vit;
}

void RsGxsNetService::locked_pushMsgRespFromList(std::list<RsNxsItem*>& itemL, const RsPeerId& sslId, const RsGxsGroupId& grp_id,const uint32_t& transN)
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

	// This time stamp is not supposed to be used on the other side. We just set it to avoid sending an uninitialiszed value.
	trItem->updateTS = mServerMsgUpdateMap[grp_id].msgUpdateTS;

#ifdef NXS_NET_DEBUG_5
	GXSNETDEBUG_P_ (sslId) << "Service " << std::hex << ((mServiceInfo.mServiceType >> 8)& 0xffff) << std::dec << " - sending messages response to peer "
	                       << sslId << " with " << itemL.size() << " messages " << std::endl;
#endif
	// signal peer to prepare for transaction
	if(locked_addTransaction(tr))
		generic_sendItem(trItem);
	else
	{
		delete tr ;
		delete trItem ;
	}
}

bool RsGxsNetService::canSendMsgIds(std::vector<const RsGxsMsgMetaData*>& msgMetas, const RsGxsGrpMetaData& grpMeta, const RsPeerId& sslId,RsGxsCircleId& should_encrypt_id)
{
#ifdef NXS_NET_DEBUG_4
    GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "RsGxsNetService::canSendMsgIds() CIRCLE VETTING" << std::endl;
#endif
	// check if that peer is a virtual peer id, in which case we only send/recv data to/from it items for the group it's requested for

	RsGxsGroupId peer_grp ;
	if(mAllowDistSync && mGxsNetTunnel != NULL && mGxsNetTunnel->isDistantPeer(RsGxsNetTunnelVirtualPeerId(sslId),peer_grp) && peer_grp != grpMeta.mGroupId)
	{
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Distant peer designed for group " << peer_grp << ": cannot request sync for different group." << std::endl;
#endif
		return false ;
	}

    // first do the simple checks
    uint8_t circleType = grpMeta.mCircleType;

    if(circleType == GXS_CIRCLE_TYPE_LOCAL)
    {
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: LOCAL => returning false" << std::endl;
#endif
        return false;
    }
    else if(circleType == GXS_CIRCLE_TYPE_PUBLIC || circleType == GXS_CIRCLE_TYPE_UNKNOWN) // this complies with the fact that p3IdService does not initialise the circle type.
    {
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: PUBLIC => returning true" << std::endl;
#endif
        return true;
    }
    else if(circleType == GXS_CIRCLE_TYPE_EXTERNAL)
    {
		const RsGxsCircleId& circleId = grpMeta.mCircleId;
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: EXTERNAL => returning true. Msgs ids list will be encrypted." << std::endl;
#endif
        should_encrypt_id = circleId ;

        // For each message ID, check that the author is in the circle. If not, do not send the message, which means, remove it from the list.
        // Unsigned messages are still transmitted. This is because in some groups (channels) the posts are not signed. Whether an unsigned post
        // is allowed at this point is anyway already vetted by the RsGxsGenExchange service.

        // Messages that stay in the list will be sent. As a consequence true is always returned.
        // Messages put in vetting list will be dealt with later

        std::vector<MsgIdCircleVet> toVet;

        for(uint32_t i=0;i<msgMetas.size();)
            if( msgMetas[i]->mAuthorId.isNull() )		// keep the message in this case
                ++i ;
            else
            {
                if(mCircles->isLoaded(circleId) && mCircles->isRecipient(circleId, grpMeta.mGroupId, msgMetas[i]->mAuthorId))
                {
                    ++i ;
                    continue ;
                }

                MsgIdCircleVet mic(msgMetas[i]->mMsgId, msgMetas[i]->mAuthorId);
                toVet.push_back(mic);
#ifdef NXS_NET_DEBUG_4
                GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   deleting MsgMeta entry for msg ID " << msgMetas[i]->mMsgId << " signed by " << msgMetas[i]->mAuthorId << " who is not in group circle " << circleId << std::endl;
#endif

                //delete msgMetas[i] ;
                msgMetas[i] = msgMetas[msgMetas.size()-1] ;
                msgMetas.pop_back() ;
            }

#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle info not loaded. Putting in vetting list and returning false." << std::endl;
#endif
        if(!toVet.empty())
            mPendingCircleVets.push_back(new MsgCircleIdsRequestVetting(mCircles, mPgpUtils, toVet, grpMeta.mGroupId, sslId, grpMeta.mCircleId));

        return true ;
    }
    else if(circleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY)
    {
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  YOUREYESONLY, checking further" << std::endl;
#endif
        bool res = checkPermissionsForFriendGroup(sslId,grpMeta) ;
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "  Final answer: " << res << std::endl;
#endif
        return res ;
    }
    else
    {
        std::cerr << "(EE) unknown value found in circle type for group " << grpMeta.mGroupId << ": " << (int)circleType << ": this is probably a bug in the design of the group creation." << std::endl;
		return false;
    }
}

/** inherited methods **/

bool RsGxsNetService::checkPermissionsForFriendGroup(const RsPeerId& sslId,const RsGxsGrpMetaData& grpMeta)
{
#ifdef NXS_NET_DEBUG_4
        GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Circle type: YOUR EYES ONLY - ID = " <<  grpMeta.mInternalCircle << std::endl;
#endif
        // a non empty internal circle id means this
        // is the personal circle owner
        if(!grpMeta.mInternalCircle.isNull())
        {
            RsGroupInfo ginfo ;
                    RsPgpId pgpId = mPgpUtils->getPGPId(sslId) ;

#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Group internal circle: " << grpMeta.mInternalCircle << ", We're owner. Sending to everyone in the group." << std::endl;
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Current destination is PGP Id: " << pgpId << std::endl;
#endif
            if(!rsPeers->getGroupInfo(RsNodeGroupId(grpMeta.mInternalCircle),ginfo))
            {
                            std::cerr << "(EE) Cannot get information for internal circle (group node) ID " << grpMeta.mInternalCircle << " which conditions dissemination of GXS group " << grpMeta.mGroupId << std::endl;
                return false ;
            }

            bool res = (ginfo.peerIds.find(pgpId) != ginfo.peerIds.end());

#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Final Answer is: " << res << std::endl;
#endif
                        return res ;
        }
        else
        {
            // An empty internal circle id means this peer can only
            // send circle related info from peer he received it from.
                    // Normally this should be a pgp-based decision, but if we do that,
                    // A ---> B ----> A' ---...--->B' , then A' will also send to B' since B is the
                    // originator for A'. So A will lose control of who can see the data. This should
                    // be discussed further...

#ifdef NXS_NET_DEBUG_4
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Empty internal circle: cannot only send/recv info to/from Peer we received it from (grpMeta.mOriginator=" << grpMeta.mOriginator << ")" << std::endl;
            GXSNETDEBUG_PG(sslId,grpMeta.mGroupId) << "   Final answer is: " << (grpMeta.mOriginator == sslId) << std::endl;
#endif
            if(grpMeta.mOriginator == sslId)
                return true;
            else
                return false;
        }
}

void RsGxsNetService::pauseSynchronisation(bool /* enabled */)
{
	std::cerr << "(EE) RsGxsNetService::pauseSynchronisation() called, but not implemented." << std::endl;
}

void RsGxsNetService::setSyncAge(const RsGxsGroupId &grpId, uint32_t age_in_secs)
{
	RS_STACK_MUTEX(mNxsMutex) ;

    locked_checkDelay(age_in_secs) ;

    RsGxsGrpConfig& conf(locked_getGrpConfig(grpId));

    if(conf.msg_req_delay != age_in_secs)
    {
    	conf.msg_req_delay = age_in_secs;

        // we also need to zero the client TS, in order to trigger a new sync
		locked_resetClientTS(grpId);

        IndicateConfigChanged();

        // also send an event so that UI is updated

        mNewGrpSyncParamsToNotify.insert(grpId);
    }
}
void RsGxsNetService::setKeepAge(const RsGxsGroupId &grpId, uint32_t age_in_secs)
{
	RS_STACK_MUTEX(mNxsMutex) ;

    locked_checkDelay(age_in_secs) ;

    RsGxsGrpConfig& conf(locked_getGrpConfig(grpId));

    if(conf.msg_keep_delay != age_in_secs)
    {
    	conf.msg_keep_delay = age_in_secs;
        IndicateConfigChanged();
    }
}

RsGxsGrpConfig& RsGxsNetService::locked_getGrpConfig(const RsGxsGroupId& grp_id)
{
	GrpConfigMap::iterator it = mServerGrpConfigMap.find(grp_id);

	if(it == mServerGrpConfigMap.end())
	{
		RsGxsGrpConfig& conf(mServerGrpConfigMap[grp_id]) ;

		conf.msg_keep_delay = mDefaultMsgStorePeriod;
		conf.msg_send_delay = mDefaultMsgSyncPeriod;
		conf.msg_req_delay  = mDefaultMsgSyncPeriod;

		conf.max_visible_count = 0 ;
		conf.statistics_update_TS = 0 ;
		conf.last_group_modification_TS = 0 ;

		return conf ;
	}
	else
		return it->second;
}

uint32_t RsGxsNetService::getSyncAge(const RsGxsGroupId& grpId)
{
	RS_STACK_MUTEX(mNxsMutex) ;

	return locked_getGrpConfig(grpId).msg_req_delay ;
}
uint32_t RsGxsNetService::getKeepAge(const RsGxsGroupId& grpId)
{
    RS_STACK_MUTEX(mNxsMutex) ;

	return locked_getGrpConfig(grpId).msg_keep_delay ;
}

int RsGxsNetService::requestGrp(const std::list<RsGxsGroupId>& grpId, const RsPeerId& peerId)
{
	RS_STACK_MUTEX(mNxsMutex) ;
#ifdef NXS_NET_DEBUG_0
    GXSNETDEBUG_P_(peerId) << "RsGxsNetService::requestGrp(): adding explicit group requests to peer " << peerId << std::endl;

    for(std::list<RsGxsGroupId>::const_iterator it(grpId.begin());it!=grpId.end();++it)
        GXSNETDEBUG_PG(peerId,*it) << "   Group ID: " << *it << std::endl;
#endif
    mExplicitRequest[peerId].assign(grpId.begin(), grpId.end());
	return 1;
}

void RsGxsNetService::processExplicitGroupRequests()
{
	RS_STACK_MUTEX(mNxsMutex) ;

	std::map<RsPeerId, std::list<RsGxsGroupId> >::const_iterator cit = mExplicitRequest.begin();

	for(; cit != mExplicitRequest.end(); ++cit)
	{
#ifdef NXS_NET_DEBUG_0
        GXSNETDEBUG_P_(cit->first) << "RsGxsNetService::sending pending explicit group requests to peer " << cit->first << std::endl;
#endif
        const RsPeerId& peerId = cit->first;
		const std::list<RsGxsGroupId>& groupIdList = cit->second;

		std::list<RsNxsItem*> grpSyncItems;
		std::list<RsGxsGroupId>::const_iterator git = groupIdList.begin();
		uint32_t transN = locked_getTransactionId();
		for(; git != groupIdList.end(); ++git)
		{
#ifdef NXS_NET_DEBUG_0
            GXSNETDEBUG_P_(peerId) << "   group request for grp ID " << *git << " to peer " << peerId << std::endl;
#endif
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
        RsGxsGrpMetaTemporaryMap grpMetaMap;
        grpMetaMap[mit->first] = NULL;
        mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

        // Find the publish keys in the retrieved info

        const RsGxsGrpMetaData *grpMeta = grpMetaMap[mit->first] ;

        if(grpMeta == NULL)
        {
            std::cerr << "(EE) RsGxsNetService::sharePublishKeys() Publish keys cannot be found for group " << mit->first << std::endl;
            continue ;
        }

        const RsTlvSecurityKeySet& keys = grpMeta->keys;

        std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator kit = keys.private_keys.begin(), kit_end = keys.private_keys.end();
        bool publish_key_found = false;
        RsTlvPrivateRSAKey publishKey ;

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

            publishKeyItem->private_key = publishKey ;
            publishKeyItem->PeerId(*it);

            generic_sendItem(publishKeyItem);
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
	GXSNETDEBUG_PG(item->PeerId(),item->grpId) << "  Got key Item: " << item->private_key.keyId << std::endl;
#endif

	// Get the meta data for this group Id
	//
	RsGxsGrpMetaTemporaryMap grpMetaMap;
	grpMetaMap[item->grpId] = NULL;

	mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

	// update the publish keys in this group meta info

	const RsGxsGrpMetaData *grpMeta = grpMetaMap[item->grpId] ;
	if (!grpMeta) {
		std::cerr << "(EE) RsGxsNetService::handleRecvPublishKeys() grpMeta not found." << std::endl;
		return ;
	}

	// Check that the keys correspond, and that FULL keys are supplied, etc.

#ifdef NXS_NET_DEBUG_3
	GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "  Key received: " << std::endl;
#endif

	bool admin = (item->private_key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)   && (item->private_key.keyFlags & RSTLV_KEY_TYPE_FULL) ;
	bool publi = (item->private_key.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH) && (item->private_key.keyFlags & RSTLV_KEY_TYPE_FULL) ;

#ifdef NXS_NET_DEBUG_3
	GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "    Key id = " << item->private_key.keyId << "  admin=" << admin << ", publish=" << publi << " ts=" << item->private_key.endTS << std::endl;
#endif

	if(!(!admin && publi))
	{
		std::cerr << "  Key is not a publish private key. Discarding!" << std::endl;
		return ;
	}
	// Also check that we don't already have full keys for that group.

	if(grpMeta->keys.public_keys.find(item->private_key.keyId) == grpMeta->keys.public_keys.end())
	{
		std::cerr << "   (EE) Key not found in known group keys. This is an inconsistency." << std::endl;
		return ;
	}

	if(grpMeta->keys.private_keys.find(item->private_key.keyId) != grpMeta->keys.private_keys.end())
	{
#ifdef NXS_NET_DEBUG_3
		GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "   (EE) Publish key already present in database. Discarding message." << std::endl;
#endif
        mNewPublishKeysToNotify.insert(item->grpId) ;
		return ;
	}

	// Store/update the info.

	RsTlvSecurityKeySet keys = grpMeta->keys ;
	keys.private_keys[item->private_key.keyId] = item->private_key ;

	bool ret = mDataStore->updateGroupKeys(item->grpId,keys, grpMeta->mSubscribeFlags | GXS_SERV::GROUP_SUBSCRIBE_PUBLISH) ;

	if(ret)
	{
#ifdef NXS_NET_DEBUG_3
		GXSNETDEBUG_PG(item->PeerId(),item->grpId)<< "  updated database with new publish keys." << std::endl;
#endif
        mNewPublishKeysToNotify.insert(item->grpId) ;
	}
	else
	{
		std::cerr << "(EE) could not update database. Something went wrong." << std::endl;
	}
}

bool RsGxsNetService::getGroupServerUpdateTS(const RsGxsGroupId& gid,rstime_t& group_server_update_TS, rstime_t& msg_server_update_TS)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    group_server_update_TS = mGrpServerUpdate.grpUpdateTS ;

    ServerMsgMap::iterator it = mServerMsgUpdateMap.find(gid) ;

    if(mServerMsgUpdateMap.end() == it)
	    msg_server_update_TS = 0 ;
    else
	    msg_server_update_TS = it->second.msgUpdateTS ;

    return true ;
}

bool RsGxsNetService::removeGroups(const std::list<RsGxsGroupId>& groups)
{
    RS_STACK_MUTEX(mNxsMutex) ;

#ifdef NXS_NET_DEBUG_0
	GXSNETDEBUG___ << "Removing group information from deleted groups:" << std::endl;
#endif

    for(std::list<RsGxsGroupId>::const_iterator git(groups.begin());git!=groups.end();++git)
    {
#ifdef NXS_NET_DEBUG_0
		GXSNETDEBUG__G(*git) << "  deleting info for group " << *git << std::endl;
#endif

		// Here we do not use locked_getGrpConfig() because we dont want the entry to be created if it doesnot already exist.

        GrpConfigMap::iterator it = mServerGrpConfigMap.find(*git) ;

        if(it != mServerGrpConfigMap.end())
        {
            it->second.suppliers.TlvClear();			// we dont erase the entry, because we want to keep the user-defined sync parameters.
            it->second.max_visible_count = 0;
        }

        mServerMsgUpdateMap.erase(*git) ;

        for(ClientMsgMap::iterator it(mClientMsgUpdateMap.begin());it!=mClientMsgUpdateMap.end();++it)
            it->second.msgUpdateInfos.erase(*git) ;

        // This last step is very important: it makes RS re-sync all groups after deleting, with every new peer. If may happen indeed that groups
        // are deleted because there's no suppliers since the actual supplier friend is offline for too long. In this case, the group needs
        // to re-appear when the friend who is a subscriber comes online again.

        mClientGrpUpdateMap.clear();
    }

	IndicateConfigChanged();
    return true ;
}

bool RsGxsNetService::isDistantPeer(const RsPeerId& pid)
{
    RS_STACK_MUTEX(mNxsMutex) ;

	if(!mAllowDistSync || mGxsNetTunnel == NULL)
		return false ;

	RsGxsGroupId group_id ;

	return mGxsNetTunnel->isDistantPeer(RsGxsNetTunnelVirtualPeerId(pid),group_id);
}

bool RsGxsNetService::stampMsgServerUpdateTS(const RsGxsGroupId& gid)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    return locked_stampMsgServerUpdateTS(gid) ;
}

bool RsGxsNetService::locked_stampMsgServerUpdateTS(const RsGxsGroupId& gid)
{
    RsGxsServerMsgUpdate& m(mServerMsgUpdateMap[gid]);

	m.msgUpdateTS = time(NULL) ;

    return true;
}

DistantSearchGroupStatus RsGxsNetService::getDistantSearchStatus(const RsGxsGroupId& group_id)
{
    auto it = mSearchedGroups.find(group_id);

    if(it != mSearchedGroups.end())
        return it->second.status;

    for(auto it2:mDistantSearchResults)
        if(it2.second.find(group_id) != it2.second.end())
            return DistantSearchGroupStatus::CAN_BE_REQUESTED;

    return DistantSearchGroupStatus::UNKNOWN;
}

TurtleRequestId RsGxsNetService::turtleGroupRequest(const RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mNxsMutex) ;

    rstime_t now = time(NULL);
    auto it = mSearchedGroups.find(group_id) ;

    if(mSearchedGroups.end() != it && (it->second.ts + MIN_DELAY_BETWEEN_GROUP_SEARCH > now))
    {
        std::cerr << "(WW) Last turtle request was " << now - it->second.ts << " secs ago. Not searching again." << std::endl;
        return it->second.request_id;
    }

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG__G(group_id) << "  requesting group id " << group_id << " using turtle" << std::endl;
#endif
	TurtleRequestId req = mGxsNetTunnel->turtleGroupRequest(group_id,this) ;

    GroupRequestRecord& rec(mSearchedGroups[group_id]) ;

	rec.request_id = req;
	rec.ts         = now;
    rec.status     = DistantSearchGroupStatus::ONGOING_REQUEST;

    mSearchRequests[req] = group_id;

    return req;
}
TurtleRequestId RsGxsNetService::turtleSearchRequest(const std::string& match_string)
{
    return mGxsNetTunnel->turtleSearchRequest(match_string,this) ;
}

#ifndef RS_DEEP_CHANNEL_INDEX
static bool termSearch(const std::string& src, const std::string& substring)
{
		/* always ignore case */
	return src.end() != std::search( src.begin(), src.end(), substring.begin(), substring.end(), RsRegularExpression::CompareCharIC() );
}
#endif // ndef RS_DEEP_CHANNEL_INDEX

bool RsGxsNetService::retrieveDistantSearchResults(TurtleRequestId req,std::map<RsGxsGroupId,RsGxsGroupSearchResults>& group_infos)
{
    RS_STACK_MUTEX(mNxsMutex) ;

    auto it = mDistantSearchResults.find(req) ;

    if(it == mDistantSearchResults.end())
        return false ;

    group_infos = it->second;
    return true ;
}
bool RsGxsNetService::retrieveDistantGroupSummary(const RsGxsGroupId& group_id,RsGxsGroupSearchResults& gs)
{
	RS_STACK_MUTEX(mNxsMutex) ;
    for(auto it(mDistantSearchResults.begin());it!=mDistantSearchResults.end();++it)
    {
        auto it2 = it->second.find(group_id) ;

        if(it2 != it->second.end())
        {
            gs = it2->second;
            return true ;
        }
    }
    return false ;
}
bool RsGxsNetService::clearDistantSearchResults(const TurtleRequestId& id)
{
    RS_STACK_MUTEX(mNxsMutex) ;
    mDistantSearchResults.erase(id);
    return true ;
}

void RsGxsNetService::receiveTurtleSearchResults( TurtleRequestId req, const std::list<RsGxsGroupSummary>& group_infos )
{
	std::set<RsGxsGroupId> groupsToNotifyResults;

	{
		RS_STACK_MUTEX(mNxsMutex);

		RsGxsGrpMetaTemporaryMap grpMeta;
		std::map<RsGxsGroupId,RsGxsGroupSearchResults>& search_results_map(mDistantSearchResults[req]);

#ifdef NXS_NET_DEBUG_9
        std::cerr << "Received group summary through turtle search for the following groups:" << std::endl;
#endif

		for(const RsGxsGroupSummary& gps : group_infos)
        {
            std::cerr <<"  " << gps.mGroupId << "  \"" << gps.mGroupName << "\"" << std::endl;
			grpMeta[gps.mGroupId] = nullptr;
        }

		mDataStore->retrieveGxsGrpMetaData(grpMeta);

#ifdef NXS_NET_DEBUG_9
        std::cerr << "Retrieved data store group data for the following groups:" <<std::endl;
        for(auto& it:grpMeta)
            std::cerr << "  " << it.first << " : " << it.second->mGroupName << std::endl;
#endif

		for (const RsGxsGroupSummary& gps : group_infos)
		{
#ifndef RS_DEEP_CHANNEL_INDEX
			/* Only keep groups that are not locally known, and groups that are
			 * not already in the mDistantSearchResults structure.
			 * mDataStore may in some situations allocate an empty group meta data, so it's important
			 * to test that the group meta is both non null and actually corresponds to the group id we seek. */

            auto& meta(grpMeta[gps.mGroupId]);

			if(meta != nullptr && meta->mGroupId == gps.mGroupId)
                continue;

#ifdef NXS_NET_DEBUG_9
            std::cerr << "  group " << gps.mGroupId << " is not known. Adding it to search results..." << std::endl;
#endif

#else // ndef RS_DEEP_CHANNEL_INDEX
			/* When deep search is enabled search results may bring more info
			 * then we already have also about post that are indexed by xapian,
			 * so we don't apply this filter in this case. */
#endif
			const RsGxsGroupId& grpId(gps.mGroupId);

			groupsToNotifyResults.insert(grpId);

            // Find search results place for this particular group

#ifdef NXS_NET_DEBUG_9
            std::cerr << "  Adding gps=" << gps.mGroupId << " name=\"" << gps.mGroupName << "\" gps.mSearchContext=\"" << gps.mSearchContext << "\"" << std::endl;
#endif
			RsGxsGroupSearchResults& eGpS(search_results_map[grpId]);

            if(eGpS.mGroupId != grpId)	// not initialized yet. So we do it now.
            {
                eGpS.mGroupId   = gps.mGroupId;
				eGpS.mGroupName = gps.mGroupName;
				eGpS.mAuthorId  = gps.mAuthorId;
				eGpS.mPublishTs = gps.mPublishTs;
				eGpS.mSignFlags = gps.mSignFlags;
            }
            // We should check that the above values are always the same for all info that is received. In the end, we'll
            // request the group meta and check the signature, but it may be misleading to receive a forged information
            // that is not the real one.

            ++eGpS.mPopularity;	// increase popularity. This is not a real counting, but therefore some heuristic estimate.
            eGpS.mNumberOfMessages = std::max( eGpS.mNumberOfMessages, gps.mNumberOfMessages );
			eGpS.mLastMessageTs    = std::max( eGpS.mLastMessageTs, gps.mLastMessageTs );

            if(gps.mSearchContext != gps.mGroupName)			// this is a bit of a hack. We should have flags to tell where the search hit happens
				eGpS.mSearchContexts.insert(gps.mSearchContext);
		}
	} // end RS_STACK_MUTEX(mNxsMutex);

	for(const RsGxsGroupId& grpId : groupsToNotifyResults)
		mObserver->receiveDistantSearchResults(req, grpId);
}

void RsGxsNetService::receiveTurtleSearchResults(TurtleRequestId req,const unsigned char *encrypted_group_data,uint32_t encrypted_group_data_len)
{
#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " received encrypted group data for turtle search request " << std::hex << req << std::dec << ": " << RsUtil::BinToHex(encrypted_group_data,encrypted_group_data_len,50) << std::endl;
#endif
    auto it = mSearchRequests.find(req);

    if(mSearchRequests.end() == it)
    {
        std::cerr << "(EE) received search results for unknown request " << std::hex << req << std::dec ;
        return;
    }
    RsGxsGroupId grpId = it->second;

    uint8_t encryption_master_key[32];
    Sha256CheckSum s = RsDirUtil::sha256sum(grpId.toByteArray(),grpId.SIZE_IN_BYTES);
    memcpy(encryption_master_key,s.toByteArray(),32);

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " attempting data decryption with master key " << RsUtil::BinToHex(encryption_master_key,32) << std::endl;
#endif
    unsigned char *clear_group_data = NULL;
    uint32_t clear_group_data_len ;

    if(!librs::crypto::decryptAuthenticateData(encrypted_group_data,encrypted_group_data_len,encryption_master_key,clear_group_data,clear_group_data_len))
    {
        std::cerr << "(EE) Could not decrypt data. Something went wrong. Wrong key??" << std::endl;
        return ;
    }

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " successfuly decrypted data : " << RsUtil::BinToHex(clear_group_data,clear_group_data_len,50) << std::endl;
#endif
    uint32_t used_size = clear_group_data_len;
    RsItem *item = RsNxsSerialiser(mServType).deserialise(clear_group_data,&used_size) ;
    RsNxsGrp *nxs_identity_grp=nullptr;

    if(used_size < clear_group_data_len)
    {
        uint32_t remaining_size = clear_group_data_len-used_size ;
        RsItem *item2 = RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSID).deserialise(clear_group_data+used_size,&remaining_size) ;

        nxs_identity_grp = dynamic_cast<RsNxsGrp*>(item2);

        if(!nxs_identity_grp)
            std::cerr << "(EE) decrypted item contains more data that cannot be deserialized as a GxsId. Unexpected!" << std::endl;

        // We should probably check that the identity that is sent corresponds to the group author and don't add
        // it otherwise. But in any case, this won't harm to add a new public identity. If that identity is banned,
        // the group will be discarded in RsGenExchange anyway.
    }

	free(clear_group_data);
    clear_group_data = NULL ;

    RsNxsGrp *nxs_grp = dynamic_cast<RsNxsGrp*>(item) ;

    if(nxs_grp == NULL)
    {
        std::cerr << "(EE) decrypted item is not a RsNxsGrp. Weird!" << std::endl;
        return ;
    }

#ifdef NXS_NET_DEBUG_8
    if(nxs_identity_grp)
        GXSNETDEBUG___ << " Serialized clear data contains a group " << nxs_grp->grpId << " in service " << std::hex << mServType << std::dec << " and a second identity item for an identity." << nxs_identity_grp->grpId << std::endl;
    else
        GXSNETDEBUG___ << " Serialized clear data contains a single GXS group for Grp Id " << nxs_grp->grpId << " in service " << std::hex << mServType << std::dec << std::endl;
#endif

    std::vector<RsNxsGrp*> new_grps(1,nxs_grp);

    GroupRequestRecord& rec(mSearchedGroups[nxs_grp->grpId]) ;
    rec.status = DistantSearchGroupStatus::HAVE_GROUP_DATA;

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " passing the grp data to observer." << std::endl;
#endif
	mObserver->receiveNewGroups(new_grps);
	mObserver->receiveDistantSearchResults(req, grpId);

    if(nxs_identity_grp)
        mGixs->receiveNewIdentity(nxs_identity_grp);
}

bool RsGxsNetService::search( const std::string& substring,
                              std::list<RsGxsGroupSummary>& group_infos )
{
	group_infos.clear();

#ifdef RS_DEEP_CHANNEL_INDEX
	std::vector<DeepChannelsSearchResult> results;
	DeepChannelsIndex::search(substring, results);

	for(auto dsr : results)
	{
		RsUrl rUrl(dsr.mUrl);
		const auto& uQ(rUrl.query());
		auto rit = uQ.find("id");
		if(rit != rUrl.query().end())
		{
			RsGroupNetworkStats stats;
			RsGxsGroupId grpId(rit->second);
			if( !grpId.isNull() && getGroupNetworkStats(grpId, stats) )
			{
				RsGxsGroupSummary s;

				s.mGroupId = grpId;

				if((rit = uQ.find("name")) != uQ.end())
					s.mGroupName = rit->second;
				if((rit = uQ.find("signFlags")) != uQ.end())
					s.mSignFlags = static_cast<uint32_t>(std::stoul(rit->second));
				if((rit = uQ.find("publishTs")) != uQ.end())
					s.mPublishTs = static_cast<rstime_t>(std::stoll(rit->second));
				if((rit = uQ.find("authorId")) != uQ.end())
					s.mAuthorId  = RsGxsId(rit->second);

				s.mSearchContext = dsr.mSnippet;

				s.mNumberOfMessages = stats.mMaxVisibleCount;
				s.mLastMessageTs    = stats.mLastGroupModificationTS;
				s.mPopularity       = stats.mSuppliers;

				group_infos.push_back(s);
			}
		}
	}
#else // RS_DEEP_CHANNEL_INDEX
	RsGxsGrpMetaTemporaryMap grpMetaMap;
	{
		RS_STACK_MUTEX(mNxsMutex) ;
		mDataStore->retrieveGxsGrpMetaData(grpMetaMap);
	}

	RsGroupNetworkStats stats;
	for(auto it(grpMetaMap.begin());it!=grpMetaMap.end();++it)
		if(termSearch(it->second->mGroupName,substring))
		{
			getGroupNetworkStats(it->first,stats);

			RsGxsGroupSummary s;
			s.mGroupId           = it->first;
			s.mGroupName         = it->second->mGroupName;
			s.mSearchContext     = it->second->mGroupName;
			s.mSignFlags         = it->second->mSignFlags;
			s.mPublishTs         = it->second->mPublishTs;
			s.mAuthorId          = it->second->mAuthorId;
			s.mNumberOfMessages  = stats.mMaxVisibleCount;
			s.mLastMessageTs     = stats.mLastGroupModificationTS;
			s.mPopularity        = it->second->mPop;

			group_infos.push_back(s);
		}
#endif // RS_DEEP_CHANNEL_INDEX

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << "  performing local substring search in response to distant request. Found " << group_infos.size() << " responses." << std::endl;
#endif
    return !group_infos.empty();
}

bool RsGxsNetService::search(const Sha1CheckSum& hashed_group_id,unsigned char *& encrypted_group_data,uint32_t& encrypted_group_data_len)
{
    // First look into the grp hash cache

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " Received group data request for hash " << hashed_group_id << std::endl;
#endif
    auto it = mGroupHashCache.find(hashed_group_id) ;
    RsNxsGrp *grp_data = NULL ;

    if(mGroupHashCache.end() != it)
    {
        grp_data = it->second;
    }
    else
	{
		// Now check if the last request was too close in time, in which case, we dont retrieve data.

		if(mLastCacheReloadTS + 60 > time(NULL))
		{
			std::cerr << "(WW) Not found in cache, and last cache reload less than 60 secs ago. Returning false. " << std::endl;
			return false ;
		}

#ifdef NXS_NET_DEBUG_8
		GXSNETDEBUG___ << " reloading group cache information" << std::endl;
#endif
		RsNxsGrpDataTemporaryMap grpDataMap;
		{
			RS_STACK_MUTEX(mNxsMutex) ;
			mDataStore->retrieveNxsGrps(grpDataMap, true, true);
            mLastCacheReloadTS = time(NULL);
		}

        for(auto it(grpDataMap.begin());it!=grpDataMap.end();++it)
            if(it->second->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED )	// only cache subscribed groups
            {
                RsNxsGrp *grp = it->second ;
                grp->metaData->keys.TlvClear() ;// clean private keys. This technically not needed, since metaData is not serialized, but I still prefer.

                Sha1CheckSum hash(RsDirUtil::sha1sum(it->first.toByteArray(),it->first.SIZE_IN_BYTES));

				mGroupHashCache[hash] = grp ;
                it->second = NULL ; // prevents deletion

                if(hash == hashed_group_id)
                    grp_data = grp ;
            }
	}

    if(!grp_data)
    {
#ifdef NXS_NET_DEBUG_8
		GXSNETDEBUG___ << " no group found for hash " << hashed_group_id << ": returning false." << std::endl;
#endif
        return false ;
    }

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " found corresponding group data group id in cache group_id=" << grp_data->grpId << std::endl;
#endif
    // Finally, serialize and encrypt the grp data

    uint32_t size = RsNxsSerialiser(mServType).size(grp_data);
    RsNxsGrp *author_group=nullptr;

    if(!grp_data->metaData->mAuthorId.isNull())
    {
#ifdef NXS_NET_DEBUG_8
        GXSNETDEBUG___ << " this group has an author identity " << grp_data->metaData->mAuthorId << " that we need to send at the same time." << std::endl;
#endif
        mGixs->retrieveNxsIdentity(grp_data->metaData->mAuthorId,author_group);	// whatever gets the data

        if(!author_group)
        {
            std::cerr << "(EE) Cannot retrieve author group data " << grp_data->metaData->mAuthorId << " for GXS group " << grp_data->grpId << std::endl;
            return false;
        }

        delete author_group->metaData;	// delete private information, just in case, but normally it is not serialized.
        author_group->metaData = NULL ;

        size += RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSID).size(author_group);
    }

	RsTemporaryMemory mem(size) ;
    uint32_t used_size=size;

    RsNxsSerialiser(mServType).serialise(grp_data,mem,&used_size) ;

    uint32_t remaining_size=size-used_size;

    if(author_group)
    {
#ifdef NXS_NET_DEBUG_8
        GXSNETDEBUG___ << " Serializing author group data..." << std::endl;
#endif
        RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSID).serialise(author_group,mem+used_size,&remaining_size);
    }

    uint8_t encryption_master_key[32];
    Sha256CheckSum s = RsDirUtil::sha256sum(grp_data->grpId.toByteArray(),grp_data->grpId.SIZE_IN_BYTES);
    memcpy(encryption_master_key,s.toByteArray(),32);

#ifdef NXS_NET_DEBUG_8
	GXSNETDEBUG___ << " sending data encrypted with master key " << RsUtil::BinToHex(encryption_master_key,32) << std::endl;
#endif
    return librs::crypto::encryptAuthenticateData(mem,size,encryption_master_key,encrypted_group_data,encrypted_group_data_len);
}

