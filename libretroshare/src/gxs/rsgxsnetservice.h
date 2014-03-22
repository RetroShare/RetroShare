#ifndef RSGXSNETSERVICE_H
#define RSGXSNETSERVICE_H

/*
 * libretroshare/src/gxs: rsgxnetservice.h
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

#include <list>
#include <queue>

#include "rsnxs.h"
#include "rsgds.h"
#include "rsnxsobserver.h"
#include "pqi/p3linkmgr.h"
#include "serialiser/rsnxsitems.h"
#include "serialiser/rsgxsupdateitems.h"
#include "rsgxsnetutils.h"
#include "pqi/p3cfgmgr.h"
#include "rsgixs.h"

/// keep track of transaction number
typedef std::map<uint32_t, NxsTransaction*> TransactionIdMap;

/// to keep track of peers active transactions
typedef std::map<RsPeerId, TransactionIdMap > TransactionsPeerMap;


/*!
 * This class implements the RsNetWorkExchangeService
 * using transactions to handle synchrnisation of Nxs items between
 * peers in a network
 * Transactions requires the maintenance of several states between peers
 *
 * Thus a data structure maintains: peers, and their active transactions
 * Then for each transaction it needs to be noted if this is an outgoing or incoming transaction
 * Outgoing transaction are in 3 different states:
 *   1. START 2. INITIATED 3. SENDING 4. END
 * Incoming transaction are in 3 different states
 *   1. START 2. RECEIVING 3. END
 */
