/*******************************************************************************
 * libretroshare/src/gxs: rsgenexchange.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie, Evi-Parker Christopher                *
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
#ifndef RSGENEXCHANGE_H
#define RSGENEXCHANGE_H

#include <queue>
#include "util/rstime.h"

#include "rsgxs.h"
#include "rsgds.h"
#include "rsnxs.h"
#include "retroshare/rsgxsiface.h"
#include "rsgxsdataaccess.h"
#include "rsnxsobserver.h"
#include "retroshare/rsgxsservice.h"
#include "rsitems/rsnxsitems.h"
#include "gxs/rsgxsnotify.h"
#include "rsgxsutil.h"

template<class GxsItem, typename Identity = std::string>
class GxsPendingItem
{
public:
	GxsPendingItem(GxsItem item, Identity id,rstime_t ts) :
		mItem(item), mId(id), mFirstTryTS(ts)
	{}

	bool operator==(const Identity& id)
	{
		return this->mId == id;
	}

	GxsItem mItem;
	Identity mId;
	rstime_t mFirstTryTS;
};

class GxsGrpPendingSign
{
public:

	GxsGrpPendingSign(RsGxsGrpItem* item, uint32_t token): mLastAttemptTS(0), mStartTS(time(NULL)), mToken(token),
		mItem(item), mHaveKeys(false), mIsUpdate(false)
	{}

	rstime_t mLastAttemptTS, mStartTS;
	uint32_t mToken;
	RsGxsGrpItem* mItem;
	bool mHaveKeys; // mKeys->first == true if key present
	bool mIsUpdate;
	RsTlvSecurityKeySet mKeys;
};

typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> > GxsMsgDataMap;
typedef std::map<RsGxsGroupId, RsGxsGrpItem*> GxsGroupDataMap;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsGxsMsgItem*> > GxsMsgRelatedDataMap;

/*!
 * This should form the parent class to \n
 * all gxs services. This provides access to service's msg/grp data \n
 * management/publishing/sync features
 *
 * Features: \n
 *         a. Data Access: \n
 *              Provided by handle to RsTokenService. This ensure consistency \n
 *              of requests and hiearchy of groups -> then messages which are \n
 *              sectioned by group ids. \n
 *              The one caveat is that redemption of tokens are done through \n
 *              the backend of this class \n
 *         b. Publishing: \n
 *              Methods are provided to publish msg and group items and also make \n
 *              changes to meta information of both item types \n
 *         c. Sync/Notification: \n
 *              Also notifications are made here on receipt of new data from \n
 *              connected peers
 */

class RsGixs;

