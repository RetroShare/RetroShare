/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/genexchangetestservice.cc          *
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

#include "genexchangetestservice.h"

GenExchangeTestService::GenExchangeTestService(RsGeneralDataService *dataServ, RsNetworkExchangeService * netService, RsGixs* gixs)
    : RsGenExchange(dataServ, netService, mSerializer = new RsDummySerialiser(), RS_SERVICE_TYPE_DUMMY, gixs, 0)
{

}

GenExchangeTestService::~GenExchangeTestService()
{
	delete mSerializer ;
}
RsServiceInfo GenExchangeTestService::getServiceInfo()
{
	RsServiceInfo info;
	return info;
}

void GenExchangeTestService::notifyChanges(std::vector<RsGxsNotify *> &/*changes*/)
{
    return;
}

void GenExchangeTestService::publishDummyGrp(uint32_t &token, RsDummyGrp *grp)
{
    publishGroup(token, grp);
}

void GenExchangeTestService::updateDummyGrp(uint32_t &token, RsGxsGroupUpdateMeta &/*updateMeta*/, RsDummyGrp *group)
{
    //updateGroup(token, updateMeta, group);
    updateGroup(token, group);
}

void GenExchangeTestService::publishDummyMsg(uint32_t &token, RsDummyMsg *msg)
{
    publishMsg(token, msg);
}

bool GenExchangeTestService::getGroupListTS(const uint32_t &token, std::list<RsGxsGroupId> &groupIds)
{
    return getGroupList(token, groupIds);
}

bool GenExchangeTestService::getGroupMetaTS(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
    return getGroupMeta(token, groupInfo);
}

bool GenExchangeTestService::getGroupDataTS(const uint32_t &token, std::vector<RsDummyGrp *>& grpItem)
{
    return getGroupDataT<RsDummyGrp>(token, grpItem);
}

bool GenExchangeTestService::getMsgDataTS(const uint32_t &token, DummyMsgMap &msgItems)
{
    return getMsgDataT<RsDummyMsg>(token, msgItems);
}

bool GenExchangeTestService::getMsgRelatedDataTS(const uint32_t &token, GxsMsgRelatedDataMap &msgItems)
{
    return getMsgRelatedData(token, msgItems);
}

bool GenExchangeTestService::getGroupStatisticTS(const uint32_t &token, GxsGroupStatistic &stats)
{
    return getGroupStatistic(token, stats);
}

bool GenExchangeTestService::getServiceStatisticTS(const uint32_t &token, GxsServiceStatistic &stats)
{
    return getServiceStatistic(token, stats);
}

bool GenExchangeTestService::getMsgMetaTS(const uint32_t &token, GxsMsgMetaMap &msgInfo)
{
    return getMsgMeta(token, msgInfo);
}

bool GenExchangeTestService::getMsgListTS(const uint32_t &token, GxsMsgIdResult &msgIds)
{
    return getMsgList(token, msgIds);
}

bool GenExchangeTestService::getMsgRelatedListTS(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
    return getMsgRelatedList(token, msgIds);
}

void GenExchangeTestService::setGroupServiceStringTS(uint32_t &token, const RsGxsGroupId &grpId, const std::string &servString)
{
        RsGenExchange::setGroupServiceString(token, grpId, servString);
}

void GenExchangeTestService::setGroupStatusFlagTS(uint32_t &token, const RsGxsGroupId &grpId, const uint32_t &status, const uint32_t& mask)
{
    RsGenExchange::setGroupStatusFlags(token, grpId, status, mask);
}

void GenExchangeTestService::setGroupSubscribeFlagTS(uint32_t &token, const RsGxsGroupId &grpId, const uint32_t &status, const uint32_t& mask)
{
    RsGenExchange::setGroupSubscribeFlags(token, grpId, status, mask);
}

void GenExchangeTestService::setMsgServiceStringTS(uint32_t &token, const RsGxsGrpMsgIdPair &msgId, const std::string &servString)
{
    RsGenExchange::setMsgServiceString(token, msgId, servString);
}

void GenExchangeTestService::setMsgStatusFlagTS(uint32_t &token, const RsGxsGrpMsgIdPair &msgId, const uint32_t &status, const uint32_t& mask)
{
    RsGenExchange::setMsgStatusFlags(token, msgId, status, mask);
}

void GenExchangeTestService::service_tick()
{

}
