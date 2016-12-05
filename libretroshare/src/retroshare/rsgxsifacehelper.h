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

#include "retroshare/rsgxsiface.h"
#include "rsgxsflags.h"

/*!
 * The simple idea of this class is to implement the simple interface functions
 * of gen exchange.
 * This class provides convenience implementations of:
 * - Handle msg and group changes (client class must pass changes sent by RsGenExchange to it)
 * - subscription to groups
 * - retrieval of msgs and group ids and meta info
 */
class RsGxsIfaceHelper
{
public:

    /*!
     *
     * @param gxs handle to RsGenExchange instance of service (Usually the service class itself)
     */
	RsGxsIfaceHelper(RsGxsIface* gxs)
	    : mGxs(gxs)
	{}

    ~RsGxsIfaceHelper(){}

    /*!
     * Gxs services should call this for automatic handling of
     * changes, send
     * @param changes
     */
    void receiveChanges(std::vector<RsGxsNotify *> &changes)
    {
    	mGxs->receiveChanges(changes);
    }

    /*!
     * @return handle to token service for this GXS service
     */
    RsTokenService* getTokenService()
    {
        return mGxs->getTokenService();
    }

    /* Generic Lists */

    /*!
     * Retrieve list of group ids associated to a request token
     * @param token token to be redeemed for this request
     * @param groupIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupList(const uint32_t &token,
            std::list<RsGxsGroupId> &groupIds)
	{
    	return mGxs->getGroupList(token, groupIds);
	}

    /*!
     * Retrieves list of msg ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgList(const uint32_t &token,
            GxsMsgIdResult& msgIds)
	{
    	return mGxs->getMsgList(token, msgIds);
	}

    /*!
     * Retrieves list of msg related ids associated to a request token
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
    {
        return mGxs->getMsgRelatedList(token, msgIds);
    }

    /*!
     * @param token token to be redeemed for group summary request
     * @param groupInfo the ids returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupSummary(const uint32_t &token,
            std::list<RsGroupMetaData> &groupInfo)
	{
    	return mGxs->getGroupMeta(token, groupInfo);
	}

    /*!
     * @param token token to be redeemed for message summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgSummary(const uint32_t &token,
            GxsMsgMetaMap &msgInfo)
	{
    	return mGxs->getMsgMeta(token, msgInfo);
	}

    /*!
     * @param token token to be redeemed for message related summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgRelatedSummary(const uint32_t &token, GxsMsgRelatedMetaMap &msgInfo)
    {
        return mGxs->getMsgRelatedMeta(token, msgInfo);
    }

    /*!
     * subscribes to group, and returns token which can be used
     * to be acknowledged to get group Id
     * @param token token to redeem for acknowledgement
     * @param grpId the id of the group to subscribe to
     */
    bool subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
    {
    	return mGxs->subscribeToGroup(token, grpId, subscribe);
    }

    /*!
     * This allows the client service to acknowledge that their msgs has
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
    {
		return mGxs->acknowledgeTokenMsg(token, msgId);
    }

    /*!
     * This allows the client service to acknowledge that their grps has
     * been created/modified and retrieve the create/modified grp ids
     * @param token the token related to modification/create request
     * @param msgIds vector of ids of groups created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId)
    {
            return mGxs->acknowledgeTokenGrp(token, grpId);
    }

	/*!
	 * Gets service statistic for a given services
	 * @param token value to to retrieve requested stats
	 * @param stats the status
	 * @return true if token exists false otherwise
	 */
	bool getServiceStatistic(const uint32_t& token, GxsServiceStatistic& stats)
	{
		return mGxs->getServiceStatistic(token, stats);
	}

	/*!
	 *
	 * @param token to be redeemed
	 * @param stats the stats associated to token request
	 * @return true if token is false otherwise
	 */
	bool getGroupStatistic(const uint32_t& token, GxsGroupStatistic& stats)
	{
		return mGxs->getGroupStatistic(token, stats);
	}

	/*!
	 * This determines the reputation threshold messages need to surpass in order
	 * for it to be accepted by local user from remote source
	 * NOTE: threshold only enforced if service require author signature
	 * @param token value set to be redeemed with acknowledgement
	 * @param grpId group id for cutoff value to be set
	 * @param CutOff The cut off value to set
	 */
	void setGroupReputationCutOff(uint32_t& token, const RsGxsGroupId& grpId, int CutOff)
	{
		return mGxs->setGroupReputationCutOff(token, grpId, CutOff);
	}

    /*!
     * @return storage/sync time of messages in secs
     */
    uint32_t getDefaultStoragePeriod()
    {
        return mGxs->getDefaultStoragePeriod();
    }
    uint32_t getStoragePeriod(const RsGxsGroupId& grpId)
    {
        return mGxs->getStoragePeriod(grpId);
    }
    void setStoragePeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs)
    {
        mGxs->setStoragePeriod(grpId,age_in_secs);
    }
    uint32_t getDefaultSyncPeriod()
    {
        return mGxs->getDefaultSyncPeriod();
    }
    uint32_t getSyncPeriod(const RsGxsGroupId& grpId)
    {
        return mGxs->getSyncPeriod(grpId);
    }
    void setSyncPeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs)
    {
        mGxs->setSyncPeriod(grpId,age_in_secs);
    }

private:

    RsGxsIface* mGxs;
};

#endif // RSGXSIFACEIMPL_H