class RsGenExchange : public RsNxsObserver, public RsTickingThread,
        public RsGxsIface
{
public:

	/// used by class derived for RsGenExchange to indicate if service create passed or not
	enum ServiceCreate_Return { SERVICE_CREATE_SUCCESS, SERVICE_CREATE_FAIL, SERVICE_CREATE_FAIL_TRY_LATER } ;

    /*!
     * Constructs a RsGenExchange object, the owner ship of gds, ns, and serviceserialiser passes \n
     * onto the constructed object
     * @param gds Data service needed to act as store of message
     * @param ns Network service needed to synchronise data with rs peers
     * @param serviceSerialiser The users service needs this \n
     *        in order for gen exchange to deal with its data types
     * @param mServType This should be service type used by the serialiser
     * @param gixs This is used for verification of msgs and groups received by Gen Exchange using identities.
     * @param authenPolicy This determines the authentication used for verfying authorship of msgs and groups
     */
	RsGenExchange(
	        RsGeneralDataService* gds, RsNetworkExchangeService* ns,
	        RsSerialType* serviceSerialiser, uint16_t mServType, RsGixs* gixs,
	        uint32_t authenPolicy );

    virtual ~RsGenExchange();

    // Convention that this is implemented here.
    // and passes to network service.
    virtual RsServiceInfo getServiceInfo() = 0;

    void setNetworkExchangeService(RsNetworkExchangeService *ns) ;

    /** S: Observer implementation **/

    /*!
     * @param messages messages are deleted after function returns
     */
    virtual void receiveNewMessages(std::vector<RsNxsMsg*>& messages) override;

    /*!
     * @param groups groups are deleted after function returns
     */
    virtual void receiveNewGroups(std::vector<RsNxsGrp*>& groups) override;

    /*!
     * @param grpId group id
     */
    virtual void notifyReceivePublishKey(const RsGxsGroupId &grpId) override;

    virtual void notifyChangedGroupSyncParams(const RsGxsGroupId &grpId) override;
    /*!
     * \brief notifyReceiveDistantSearchResults
     * 				Should be called when new search results arrive.
     * \param grpId
     */
    virtual void receiveDistantSearchResults(TurtleRequestId id,const RsGxsGroupId &grpId) override;
    /*!
     * @param grpId group id
     */
    virtual void notifyChangedGroupStats(const RsGxsGroupId &grpId) override;

    /** E: Observer implementation **/

    /*!
     * This is called by Gxs service runner
     * periodically, use to implement non
     * blocking calls
     */
    void tick();

    /*!
     * Any backgroup processing needed by
     */
    virtual void service_tick() = 0;

    /*!
     *
     * @return handle to token service handle for making
     * request to this gxs service
     */
    RsTokenService* getTokenService();

	void threadTick() override; /// @see RsTickingThread

    /*!
     * Policy bit pattern portion
     */
    enum PrivacyBitPos { PUBLIC_GRP_BITS, RESTRICTED_GRP_BITS, PRIVATE_GRP_BITS, GRP_OPTION_BITS } ;

    /*!
     * Convenience function for setting bit patterns of the individual privacy level authentication
     * policy and group options
     * @param flag the bit pattern (and policy) set for the privacy policy
     * @param authenFlag Only the policy portion chosen will be modified with 'flag',
     * the origianl flags in the indicated bit position (pos) are over-written
     * @param pos The policy bit portion to modify
     * @see PrivacyBitPos
     */
    static bool setAuthenPolicyFlag(const uint8_t& flag, uint32_t& authenFlag, const PrivacyBitPos& pos);

public:

    /** data access functions **/

    /*!
     * Retrieve group list for a given token
     * @param token
     * @param groupIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupList(const uint32_t &token, std::list<RsGxsGroupId> &groupIds);

    /*!
     * Retrieve msg list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpId -> msgList (vector)
     * @return false if could not redeem token
     */
    bool getMsgList(const uint32_t &token, GxsMsgIdResult &msgIds);

    /*!
     * Retrieve msg list for a given token for message related info
     * @param token token to be redeemed
     * @param msgIds a map of RsGxsGrpMsgIdPair -> msgList (vector)
     * @return false if could not redeem token
     */
    bool getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult& msgIds)override;


    /*!
     * retrieve group meta data associated to a request token
     * @param token
     * @param groupInfo
     * @return false if could not redeem token
     */
    bool getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData>& groupInfo)override;

    /*!
     * retrieves message meta data associated to a request token
     * @param token token to be redeemed
     * @param msgInfo the meta data to be retrieved for token store here
     */
    bool getMsgMeta(const uint32_t &token, GxsMsgMetaMap &msgInfo)override;

    /*!
     * Retrieve msg meta for a given token for message related info
     * @param token token to be redeemed
     * @param msgIds a map of RsGxsGrpMsgIdPair -> msgList (vector)
     * @return false if could not redeem token
     */
    bool getMsgRelatedMeta(const uint32_t &token, GxsMsgRelatedMetaMap& msgMeta)override;

    /*!
     * Retrieves the meta data of a newly created group. The meta is kept in cache for the current session.
     * \param token  token that was used to create the group
     * \param meta   meta data for this group
     * \return   false if the group is not yet created.
     */
    bool getPublishedGroupMeta(const uint32_t& token,RsGroupMetaData& meta);

    /*!
     * Retrieves the meta data of a newly created post. The meta is kept in cache for the current session.
     * \param token  token that was used to create the post
     * \param meta   meta data for this post
     * \return   false if the group is not yet created.
     */
    bool getPublishedMsgMeta(const uint32_t& token,RsMsgMetaData& meta);

