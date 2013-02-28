
/*
 * libretroshare/src/gxs/: rsgxsifaceimpl.h
 *
 * RetroShare GXS. RsGxsIface
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

#ifndef RSGXSIFACE_H_
#define RSGXSIFACE_H_

#include "retroshare/rsgxsservice.h"
#include "gxs/rsgxsdata.h"
#include "retroshare/rsgxsifacetypes.h"

/*!
 * All implementations must offer thread safety
 */
class RsGxsIface
{
public:

	virtual ~RsGxsIface(){};

public:

    /*!
     * Gxs services should call this for automatic handling of
     * changes, send
     * @param changes
     */
    virtual void receiveChanges(std::vector<RsGxsNotify*>& changes) = 0;

    /*!
     * Checks to see if a change has been received for
     * for a message or group
     * @param willCallGrpChanged if this is set to true, group changed function will return list
     *        groups that have changed, if false, the group changed list is cleared
     * @param willCallMsgChanged if this is set to true, msgChanged function will return map
     *        messages that have changed, if false, the message changed map is cleared
     * @return true if a change has occured for msg or group
     * @see groupsChanged
     * @see msgsChanged
     */
    virtual bool updated(bool willCallGrpChanged = false, bool willCallMsgChanged = false) = 0;

    /*!
     * The groups changed. \n
     * class can reimplement to use to tailor
     * the group actually set for ui notification.
     * If receivedChanges is not passed RsGxsNotify changes
     * this function does nothing
     * @param grpIds returns list of grpIds that have changed
     * @see updated
     */
    virtual void groupsChanged(std::list<RsGxsGroupId>& grpIds) = 0;

    /*!
     * The msg changed. \n
     * class can reimplement to use to tailor
     * the msg actually set for ui notification.
     * If receivedChanges is not passed RsGxsNotify changes
     * this function does nothing
     * @param msgs returns map of message ids that have changed
     * @see updated
     */
    virtual void msgsChanged(std::map<RsGxsGroupId,
                             std::vector<RsGxsMessageId> >& msgs) = 0;

    /*!
     * @return handle to token service for this GXS service
     */
    virtual RsTokenService* getTokenService() = 0;

    /* Generic Lists */

    /*!
     * Retrieve list of group ids associated to a request token
     * @param token token to be redeemed for this request
     * @param groupIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getGroupList(const uint32_t &token,
                              std::list<RsGxsGroupId> &groupIds) = 0;

    /*!
     * Retrieves list of msg ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getMsgList(const uint32_t &token,
                            GxsMsgIdResult& msgIds) = 0;

    /*!
     * Retrieves list of msg related ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getMsgRelatedList(const uint32_t &token,
                           MsgRelatedIdResult& msgIds) = 0;

    /*!
     * @param token token to be redeemed for group summary request
     * @param groupInfo the ids returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getGroupMeta(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo) = 0;

    /*!
     * @param token token to be redeemed for message summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getMsgMeta(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo) = 0;

    /*!
     * @param token token to be redeemed for message related summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    virtual bool getMsgRelatedMeta(const uint32_t &token,
                               GxsMsgRelatedMetaMap &msgInfo) = 0;

    /*!
     * subscribes to group, and returns token which can be used
     * to be acknowledged to get group Id
     * @param token token to redeem for acknowledgement
     * @param grpId the id of the group to subscribe to
     */
    virtual bool subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe) = 0;

    /*!
     * This allows the client service to acknowledge that their msgs has
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    virtual bool acknowledgeTokenMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId) = 0;

    /*!
     * This allows the client service to acknowledge that their grps has
     * been created/modified and retrieve the create/modified grp ids
     * @param token the token related to modification/create request
     * @param msgIds vector of ids of groups created/modified
     * @return true if token exists false otherwise
     */
    virtual bool acknowledgeTokenGrp(const uint32_t& token, RsGxsGroupId& grpId) = 0;

};



#endif /* RSGXSIFACE_H_ */
