/*******************************************************************************
 * libretroshare/src/gxs: rsgds.h                                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie, Evi-Parker Christopher                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSGDS_H
#define RSGDS_H

#include <set>
#include <map>
#include <string>

#include "inttypes.h"

#include "rsitems/rsgxsitems.h"
#include "rsitems/rsnxsitems.h"
#include "gxs/rsgxsdata.h"
#include "rsgxs.h"
#include "rsgxsutil.h"
#include "util/contentvalue.h"

class RsGxsSearchModule  {

public:

	virtual ~RsGxsSearchModule();


};

/*!
 * This allows modification of local
 * meta data items of a message
 */
struct MsgLocMetaData {
	MsgLocMetaData() = default;
	MsgLocMetaData(const MsgLocMetaData& meta): msgId(meta.msgId), val(meta.val) {}

	RsGxsGrpMsgIdPair msgId;
	ContentValue val;
};

typedef std::map<RsGxsGroupId,RsGxsGrpMetaData*> RsGxsGrpMetaTemporaryMap;

/*!
 * This allows modification of local
 * meta data items of a group
 */
struct GrpLocMetaData {
	GrpLocMetaData() = default;
	GrpLocMetaData(const GrpLocMetaData& meta): grpId(meta.grpId), val(meta.val) {}

	RsGxsGroupId grpId;
	ContentValue val;
};

/*!
 * This is used to query network statistics for a given group. This is useful
 * to e.g. show group popularity, or number of visible messages for unsubscribed
 * group.
 */
struct RsGroupNetworkStats
{
	RsGroupNetworkStats() :
	    mSuppliers(0), mMaxVisibleCount(0), mGrpAutoSync(false),
	    mAllowMsgSync(false), mLastGroupModificationTS(0) {}

	uint32_t mSuppliers;
	uint32_t mMaxVisibleCount;
	bool     mGrpAutoSync;
	bool     mAllowMsgSync;
	rstime_t   mLastGroupModificationTS;
};

typedef std::map<RsGxsGroupId,      std::vector<RsNxsMsg*> > NxsMsgDataResult;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsNxsMsg*> > NxsMsgRelatedDataResult;
typedef std::map<RsGxsGroupId,      std::vector<RsNxsMsg*> > GxsMsgResult; // <grpId, msgs>

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

	static const uint32_t GXS_MAX_ITEM_SIZE;

    static const std::string MSG_META_SERV_STRING;
    static const std::string MSG_META_STATUS;

    static const std::string GRP_META_SUBSCRIBE_FLAG;
    static const std::string GRP_META_STATUS;
    static const std::string GRP_META_SERV_STRING;
    static const std::string GRP_META_CUTOFF_LEVEL;

public:

    typedef std::map<RsNxsGrp*, RsGxsGrpMetaData*> GrpStoreMap;
    typedef std::map<RsNxsMsg*, RsGxsMsgMetaData*> MsgStoreMap;

    RsGeneralDataService(){}
	virtual ~RsGeneralDataService(){}

    /*!
     * Retrieves all msgs
     * @param reqIds requested msg ids (grpId,msgId), leave msg list empty to get all msgs for the grp
     * @param msg result of msg retrieval
     * @param cache whether to store results of this retrieval in memory for faster later retrieval
	 * @param strictFilter if true do not request any message if reqIds is empty
     * @return error code
	 */
	virtual int retrieveNxsMsgs(
	        const GxsMsgReq& reqIds, GxsMsgResult& msg, bool cache,
	        bool withMeta = false ) = 0;

    /*!
     * Retrieves all groups stored. Caller owns the memory and is supposed to delete the RsNxsGrp pointers after use.
     * @param grp retrieved groups
     * @param withMeta if true the meta handle of nxs grps is intitialised
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    virtual int retrieveNxsGrps(std::map<RsGxsGroupId, RsNxsGrp*>& grp, bool withMeta, bool cache) = 0;

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     *
     * @param grp if null grpIds entries are made, only meta for those grpId are retrieved \n
     *            , if grpId is failed to be retrieved it will be erased from map
     * @return error code
     */
    virtual int retrieveGxsGrpMetaData(RsGxsGrpMetaTemporaryMap& grp) = 0;

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     * @param grpIds grpIds for which to retrieve meta data
     * @param msgMeta meta data result as map of grpIds to array of metadata for that grpId
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    virtual int retrieveGxsMsgMetaData(const GxsMsgReq& msgIds, GxsMsgMetaResult& msgMeta) = 0;

    /*!
     * remove msgs in data store listed in msgIds param
     * @param msgIds ids of messages to be removed
     * @return error code
     */
    virtual int removeMsgs(const GxsMsgReq& msgIds) = 0;

    /*!
     * remove groups in data store listed in grpIds param
     * @param grpIds ids of groups to be removed
     * @return error code
     */
    virtual int removeGroups(const std::vector<RsGxsGroupId>& grpIds) = 0;

    /*!
     * Retrieves all group ids in store
     * @param grpIds all grpids in store is inserted into this vector
     * @return error code
     */
    virtual int retrieveGroupIds(std::vector<RsGxsGroupId>& grpIds) = 0;

    /*!
     * Retrives all msg ids in store
     * @param grpId groupId of message ids to retrieve
     * @param msgId msgsids retrieved
     * @return error code
     */
    virtual int retrieveMsgIds(const RsGxsGroupId& grpId, RsGxsMessageId::std_set& msgId) = 0;

    /*!
     * @return the cache size set for this RsGeneralDataService in bytes
     */
    virtual uint32_t cacheSize() const = 0;

    /*!
     * @param size size of cache to set in bytes
     */
    virtual int setCacheSize(uint32_t size) = 0;

    /*!
     * Stores a list of signed messages into data store
     * @param msg map of message and decoded meta data information
     * @return error code
     */
    virtual int storeMessage(const std::list<RsNxsMsg*>& msgs) = 0;

    /*!
     * Stores a list of groups in data store
     * @param grp map of group and decoded meta data
     * @return error code
     */
    virtual int storeGroup(const std::list<RsNxsGrp*>& grsp) = 0;


    /*!
	 * Updates group entries in Db
	 * @param grp map of group and decoded meta data
	 * @return error code
	 */
    virtual int updateGroup(const std::list<RsNxsGrp*>& grsp) = 0;

    /*!
     * @param metaData
     */
    virtual int updateMessageMetaData(MsgLocMetaData& metaData) = 0;

    /*!
     * @param metaData
     */
    virtual int updateGroupMetaData(GrpLocMetaData& meta) = 0;

    virtual int updateGroupKeys(const RsGxsGroupId& grpId,const RsTlvSecurityKeySet& keys,uint32_t subscribed_flags) = 0 ;

    /*!
     * Completely clear out data stored in
     * and returns this to a state
     * as it was when first constructed
     */
    virtual int resetDataStore() = 0;

    /*!
     * Use to determine if message isn't over the storage
     * limit for a single message item
     * @param msg the message to check size validity
     * @return whether the size of of msg is valid
     */
    virtual bool validSize(RsNxsMsg* msg) const = 0 ;

    /*!
     * Use to determine if group isn't over the storage limit
     * for a single group item
     * @param grp the group to check size validity
     * @return whether the size of grp is valid for storage
     */
    virtual bool validSize(RsNxsGrp* grp) const = 0 ;

};




#endif // RSGDS_H