#ifdef TO_REMOVE
    /*!
     * Gxs services should call this for automatic handling of
     * changes, send
     * @param changes
     */
    virtual void receiveChanges(std::vector<RsGxsNotify*>& changes);
#endif

    /*!
     * \brief acceptNewGroup
     * 		Early checks if the group can be accepted. This is mainly used to check wether the group is banned for some reasons.
     * 		Returns true unless derived in GXS services.
     *
     * \param grpMeta Group metadata to check
     * \return
     */
    virtual bool acceptNewGroup(const RsGxsGrpMetaData *grpMeta) ;

	/*!
     * \brief acceptNewMessage
     * 		Early checks if the message can be accepted. This is mainly used to check wether the group is for instance overloaded and the service wants
     * 		to put limitations to it.
     * 		Returns true unless derived in GXS services.
     *
     * \param grpMeta Group metadata to check
     * \return
     */
	virtual bool acceptNewMessage(const RsGxsMsgMetaData *msgMeta, uint32_t size) ;

    bool subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe) override;

	/*!
	 * Gets service statistic for a given services
	 * @param token value to to retrieve requested stats
	 * @param stats the status
	 * @return true if token exists false otherwise
	 */
    bool getServiceStatistic(const uint32_t& token, GxsServiceStatistic& stats) override;

	/*!
	 * Get group statistic
	 * @param token to be redeemed
	 * @param stats the stats associated to token requ
	 * @return true if token is false otherwise
	 */
    bool getGroupStatistic(const uint32_t& token, GxsGroupStatistic& stats) override;

    /*!
     * \brief turtleGroupRequest
     * 			Issues a browadcast group request using the turtle router generic search system. The request is obviously asynchroneous and will be
     * 			handled in RsGenExchange when received.
     * \param group_id
     */
    void turtleGroupRequest(const RsGxsGroupId& group_id);
    void turtleSearchRequest(const std::string& match_string);

    /*!
     * \brief getDistantSearchStatus
     * 			Returns the status of ongoing search: unknown (probably not even searched), known as a search result,
     *          data request ongoing and data available
     */
    DistantSearchGroupStatus getDistantSearchStatus(const RsGxsGroupId& group_id) ;

    /**
	 * @brief Search local groups. Blocking API.
	 * @param matchString string to look for in the search
	 * @param results storage for results
	 * @return false on error, true otherwise
	 */
	bool localSearch( const std::string& matchString,
	                  std::list<RsGxsGroupSummary>& results );

	/// @see RsGxsIface
	bool exportGroupBase64(
	        std::string& radix, const RsGxsGroupId& groupId,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsGxsIface
	bool importGroupBase64(
	        const std::string& radix,
	        RsGxsGroupId& groupId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

protected:

	bool messagePublicationTest(const RsGxsMsgMetaData&) ;
    /*!
     * retrieves group data associated to a request token
     * @param token token to be redeemed for grpitem retrieval
     * @param grpItem the items to be retrieved for token are stored here
     */
    bool getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem*>& grpItem);

    /*!
     * \brief getSerializedGroupData
     * 			Retrieves the complete group data serialized into a chunk of memory. This can be useful to
     * 		  transfer a full group from one machine to another.
     *
     * \param token		token previously obtained from cache request
     * \param data		memory chunk allocated (using malloc)
     * \param size		size of the memory chunk.
     * \return
     */

	bool getSerializedGroupData(uint32_t token, RsGxsGroupId &id,
	                            unsigned char *& data, uint32_t& size);
	bool deserializeGroupData(unsigned char *data, uint32_t size,
	                          RsGxsGroupId* gId = nullptr);

    template<class GrpType>
    bool getGroupDataT(const uint32_t &token, std::vector<GrpType*>& grpItem)
    {
    	std::vector<RsGxsGrpItem*> items;
    	bool ok = getGroupData(token, items);
    	std::vector<RsGxsGrpItem*>::iterator vit = items.begin();

    	for(; vit != items.end(); ++vit)
    	{
    		RsGxsGrpItem* gi = *vit;

    		GrpType* item = dynamic_cast<GrpType*>(gi);

    		if(item)
    		{
    			grpItem.push_back(item);
    		}
    		else
    		{
#ifdef GXS_DEBUG
    			std::cerr << "\nRsGenExchange::getGroupDataT(): Wrong type!\n";
#endif
    			delete gi;
    		}
    	}

    	return ok;
    }

public:

    /*!
     * retrieves message data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
	bool getMsgData(uint32_t token, GxsMsgDataMap& msgItems);

    template <class MsgType>
	bool getMsgDataT( uint32_t token, std::map<RsGxsGroupId,
	                  std::vector<MsgType*> >& msgItems)
    {
    	GxsMsgDataMap msgData;
    	bool ok = getMsgData(token, msgData);

    	GxsMsgDataMap::iterator mit = msgData.begin();

    	for(; mit != msgData.end(); ++mit)
    	{
    		const RsGxsGroupId& grpId = mit->first;
    		std::vector<RsGxsMsgItem*>& mv = mit->second;
    		std::vector<RsGxsMsgItem*>::iterator vit = mv.begin();
    		for(; vit != mv.end(); ++vit)
    		{
    			RsGxsMsgItem* mi = *vit;
    			MsgType* mt = dynamic_cast<MsgType*>(mi);

    			if(mt != NULL)
    			{
    				msgItems[grpId].push_back(mt);
    			}
    			else
    			{
    				std::cerr << "RsGenExchange::getMsgDataT(): bad cast to msg type" << std::endl;
    				delete mi;
    			}
    		}
    	}

    	return ok;
    }

    /*!
     * retrieves message related data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
	bool getMsgRelatedData(uint32_t token, GxsMsgRelatedDataMap& msgItems);

protected:

    /*!
     * Convenience template function for retrieve
     * msg related data from
     * @param GxsMsgType This represent derived msg class type of the service (i.e. msg type that derives from RsGxsMsgItem
     * @param MsgType Represents the final type the core data is converted to
     * @param token token to be redeemed
     */
    template <class GxsMsgType, class MsgType>
    bool getMsgRelatedDataT(const uint32_t &token, std::map<RsGxsGrpMsgIdPair, std::vector<MsgType> > &msgItems)
    {

        RsStackMutex stack(mGenMtx);
        NxsMsgRelatedDataResult msgResult;
        bool ok = mDataAccess->getMsgRelatedData(token, msgResult);
        NxsMsgRelatedDataResult::iterator mit = msgResult.begin();

        if(ok)
        {
            for(; mit != msgResult.end(); ++mit)
            {
                std::vector<MsgType> gxsMsgItems;
                const RsGxsGrpMsgIdPair& msgId = mit->first;
                std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
                std::vector<RsNxsMsg*>::iterator vit
                = nxsMsgsV.begin();
                for(; vit != nxsMsgsV.end(); ++vit)
                {
                    RsNxsMsg*& msg = *vit;
                    RsItem* item = NULL;

                    if(msg->msg.bin_len != 0)
                    	item = mSerialiser->deserialise(msg->msg.bin_data,
                                    &msg->msg.bin_len);

                    GxsMsgType* mItem = NULL;

                    if(item)
                    	mItem = dynamic_cast<GxsMsgType*>(item);

                    if(mItem == NULL)
                    {
                        delete msg;
                        continue;
                    }

                    mItem->meta = *((*vit)->metaData); // get meta info from nxs msg
                  //  GxsMsgType m = (*mItem); // doesn't work! don't know why, even with overloading done.
                    MsgType theServMsg = (MsgType)*mItem;
                    gxsMsgItems.push_back(theServMsg);
                    delete msg;
                }
                msgItems[msgId] = gxsMsgItems;
            }
        }
        return ok;
    }

public:
    /*!
	 * Generate a new token, the status of the token can be queried from request
	 * status feature.
	 * @attention the token space is shared with RsGenExchange backend.
	 * @return Generated token
     */
    uint32_t generatePublicToken();

    /*!
     * Updates the status of associate token
     * @warning the token space is shared with RsGenExchange backend, so do not
     * modify tokens except does you have created by calling generatePublicToken()
     * @param token
     * @param status
     * @return false if token could not be found, true if token disposed of
     */
	bool updatePublicRequestStatus(
	        uint32_t token, RsTokenService::GxsRequestStatus status);

    /*!
     * This gets rid of a publicly issued token
     * @param token
     * @return false if token could not found, true if token is disposed of
     */
    bool disposeOfPublicToken(const uint32_t &token);

protected:
    /*!
     * This gives access to the data store which hold msgs and groups
     * for the service
     * @return Data store for retrieving msgs and groups
     */
    RsGeneralDataService* getDataStore();

    /*!
     * Retrieve keys for a given group, \n
     * call is blocking retrieval from underlying db
     * @warning under normal circumstance a service should not need this
     * @param grpId the id of the group to retrieve keys for
     * @param keys set to the retrieved keys
     * @return false if group does not exist or grpId is empty
     */
    bool getGroupKeys(const RsGxsGroupId& grpId, RsTlvSecurityKeySet& keySet);

public:

    /*!
     * This allows the client service to acknowledge that their msgs has \n
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeTokenMsg(const uint32_t& token, RsGxsGrpMsgIdPair& msgId);

    /*!
	 * This allows the client service to acknowledge that their grps has \n
	 * been created/modified and retrieve the create/modified grp ids
	 * @param token the token related to modification/create request
	 * @param grpId ids of created/modified group
	 * @return true if token exists false otherwise
	 */
    bool acknowledgeTokenGrp(const uint32_t& token, RsGxsGroupId& grpId);

protected:

    /** Modifications **/

    /*!
     * Enables publication of a group item \n
     * This will induce a related change message \n
     * Ownership of item passes to this rsgenexchange \n
     * @param token
     * @param grpItem
     */
    void publishGroup(uint32_t& token, RsGxsGrpItem* grpItem);

    /*!
     * Updates an existing group item \n
     * This will induce a related change message \n
     * Ownership of item passes to this rsgenexchange \n
     * @param token
     * @param grpItem
     */
    void updateGroup(uint32_t& token, RsGxsGrpItem* grpItem);

    /*!
     * Deletes an existing group item \n
     * This will induce a related change message \n
     * Ownership of item passes to this rsgenexchange \n
     * @param token
     * @param grpItem
     */
    void deleteGroup(uint32_t& token, const RsGxsGroupId &grpId);

public:
    /*!
     * Enables publication of a message item \n
     * Setting mOrigMsgId meta member to blank \n
     * leads to this msg being an original msg \n
     * if mOrigMsgId is not blank the msgId then this msg is \n
     * considered a versioned msg \n
     * Ownership of item passes to this rsgenexchange
     * @param token
     * @param msgItem
     */
    void publishMsg(uint32_t& token, RsGxsMsgItem* msgItem);

    /*!
     * Deletes the messages \n
     * This will induce a related change message \n
     * @param token
     * @param msgs
     */
    void deleteMsgs(uint32_t& token, const GxsMsgReq& msgs);

protected:
    /*!
     * This represents the group before its signature is calculated
     * Reimplement this function if you need to access keys to further extend
     * security of your group items using keyset properties
     * Derived service should return one of three ServiceCreate_Return enum values below
     * @warning do not modify keySet!
     * @param grp The group which is stored by GXS prior
     *            service can make specific modifications need
     *            in particular access to its keys and meta
     * @param keySet this is the key set used to define the group
     *               contains private and public admin and publish keys
     *               (use key flags to distinguish)
     * @return SERVICE_CREATE_SUCCESS, SERVICE_CREATE_FAIL, SERVICE_FAIL_TRY_LATER
     */
    virtual ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

public:

    /*!
     * sets the group subscribe flag
     * @param token this is set to token value associated to this request
     * @param grpId Id of group whose subscribe file will be changed
     * @param status
     * @param mask
     */
    void setGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    /*!
	 * sets the group subscribe flag
	 * @param token this is set to token value associated to this request
	 * @param grpId Id of group whose subscribe file will be changed
	 * @param status
	 * @param mask
	 */
    void setGroupStatusFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    /*!
	 * sets the group service string
	 * @param token this is set to token value associated to this request
	 * @param grpId Id of group whose subscribe file will be changed
	 * @param servString
	 */
    void setGroupServiceString(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString);

	/*!
	 *
	 * @param token value set to be redeemed with acknowledgement
	 * @param grpId group id for cutoff value to be set
	 * @param CutOff The cut off value to set
	 */
    void setGroupReputationCutOff(uint32_t& token, const RsGxsGroupId& grpId, int CutOff);

    /*!
     *
     * @param token value set to be redeemed with acknowledgement
     * @param grpId group id of the group to update
     * @param CutOff The cut off value to set
     */
    void updateGroupLastMsgTimeStamp(uint32_t& token, const RsGxsGroupId& grpId);

    /*!
     * sets the msg status flag
     * @param token this is set to token value associated to this request
     * @param grpId Id of group whose subscribe file will be changed
     * @param status
     * @param mask Mask to apply to status flag
     */
    void setMsgStatusFlags(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask);

    /*!
     * sets the message service string
     * @param token this is set to token value associated to this request
     * @param msgId Id of message whose service string will be changed
     * @param servString The service string to set msg to
     */
    void setMsgServiceString(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString );

    /*!
     * sets the message service string
     */

    void shareGroupPublishKey(const RsGxsGroupId& grpId,const std::set<RsPeerId>& peers) ;

    /*!
     * Returns the local TS of the group as known by the network service.
     * This is useful to allow various network services to sync their update TS
     * when needed. Typical use case is forums and circles.
     * @param gid GroupId the TS is which is requested
     */
    bool getGroupServerUpdateTS(const RsGxsGroupId& gid,rstime_t& grp_server_update_TS,rstime_t& msg_server_update_TS) ;

    /*!
     * \brief getDefaultStoragePeriod. All times in seconds.
     * \return
     */
    virtual uint32_t getDefaultStoragePeriod() override{ return mNetService->getDefaultKeepAge() ; }

    virtual uint32_t getStoragePeriod(const RsGxsGroupId& grpId) override;
    virtual void     setStoragePeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs) override;
    virtual void     setStoragePeriodAll(uint32_t age_in_secs) ;
    virtual uint32_t getDefaultSyncPeriod()override;
    virtual uint32_t getSyncPeriod(const RsGxsGroupId& grpId) override;
    virtual void     setSyncPeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs) override;
    virtual void     setSyncPeriodAll(uint32_t age_in_secs) ;
    virtual bool     getGroupNetworkStats(const RsGxsGroupId& grpId,RsGroupNetworkStats& stats);

    uint16_t serviceType() const override { return mServType ; }
    uint32_t serviceFullType() const { return RsServiceInfo::RsServiceInfoUIn16ToFullServiceId(mServType); }

	virtual RsReputationLevel minReputationForForwardingMessages(
            uint32_t group_sign_flags, uint32_t identity_flags )override;
