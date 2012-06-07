#ifndef RSGXSNETSERVICE_H
#define RSGXSNETSERVICE_H

#include <list>
#include <queue>

#include "rsnxs.h"
#include "rsgds.h"
#include "rsnxsobserver.h"

#include "serialiser/rsnxsitems.h"

#include "pqi/p3cfgmgr.h"


/*!
 * This represents a transaction made
 * with the NxsNetService in all states
 * of operation untill completion
 */
class NxsTransaction
{

public:

    static const uint8_t FLAG_STATE_RECEIVING;
    static const uint8_t FLAG_STATE_DONE;
    static const uint8_t FLGA_STATE_COMPLETED;

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

/// keep track of transaction number
typedef std::map<uint32_t, NxsTransaction*> TransactionIdMap;

/// to keep track of peers active transactions
typedef std::map<std::string, TransactionIdMap > TransactionsPeerMap;


/*!
 *
 *
 */
class RsGxsNetService : public RsNetworkExchangeService, public RsThread,
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
    RsGxsNetService(uint16_t servType, RsGeneralDataService* gds, RsNxsObserver* nxsObs = NULL);

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
    void requestGroupsOfPeer(const std::string& peerId);

    /*!
     * get messages of a peer for a given group id, this circumvents the normal
     * polling of peers for messages of given group id
     * @param peerId Id of peer
     * @param grpId id of group to request messages for
     */
    void requestMessagesOfPeer(const std::string& peerId, const RsGroupId& grpId);

    /*!
     * subscribes the associated service to this group. This RsNetworktExchangeService
     * now regularly polls all peers for new messages of this group
     * @param grpId the id of the group to subscribe to
     * @param subscribe set to true to subscribe or false to unsubscribe
     */
    void subscribeToGroup(const RsGroupId& grpId, bool subscribe);

    /*!
     * Initiates a search through the network
     * This returns messages which contains the search terms set in RsGxsSearch
     * @param search contains search terms of requested from service
     * @param hops how far into friend tree for search
     * @return search token that can be redeemed later, implementation should indicate how this should be used
     */
    int searchMsgs(RsGxsSearch* search, uint8_t hops = 1, bool retrieve = 0);

    /*!
     * Initiates a search of groups through the network which goes
     * a given number of hosp deep into your friend's network
     * @param search contains search term requested from service
     * @param hops number of hops deep into peer network
     * @return search token that can be redeemed later
     */
    int searchGrps(RsGxsSearch* search, uint8_t hops = 1, bool retrieve = 0);


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
    int requestMsg(const std::list<RsGxsMsgId>& msgId, uint8_t hops);

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param enabled set to false to disable pause, and true otherwise
     * @return request token to be redeemed
     */
    int requestGrp(const std::list<RsGxsGrpId>& grpId, uint8_t hops);



public:

    /*!
     * initiates synchronisation
     */
    void tick();

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

    /*!
     * Processes active tansaction map
     */
    void processTransactions();

    /*!
     * Process completed transaction map
     */
    void processCompleteTransactions();

    /*!
     * Processes synchronisation requests
     */
    void processSyncRequests();

    /*!
     * This adds a transaction to
     * @param
     */
    void locked_addTransaction(NxsTransaction* trans);

    /*!
     * This adds a transaction
     * completeted transaction list
     * If this is an outgoing transaction, transaction id is
     * decrement
     * @param trans transaction to add
     */
    void locked_completeTransaction(NxsTransaction* trans);

    uint32_t getTransactionId();

    bool attemptRecoverIds();

    /** item handlers **/

    void handleTransactionContent(RsNxsItem*);

    void handleRecvSyncGroup(RsSyncGrp*);

    void handleRecvSyncMessage(RsNxsItem*);

    void handleRecvTransaction(RsNxsItem*);

    /** item handlers **/


private:

    /*** transactions ***/

    /// active transactions
    TransactionsPeerMap mInTransactions;

    /// completed transactions
    std::list<NxsTransaction*> mComplTransactions;

    /// transaction id
    uint32_t mTransactionN;

    /*** transactions ***/

    /*** synchronisation ***/
    std::list<RsSyncGrp*> mSyncGrp;
    std::list<RsSyncGrpMsg*> mSyncMsg;
    /*** synchronisation ***/

    RsNxsObserver* mObserver;
    RsGeneralDataService* mDataStore;
    uint16_t mServType;


    /// for transaction members
    RsMutex mTransMutex;
    /// for other members save transactions
    RsMutex mNxsMutex;

};

#endif // RSGXSNETSERVICE_H
