/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/genexchangetestservice.h           *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/

#ifndef GENEXCHANGETESTSERVICE_H
#define GENEXCHANGETESTSERVICE_H

#include "gxs/rsgenexchange.h"
#include "retroshare/rsgxsifacehelper.h"
#include "rsdummyservices.h"

typedef std::map<RsGxsGroupId, std::vector<RsDummyMsg*> > DummyMsgMap;

class GenExchangeTestService : public RsGenExchange
{
public:
    GenExchangeTestService(RsGeneralDataService* dataServ, RsNetworkExchangeService* nxs, RsGixs* gixs);
	virtual ~GenExchangeTestService();

    void notifyChanges(std::vector<RsGxsNotify*>& changes);

    void publishDummyGrp(uint32_t& token, RsDummyGrp* grp);
    void updateDummyGrp(uint32_t &token, RsGxsGroupUpdateMeta& meta, RsDummyGrp *group);
    void publishDummyMsg(uint32_t& token, RsDummyMsg* msg);

	RsServiceInfo getServiceInfo();

    /*!
     * Retrieve group list for a given token
     * @param token
     * @param groupIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupListTS(const uint32_t &token, std::list<RsGxsGroupId> &groupIds);

    /*!
     * Retrieve msg list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpId -> msgList (vector)
     */
    bool getMsgListTS(const uint32_t &token, GxsMsgIdResult &msgIds);


    /*!
     * retrieve group meta data associated to a request token
     * @param token
     * @param groupInfo
     */
    bool getGroupMetaTS(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);

    /*!
     * retrieves message meta data associated to a request token
     * @param token token to be redeemed
     * @param msgInfo the meta data to be retrieved for token store here
     */
    bool getMsgMetaTS(const uint32_t &token, GxsMsgMetaMap &msgInfo);

    /*!
     * retrieves group data associated to a request token
     * @param token token to be redeemed for grpitem retrieval
     * @param grpItem the items to be retrieved for token are stored here
     */
    bool getGroupDataTS(const uint32_t &token, std::vector<RsDummyGrp*>& grpItem);



    /*!
     * retrieves message data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
    bool getMsgDataTS(const uint32_t &token, DummyMsgMap& msgItems);

    /*!
     * Retrieve msg related list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpMsgIdPair -> msgList (vector)
     */
    bool getMsgRelatedListTS(const uint32_t &token, MsgRelatedIdResult &msgIds);

    /*!
     * retrieves msg related data msgItems as a map of msg-grpID pair to vector
     * of items
     * @param token token to be redeemed
     * @param msgItems map of msg items
     */
    bool getMsgRelatedDataTS(const uint32_t &token, GxsMsgRelatedDataMap& msgItems);

    /*!
     *
     *
     *
     */
    bool getGroupStatisticTS(const uint32_t &token, GxsGroupStatistic &stats);


    bool getServiceStatisticTS(const uint32_t &token, GxsServiceStatistic &stats);


    void setGroupSubscribeFlagTS(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupStatusFlagTS(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupServiceStringTS(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString);

    void setMsgStatusFlagTS(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask);

    void setMsgServiceStringTS(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString );

    void service_tick();

	RsSerialType *mSerializer ;
};

#endif // GENEXCHANGETESTSERVICE_H
