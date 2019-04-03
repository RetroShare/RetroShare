/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxstesthub.cc                          *
 *                                                                             *
 * Copyright (C) 2014, Crispy <retroshare.team@gmailcom>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include "nxstesthub.h"

#include <unistd.h>

class NotifyWithPeerId : public RsNxsObserver
{
public:

	NotifyWithPeerId(RsPeerId val, rs_nxs_test::NxsTestHub& hub)
	: mPeerId(val), mTestHub(hub){

	}
	virtual ~NotifyWithPeerId(){}

    void notifyNewMessages(std::vector<RsNxsMsg*>& messages)
    {
    	mTestHub.notifyNewMessages(mPeerId, messages);
    }

    void notifyNewGroups(std::vector<RsNxsGrp*>& groups)
    {
    	mTestHub.notifyNewGroups(mPeerId, groups);
    }

    void notifyReceivePublishKey(const RsGxsGroupId& )
    {

    }

    void notifyChangedGroupStats(const RsGxsGroupId&)
    {

    }

private:

	RsPeerId mPeerId;
	rs_nxs_test::NxsTestHub& mTestHub;
};

using namespace rs_nxs_test;

class NxsTestHubConnection : public p3ServiceServerIface
{

public:
	NxsTestHubConnection(const RsPeerId& id, RecvPeerItemIface* recvIface) : mPeerId(id), mRecvIface(recvIface) {}

	bool	recvItem(RsRawItem * i)
	{
		return mRecvIface->recvItem(i, mPeerId);
	}
	bool	sendItem(RsRawItem * i)
	{
		return recvItem(i);
	}
	bool getServiceItemNames(uint32_t /*service_type*/, std::map<uint8_t,std::string>& /*names*/) { return false; }
private:
	RsPeerId mPeerId;
	RecvPeerItemIface* mRecvIface;

};

rs_nxs_test::NxsTestHub::NxsTestHub(NxsTestScenario *testScenario)
 : mTestScenario(testScenario), mMtx("NxsTestHub Mutex")
{
	std::list<RsPeerId> peers;
	mTestScenario->getPeers(peers);

	// for each peer get initialise a nxs net instance
	// and pass this to the simulator

	std::list<RsPeerId>::const_iterator cit = peers.begin();

	for(; cit != peers.end(); cit++)
	{
		NotifyWithPeerId *noti =  new NotifyWithPeerId(*cit, *this) ;

		mNotifys.push_back(noti) ;

		RsGxsNetService *ns =  new RsGxsNetService(
									mTestScenario->getServiceType(),
									mTestScenario->getDataService(*cit),
									mTestScenario->getDummyNetManager(*cit),
									noti,
									mTestScenario->getServiceInfo(),
									mTestScenario->getDummyReputations(*cit),
									mTestScenario->getDummyCircles(*cit),
									NULL,
									mTestScenario->getDummyPgpUtils(),
									true
									);

		NxsTestHubConnection *connection = new NxsTestHubConnection(*cit, this);
		ns->setServiceServer(connection);

		mConnections.push_back(connection) ;

		mPeerNxsMap.insert(std::make_pair(*cit, ns));
	}
}


rs_nxs_test::NxsTestHub::~NxsTestHub()
{
	for(PeerNxsMap::const_iterator it(mPeerNxsMap.begin());it!=mPeerNxsMap.end();++it)
		delete it->second ;

	for(std::list<NotifyWithPeerId*>::const_iterator it(mNotifys.begin());it!=mNotifys.end();++it)
		delete *it ;

	for(std::list<NxsTestHubConnection*>::const_iterator it(mConnections.begin());it!=mConnections.end();++it)
		delete *it ;
}


bool rs_nxs_test::NxsTestHub::testsPassed()
{
	return mTestScenario->checkTestPassed();
}

void rs_nxs_test::NxsTestHub::StartTest()
{
	// get all services up and running
	PeerNxsMap::iterator mit = mPeerNxsMap.begin();
	for(; mit != mPeerNxsMap.end(); mit++)
	{
        (mit->second)->start() ;
	}

    start() ;
}


void rs_nxs_test::NxsTestHub::EndTest()
{
	// then stop this thread
    join();

	// stop services
	PeerNxsMap::iterator mit = mPeerNxsMap.begin();
	for(; mit != mPeerNxsMap.end(); mit++)
	{
		mit->second->join();
	}
}

void rs_nxs_test::NxsTestHub::notifyNewMessages(const RsPeerId& pid,
		std::vector<RsNxsMsg*>& messages)
{
    RS_STACK_MUTEX(mMtx); /***** MTX LOCKED *****/

	RsNxsMsgDataTemporaryList toStore;
	std::vector<RsNxsMsg*>::iterator it = messages.begin();
	for(; it != messages.end(); it++)
	{
		RsNxsMsg* msg = *it;
		RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();
        // local meta is not touched by the deserialisation routine
        // have to initialise it

		msg->metaData = meta ;

        meta->mMsgStatus = 0;
        meta->mMsgSize = 0;
        meta->mChildTs = 0;
        meta->recvTS = 0;
        meta->validated = false;
		meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len));

		toStore.push_back(msg);
	}

	RsGeneralDataService* ds = mTestScenario->getDataService(pid);
	ds->storeMessage(toStore);
}


void rs_nxs_test::NxsTestHub::notifyNewGroups(const RsPeerId& pid, std::vector<RsNxsGrp*>& groups)
{
    RS_STACK_MUTEX(mMtx); /***** MTX LOCKED *****/

	RsNxsGrpDataTemporaryList toStore;
	std::vector<RsNxsGrp*>::iterator it = groups.begin();
	for(; it != groups.end(); it++)
	{
		RsNxsGrp* grp = *it;
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		grp->metaData = meta ;
		meta->deserialise(grp->meta.bin_data, grp->meta.bin_len);
		toStore.push_back(grp);
	}

	RsGeneralDataService* ds = mTestScenario->getDataService(pid);
	ds->storeGroup(toStore);
}

void rs_nxs_test::NxsTestHub::Wait(int seconds) {

	double dsecs = seconds;

#ifndef WINDOWS_SYS
        usleep((int) (dsecs * 1000000));
#else
        Sleep((int) (dsecs * 1000));
#endif
}

bool rs_nxs_test::NxsTestHub::recvItem(RsRawItem* item, const RsPeerId& peerFrom)
{
    RS_STACK_MUTEX(mMtx); /***** MTX LOCKED *****/
	PayLoad p(peerFrom, item);
	mPayLoad.push(p);
	return true;
}

void rs_nxs_test::NxsTestHub::CleanUpTest()
{
	mTestScenario->cleanTestScenario();
}

void rs_nxs_test::NxsTestHub::data_tick()
{
	// for each nxs instance pull out all items from each and then move to destination peer

	PeerNxsMap::iterator it = mPeerNxsMap.begin();

	// deliver payloads to peer's net services
    mMtx.lock();
	while(!mPayLoad.empty())
	{
		PayLoad& p = mPayLoad.front();

		RsRawItem* item = p.second;
		RsPeerId peerFrom = p.first;
		RsPeerId peerTo = item->PeerId();
		item->PeerId(peerFrom);
        mMtx.unlock();
            mPeerNxsMap[peerTo]->recv(item);
        mMtx.lock();
		mPayLoad.pop();
	}
    mMtx.unlock();

	// then tick net services
	for(; it != mPeerNxsMap.end(); it++)
	{
		RsGxsNetService *s = it->second;
		s->tick();

	}

    double timeDelta = .2;
    usleep(timeDelta * 1000000);
}


