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


bool RsGenExchange::getGroupList(const uint32_t &token, std::list<std::string> &groupIds)
{


    return false;
}

bool RsGenExchange::getMsgList(const uint32_t &token,
                               std::map<std::string, std::vector<std::string> > &msgIds)
{

    return false;
}

bool RsGenExchange::getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
    return false;
}

bool RsGenExchange::getMsgMeta(const uint32_t &token,
                               std::map<std::string, std::vector<RsMsgMetaData> > &msgInfo)
{

}

bool RsGenExchange::getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem *> grpItem)
{
    return false;
}

bool RsGenExchange::getMsgData(const uint32_t &token,
                               std::map<std::string, std::vector<RsGxsMsgItem *> > &msgItems)
{
    return false;
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

bool RsGenExchange::subscribeToGroup(std::string &grpId, bool subscribe)
{
    return false;
}

bool RsGenExchange::publishGroup(RsGxsGrpItem *grpItem)
{

    return false;
}

bool RsGenExchange::publishMsg(RsGxsMsgItem *msgItem)
{
    return false;
}
