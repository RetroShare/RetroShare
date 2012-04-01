#ifndef RSGDP_H
#define RSGDP_H

/*
 * libretroshare/src/gxp: gxp.h
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Evi-Parker Christopher
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

#include <set>
#include <map>
#include <string>

#include "inttypes.h"
#include "rsgnp.h"

#include "gxs/rsgxs.h"


class RsGxsSignedMsg {


    RsGxsMsgId mMsgId;
    RsTlvBinaryData mMsgData;
};

/*!
 * Might be better off simply sending request codes
 *
 */
class RsGxsSearch {

    /*!
     *
     */
    virtual int code() = 0;
};

/*!
 * The main role of GDS is the preparation and handing out of messages requested from
 * RsGeneralExchangeService and RsGeneralExchangeService
 * It is important to note that no actual messages are passed by this interface as its is expected
 * architecturally to pass messages to the service via a call back
 *
 * It also acts as a layer between RsGeneralStorageService and its parent RsGeneralExchangeService
 * thus allowing for non-blocking requests, etc.
 * It also provides caching ability
 *
 *
 * Caching feature:
 *   - A cache index should be maintained which is faster than normal message request
 *   - This should allow fast retrieval of message based on grp id or msg id
 *
 * Identity Exchange Service:
 *   - As this is the point where data is accessed by both the GNP and GXS the identities
 *     used to decrypt, encrypt and verify is handle here.
 *
 * Please note all function are blocking.
 */
class RsGeneralDataService
{

public:

    /*!
     * Retrieves signed message
     * @param msgGrp this contains grp and the message to retrieve
     * @return error code
     */
    virtual int retrieveMsgs(const std::list<RsGxsMsgId>& msgId, std::set<RsGxsMsg*> msg, bool cache) = 0;

    /*!
     * Retrieves a group item by grpId
     * @param grpId the ID of the group to retrieve
     * @return error code
     */
    virtual int retrieveGrps(const std::list<RsGxsGrpId>& grpId, std::set<RsGxsGroup*> grp, bool cache) = 0;


    /*!
     * allows for more complex queries specific to the service
     * BLOBs type columns will not be searched
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    virtual int searchMsgs(RsGxsSearch* search, std::list<RsMsgId>& msgId) = 0;

    /*!
     * allows for more complex queries specific to the associated service
     * BLOBs type columns will not be searched
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    virtual int searchGrps(RsGxsSearch* search, std::list<RsGroupId>& grpId) = 0;

    /*!
     * @return the cache size set for this RsGeneralDataService
     */
    virtual uint32_t cacheSize() const;


    /*!
     * Stores a list signed messages into data store
     * @param msg list of signed messages to store
     * @return error code
     */
    virtual int storeMessage(std::set<RsGxsSignedMsg*> msg) = 0;

    /*!
     * Stores a list of groups in data store
     * @param msg list of messages
     * @return error code
     */
    virtual int storeGroup(std::set<RsGxsGroup*> grp) = 0;

    /*!
     * Retrieves group ids
     * @param grpIds
     */
    virtual void retrieveGrpIds(std::list<std::string>& grpIds) = 0;


    /*!
     * Retrieves msg ids
     */
    virtual void retrieveMsgIds(std::list<std::string>& msgIds) = 0;


protected:


    /*!
     * Retrieves signed message
     * @param msgGrp this contains grp and the message to retrieve
     * @return request code to be redeemed later
     */
    virtual int retrieveMsgs(const std::list<RsGxsMsgId>& msgId, std::set<RsGxsMsg*> msg) = 0;

    /*!
     * Retrieves a group item by grpId
     * @param grpId the ID of the group to retrieve
     * @return request code to be redeemed later
     */
    virtual int retrieveGrps(const std::list<RsGxsGrpId>& grpId, std::set<RsGxsGroup*> grp) = 0;


};




#endif // RSGDP_H
