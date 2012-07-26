#include "rsgenexchange.h"

RsGenExchange::RsGenExchange(RsGeneralDataService *gds,
                             RsNetworkExchangeService *ns, RsSerialType *serviceSerialiser, uint16_t servType)
: mGenMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser), mServType(servType)
{

    mDataAccess = new RsGxsDataAccess(gds);

}

RsGenExchange::~RsGenExchange()
{
    // need to destruct in a certain order (prob a bad thing!)
    delete mNetService;

    delete mDataAccess;
    mDataAccess = NULL;

    delete mDataStore;
    mDataStore = NULL;

}


void RsGenExchange::tick()
{
	mDataAccess->processRequests();

	publishGrps();

	publishMsgs();

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

	std::list<RsGxsGrpMetaData*>::iterator lit = metaL.begin();

	for(; lit != metaL.end(); lit++)
	{
		RsGroupMetaData m = *(*lit);
		groupInfo.push_back(m);
	}

	std::list<RsGxsGrpMetaData*>::iterator cit = metaL;
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
			RsTlvBinaryData& data = (*lit)->grp;
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
			const RsGxsGroupId& grpId = mit->first;
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

	RsStackMutex stack(mGenMtx);

	mGrpsToPublish.push_back(grpItem);

    return true;
}

bool RsGenExchange::publishMsg(RsGxsMsgItem *msgItem)
{

	RsStackMutex stack(mGenMtx);

	mMsgsToPublish.push_back(msgItem);
    return true;
}


void RsGenExchange::publishMsgs()
{
	RsStackMutex stack(mGenMtx);

	std::vector<RsGxsMsgItem*>::iterator vit = mMsgsToPublish.begin();

	for(; vit != mMsgsToPublish.end(); )
	{

		RsNxsMsg* msg = new RsNxsMsg(mServType);
		RsGxsMsgItem* msgItem = *vit;
		uint32_t size = mSerialiser->size(msgItem);
		char mData[size];
		bool ok = mSerialiser->serialise(msgItem, mData, &size);

		if(ok)
		{
			msg->metaData = new RsGxsMsgMetaData();
			ok = mDataAccess->addMsgData(msg);

			if(ok)
			{
				RsGxsMsgChange* mc = new RsGxsMsgChange();
				mNotifications.push_back(mc);
			}
		}

		if(!ok)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishMsgs() failed to publish msg " << std::endl;
#endif
			delete msg;
			continue;

		}

		delete msgItem;
	}
}

void RsGenExchange::publishGrps()
{

	RsStackMutex stack(mGenMtx);

	std::vector<RsGxsGrpItem*>::iterator vit = mGrpsToPublish.begin();

	for(; vit != mGrpsToPublish.end();)
	{

		RsNxsGrp* grp = new RsNxsGrp(mServType);
		RsGxsGrpItem* grpItem = *vit;
		uint32_t size = mSerialiser->size(grpItem);

		char gData[size];
		bool ok = mSerialiser->serialise(grpItem, gData, &size);

		if(ok)
		{
			grp->metaData = new RsGxsGrpMetaData();
			ok = mDataAccess->addGroupData(grp);
			RsGxsGroupChange* gc = RsGxsGroupChange();
			mNotifications.push_back(gc);
		}

		if(!ok)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
#endif
			delete grp;
			continue;
		}

		delete grpItem;

		vit = mGrpsToPublish.erase(vit);
	}

}
void RsGenExchange::processRecvdData() {
}


void RsGenExchange::processRecvdMessages() {
}


void RsGenExchange::processRecvdGroups() {
}

