#ifndef RSGXS_H
#define RSGXS_H

/*
 * libretroshare/src/gxs   : rsgxs.h
 *
 * GXS  interface for RetroShare.
 * Convenience header
 *
 * Copyright 2011 Christopher Evi-Parker
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
 * This is *THE* auth manager. It provides the web-of-trust via
 * gpgme, and authenticates the certificates that are managed
 * by the sublayer AuthSSL.
 *
 */


#include <inttypes.h>
#include <string>
#include <list>
#include <set>
#include <map>

#include "serialiser/rsgxsitems.h"

#define GXS_STATUS_GRP_NOT_FOUND 1                /* request resulted in grp not found error */
#define GXS_STATUS_MSG_NOT_FOUND 2                /* request resulted in msg not found */
#define GXS_STATUS_ERROR 3                        /* request is in error */
#define GXS_STATUS_OK 4                           /* request was successful */



class RsGxsMsgId {

public:

    std::string grpId;
    std::string msgId;
};


class RsGxsGrpId {

public:

    std::string grpId;
    RsTlvKeySignature adminSign;
};

typedef time_t RsGxsTime;
typedef std::map<std::string, uint32_t> IdVersionM;
typedef std::multimap<std::string, std::pair<std::string, uint32_t> > GrpMsgMap;
typedef uint64_t    RsGroupId ;


class RsGxsSearchModule  {

public:

    virtual bool searchMsg(const RsGxsSearch&, RsGxsMsg* msg) = 0;
    virtual bool searchGroup(const RsGxsSearch&, RsGxsGroup* grp) = 0;

};


/*!
 * The whole idea is to provide a broad enough base class from which
 * all the services listed in gxs/db_apps.h can be implemented
 * The exchange service features a token redemption request/receive
 * design in which services make as request to the underlying base
 * class which returns a token which the service should redeem later
 * when the request's corresponding receive is called later
 * Main functionality: \n
 *
 *  Compared to previous cache-system, some improvements are: \n
 *
 *    On-demand:  There is granularity both in time and hiearchy on whats \n
 *    locally resident, all this is controlled by service \n
 *      * hiearchy - only grps are completely sync'd, have to request msgs \n
 *      * time - grps and hence messages to sync if in time range, grps locally resident but outside range are kept \n
 *
 *    Search: \n
 *      User can provide search terms (RsGxsSearchItem) that each service can use to retrieve \n
 *      information on items and groups available in their exchange network \n
 *
 *
 *    Actual data exchanging: \n
 *       Currently naming convention is a bit confused between 'items' or 'messages' \n
 *       - consider item and msgs to be the same for now, RsGxsId, gives easy means of associate msgs to grps \n
 *       - all data is received via call back \n
 *       - therefore interface to UI should not be based on being alerted to msg availability \n
 */
class RsGxsService
{

public:

    RsGxsService(RsGxsSearchModule* mod);
    virtual ~RsGxsService();

    /***************** Group request receive API ********************/

    /*!
     * Request group, not intialising version member of RsGxsGrpId
     * results in the latest version of groups being returned
     * @return token for request
     * @see receiveGrp()
     */
    int requestGrps();

    /*!
     * Request group, not intialising version member of RsGxsGrpId
     * results in the latest version of group being returned
     * @return token for request
     * @see receiveGrpList()
     */
    int requestSubscribedGrps();

    /*!
     * Request all versions of a group
     * @param grpId group id to request version for
     * @return token for request
     */
    int requestAllGrpVersions(const std::string& grpId);

    /*!
     * Event call back from Gxs runner
     * after requested Grp(s) have been retrieved
     * @param token the token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param grps list of groups associated to request token
     */
    virtual void receiveGrp(int token, int errCode, std::set<RsGxsGroup*>& grps) = 0;


    /*************** Start: Msg request receive API ********************/

    /*  Messages a level below grps on the hiearchy hence group requests */
    /*  are intrinsically linked to their grp Id                         */

    /*!
     * request latest version of messages for groups
     * listed
     * @param grpIds list of grpIds to get msgs for
     * @return token for request
     * @see receiveMsg()
     */
    int requestGrpMsgs(const std::list<string>& grpIds);

