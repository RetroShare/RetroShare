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
#include "rsnxsobserver.h"
#include "rsnxs.h"
#include "rsgds.h"

#define GXS_STATUS_GRP_NOT_FOUND 1                /* request resulted in grp not found error */
#define GXS_STATUS_MSG_NOT_FOUND 2                /* request resulted in msg not found */
#define GXS_STATUS_ERROR 3                        /* request is in error */
#define GXS_STATUS_OK 4                           /* request was successful */

typedef uint64_t    RsGroupId ;

class RsTokReqOptions
{
        public:
        RsTokReqOptions() { mOptions = 0; mBefore = 0; mAfter = 0; }

        uint32_t mOptions;
        time_t   mBefore;
        time_t   mAfter;
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
class RsGxsService : public RsNxsObserver
{

public:

    RsGxsService(RsGeneralDataService* gds, RsNetworkExchangeService* ns);
    virtual ~RsGxsService();

    /***************** Group request receive API ********************/

    /*!
     * It is critical this service implements
     * this function and returns a valid service id
     * @return the service type of the Gxs Service
     */
    virtual uint16_t getServiceType() const = 0;

    /*!
     * Request group information
     * @param token this initialised with a token that can be redeemed for this request
     * @param ansType type of group result wanted, summary
     * @param opts token options
     * @param groupIds list of group ids wanted, leaving this empty will result in a request for only list of group ids for the service
     * @return false if request failed, true otherwise
     * @see receiveGrp()
     */
    bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);

    /*!
     * Request message information for a list of groups
     * @param token this initialised with a token that can be redeemed for this request
     * @param ansType type of msg info result
     * @param opts token options
     * @param groupIds groups ids for which message info will be requested
     * @return false if request failed, true otherwise
     * @see receiveGrp()
     */
    virtual bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);

    /*!
     * Request message information for a list of groups
     * @param token this initialised with a token that can be redeemed for this request
     * @param ansType type of msg info result
     * @param opts token options
     * @param groupIds groups ids for which message info will be requested
     * @return false if request failed, true otherwise
     * @see receiveGrp()
     */
    bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);

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
    int flagMsgRead(const std::string& msgId, bool read) ;


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
     bool requestDeleteGrp(uint32_t& token, const RsGxsGroup& grp);


    /*!
     * Use to subscribe or unsubscribe to group
     * @param grpId the id of the group to scubscribe to
     * @param subscribe set to false to unsubscribe and true otherwise
     * @return token to redeem
     */
    bool requestSubscribeToGrp(uint32_t token, const std::string& grpId, bool subscribe);

public:

    /*!
     * This called by event runner on status of subscription
     * @param token the token to be redeemed
     * @param errCode error code, can be exchanged for error string
     * @param gxsStatus the status of subscription request
     */
    virtual void receiveSubscribeToGrp(int token, int errCode);

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
    virtual void notifyGroupChanged(std::list<std::string> grpIds);


    /*!
     * This is called by event runner when a msg or msg \n
     * have been updated or newly arrived
     * @param msgId ids of msgs that have changed or newly arrived
     */
    virtual void notifyMsgChanged(std::list<std::string> msgId);


    /****************** End: Notifications from event runner ***************/

    // Users can either search groups or messages


    /* Generic Lists */
    bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds);
    bool getMsgList(const uint32_t &token, std::list<std::string> &msgIds);



    /* Poll */
    uint32_t requestStatus(const uint32_t token);

    /* Cancel Request */
    bool cancelRequest(const uint32_t &token);

    bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);
};

#endif // RSGXS_H
