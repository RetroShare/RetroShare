
/*
 * libretroshare/src/gxs/rsgxsifaceimpl.cc: rsgxsifaceimpl.cc
 *
 * RetroShare GXS.
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

bool RsGxsIfaceImpl::getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
    return mGxs->getMsgRelatedList(token, msgIds);
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

bool RsGxsIfaceImpl::getMsgrelatedSummary(const uint32_t &token, GxsMsgRelatedMetaMap &msgInfo)
{
    return mGxs->getMsgRelatedMeta(token, msgInfo);
}



bool RsGxsIfaceImpl::subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{
    if(subscribe)
        mGxs->setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED,  GXS_SERV::GROUP_SUBSCRIBE_MASK);
    else
        mGxs->setGroupSubscribeFlags(token, grpId, 0, GXS_SERV::GROUP_SUBSCRIBE_MASK);

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
