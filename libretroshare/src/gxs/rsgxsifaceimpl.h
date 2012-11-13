#ifndef RSGXSIFACEIMPL_H
#define RSGXSIFACEIMPL_H

/*
 * libretroshare/src/gxs/: rsgxsifaceimpl.h
 *
 * RetroShare GXS. Convenience interface implementation
 *
 * Copyright 2012 by Christopher Evi-Parker
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

#include "gxs/rsgenexchange.h"

/*!
 * The simple idea of this class is to implement the simple interface functions
 * of gen exchange.
 * This class provides convenience implementations of:
 * - Handle msg and group changes (client class must pass changes sent by RsGenExchange to it)
 * - subscription to groups
 * - retrieval of msgs and group ids and meta info
 */
class RsGxsIfaceImpl
{
public:

    /*!
     *
     * @param gxs handle to RsGenExchange instance of service (Usually the service class itself)
     */
    RsGxsIfaceImpl(RsGenExchange* gxs);

    /*!
     * Gxs services should call this for automatic handling of
     * changes, send
     * @param changes
     */
    void receiveChanges(std::vector<RsGxsNotify*>& changes);

    /*!
     * Checks to see if a change has been received for
     * for a message or group
     * @return true if a change has occured for msg or group
     */
    virtual bool updated();

public:

    /*!
     * The groups changed. \n
     * class can reimplement to use to tailor
     * the group actually set for ui notification.
     * If receivedChanges is not passed RsGxsNotify changes
     * this function does nothing
     * @param grpIds
     */
    virtual void groupsChanged(std::list<RsGxsGroupId>& grpIds);

    /*!
     * The msg changed. \n
     * class can reimplement to use to tailor
     * the msg actually set for ui notification.
     * If receivedChanges is not passed RsGxsNotify changes
     * this function does nothing
     * @param msgs
     */
    virtual void msgsChanged(std::map<RsGxsGroupId,
                             std::vector<RsGxsMessageId> >& msgs);

    /*!
     * @return handle to token service for this GXS service
     */
    RsTokenService* getTokenService();

    /* Generic Lists */

    /*!
     * Retrieve list of group ids associated to a request token
     * @param token token to be redeemed for this request
     * @param groupIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupList(const uint32_t &token,
                              std::list<RsGxsGroupId> &groupIds);

    /*!
     * Retrieves list of msg ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgList(const uint32_t &token,
                            GxsMsgIdResult& msgIds);

    /*!
     * Retrieves list of msg related ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgRelatedList(const uint32_t &token,
                           MsgRelatedIdResult& msgIds);

    /*!
     * @param token token to be redeemed for group summary request
     * @param groupInfo the ids returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupSummary(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo);

    /*!
     * @param token token to be redeemed for message summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgSummary(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo);

    /*!
     * @param token token to be redeemed for message related summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgrelatedSummary(const uint32_t &token,
                               GxsMsgRelatedMetaMap &msgInfo);

    /*!
     * subscribes to group, and returns token which can be used
     * to be acknowledged to get group Id
     * @param token token to redeem for acknowledgement
     * @param grpId the id of the group to subscribe to
     */
    virtual bool subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe);

    /*!
     * This allows the client service to acknowledge that their msgs has
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId);

    /*!
     * This allows the client service to acknowledge that their grps has
     * been created/modified and retrieve the create/modified grp ids
     * @param token the token related to modification/create request
     * @param msgIds vector of ids of groups created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId);


private:

    RsGenExchange* mGxs;

    std::vector<RsGxsGroupChange*> mGroupChange;
    std::vector<RsGxsMsgChange*> mMsgChange;

    RsMutex mGxsIfaceMutex;
};

#endif // RSGXSIFACEIMPL_H