protected:

    /** Notifications **/

    /*!
     * This confirms this class as an abstract one that \n
     * should not be instantiated \n
     * The deriving class should implement this function \n
     * as it is called by the backend GXS system to \n
     * update client of changes which should \n
     * instigate client to retrieve new content from the system
     * Note! For newly received message and groups, bit 0xf00 is set to
     * GXS_SERV::GXS_MSG_STATUS_UNPROCESSED and GXS_SERV::GXS_MSG_STATUS_UNREAD
     * @param changes the changes that have occured to data held by this service
     */
    virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) = 0;

private:

    void processRecvdData();

    void processRecvdMessages();

    void processRecvdGroups();

    void publishGrps();

    void processGroupUpdatePublish();

    void processGroupDelete();
    void processMessageDelete();
    void processRoutingClues();

    void publishMsgs();

	bool checkGroupMetaConsistency(const RsGroupMetaData& meta);

    /*!
     * processes msg local meta changes
     */
    void processMsgMetaChanges();

    /*!
     * Processes group local meta changes
     */
    void processGrpMetaChanges();

    /*!
     * Convenience function for properly applying masks for status and subscribe flag
     * of a group.
     * @warning mask entry is removed from grpCv
     */
    bool processGrpMask(const RsGxsGroupId& grpId, ContentValue& grpCv);

    /*!
     * This completes the creation of an instance on RsNxsGrp
     * by assigning it a groupId and signature via SHA1 and EVP_sign respectively \n
     * @param grp Nxs group to create
     * @return CREATE_SUCCESS for success, CREATE_FAIL for fail,
     * 		   CREATE_FAIL_TRY_LATER for Id sign key not avail (but requested)
     */
    uint8_t createGroup(RsNxsGrp* grp, RsTlvSecurityKeySet& keySet);

