
/* Generalised Service Base Class.
 * The base class interfaces with gixp, disk, net services.
 */

#include <inttypes.h>
#include <string>
#include <list>
#include <set>
#include <map>

typedef time_t RsGxsTime;

class RsGxsLink
{
	uint32_t type;
	std::string msgId;
};

class RsGxsGroupItem {

    RsGxsId Id;

};


class RsGxsId {

    std::string msgId;
    std::string grpId;

};

class RsGxsSearchResult {

};

class RsGxsItem
{
        RsGxsId mId;
        std::string mMsgId;
        std::string mOrigMsgId;

        RsGxsTime mTime;
	std::list<std::string> hashtags;
        std::list<RsGxsLink> linked;

	gxpsignature sign;
};

/*!
 * This provides a base class which
 * can be intepreted by the service
 * into concrete search terms
 */
class RsGxsSearchItem {
};


/*!
 * The whole idea is to provide a broad enough base class from which
 * all the services listed in gxs/db_apps.h
 *
 * Main functionality: \n
 *
 *  Compared to previous cache-system, some improvements are: \n
 *
 *    On-demand:  There is granularity both in time and hiearchy on whats \n
 *    locally resident, all this is controlled by service \n
 *      hiearchy - only grps are completely sync'd, have to request msgs \n
 *      time - grps and hence messages to sync if in time range, grp locally resident but outside range are kept \n
 *
 *    Search: \n
 *      User can provide search terms (RsGxsSearchItem) that each service can use to retrieve \n
 *      information on items and groups available in their exchange network \n
 *
 *
 *    Actual data exchanging: \n
 *       Currently naming convention is a bit confused between 'items' or 'messages' \n
 *       - consider item and msgs to be the same for now RsGxsId, gives easy means of associate msgs to grps \n
 *       - all data is received via call back \n
 *       - therefore interface to UI should not be based on being alerted to msg availability \n
 */
class RsGxsService
{

public:


    /*!
     * For requesting Msgs
     *
     */
    void requestGrp(std::string& grpId);


    /*!
     * Event call back for GXS runner
     * this is called alerting the user that messages have been received
     * @param msgsReceived set of messages received
     */
    void received(std::list<RsGxsId>& itemsReceived);

    /*!
     *
     * @param searchToken token to be redeemed
     * @param result result set
     */
    void receivedSearchResult(int searchToken, std::set<RsGxsSearchResult*> results);

    /*!
     * Pushes a RsGxsItem for publication
     * @param msg set of messages to push onto service for publication
     */
    void pushMsg(std::set<RsGxsItem*>& msg);


    /*!
     * Pushes a RsGxsGroupItem
     * @param msg set of messages to push onto service for publication
     */
    void pushGrp(std::set<RsGxsGroupItem*>& grp);

    /*!
     * Event call back from GxsRunner
     * after request msg(s) have been received
     */
    void receiveItem(std::set<RsGxsItem*>& items);

    /*!
     * Event call back from Gxs runner
     * after requested Grp(s) have been retrieved
     */
    void receiveGrp(std::set<RsGxsGroupItem*>& grps);

    /****************************************************************************************************/
    // Event Callback for the service.
    /****************************************************************************************************/

    notify_groupChanged();  // Mainly for GUI display.
    notify_newMessage(); // used for newsFeeds.
    notify_duplicateMessage(); // Channels needs this for Downloading stuff, can probably be moved above.
    locked_checkDistribMsg(); // required overload?


    /****************************************************************************************************/
    // Must worry about sharing keys.
    // Can gixp handle it?
    // What interfaces do we need here?
    // RsGDS deals with most things internally (decrypting, verifying), can put identity creation here to ease
    /****************************************************************************************************/

    /*!
     * can choose the share created identity immediately or use identity service later to share it
     * @param type
     * @param peers peers to share this identity with
     * @param pseudonym if type is pseudonym then uses this as the name of the pseudonym Identity
     */
    void createIdentity(int type, std::set<std::string> peers, const std::string pseudonymName);


    /****************************************************************************************************/
    // Interface for Message Read Status
    // At the moment this is overloaded to handle autodownload flags too.
    // This is a configuration thing.
    /****************************************************************************************************/

    int flagItemRead(const RsGxsId& id);
    int flagItemUnread(const RsGxsId& id);
    int flagGroupRead(const RsGxsId& id);
    int flagGroupUnread(const RsGxsId& id);

    /****************************************************************************************************/
    // External Interface for Data.
    /****************************************************************************************************/

    /*!
     * completes the creating of a group by associating it to an identity and creating its signature
     * grp can then be pushed onto exchange
     * @param grp the created grp
     * @param IdentityId identity to associate created grp. Associates to an anonymous id which is created on the fly (or default anonymous id?)
     */
    int createGroup(RsGxsGroupItem* grp, std::string& IdentityId);

    /*!
     * completes the creation of a message by associating to an identity and creating its signature
     * item can then be pushed onto exhange
     * @param item the created item, ensure the grp id is set
     * @return 0 if creationg failed
     */
    int createItem(RsGxsItem* item);

    // Group Lists & Message Lists.

    /*!
     * A group is marked as changed when it receives a new message,
     * or its description has been modified
     * @param groupIds set with ids of groups that have changed
     */
    int getGroupsChanged(std::set<std::string> &groupIds);

    /*!
     * @param groupIds all groups locally resident on this exchange
     */
    int getGroups(std::set<std::string> &groupIds);

    /*!
     * NB: looks useful, but not sure of usecase?
     * @param from
     * @param to
     * @param grpIds
     */
    int getGrpsForTimeRange(RsGxsTime from, RsGxsTime to, std::set<std::string> grpIds);

    /*!
     * Use this to get list of replacement Ids
     * @param origIds id to check for replacements
     * @param replaceIds list of ids that replace this id
     */
    int getReplacementMsgs(const RsGxsId& origId, std::set<RsGxsId> replaceIds);


    /*!
     * Sets the oldest group one can pull off the exhange service
     * and by definition the oldest message
     * Does not affect locally resident data
     * @param range sets the oldest allowable groupId to pull from exchange service
     */
    void setGroupTimeRange(uint64_t cutoff);

    // Getting the Actual Data, data returned via 'received' call back
    void getMsg(const std::set<RsGxsId>& itemIds);
    void getGroup(const std::set<std::string>& grpIds);
    void getMsgsOfGroup(const std::string& grpId);


    // Interface with GIXP Stuff... everything else should be behind the scenes.

    /*!
     * @param retrieve the profile of a given Identity id
     * @return identity profile
     */
    RsGixpProfile *getProfile(const RsGixsId&);


    // Immediate Search...
    int localSearch(const RsGxsSearchItem&);

    // Remote Search...(2 hops ?)
    int remoteSearch(const RsGxsSearchItem&);

};




