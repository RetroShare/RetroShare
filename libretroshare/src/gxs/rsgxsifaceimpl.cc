#include "rsgxsifaceimpl.h"
#include "gxs/rsgxs.h"
#include "gxs/rsgxsflags.h"

RsGxsIfaceImpl::RsGxsIfaceImpl(RsGenExchange *gxs)
    : mGxsIfaceMutex("RsGxsIfaceImpl"), mGxs(gxs)
{
}



void RsGxsIfaceImpl::groupsChanged(std::list<RsGxsGroupId> &grpIds)
{
    RsStackMutex stack(mGxsIfaceMutex);

    while(!mGroupChange.empty())
    {
            RsGxsGroupChange* gc = mGroupChange.back();
            std::list<RsGxsGroupId>& gList = gc->grpIdList;
            std::list<RsGxsGroupId>::iterator lit = gList.begin();
            for(; lit != gList.end(); lit++)
                    grpIds.push_back(*lit);

            mGroupChange.pop_back();
            delete gc;
    }
}

bool RsGxsIfaceImpl::updated()
{
    RsStackMutex stack(mGxsIfaceMutex);

    bool changed =  (!mGroupChange.empty() || !mMsgChange.empty());

    return changed;
}

void RsGxsIfaceImpl::msgsChanged(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgs)
{
    RsStackMutex stack(mGxsIfaceMutex);

    while(!mMsgChange.empty())
    {
        RsGxsMsgChange* mc = mMsgChange.back();
        msgs = mc->msgChangeMap;
        mMsgChange.pop_back();
        delete mc;
    }
}

void RsGxsIfaceImpl::receiveChanges(std::vector<RsGxsNotify *> &changes)
{

    RsStackMutex stack(mGxsIfaceMutex);

    std::vector<RsGxsNotify*>::iterator vit = changes.begin();

    for(; vit != changes.end(); vit++)
    {
        RsGxsNotify* n = *vit;
        RsGxsGroupChange* gc;
        RsGxsMsgChange* mc;
        if((mc = dynamic_cast<RsGxsMsgChange*>(n)) != NULL)
        {
                mMsgChange.push_back(mc);
        }
        else if((gc = dynamic_cast<RsGxsGroupChange*>(n)) != NULL)
        {
                mGroupChange.push_back(gc);
        }
        else
        {
                delete n;
        }
    }
}

RsTokenService* RsGxsIfaceImpl::getTokenService()
{
    return mGxs->getTokenService();
}

bool RsGxsIfaceImpl::getGroupList(const uint32_t &token,
                          std::list<RsGxsGroupId> &groupIds)
{

    return mGxs->getGroupList(token, groupIds);
}

bool RsGxsIfaceImpl::getMsgList(const uint32_t &token,
                        GxsMsgIdResult& msgIds)
{

        return mGxs->getMsgList(token, msgIds);
}

/* Generic Summary */
bool RsGxsIfaceImpl::getGroupSummary(const uint32_t &token,
                             std::list<RsGroupMetaData> &groupInfo)
{

        return mGxs->getGroupMeta(token, groupInfo);
}

bool RsGxsIfaceImpl::getMsgSummary(const uint32_t &token,
                           GxsMsgMetaMap &msgInfo)
{

        return mGxs->getMsgMeta(token, msgInfo);
}

bool RsGxsIfaceImpl::subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{
    if(subscribe)
        mGxs->setGroupSubscribeFlag(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);
    else
        mGxs->setGroupSubscribeFlag(token, grpId, ~GXS_SERV::GROUP_SUBSCRIBE_MASK);

    return true;
}


bool RsGxsIfaceImpl::acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{

        return mGxs->acknowledgeTokenMsg(token, msgId);
}

bool RsGxsIfaceImpl::acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId)
{
        return mGxs->acknowledgeTokenGrp(token, grpId);
}
