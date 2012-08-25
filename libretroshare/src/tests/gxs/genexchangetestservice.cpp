#include "genexchangetestservice.h"

GenExchangeTestService::GenExchangeTestService(RsGeneralDataService *dataServ, RsNetworkExchangeService * netService)
    : RsGenExchange(dataServ, netService, new RsDummySerialiser(), RS_SERVICE_TYPE_DUMMY)
{

}

void GenExchangeTestService::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
    return;
}

void GenExchangeTestService::publishDummyGrp(uint32_t &token, RsDummyGrp *grp)
{
    publishGroup(token, grp);
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

bool GenExchangeTestService::getGroupDataTS(const uint32_t &token, std::vector<RsGxsGrpItem *> &grpItem)
{
    return getGroupData(token, grpItem);
}

bool GenExchangeTestService::getMsgDataTS(const uint32_t &token, GxsMsgDataMap &msgItems)
{
    return getMsgData(token, msgItems);
}

bool GenExchangeTestService::getMsgMetaTS(const uint32_t &token, GxsMsgMetaMap &msgInfo)
{
    return getMsgMeta(token, msgInfo);
}

bool GenExchangeTestService::getMsgListTS(const uint32_t &token, GxsMsgIdResult &msgIds)
{
    return getMsgList(token, msgIds);
}
