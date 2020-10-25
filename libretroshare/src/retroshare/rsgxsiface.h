/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsiface.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Christopher Evi-Parker                                  *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#include "retroshare/rsreputations.h"
#include "retroshare/rsgxsservice.h"
#include "gxs/rsgxsdata.h"
#include "retroshare/rsgxsifacetypes.h"
#include "util/rsdeprecate.h"
#include "serialiser/rsserializable.h"
#include "rsitems/rsserviceids.h"
#include "retroshare/rsevents.h"

/*!
 * This structure is used to transport group summary information when a GXS
 * service is searched. It contains the group information as well as a context
 * string to tell where the information was found. It is more compact than a
 * GroupMeta object, so as to make search responses as light as possible.
 */
struct RsGxsGroupSummary : RsSerializable
{
	RsGxsGroupSummary() :
	    mPublishTs(0), mNumberOfMessages(0),mLastMessageTs(0),
	    mSignFlags(0),mPopularity(0) {}

	RsGxsGroupId mGroupId;
	std::string  mGroupName;
	RsGxsId      mAuthorId;
	rstime_t       mPublishTs;
	uint32_t     mNumberOfMessages;
	rstime_t       mLastMessageTs;
	uint32_t     mSignFlags;
	uint32_t     mPopularity;

	std::string  mSearchContext;

	/// @see RsSerializable::serial_process
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mGroupId);
		RS_SERIAL_PROCESS(mGroupName);
		RS_SERIAL_PROCESS(mAuthorId);
		RS_SERIAL_PROCESS(mPublishTs);
		RS_SERIAL_PROCESS(mNumberOfMessages);
		RS_SERIAL_PROCESS(mLastMessageTs);
		RS_SERIAL_PROCESS(mSignFlags);
		RS_SERIAL_PROCESS(mPopularity);
		RS_SERIAL_PROCESS(mSearchContext);
	}

	~RsGxsGroupSummary();
};

/*!
 * This structure is used to locally store group search results for a given service.
 * It contains the group information as well as a context
 * strings to tell where the information was found. It is more compact than a
 * GroupMeta object, so as to make search responses as light as possible.
 */
struct RsGxsGroupSearchResults : RsSerializable
{
	RsGxsGroupSearchResults()
        : mPublishTs(0), mNumberOfMessages(0),mLastMessageTs(0), mSignFlags(0),mPopularity(0)
    {}

	RsGxsGroupId mGroupId;
	std::string  mGroupName;
	RsGxsId      mAuthorId;
	rstime_t     mPublishTs;
	uint32_t     mNumberOfMessages;
	rstime_t     mLastMessageTs;
	uint32_t     mSignFlags;
	uint32_t     mPopularity;

    std::set<std::string> mSearchContexts;

	/// @see RsSerializable::serial_process
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mGroupId);
		RS_SERIAL_PROCESS(mGroupName);
		RS_SERIAL_PROCESS(mAuthorId);
		RS_SERIAL_PROCESS(mPublishTs);
		RS_SERIAL_PROCESS(mNumberOfMessages);
		RS_SERIAL_PROCESS(mLastMessageTs);
		RS_SERIAL_PROCESS(mSignFlags);
		RS_SERIAL_PROCESS(mPopularity);
		RS_SERIAL_PROCESS(mSearchContexts);
	}

	virtual ~RsGxsGroupSearchResults() = default;
};

/*!
 * Stores ids of changed gxs groups and messages.
 * It is used to notify about GXS changes.
 */
struct RsGxsChanges : RsEvent
{
	RsGxsChanges();

	/// Type of the service
	RsServiceType mServiceType;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgs;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgsMeta;
	std::list<RsGxsGroupId> mGrps;
	std::list<RsGxsGroupId> mGrpsMeta;
	std::list<TurtleRequestId> mDistantSearchReqs;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j,ctx);
		RS_SERIAL_PROCESS(mServiceType);
		RS_SERIAL_PROCESS(mMsgs);
		RS_SERIAL_PROCESS(mMsgsMeta);
		RS_SERIAL_PROCESS(mGrps);
		RS_SERIAL_PROCESS(mGrpsMeta);
		RS_SERIAL_PROCESS(mDistantSearchReqs);
	}

	RsTokenService* mService; /// Weak pointer, not serialized
};

enum class DistantSearchGroupStatus:uint8_t
{
    UNKNOWN            = 0x00,	// no search ongoing for this group
    CAN_BE_REQUESTED   = 0x01,	// a search result mentions this group, so the group data can be requested
    ONGOING_REQUEST    = 0x02,	// the group data has been requested and the request is pending
    HAVE_GROUP_DATA    = 0x03,	// group data has been received. Group can be subscribed.
};

/*!
 * All implementations must offer thread safety
 */
struct RsGxsIface
{
    /*!
     * \brief serviceType
     * \return  The 16-bits service type. See @serialiser/rsserviceids.h
     */
    virtual uint16_t serviceType() const =0;

#ifdef TO_REMOVE
    /*!
     * Gxs services should call this for automatic handling of
     * changes, send
     * @param changes
     */
    virtual void receiveChanges(std::vector<RsGxsNotify*>& changes) = 0;
#endif

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

	/*!
	 * Gets service statistic for a given services
	 * @param token value to to retrieve requested stats
	 * @param stats the status
	 * @return true if token exists false otherwise
	 */
	virtual bool getServiceStatistic(const uint32_t& token, GxsServiceStatistic& stats) = 0;

	/*!
	 *
	 * @param token to be redeemed
	 * @param stats the stats associated to token request
	 * @return true if token is false otherwise
	 */
	virtual bool getGroupStatistic(const uint32_t& token, GxsGroupStatistic& stats) = 0;

	/*!
	 *
	 * @param token value set to be redeemed with acknowledgement
	 * @param grpId group id for cutoff value to be set
	 * @param CutOff The cut off value to set
	 */
	virtual void setGroupReputationCutOff(uint32_t& token, const RsGxsGroupId& grpId, int CutOff) = 0;

    /*!
     * @return storage/sync time of messages in secs
     */
    virtual uint32_t getDefaultStoragePeriod() = 0;
    virtual uint32_t getStoragePeriod(const RsGxsGroupId& grpId) = 0;
    virtual void     setStoragePeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs) = 0;

    virtual uint32_t getDefaultSyncPeriod() = 0;
    virtual uint32_t getSyncPeriod(const RsGxsGroupId& grpId) = 0;
    virtual void     setSyncPeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs) = 0;

	virtual RsReputationLevel minReputationForForwardingMessages(
	        uint32_t group_sign_flags,uint32_t identity_flags ) = 0;

	/**
	 * @brief Export group public data in base64 format
	 * @jsonapi{development}
	 * @param[out] radix storage for the generated base64 data
	 * @param[in] groupId Id of the group of which to output the data
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if something failed, true otherwhise
	 */
	virtual bool exportGroupBase64(
	        std::string& radix, const RsGxsGroupId& groupId,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

	/**
	 * @brief Import group public data from base64 string
	 * @param[in] radix group invite in radix format
	 * @param[out] groupId optional storage for imported group id
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool importGroupBase64(
	        const std::string& radix,
	        RsGxsGroupId& groupId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

	virtual ~RsGxsIface();
};