    /*!
     * request latest version of messages for group
     * @param grpIds list of grpIds to get msgs for
     * @return token for request
     * @see receiveMsg()
     */
    int requestAllMsgVersions(const std::list<RsGxsMsgId>& msgIds);

    /*!
     *
     *
     */
    int requestMsgVersion(const RsGxsMsgId& msgId);

    /*!
     * Event call back from GxsRunner
     * after request msg(s) have been received
     * @param token token to be redeemed
     */
    virtual void receiveMsg(int token, int errCode, std::set<RsGxsMsg*>& msgs) = 0;

    /*************** End: Msg request receive API ********************/


    /*********** Start: publication *****************/

    /*!
     * Pushes a RsGxsItem for publication
     * @param msg set of messages to push onto service for publication
     */
    int pushMsg(std::set<RsGxsMsg*>& msg);

    /*!
     * Updates an already published message
     * on your network with this one
     * Message must be associated to an identity on
     * publication for this to work
     * @param msg the message to update
     * @return
     */
    int updateMsg(std::set<RsGxsMsg*>& msg);


    /*!
     * Pushes a RsGxsGroup on RsGxsNetwork and associates it with a \n
     * given RsGixs profile
     * @param grp group to push onto network

     * @return error code
     */
    int pushGrp(const RsGxsGroup& grp);

    /*!
     * Update an already published group with
     * new information
     * This increments the version number
     * @param grp grp to update
     * @return token to be redeemed
     */
    int updateGrp(const RsGxsGroup& grp);


    /*********** End: publication *****************/

    /*************************************************************************/
    // The main idea here is each RsGroup needs permission set for each RsGxsGroup
    // Thus a
    // This is a configuration thing.
    /*************************************************************************/
    /*********** Start: groups *****************/


    /*!
     * This says listed group can are only ones that can
     * share grp/msgs between themselves for grpId
     * Default behaviour on group sharing is share with between all peers
     * @param rsGrpId list of group id sharing will be allowed
     * @param grpId groupId that share behaviour is being set
     * @return status token to be redeemed
     */
    int setShareGroup(const std::list<RsGroupId>& rsGrpId, const std::string grpId);

    /*********** End: Identity control and groups *****************/


    /****************************************************************************************************/
    // Interface for Message Read Status
    // At the moment this is overloaded to handle autodownload flags too.
    // This is a configuration thing.
    /****************************************************************************************************/

    /*********** Start: Update of groups/messages locally *****************/

    /*!
     * flags latest message in store as read or not read
     * @param GrpMsgMap flags this msg,version pair as read
     * @param read set to true for read and false for unread
     */
    int flagMsgRead(const RsGxsMsgId& msgId, bool read) ;


    /*!
     * To flag group as read or not read
     * @param grpId the
     * @param
     */
    int flagGroupRead(const std::string grpId, bool read);

    /*!
     * This marks a local message created by yourself to
     * no longer distributed and removed
     * Entry in db is discarded after expiration date has passed
     * TODO: this may send a standard marker to user that message
     * should be removed
     * @param msgId
     */
    int requestDeleteMsg(const RsGxsMsg& msg);

    /*!
     * This is grpId is marked in database to be removed
     * and not circulated. Entry will later be removed
     * once discard age is reached
     */
    int requestDeleteGrp(const RsGxsGroup& grp);


    /******************* Start: Configuration *************************/

    /*!
     * How long to keep subscribed groups
     * @param length how long to keep group in seconds
     */
    int setSubscribedGroupDiscardAge(uint32_t length);

    /*!
     * How long to keep unsubscribed groups
     * @param length how long to keep groups in seconds
     * @return status token to be redeemed
     * @see statusMsg()
     */
    int setUnsubscribedGrpDiscardAge(uint32_t length);

    /*!
     * How long to keep messages, group discard age supercedes this \n
     * discard age
     * @param length how long to keep messages in seconds
     */
    int setMsgDiscardAge(uint32_t length);


