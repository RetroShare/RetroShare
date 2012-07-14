/*
 * nxstestscenario.cc
 *
 *  Created on: 10 Jul 2012
 *      Author: crispy
 */

#include "nxstestscenario.h"
#include "gxs/rsdataservice.h"
#include "data_support.h"

NxsMessageTest::NxsMessageTest()
{
	mStorePair.first = new RsDataService(".", "dStore1", 0);
	mStorePair.second = new RsDataService(".", "dStore2", 0);

	setUpDataBases();
}

std::string NxsMessageTest::getTestName()
{
	return std::string("Nxs Message Test!");
}

NxsMessageTest::~NxsMessageTest(){
	delete mStorePair.first;
	delete mStorePair.second;
}
void NxsMessageTest::setUpDataBases()
{
	// create several groups and then messages of that
	// group for both second and first of pair
	RsDataService* dStore = dynamic_cast<RsDataService*>(mStorePair.first);
	populateStore(dStore);

	dStore = dynamic_cast<RsDataService*>(mStorePair.second);
	populateStore(dStore);

	dStore = NULL;
	return;
}

void NxsMessageTest::populateStore(RsGeneralDataService* dStore)
{

	int nGrp = rand()%7;
	std::vector<std::string> grpIdList;
	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;
	RsNxsGrp* grp = NULL;
	RsGxsGrpMetaData* grpMeta =NULL;
	for(int i = 0; i < nGrp; i++)
	{
		std::pair<RsNxsGrp*, RsGxsGrpMetaData*> p;
	   grp = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
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


	std::map<RsNxsGrp*, RsGxsGrpMetaData*>::iterator grp_it
	= grps.begin();
    for(; grp_it != grps.end(); grp_it++)
	{
		delete grp_it->first;
		delete grp_it->second;
	}


	int nMsgs = rand()%23;
	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;
	RsNxsMsg* msg = NULL;
	RsGxsMsgMetaData* msgMeta = NULL;

    for(int i=0; i<nMsgs; i++)
    {
        msg = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
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

    // clean up
    std::map<RsNxsMsg*, RsGxsMsgMetaData*>::iterator msg_it
    = msgs.begin();

    for(; msg_it != msgs.end(); msg_it++)
    {
    	delete msg_it->first;
    	delete msg_it->second;
    }

    return;
}

void NxsMessageTest::notifyNewMessages(std::vector<RsNxsMsg*>& messages)
{
	std::vector<RsNxsMsg*>::iterator vit = messages.begin();

	for(; vit != messages.end(); vit++)
	{
		mPeerMsgs[(*vit)->PeerId()].push_back(*vit);
	}
}

void NxsMessageTest::notifyNewGroups(std::vector<RsNxsGrp*>& groups)
{
	std::vector<RsNxsGrp*>::iterator vit = groups.begin();

	for(; vit != groups.end(); vit++)
	{
		mPeerGrps[(*vit)->PeerId()].push_back(*vit);
	}
}

RsGeneralDataService* NxsMessageTest::dummyDataService1()
{
	return mStorePair.first;
}

RsGeneralDataService* NxsMessageTest::dummyDataService2()
{
	return mStorePair.second;
}

void NxsMessageTest::cleanUp()
{
	mStorePair.first->resetDataStore();
	mStorePair.second->resetDataStore();

	return;
}