protected:
    /*!
     * This completes the creation of an instance on RsNxsMsg
     * by assigning it a groupId and signature via SHA1 and EVP_sign respectively
     * What signatures are calculated are based on the authentication policy
     * of the service
     * @param msg the Nxs message to create
     * @return CREATE_SUCCESS for success, CREATE_FAIL for fail,
     * 		   CREATE_FAIL_TRY_LATER for Id sign key not avail (but requested)
     */
    int createMessage(RsNxsMsg* msg);

    RsNetworkExchangeService *netService() const { return mNetService ; }

private:
    /*!
     * convenience function to create sign
     * @param signSet signatures are stored here
     * @param msgData message data to be signed
     * @param grpMeta the meta data for group the message belongs to
     * @return SIGN_SUCCESS for success, SIGN_FAIL for fail,
     * 		   SIGN_FAIL_TRY_LATER for Id sign key not avail (but requested), try later
     */
    int createMsgSignatures(RsTlvKeySignatureSet& signSet, RsTlvBinaryData& msgData,
                             const RsGxsMsgMetaData& msgMeta, const RsGxsGrpMetaData& grpMeta);

    /*!
     * convenience function to create sign for groups
     * @param signSet signatures are stored here
     * @param grpData group data to be signed
     * @param grpMeta the meta data for group to be signed
     * @return SIGN_SUCCESS for success, SIGN_FAIL for fail,
     * 		   SIGN_FAIL_TRY_LATER for Id sign key not avail (but requested), try later
     */
    int createGroupSignatures(RsTlvKeySignatureSet& signSet, RsTlvBinaryData& grpData,
    							RsGxsGrpMetaData& grpMeta);

    /*!
     * check meta change is legal
     * @return false if meta change is not legal
     */
    bool locked_validateGrpMetaChange(GrpLocMetaData&);

    /*!
     * Generate a set of keys that can define a GXS group
     * @param privatekeySet contains private generated keys
     * @param publickeySet contains public generated keys (counterpart of private)
     * @param genPublicKeys should publish key pair also be generated
     */
    void generateGroupKeys(RsTlvSecurityKeySet& keySet, bool genPublishKeys);

    /*!
     * Attempts to validate msg signatures
     * @param msg message to be validated
     * @param grpFlag the distribution flag for the group the message belongs to
     * @param grpFlag the signature flag for the group the message belongs to
     * @param grpKeySet the key set user has for the message's group
     * @return VALIDATE_SUCCESS for success, VALIDATE_FAIL for fail,
     * 		   VALIDATE_ID_SIGN_NOT_AVAIL for Id sign key not avail (but requested)
     */
    int validateMsg(RsNxsMsg* msg, const uint32_t& grpFlag, const uint32_t &signFlag, RsTlvSecurityKeySet& grpKeySet);

    /*!
	 * Attempts to validate group signatures
	 * @param grp group to be validated
	 * @return VALIDATE_SUCCESS for success, VALIDATE_FAIL for fail,
	 * 		   VALIDATE_ID_SIGN_NOT_AVAIL for Id sign key not avail (but requested)
	 */
	int validateGrp(RsNxsGrp* grp);

    /*!
     * Checks flag against a given privacy bit block
     * @param pos Determines 8 bit wide privacy block to check
     * @param flag the flag to and(&) against
     * @param the result of the (bit-block & flag)
     */
    bool checkAuthenFlag(const PrivacyBitPos& pos, const uint8_t& flag) const;

    void  groupShareKeys(std::list<std::string> peers);

    static void computeHash(const RsTlvBinaryData& data, RsFileHash& hash);

    /*!
     * Checks validation of recently received groups to be
     * updated (and updates them, a bit of a misnomer)
     */
    void performUpdateValidation();

    /*!
     * Checks if the update is valid (i.e. the new admin signature is by the old admin key)
     * @param oldGrp the old group to be updated (must have meta data member initialised)
     * @param newGrp the new group that updates the old group (must have meta data member initialised)
     * @return
     */
    bool updateValid(const RsGxsGrpMetaData& oldGrp, const RsNxsGrp& newGrp) const;

    /*!
     * convenience function for checking private publish and admin keys are present
     * @param keySet The keys set to split into a private and public set
     * @return false, if private admin and publish keys cannot be found, true otherwise
     */
    bool checkKeys(const RsTlvSecurityKeySet& keySet);

    /*!
     * Message and notification map passed to method
     * are cleansed of msgs and ids that already exist in database
     * @param msgs messages to be filtered
     * @param msgIdsNotify message notification map to be filtered
     */
    void removeDeleteExistingMessages(std::list<RsNxsMsg*>& msgs, GxsMsgReq& msgIdsNotify);

    RsMutex mGenMtx;
    RsGxsDataAccess* mDataAccess;
    RsGeneralDataService* mDataStore;
    RsNetworkExchangeService *mNetService;
    RsSerialType *mSerialiser;
    /// service type
    uint16_t mServType;
    RsGixs* mGixs;

    std::vector<RsNxsMsg*> mReceivedMsgs;

    typedef std::map<RsGxsGroupId,GxsPendingItem<RsNxsGrp*, RsGxsGroupId> > NxsGrpPendValidVect;
    NxsGrpPendValidVect mGrpPendingValidate;

    std::vector<GxsGrpPendingSign> mGrpsToPublish;
    typedef std::vector<GxsGrpPendingSign> NxsGrpSignPendVect;

    std::map<uint32_t,RsGxsGrpMetaData> mPublishedGrps ;		// keeps track of which group was created using which token
    std::map<uint32_t,RsGxsMsgMetaData> mPublishedMsgs ;		// keeps track of which message was created using which token

    std::map<uint32_t, RsGxsMsgItem*> mMsgsToPublish;

    std::map<uint32_t, RsGxsGrpMsgIdPair > mMsgNotify;
    std::map<uint32_t, RsGxsGroupId> mGrpNotify;

    // for loc meta changes
    std::map<uint32_t, GrpLocMetaData > mGrpLocMetaMap;
    std::map<uint32_t,  MsgLocMetaData> mMsgLocMetaMap;

    std::vector<RsGxsNotify*> mNotifications;



    /// authentication policy
    uint32_t mAuthenPolicy;

    std::map<uint32_t, GxsPendingItem<RsGxsMsgItem*, uint32_t> > mMsgPendingSign;

    typedef std::map<RsGxsMessageId,GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair> > NxsMsgPendingVect;
    NxsMsgPendingVect mMsgPendingValidate;

    bool mCleaning;
    rstime_t mLastClean;
    RsGxsMessageCleanUp* mMsgCleanUp;


    bool mChecking, mCheckStarted;
    rstime_t mLastCheck;
    RsGxsIntegrityCheck* mIntegrityCheck;

protected:
	enum CreateStatus { CREATE_FAIL, CREATE_SUCCESS, CREATE_FAIL_TRY_LATER };
	const uint8_t SIGN_MAX_WAITING_TIME;
	// TODO: cleanup this should be an enum!
    const uint8_t SIGN_FAIL, SIGN_SUCCESS, SIGN_FAIL_TRY_LATER;
    const uint8_t VALIDATE_FAIL, VALIDATE_SUCCESS, VALIDATE_FAIL_TRY_LATER, VALIDATE_MAX_WAITING_TIME;

private:

    std::vector<GroupUpdate> mGroupUpdates, mPeersGroupUpdate;

    std::vector<GroupUpdatePublish> mGroupUpdatePublish;
    std::vector<GroupDeletePublish> mGroupDeletePublish;
    std::vector<MsgDeletePublish>   mMsgDeletePublish;

    std::map<RsGxsId,std::set<RsPeerId> > mRoutingClues ;
};

#endif // RSGENEXCHANGE_H
