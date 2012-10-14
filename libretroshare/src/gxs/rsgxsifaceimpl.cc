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

	return false;
}

bool RsGxsIfaceImpl::getMsgList(const uint32_t &token,
                        GxsMsgIdResult& msgIds)
{

	return false;
}

/* Generic Summary */
bool RsGxsIfaceImpl::getGroupSummary(const uint32_t &token,
                             std::list<RsGroupMetaData> &groupInfo)
{

	return false;
}

bool RsGxsIfaceImpl::getMsgSummary(const uint32_t &token,
                           GxsMsgMetaMap &msgInfo)
{

	return false;
}

bool RsGxsIfaceImpl::subscribeToAlbum(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{

	return false;

}


bool RsGxsIfaceImpl::acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{

	return false;
}

bool RsGxsIfaceImpl::acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId)
{
	return false;
}
