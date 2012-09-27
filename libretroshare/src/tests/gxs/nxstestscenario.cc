/*
 * nxstestscenario.cc
 *
 *  Created on: 10 Jul 2012
 *      Author: crispy
 */

#include "nxstestscenario.h"
#include "gxs/rsdataservice.h"
#include "data_support.h"
#include <stdio.h>

NxsMessageTest::NxsMessageTest(uint16_t servtype)
: mServType(servtype), mMsgTestMtx("mMsgTestMtx")
{

}

std::string NxsMessageTest::getTestName()
{
	return std::string("Nxs Message Test!");
}

NxsMessageTest::~NxsMessageTest(){

    std::map<std::string, RsGeneralDataService*>::iterator mit = mPeerStoreMap.begin();

    for(; mit != mPeerStoreMap.end(); mit++)
    {
        delete mit->second;
    }

    std::set<std::string>::iterator sit = mStoreNames.begin();

    // remove db file
    for(; sit != mStoreNames.end(); sit++)
    {
        const std::string& name = *sit;
        remove(name.c_str());
    }
}
RsGeneralDataService* NxsMessageTest::getDataService(const std::string& peer)
{
    if(mPeerStoreMap.find(peer) != mPeerStoreMap.end()) return NULL;

    RsDataService* dStore = new RsDataService("./", peer, mServType);
    mStoreNames.insert(peer);
    mPeerStoreMap.insert(std::make_pair(peer, dStore));
    populateStore(dStore);

    return dStore;
}

uint16_t NxsMessageTest::getServiceType()
{
	return mServType;
}

void NxsMessageTest::populateStore(RsGeneralDataService* dStore)
{

        int nGrp = (rand()%2)+1;
	std::vector<std::string> grpIdList;
	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;
	RsNxsGrp* grp = NULL;
	RsGxsGrpMetaData* grpMeta =NULL;
	for(int i = 0; i < nGrp; i++)
	{
		std::pair<RsNxsGrp*, RsGxsGrpMetaData*> p;
	   grp = new RsNxsGrp(mServType);
	   grpMeta = new RsGxsGrpMetaData();
	   p.first = grp;
	   p.second = grpMeta;
	   init_item(*grp);
	   init_item(grpMeta);
	   grpMeta->mGroupId = grp->grpId;
	   grps.insert(p);
	   grpIdList.push_back(grp->grpId);
	   grpMeta = NULL;
	   grp = NULL;
	}

	dStore->storeGroup(grps);

	int nMsgs = rand()%23;
	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;
	RsNxsMsg* msg = NULL;
	RsGxsMsgMetaData* msgMeta = NULL;

    for(int i=0; i<nMsgs; i++)
    {
        msg = new RsNxsMsg(mServType);
        msgMeta = new RsGxsMsgMetaData();
        init_item(*msg);
        init_item(msgMeta);
        std::pair<RsNxsMsg*, RsGxsMsgMetaData*> p(msg, msgMeta);

        // pick a grp at random to associate the msg to
        const std::string& grpId = grpIdList[rand()%nGrp];
        msgMeta->mMsgId = msg->msgId;
        msgMeta->mGroupId = msg->grpId = grpId;

        msg = NULL;
        msgMeta = NULL;

        msgs.insert(p);
    }


    dStore->storeMessage(msgs);

    return;
}

void NxsMessageTest::cleanUp()
{

    std::map<std::string, RsGeneralDataService*>::iterator mit = mPeerStoreMap.begin();

    for(; mit != mPeerStoreMap.end(); mit++)
    {
        RsGeneralDataService* d = mit->second;
        d->resetDataStore();
    }

    return;
}

bool NxsMessageTest::testPassed(){
    return false;
}

/*******************************/

NxsMessageTestObserver::NxsMessageTestObserver(RsGeneralDataService *dStore)
    :mStore(dStore)
{

}

void NxsMessageTestObserver::notifyNewGroups(std::vector<RsNxsGrp *> &groups)
{
    std::vector<RsNxsGrp*>::iterator vit = groups.begin();
    std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;

    for(; vit != groups.end(); vit++)
    {
        RsNxsGrp* grp = *vit;
        RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
        meta->mGroupId = grp->grpId;
        grps.insert(std::make_pair(grp, meta));
    }

    mStore->storeGroup(grps);
}

void NxsMessageTestObserver::notifyNewMessages(std::vector<RsNxsMsg *> &messages)
{

	std::vector<RsNxsMsg*>::iterator vit = messages.begin();
	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;

	for(; vit != messages.end(); vit++)
	{
		RsNxsMsg* msg = *vit;
		RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();
		meta->mGroupId = msg->grpId;
		meta->mMsgId = msg->msgId;
		msgs.insert(std::make_pair(msg, meta));
	}

	mStore->storeMessage(msgs);
}

