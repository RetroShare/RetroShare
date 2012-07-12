#ifndef RSGXSNETSERVICE_H
#define RSGXSNETSERVICE_H

#include <list>
#include <queue>

#include "rsnxs.h"
#include "rsgds.h"
#include "rsnxsobserver.h"
#include "pqi/p3linkmgr.h"
#include "serialiser/rsnxsitems.h"

#include "pqi/p3cfgmgr.h"


/*!
 * This represents a transaction made
 * with the NxsNetService in all states
 * of operation until completion
 *
 */
class NxsTransaction
{

public:

    static const uint8_t FLAG_STATE_STARTING; // when
    static const uint8_t FLAG_STATE_RECEIVING; // begin receiving items for incoming trans
    static const uint8_t FLAG_STATE_SENDING; // begin sending items for outgoing trans
    static const uint8_t FLAG_STATE_COMPLETED;
    static const uint8_t FLAG_STATE_FAILED;
    static const uint8_t FLAG_STATE_WAITING_CONFIRM;


    NxsTransaction();
    ~NxsTransaction();

    uint32_t mFlag; // current state of transaction
    uint32_t mTimestamp;

    /*!
     * this contains who we
     * c what peer this transaction involves.
     * c The type of transaction
     * c transaction id
     * c timeout set for this transaction
     * c and itemCount
     */
    RsNxsTransac* mTransaction;
    std::list<RsNxsItem*> mItems; // items received or sent
};


class RsNxsNetMgr
{


public:

    virtual std::string getOwnId() = 0;
    virtual void getOnlineList(std::set<std::string>& ssl_peers) = 0;

};

//class RsNxsNetMgrImpl : public RsNxsNetMgrImpl
//{

//public:

//    RsNxsNetMgrImpl(p3LinkMgr* lMgr);

//    std::string getOwnId();
//    void getOnlineList(std::set<std::string>& ssl_peers);

//};


/// keep track of transaction number
typedef std::map<uint32_t, NxsTransaction*> TransactionIdMap;

/// to keep track of peers active transactions
typedef std::map<std::string, TransactionIdMap > TransactionsPeerMap;


/*!
 * This class implements the RsNetWorkExchangeService
 * using transactions to handle synchrnisation of Nxs items between
 * peers in a network
 * Transactions requires the maintenance of several states between peers, and whether
 *
 * Thus a data structure maintains: peers, and their active transactions
 * Then for each transaction it needs to be noted if this is an outgoing or incoming transaction
 * Outgoing transaction are in 3 different states:
 *   1. START 2. INITIATED 3. SENDING 4. END
 * Incoming transaction are also in 3 different states
 *   1. START 2. RECEIVING 3. END
 */
class RsGxsNetService : public RsNetworkExchangeService, public p3ThreadedService,
    public p3Config
{
public:


    /*!
     * only one observer is allowed
     * @param servType service type
     * @param gds The data service which allows read access to a service/store
     * @param nxsObs observer will be notified whenever new messages/grps
     * arrive
     */
    RsGxsNetService(uint16_t servType, RsGeneralDataService* gds, RsNxsNetMgr* netMgr, RsNxsObserver* nxsObs = NULL);

    virtual ~RsGxsNetService();

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
    void requestGroupsOfPeer(const std::string& peerId){ return;}

    /*!
     * get messages of a peer for a given group id, this circumvents the normal
     * polling of peers for messages of given group id
     * @param peerId Id of peer
     * @param grpId id of group to request messages for
     */
    void requestMessagesOfPeer(const std::string& peerId, const std::string& grpId){ return; }

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
    int requestMsg(const std::string& msgId, uint8_t hops){ return 0;}

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param enabled set to false to disable pause, and true otherwise
     * @return request token to be redeemed
     */
    int requestGrp(const std::list<std::string>& grpId, uint8_t hops){ return 0;}

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
    uint32_t getTransactionId();

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
     * @param tr transaction responsible for generating msg request
     */
	void genReqMsgTransaction(NxsTransaction* tr);

    /*!
     * @param tr transaction responsible for generating grp request
     */
	void genReqGrpTransaction(NxsTransaction* tr);

	/*!
	 * @param tr transaction to add
	 */
	bool locked_addTransaction(NxsTransaction* tr);

	void cleanTransactionItems(NxsTransaction* tr) const;

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


    void syncWithPeers();

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
    uint32_t mTransactionTimeOut;

    std::string mOwnId;

    RsNxsNetMgr* mNetMgr;

    /// for other members save transactions
    RsMutex mNxsMutex;

    uint32_t mSyncTs;
    const uint32_t mSYNC_PERIOD;

};

#endif // RSGXSNETSERVICE_H