class RsGxsNetService : public RsNetworkExchangeService, public p3ThreadedService,
    public p3Config
{
public:

	static const uint32_t FRAGMENT_SIZE;
    /*!
     * only one observer is allowed
     * @param servType service type
     * @param gds The data service which allows read access to a service/store
     * @param nxsObs observer will be notified whenever new messages/grps
     * @param nxsObs observer will be notified whenever new messages/grps
     * arrive
     */
    RsGxsNetService(uint16_t servType, RsGeneralDataService *gds,
                RsNxsNetMgr *netMgr, 
 		RsNxsObserver *nxsObs,  // used to be = NULL.
		const RsServiceInfo serviceInfo,
		RsGixsReputation* reputations = NULL, RsGcxs* circles = NULL, bool grpAutoSync = true);

    virtual ~RsGxsNetService();

    virtual RsServiceInfo getServiceInfo() { return mServiceInfo; }

public:


    /*!
     * Use this to set how far back synchronisation of messages should take place
     * @param age the max age a sync item can to be allowed in a synchronisation
     */
    void setSyncAge(uint32_t age);

    /*!
     * Explicitly requests all the groups contained by a peer
     * Circumvents polling of peers for message
     * @param peerId id of peer
     */
    void requestGroupsOfPeer(const RsPeerId& peerId){ return;}

    /*!
     * get messages of a peer for a given group id, this circumvents the normal
     * polling of peers for messages of given group id
     * @param peerId Id of peer
     * @param grpId id of group to request messages for
     */
    void requestMessagesOfPeer(const RsPeerId& peerId, const std::string& grpId){ return; }

    /*!
     * pauses synchronisation of subscribed groups and request for group id
     * from peers
     * @param enabled set to false to disable pause, and true otherwise
     */
    void pauseSynchronisation(bool enabled);


    /*!
     * Request for this message is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param msgId the messages to retrieve
     * @return request token to be redeemed
     */
    int requestMsg(const RsGxsGrpMsgIdPair& msgId){ return 0;}

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param enabled set to false to disable pause, and true otherwise
     * @return request token to be redeemed
     */
    int requestGrp(const std::list<RsGxsGroupId>& grpId, const RsPeerId& peerId);

    /* p3Config methods */

public:

    bool	loadList(std::list<RsItem *>& load);
    bool saveList(bool &cleanup, std::list<RsItem *>&);
    RsSerialiser *setupSerialiser();

public:

    /*!
     * initiates synchronisation
     */
    int tick();

    /*!
     * Processes transactions and job queue
     */
    void run();

private:

    /*!
     * called when
     * items are deemed to be waiting in p3Service item queue
     */
    void recvNxsItemQueue();


    /** S: Transaction processing **/

    /*!
     * These process transactions which are in a wait state
     * Also moves transaction which have been completed to
     * the completed transactions list
     */
    void processTransactions();

    /*!
     * Process completed transaction, which either simply
     * retires a transaction or additionally generates a response
     * to the completed transaction
     */
    void processCompletedTransactions();

    /*!
     * Process transaction owned/started by user
     * @param tr transaction to process, ownership stays with callee
     */
    void locked_processCompletedOutgoingTrans(NxsTransaction* tr);

    /*!
     * Process transactions started/owned by other peers
     * @param tr transaction to process, ownership stays with callee
     */
    void locked_processCompletedIncomingTrans(NxsTransaction* tr);


    /*!
     * Process a transaction item, assumes a general lock
     * @param item the transaction item to process
     */
    bool locked_processTransac(RsNxsTransac* item);

    /*!
     * This adds a transaction
     * completeted transaction list
     * If this is an outgoing transaction, transaction id is
     * decrement
     * @param trans transaction to add
     */
    void locked_completeTransaction(NxsTransaction* trans);

    /*!
     * This retrieves a unique transaction id that
     * can be used in an outgoing transaction
     */
    uint32_t locked_getTransactionId();

    /*!
     * This attempts to push the transaction id counter back if you have
     * active outgoing transactions in play
     */
    bool attemptRecoverIds();

    /*!
     * The cb listener is the owner of the grps
     * @param grps
     */
    //void notifyListenerGrps(std::list<RsNxsGrp*>& grps);

    /*!
     * The cb listener is the owner of the msgs
     * @param msgs
     */
    //void notifyListenerMsgs(std::list<RsNxsMsg*>& msgs);

    /*!
     * Generates new transaction to send msg requests based on list
     * of msgs received from peer stored in passed transaction
     * @param tr transaction responsible for generating msg request
     */
    void locked_genReqMsgTransaction(NxsTransaction* tr);

    /*!
     * Generates new transaction to send grp requests based on list
     * of grps received from peer stored in passed transaction
     * @param tr transaction responsible for generating grp request
     */
    void locked_genReqGrpTransaction(NxsTransaction* tr);

    /*!
     * Generates new transaction to send msg data based on list
     * of grpids received from peer stored in passed transaction
     * @param tr transaction responsible for generating grp request
     */
    void locked_genSendMsgsTransaction(NxsTransaction* tr);

    /*!
     * Generates new transaction to send grp data based on list
     * of grps received from peer stored in passed transaction
     * @param tr transaction responsible for generating grp request
     */
    void locked_genSendGrpsTransaction(NxsTransaction* tr);

    /*!
     * convenience function to add a transaction to list
     * @param tr transaction to add
     */
    bool locked_addTransaction(NxsTransaction* tr);

    void cleanTransactionItems(NxsTransaction* tr) const;

    /*!
     *  @param tr the transaction to check for timeout
     *  @return false if transaction has timed out, true otherwise
     */
    bool locked_checkTransacTimedOut(NxsTransaction* tr);

    /** E: Transaction processing **/

    /** S: item handlers **/

    /*!
     * This attempts handles transaction items
     * ownership of item is left with callee if this method returns false
     * @param item transaction item to handle
     * @return false if transaction could not be handled, ownership of item is left with callee
     */
    bool handleTransaction(RsNxsItem* item);

    /*!
     * Handles an nxs item for group synchronisation
     * by startin a transaction and sending a list
     * of groups held by user
     * @param item contains grp sync info
     */
    void handleRecvSyncGroup(RsNxsSyncGrp* item);

    /*!
     * Handles an nxs item for msgs synchronisation
     * @param item contaims msg sync info
     */
    void handleRecvSyncMessage(RsNxsSyncMsg* item);

    /** E: item handlers **/


    void runVetting();

    /*!
     * @param peerId The peer to vet to see if they can receive this groupid
     * @param grpMeta this is the meta item to determine if it can be sent to given peer
     * @param toVet groupid/peer to vet are stored here if their circle id is not cached
     * @return false, if you cannot send to this peer, true otherwise
     */
    bool canSendGrpId(const RsPeerId& sslId, RsGxsGrpMetaData& grpMeta, std::vector<GrpIdCircleVet>& toVet);


    bool canSendMsgIds(const std::vector<RsGxsMsgMetaData*>& msgMetas, const RsGxsGrpMetaData&, const RsPeerId& sslId);

    void locked_createTransactionFromPending(MsgRespPending* grpPend);
    void locked_createTransactionFromPending(GrpRespPending* msgPend);
    void locked_createTransactionFromPending(GrpCircleIdRequestVetting* grpPend);
    void locked_createTransactionFromPending(MsgCircleIdsRequestVetting* grpPend);

    void locked_pushMsgTransactionFromList(std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN);
    void locked_pushGrpTransactionFromList(std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN);
    void locked_pushGrpRespFromList(std::list<RsNxsItem*>& respList, const RsPeerId& peer, const uint32_t& transN);
    void locked_pushMsgRespFromList(std::list<RsNxsItem*>& itemL, const RsPeerId& sslId, const uint32_t& transN);
    void syncWithPeers();
    void addGroupItemToList(NxsTransaction*& tr,
    		const RsGxsGroupId& grpId, uint32_t& transN,
    		std::list<RsNxsItem*>& reqList);

    bool locked_canReceive(const RsGxsGrpMetaData * const grpMeta, const RsPeerId& peerId);

    void processExplicitGroupRequests();
    
    void locked_doMsgUpdateWork(const RsNxsTransac* nxsTrans, const RsGxsGroupId& grpId);

    void updateServerSyncTS();

    bool locked_CanReceiveUpdate(const RsNxsSyncGrp* item);
    bool locked_CanReceiveUpdate(const RsNxsSyncMsg* item);

private:

    typedef std::vector<RsNxsGrp*> GrpFragments;
	typedef std::vector<RsNxsMsg*> MsgFragments;

    /*!
     * Fragment a message into individual fragments which are at most 150kb
     * @param msg message to fragment
     * @param msgFragments fragmented message
     * @return false if fragmentation fails true otherwise
     */
    bool fragmentMsg(RsNxsMsg& msg, MsgFragments& msgFragments) const;

    /*!
     * Fragment a group into individual fragments which are at most 150kb
     * @param grp group to fragment
     * @param grpFragments fragmented group
     * @return false if fragmentation fails true other wise
     */
    bool fragmentGrp(RsNxsGrp& grp, GrpFragments& grpFragments) const;

    /*!
     * Fragment a message into individual fragments which are at most 150kb
     * @param msg message to fragment
     * @param msgFragments fragmented message
     * @return NULL if not possible to reconstruct message from fragment,
     *              pointer to defragments nxs message is possible
     */
    RsNxsMsg* deFragmentMsg(MsgFragments& msgFragments) const;

    /*!
     * Fragment a group into individual fragments which are at most 150kb
     * @param grp group to fragment
     * @param grpFragments fragmented group
     * @return NULL if not possible to reconstruct group from fragment,
     *              pointer to defragments nxs group is possible
     */
    RsNxsGrp* deFragmentGrp(GrpFragments& grpFragments) const;


    /*!
     * Note that if all fragments for a message are not found then its fragments are dropped
     * @param fragments message fragments which are not necessarily from the same message
     * @param partFragments the partitioned fragments (into message ids)
     */
    void collateMsgFragments(MsgFragments fragments, std::map<RsGxsMessageId, MsgFragments>& partFragments) const;

    /*!
	 * Note that if all fragments for a group are not found then its fragments are dropped
	 * @param fragments group fragments which are not necessarily from the same group
	 * @param partFragments the partitioned fragments (into message ids)
	 */
    void collateGrpFragments(GrpFragments fragments, std::map<RsGxsGroupId, GrpFragments>& partFragments) const;
private:


    /*** transactions ***/

    /// active transactions
    TransactionsPeerMap mTransactions;

    /// completed transactions
    std::list<NxsTransaction*> mComplTransactions;

    /// transaction id counter
    uint32_t mTransactionN;

    /*** transactions ***/

    /*** synchronisation ***/
    std::list<RsNxsSyncGrp*> mSyncGrp;
    std::list<RsNxsSyncMsg*> mSyncMsg;
    /*** synchronisation ***/

    RsNxsObserver* mObserver;
    RsGeneralDataService* mDataStore;
    uint16_t mServType;

    // how much time must elapse before a timeout failure
    // for an active transaction
    uint32_t mTransactionTimeOut;


    RsPeerId mOwnId;

    RsNxsNetMgr* mNetMgr;

    /// for other members save transactions
    RsMutex mNxsMutex;

    uint32_t mSyncTs;

    const uint32_t mSYNC_PERIOD;

    RsGcxs* mCircles;
    RsGixsReputation* mReputations;
    bool mGrpAutoSync;

    // need to be verfied
    std::vector<AuthorPending*> mPendingResp;
    std::vector<GrpCircleVetting*> mPendingCircleVets;

    std::map<RsPeerId, std::list<RsGxsGroupId> > mExplicitRequest;

    // nxs sync optimisation
    // can pull dynamically the latest timestamp for each message

public:

    typedef std::map<RsPeerId, RsGxsMsgUpdateItem*> ClientMsgMap;
    typedef std::map<RsGxsGroupId, RsGxsServerMsgUpdateItem*> ServerMsgMap;
    typedef std::map<RsPeerId, RsGxsGrpUpdateItem*> ClientGrpMap;

private:

    ClientMsgMap mClientMsgUpdateMap;
    ServerMsgMap mServerMsgUpdateMap;
    ClientGrpMap mClientGrpUpdateMap;

    RsGxsServerGrpUpdateItem* mGrpServerUpdateItem;
    RsServiceInfo mServiceInfo;
};

#endif // RSGXSNETSERVICE_H
