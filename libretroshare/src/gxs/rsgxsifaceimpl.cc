#include "rsgxsifaceimpl.h"
#include "gxs/rsgxs.h"

RsGxsIfaceImpl::RsGxsIfaceImpl(RsGenExchange *gxs)
    : mGxs(gxs)
{
}



void RsGxsIfaceImpl::groupsChanged(std::list<RsGxsGroupId>& grpIds)
{

}


void RsGxsIfaceImpl::msgsChanged(std::map<RsGxsGroupId,
                         std::vector<RsGxsMessageId> >& msgs)
{

}

RsTokenService* RsGxsIfaceImpl::getTokenService()
{
    return mGxs->getTokenService();
}

bool RsGxsIfaceImpl::getGroupList(const uint32_t &token,
                          std::list<RsGxsGroupId> &groupIds)
{

}

bool RsGxsIfaceImpl::getMsgList(const uint32_t &token,
                        GxsMsgIdResult& msgIds)
{

}

/* Generic Summary */
bool RsGxsIfaceImpl::getGroupSummary(const uint32_t &token,
                             std::list<RsGroupMetaData> &groupInfo)
{

}

bool RsGxsIfaceImpl::getMsgSummary(const uint32_t &token,
                           GxsMsgMetaMap &msgInfo)
{

}

bool RsGxsIfaceImpl::acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{

}

bool RsGxsIfaceImpl::acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId)
{

}