    /*!
     * Use to subscribe or unsubscribe to group
     * @param grpId the id of the group to scubscribe to
     * @param subscribe set to false to unsubscribe and true otherwise
     * @return token to redeem
     */
    int requestSubscribeToGrp(const std::string& grpId, bool subscribe);

    /*!
     * This called by event runner on status of subscription
     * @param token the token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param gxsStatus the status of subscription request
     */
    virtual void receiveSubscribeToGrp(int token, int errCode) = 0;

    /*!
     *
     * @param error_code
     * @param errorMsg
     */
    virtual void statusMsg(int error_code, const std::string& errorMsg);

    /******************* End: Configuration *************************/

    /****************** Start: Notifications from event runner ***************/

    /*!
     * This is called by event runner when a group or groups \n
     * have been updated or newly arrived
     * @param grpId ids of groups that have changed or newly arrived
     */
    virtual void notifyGroupChanged(std::list<std::string> grpIds) = 0;


    /*!
     * This is called by event runner when a msg or msg \n
     * have been updated or newly arrived
     * @param msgId ids of msgs that have changed or newly arrived
     */
    virtual void notifyMsgChanged(std::list<RsGxsMsgId> msgId) = 0;


    /****************** End: Notifications from event runner ***************/


    /****************** Start: Search from event runner ***************/

    // Users can either search groups or messages

    /*!
     * Users
     * Some standard search terms are supported
     * @param term this allows users to search their local storage defined terms
     * @return token to redeem search
     */
    int requestLocalSearch(RsGxsSearch* term);

    // Remote Search...(2 hops ?)
    /*!
     *
     * @param term This contains the search term
     * @param hops how many hops from your immediate peer on the network for search
     * @param delay lenght of time search should persist (if you are online search will be saved \n
     *        delay seconds, max is 2 days
     * @return token to redeem search
     */
    int requestRemoteSearchGrps(RsGxsSearch* term, uint32_t hops, uint32_t delay, bool immediate);

    // Remote Search...(2 hops ?)
    /*!
     *
     * @param term This contains the search term
     * @param hops how many hops from your immediate peer on the network for search
     * @param delay lenght of time search should persist (if you are online search will be saved \n
     *        delay seconds, max is 2 days
     * @return token to redeem search
     */
    int requestRemoteSearchMsgs(RsGxsSearch* term, uint32_t hops, uint32_t delay, bool immediate);


    /*!
     *
     * @param token
     * @param errCode error code, can be exchanged for error string
     * @param grpIds list of grp ids
     */
    virtual void receiveLocalSearchGrps(int token, int errCode, std::list<RsGxsGroup*>& grps);

    /*!
     *
     * @param token the request token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param msgIds the message ids that contain contain search term
     */
    virtual void receiveLocalSearchMsgs(int token, int errCode, std::list<RsGxsMsg*> &msgs) = 0;

    /*!
     *
     * @param token request token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param grps
     */
    virtual void receiveRemoteSearchGrps(int token, int errCode, std::list<RsGxsGroup*>& grps) = 0;

    /*!
     *
     * @param token the request token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param msgIds
     */
    virtual void recieveRemoteSearchMsgs(int token, int errCode, std::list<RsGxsMsg*>& msgs) = 0;

    /*!
     * Note if parent group is not present this will be requested \n
     * by the event runner
     * @param msgIds the ids of the remote messages being requested
     * @return token to be redeemed
     * @see receiveMsg()
     */
    virtual int requestRemoteMsg(std::list<RsGxsMsgId>& msgIds);

    /*!
     * @param grpIds the ids of the group being requested
     * @return token to be redeemed
     * @see receiveGrp()
     */
    virtual int requestRemoteGrp(std::list<RsGxsGrpId>& grpIds);

    /****************** End: Search from event runner ***************/

    /*!
     * @param msgIds ids of messages to be deleted
     * @return token to be redeemed
     */
    virtual int deleteRemoteMsg(std::list<RsGxsMsgId>& msgIds);

    virtual int deleteRemoteGrp(std::list<RsGxsGrpId>& grpIds);
};

#endif // RSGXS_H
