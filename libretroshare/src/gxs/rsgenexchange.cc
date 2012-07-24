#include "rsgenexchange.h"

RsGenExchange::RsGenExchange(RsGeneralDataService *gds,
                             RsNetworkExchangeService *ns, RsSerialType *serviceSerialiser)
: mReqMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser)
{

    mDataAccess = new RsGxsDataAccess(gds);

}

RsGenExchange::~RsGenExchange()
{

    // need to destruct in a certain order
    delete mNetService;

    delete mDataAccess;
    mDataAccess = NULL;

    delete mDataStore;
    mDataStore = NULL;

}


void RsGenExchange::tick()
{
	mDataAccess->processRequests();
}


bool RsGenExchange::getGroupList(const uint32_t &token, std::list<RsGxsGroupId> &groupIds)
{
	return mDataAccess->getGroupList(token, groupIds);
}

bool RsGenExchange::getMsgList(const uint32_t &token,
                               GxsMsgIdResult &msgIds)
{
	return mDataAccess->getMsgList(token, msgIds);
}

bool RsGenExchange::getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	std::list<RsGxsGrpMetaData*> metaL;
	bool ok = mDataAccess->getGroupSummary(token, metaL);
	groupInfo = metaL;

	std::list<RsGroupMetaData*>::iterator cit = metaL;
	for(; cit != metaL.end(); cit++)
		delete *cit;

	return ok;
}


bool RsGenExchange::getMsgMeta(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo)
{
	std::list<RsGxsMsgMetaData*> metaL;
	GxsMsgMetaResult result;
	bool ok = mDataAccess->getMsgSummary(token, result);

	GxsMsgMetaResult::iterator mit = result.begin();

	for(; mit != result.end(); mit++)
	{
		std::vector<RsGxsMsgMetaData*>& metaV = mit->second;
		msgInfo[mit->first] = metaV;

		std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();

		for(; vit != metaV.end(); vit++)
		{
			delete *vit;
		}
	}

	return ok;
}

bool RsGenExchange::getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem *> grpItem)
{

	std::list<RsNxsGrp*> nxsGrps;
	bool ok = mDataAccess->getGroupData(token, nxsGrps);

	std::list<RsNxsGrp*>::iterator lit = nxsGrps.begin();

	if(ok)
	{
		for(; lit != nxsGrps.end(); lit++)
		{
			RsTlvBinaryData& data = *lit->grp;
			RsItem* item = mSerialiser->deserialise(data.bin_data, &data.bin_len);
			RsGxsGrpItem* gItem = dynamic_cast<RsGxsGrpItem*>(item);
			grpItem.push_back(gItem);
			delete *lit;
		}
	}
    return ok;
}

bool RsGenExchange::getMsgData(const uint32_t &token,
                               GxsMsgDataMap &msgItems)
{
	NxsMsgDataResult msgResult;
	bool ok = mDataAccess->getMsgData(token, msgResult);
	NxsMsgDataResult::iterator mit = msgResult.begin();

	if(ok)
	{
		for(; mit != msgResult.end(); mit++)
		{
			std::vector<RsGxsMsgItem*> gxsMsgItems;
			RsGxsGroupId& grpId = mit->first;
			std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
			std::vector<RsNxsMsg*>::iterator vit
			= nxsMsgsV.begin();
			for(; vit != nxsMsgsV.end(); vit++)
			{
				RsNxsMsg*& msg = *vit;

				RsItem* item = mSerialiser->deserialise(msg->msg.bin_data,
						&msg->msg.bin_len);
				RsGxsMsgItem* mItem = dynamic_cast<RsGxsMsgItem*>(item);
				gxsMsgItems.push_back(mItem);
				delete msg;
			}
			msgItems[grpId] = gxsMsgItems;
		}
	}
    return ok;
}

void RsGenExchange::operator =(std::list<RsGroupMetaData>& lMeta, std::list<RsGxsGrpMetaData*>& rGxsMeta)
{
	std::list<RsGxsGrpMetaData*>::const_iterator cit = rGxsMeta.begin();

	for(; cit != rGxsMeta.end(); cit++)
	{
		const RsGxsGrpMetaData*& gxm = *cit;
		RsGroupMetaData gm;
		gm.mAuthorId = gxm->mAuthorId;
		gm.mGroupFlags = gxm->mGroupFlags;
		gm.mGroupId = gxm->mGroupId;
		gm.mGroupStatus = gxm->mGroupStatus;
		gm.mLastPost = gxm->mLastPost;
		gm.mMsgCount = gxm->mMsgCount;
		gm.mPop = gxm->mPop;
		gm.mPublishTs = gxm->mPublishTs;
		gm.mSubscribeFlags = gxm->mSubscribeFlags;
		gm.mGroupName = gxm->mGroupName;
		lMeta.push_back(gm);
	}
}

void RsGenExchange::operator =(std::vector<RsMsgMetaData>& lMeta, std::vector<RsGxsMsgMetaData*>& rGxsMeta)
{
	std::vector<RsMsgMetaData*>::const_iterator vit = rGxsMeta.begin();

	for(; vit != rGxsMeta.end(); vit++)
	{
		const RsGxsMsgMetaData*& mxm = *vit;
		RsMsgMetaData mm;
		mm.mAuthorId = mxm->mAuthorId;
		mm.mChildTs = mxm->mChildTs;
		mm.mGroupId = mxm->mGroupId;
		mm.mMsgFlags = mxm->mMsgFlags;
		mm.mMsgId = mxm->mMsgId;
		mm.mMsgName = mxm->mMsgName;
		mm.mMsgStatus = mxm->mMsgStatus;
		mm.mOrigMsgId = mxm->mOrigMsgId;
		mm.mParentId = mxm->mParentId;
		mm.mPublishTs = mxm->mPublishTs;
		mm.mThreadId = mxm->mThreadId;
		lMeta.push_back(mm);
	}
}

RsTokenService* RsGenExchange::getTokenService()
{
    return mDataAccess;
}


void RsGenExchange::notifyNewGroups(std::vector<RsNxsGrp *> &groups)
{
    std::vector<RsNxsGrp*>::iterator vit = groups.begin();

    // store these for tick() to pick them up
    for(; vit != groups.end(); vit++)
       mReceivedGrps.push_back(*vit);

}

void RsGenExchange::notifyNewMessages(std::vector<RsNxsMsg *> messages)
{
    std::vector<RsNxsMsg*>::iterator vit = messages.begin();

    // store these for tick() to pick them up
    for(; vit != messages.end(); vit++)
        mReceivedMsgs.push_back(*vit);

}


bool RsGenExchange::publishGroup(RsGxsGrpItem *grpItem)
{


    return false;
}

bool RsGenExchange::publishMsg(RsGxsMsgItem *msgItem)
{
    return false;
}
