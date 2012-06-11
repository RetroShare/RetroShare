#ifndef RSGDS_H
#define RSGDS_H

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

#include "serialiser/rsgxsitems.h"
#include "serialiser/rsnxsitems.h"


class RsGxsSearchModule  {

public:

	virtual ~RsGxsSearchModule();

    virtual bool searchMsg(const RsGxsSearch&, RsGxsMsg* msg) = 0;
    virtual bool searchGroup(const RsGxsSearch&, RsGxsGroup* grp) = 0;

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

    virtual ~RsGeneralDataService(){return;};
    /*!
     * Retrieves signed message
     * @param grpId
     * @param msg result of msg retrieval
     * @param cache whether to store retrieval in memory for faster later retrieval
     * @return error code
     */
    virtual int retrieveMsgs(const std::string& grpId, std::map<std::string, RsNxsMsg*>& msg, bool cache) = 0;

    /*!
     * Retrieves a group item by grpId
     * @param grpId the Id of the groups to retrieve
     * @param grp results of retrieval
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    virtual int retrieveGrps(std::map<std::string, RsNxsGrp*>& grp, bool cache) = 0;

    /*!
     * @param grpId the id of the group to get versions for
     * @param cache whether to store the result in memory
     * @param errCode
     */
    virtual int retrieveGrpVersions(const std::string& grpId, std::set<RsNxsGrp*>& grp, bool cache) = 0;

    /*!
     * @param grpId the id of the group to retrieve
     * @return NULL if group does not exist or pointer to grp if found
     */
    virtual RsNxsGrp* retrieveGrpVersion(const RsGxsGrpId& grpId) = 0;

    /*!
     * allows for more complex queries specific to the service
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    virtual int searchMsgs(RsGxsSearch* search, std::list<RsGxsSrchResMsgCtx*>& result) = 0;

    /*!
     * allows for more complex queries specific to the associated service
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    virtual int searchGrps(RsGxsSearch* search, std::list<RsGxsSrchResGrpCtx*>& result) = 0;

    /*!
     * remove msgs in data store listed in msgIds param
     * @param msgIds ids of messages to be removed
     * @return error code
     */
    virtual int removeMsgs(const std::list<RsGxsMsgId>& msgIds) = 0;

    /*!
     * remove groups in data store listed in grpIds param
     * @param grpIds ids of groups to be removed
     * @return error code
     */
    virtual int removeGroups(const std::list<RsGxsGrpId>& grpIds) = 0;

    /*!
     * @return the cache size set for this RsGeneralDataService in bytes
     */
    virtual uint32_t cacheSize() const = 0;

    /*!
     * @param size size of cache to set in bytes
     */
    virtual int setCacheSize(uint32_t size) = 0;

    /*!
     * Stores a list signed messages into data store
     * @param msg list of signed messages to store
     * @return error code
     */
    virtual int storeMessage(std::set<RsNxsMsg*>& msg) = 0;

    /*!
     * Stores a list of groups in data store
     * @param msg list of messages
     * @return error code
     */
    virtual int storeGroup(std::set<RsNxsGrp*>& grp) = 0;


    /*!
     * Completely clear out data stored in
     * and returns this to a state
     * as it was when first constructed
     */
    virtual int resetDataStore() = 0;

};




#endif // RSGDS_H
