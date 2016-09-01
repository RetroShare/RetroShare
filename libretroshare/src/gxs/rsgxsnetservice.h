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
#include "util/rssharedptr.h"

/// keep track of transaction number
typedef std::map<uint32_t, NxsTransaction*> TransactionIdMap;

/// to keep track of peers active transactions
typedef std::map<RsPeerId, TransactionIdMap > TransactionsPeerMap;

class PgpAuxUtils;

class RsGroupNetworkStatsRecord
{
    public:
        RsGroupNetworkStatsRecord() { max_visible_count= 0 ; update_TS=0; }

        std::set<RsPeerId> suppliers ;
	uint32_t max_visible_count ;
    time_t update_TS ;
};

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

	typedef RsSharedPtr<RsGxsNetService> pointer;

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
        RsGixsReputation* reputations = NULL, RsGcxs* circles = NULL, RsGixs *gixs=NULL,
        PgpAuxUtils *pgpUtils = NULL,
        bool grpAutoSync = true, bool msgAutoSync = true);

    virtual ~RsGxsNetService();

    virtual RsServiceInfo getServiceInfo() { return mServiceInfo; }

public:


    /*!
     * Use this to set how far back synchronisation of messages should take place
     * @param age the max age a sync item can to be allowed in a synchronisation
     */
    // NOT IMPLEMENTED
    virtual void setSyncAge(uint32_t age);

    /*!
     * pauses synchronisation of subscribed groups and request for group id
     * from peers
     * @param enabled set to false to disable pause, and true otherwise
     */
    // NOT IMPLEMENTED
    virtual void pauseSynchronisation(bool enabled);


    /*!
     * Request for this message is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param msgId the messages to retrieve
     * @return request token to be redeemed
     */
    virtual int requestMsg(const RsGxsGrpMsgIdPair& /* msgId */){ return 0;}

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param enabled set to false to disable pause, and true otherwise
     * @return request token to be redeemed
     */
    virtual int requestGrp(const std::list<RsGxsGroupId>& grpId, const RsPeerId& peerId);

    /*!
     * share publish keys for the specified group with the peers in the specified list.
     */

    virtual int sharePublishKey(const RsGxsGroupId& grpId,const std::set<RsPeerId>& peers) ;

    /*!
     * Returns statistics for the group networking activity: popularity (number of friends subscribers) and max_visible_msg_count,
     * that is the max nnumber of messages reported by a friend.
     */
    virtual bool getGroupNetworkStats(const RsGxsGroupId& id,RsGroupNetworkStats& stats) ;

    /*!
     * Used to inform the net service that we changed subscription status. That helps
     * optimising data transfer when e.g. unsubsribed groups are updated less often, etc
     */
    virtual void subscribeStatusChanged(const RsGxsGroupId& id,bool subscribed) ;

    virtual void rejectMessage(const RsGxsMessageId& msg_id) ;
    
    virtual bool getGroupServerUpdateTS(const RsGxsGroupId& gid,time_t& grp_server_update_TS,time_t& msg_server_update_TS) ;
    virtual bool stampMsgServerUpdateTS(const RsGxsGroupId& gid) ;

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
    virtual void data_tick();
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
     * @return false ownership of item left with callee
     */
    bool locked_processTransac(RsNxsTransacItem* item);

    /*!
     * This adds a transaction
     * completeted transaction list
     * If this is an outgoing transaction, transaction id is
     * decrement
     * @param trans transaction to add
     */
    void locked_completeTransaction(NxsTransaction* trans);

    /*!
     * \brief locked_stampMsgServerUpdateTS
     * 		updates the server msg time stamp. This function is the locked method for the one above with similar name
     * \param gid group id to stamp.
     * \return
     */
    bool locked_stampMsgServerUpdateTS(const RsGxsGroupId& gid);
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
     * This first checks if one can send a grpId based circles
     * If it can send, then it call locked_genSendMsgsTransaction
     * @param tr transaction responsible for generating grp request
     * @see locked_genSendMsgsTransaction
     */
    void locked_checkSendMsgsTransaction(NxsTransaction* tr);

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
    void handleRecvSyncGroup(RsNxsSyncGrpReqItem* item);

    /*!
     * Handles an nxs item for group statistics
     * @param item contaims update time stamp and number of messages
     */
    void handleRecvSyncGrpStatistics(RsNxsSyncGrpStatsItem *grs);
    
    /*!
     * Handles an nxs item for msgs synchronisation
     * @param item contaims msg sync info
     */
    void handleRecvSyncMessage(RsNxsSyncMsgReqItem* item,bool item_was_encrypted);

    /*!
     * Handles an nxs item for group publish key
     * @param item contaims keys/grp info
     */
    void handleRecvPublishKeys(RsNxsGroupPublishKeyItem*) ;

    /** E: item handlers **/


    void runVetting();

    /*!
     * @param peerId The peer to vet to see if they can receive this groupid
     * @param grpMeta this is the meta item to determine if it can be sent to given peer
     * @param toVet groupid/peer to vet are stored here if their circle id is not cached
     * @return false, if you cannot send to this peer, true otherwise
     */
    bool canSendGrpId(const RsPeerId& sslId, RsGxsGrpMetaData& grpMeta, std::vector<GrpIdCircleVet>& toVet, bool &should_encrypt);
    bool canSendMsgIds(std::vector<RsGxsMsgMetaData*>& msgMetas, const RsGxsGrpMetaData&, const RsPeerId& sslId, RsGxsCircleId &should_encrypt_id);

    /*!
     * \brief checkPermissionsForFriendGroup
     * 			Checks that we can send/recv from that node, given that the grpMeta has a distribution limited to a local circle.
     * \param sslId		Candidate peer to send to or to receive from.
     * \param grpMeta	Contains info about the group id, internal circle id, etc.
     * \return 			true only when the internal exists and validates as a friend node group, and contains the owner of sslId.
     */
    bool checkPermissionsForFriendGroup(const RsPeerId& sslId,const RsGxsGrpMetaData& grpMeta) ;

    bool checkCanRecvMsgFromPeer(const RsPeerId& sslId, const RsGxsGrpMetaData& meta, RsGxsCircleId& should_encrypt_id);

    void locked_createTransactionFromPending(MsgRespPending* grpPend);
    void locked_createTransactionFromPending(GrpRespPending* msgPend);
    bool locked_createTransactionFromPending(GrpCircleIdRequestVetting* grpPend) ;
    bool locked_createTransactionFromPending(MsgCircleIdsRequestVetting* grpPend) ;

    void locked_pushGrpTransactionFromList(std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN); // forms a grp list request
    void locked_pushMsgTransactionFromList(std::list<RsNxsItem*>& reqList, const RsPeerId& peerId, const uint32_t& transN);	// forms a msg list request
    void locked_pushGrpRespFromList(std::list<RsNxsItem*>& respList, const RsPeerId& peer, const uint32_t& transN);
    void locked_pushMsgRespFromList(std::list<RsNxsItem*>& itemL, const RsPeerId& sslId, const RsGxsGroupId &grp_id, const uint32_t& transN);
    
    void syncWithPeers();
    void syncGrpStatistics();
    void addGroupItemToList(NxsTransaction*& tr,
    		const RsGxsGroupId& grpId, uint32_t& transN,
    		std::list<RsNxsItem*>& reqList);

    //bool locked_canReceive(const RsGxsGrpMetaData * const grpMeta, const RsPeerId& peerId);

    void processExplicitGroupRequests();

    void locked_doMsgUpdateWork(const RsNxsTransacItem* nxsTrans, const RsGxsGroupId& grpId);

    void updateServerSyncTS();
