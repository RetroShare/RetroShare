#ifndef RSGENEXCHANGE_H
#define RSGENEXCHANGE_H

#include <queue>

#include "rsgxs.h"
#include "rsgds.h"
#include "rsnxs.h"
#include "rsgxsdataaccess.h"
#include "retroshare/rsgxsservice.h"
#include "serialiser/rsnxsitems.h"



/*!
 * This should form the parent class to \n
 * all gxs services. This provides access to service's msg/grp data \n
 * management/publishing/sync features
 *
 * Features: \n
 *         a. Data Access:
 *              Provided by handle to RsTokenService. This ensure consistency
 *              of requests and hiearchy of groups -> then messages which are
 *              sectioned by group ids.
 *              The one caveat is that redemption of tokens are done through
 *              the backend of this class
 *         b. Publishing:
 *              Methods are provided to publish msg and group items and also make
 *              changes to meta information of both item types
 *         c. Sync/Notification:
 *              Also notifications are made here on receipt of new data from
 *              connected peers
 */
class RsGenExchange : public RsGxsService
{
public:

    /*!
     * Constructs a RsGenExchange object, the owner ship of gds, ns, and serviceserialiser passes
     * onto the constructed object
     * @param gds Data service needed to act as store of message
     * @param ns Network service needed to synchronise data with rs peers
     * @param serviceSerialiser The users service needs this \n
     *        in order for gen exchange to deal with its data types
     */
    RsGenExchange(RsGeneralDataService* gds, RsNetworkExchangeService* ns, RsSerialType* serviceSerialiser);

    virtual ~RsGenExchange();


    /** S: Observer implementation **/

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewMessages(std::vector<RsNxsMsg*> messages);

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewGroups(std::vector<RsNxsGrp*>& groups);

    /** E: Observer implementation **/

    /*!
     * This is called by Gxs service runner
     * periodically, use to implement non
     * blocking calls
     */
    void tick();

    /*!
     *
     * @return handle to token service handle for making
     * request to this gxs service
     */
    RsTokenService* getTokenService();

public:

    /** data access functions **/

    /*!
     * Retrieve group list for a given token
     * @param token
     * @param groupIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds);

    /*!
     * Retrieve msg list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpId -> msgList (vector)
     */
    bool getMsgList(const uint32_t &token, std::map<std::string, std::vector<std::string> > &msgIds);


    /*!
     * retrieve group meta data associated to a request token
     * @param token
     * @param groupInfo
     */
    bool getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);

    /*!
     * retrieves message meta data associated to a request token
     * @param token token to be redeemed
     * @param msgInfo the meta data to be retrieved for token store here
     */
    bool getMsgMeta(const uint32_t &token, std::map<std::string, std::vector<RsMsgMetaData> > &msgInfo);

    /*!
     * retrieves group data associated to a request token
     * @param token token to be redeemed for grpitem retrieval
     * @param grpItem the items to be retrieved for token are stored here
     */
    bool getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem*> grpItem);

    /*!
     * retrieves message data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
    bool getMsgData(const uint32_t &token, std::map<std::string, std::vector<RsGxsMsgItem*> >& msgItems);

public:

    /** Modifications **/

    /*!
     * Enables publication of a group item
     * If the item exists already this is simply versioned
     * This will induce a related change message
     * @param grpItem
     * @param
     */
    bool publishGroup(RsGxsGrpItem* grpItem);

    /*!
     * Enables publication of a message item
     * If the item exists already this is simply versioned
     * This will induce a related a change message
     * @param msgItem
     * @return false if msg creation failed.
     */
    bool publishMsg(RsGxsMsgItem* msgItem);


    /*!
     *
     * @param grpId
     * @param subscribe
     * @return false subscription fails
     */
    bool subscribeToGroup(std::string& grpId, bool subscribe);

protected:

    /** Notifications **/

    /*!
     * This confirm this class as an abstract one that \n
     * should not be instantiated \n
     * The deriving class should implement this function \n
     * as its is called by the backend GXS system to \n
     * update client of changes which should \n
     * instigate client to retrieve new content from system
     * @param changes the changes that have occured to data held by this service
     */
    virtual void notifyChanges(std::vector<RsGxsChange*>& changes) = 0;

private:

    RsMutex mReqMtx;
    std::map<uint32_t, gxsRequest> mRequests;
    RsGxsDataAccess* mDataAccess;
    RsGeneralDataService* mDataStore;
    RsNetworkExchangeService *mNetService;
    RsSerialType *mSerialiser;

    std::vector<RsNxsMsg*> mReceivedMsgs;
    std::vector<RsNxsGrp*> mReceivedGrps;


private:

    std::vector<RsGxsChange*> mChanges;
};

#endif // RSGENEXCHANGE_H