#ifdef TO_REMOVE
    void updateClientSyncTS();
#endif

    bool locked_CanReceiveUpdate(const RsNxsSyncGrpReqItem *item);
    bool locked_CanReceiveUpdate(RsNxsSyncMsgReqItem *item, bool &grp_is_known);

    static RsGxsGroupId hashGrpId(const RsGxsGroupId& gid,const RsPeerId& pid) ;
    
private:

    typedef std::vector<RsNxsGrp*> GrpFragments;
	typedef std::vector<RsNxsMsg*> MsgFragments;

    /*!
     * Loops over pending publish key orders.
     */
    void sharePublishKeysPending() ;

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
    void collateMsgFragments(MsgFragments &fragments, std::map<RsGxsMessageId, MsgFragments>& partFragments) const;

    /*!
	 * Note that if all fragments for a group are not found then its fragments are dropped
	 * @param fragments group fragments which are not necessarily from the same group
	 * @param partFragments the partitioned fragments (into message ids)
	 */
    void collateGrpFragments(GrpFragments fragments, std::map<RsGxsGroupId, GrpFragments>& partFragments) const;

    /*!
    * stamp the group info from that particular peer at the given time.
    */

    void locked_stampPeerGroupUpdateTime(const RsPeerId& pid,const RsGxsGroupId& grpId,time_t tm,uint32_t n_messages) ;

    /*!
    * encrypts/decrypts the transaction for the destination circle id.
    */
    bool encryptSingleNxsItem(RsNxsItem *item, const RsGxsCircleId& destination_circle, const RsGxsGroupId &destination_group, RsNxsItem *& encrypted_item, uint32_t &status) ;
    bool decryptSingleNxsItem(const RsNxsEncryptedDataItem *encrypted_item, RsNxsItem *&nxsitem, std::vector<RsTlvPrivateRSAKey> *private_keys=NULL);
    bool processTransactionForDecryption(NxsTransaction *tr); // return false when the keys are not loaded => need retry later

    void cleanRejectedMessages();
    void processObserverNotifications();

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
    std::list<RsNxsSyncGrpItem*> mSyncGrp;
    std::list<RsNxsSyncMsgItem*> mSyncMsg;
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
    uint32_t mLastKeyPublishTs;
    uint32_t mLastCleanRejectedMessages;

    const uint32_t mSYNC_PERIOD;
    int mUpdateCounter ;

    RsGcxs* mCircles;
    RsGixs *mGixs;
    RsGixsReputation* mReputations;
    PgpAuxUtils *mPgpUtils;
    bool mGrpAutoSync;
    bool mAllowMsgSync;

    // need to be verfied
    std::vector<AuthorPending*> mPendingResp;
    std::vector<GrpCircleVetting*> mPendingCircleVets;
    std::map<RsGxsGroupId,std::set<RsPeerId> > mPendingPublishKeyRecipients ;
    std::map<RsPeerId, std::list<RsGxsGroupId> > mExplicitRequest;
    std::map<RsPeerId, std::set<RsGxsGroupId> > mPartialMsgUpdates ;

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

    std::map<RsGxsGroupId,RsGroupNetworkStatsRecord> mGroupNetworkStats ;

    RsGxsServerGrpUpdateItem* mGrpServerUpdateItem;
    RsServiceInfo mServiceInfo;
    
    std::map<RsGxsMessageId,time_t> mRejectedMessages;
    std::vector<RsNxsGrp*> mNewGroupsToNotify ;
    std::vector<RsNxsMsg*> mNewMessagesToNotify ;
    
    void debugDump();
};

#endif // RSGXSNETSERVICE_H
