#include "nxstesthub.h"



class NotifyWithPeerId : public RsNxsObserver
{
public:

	NotifyWithPeerId(RsPeerId val, rs_nxs_test::NxsTestHub& hub)
	: mPeerId(val), mTestHub(hub){

	}

    void notifyNewMessages(std::vector<RsNxsMsg*>& messages)
    {
    	mTestHub.notifyNewMessages(mPeerId, messages);
    }

    void notifyNewGroups(std::vector<RsNxsGrp*>& groups)
    {
    	mTestHub.notifyNewGroups(mPeerId, groups);
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
private:
	RsPeerId mPeerId;
	RecvPeerItemIface* mRecvIface;

};

rs_nxs_test::NxsTestHub::NxsTestHub(NxsTestScenario::pointer testScenario)
 : mTestScenario(testScenario)
{
	std::list<RsPeerId> peers;
	mTestScenario->getPeers(peers);

	// for each peer get initialise a nxs net instance
	// and pass this to the simulator

	std::list<RsPeerId>::const_iterator cit = peers.begin();

	for(; cit != peers.end(); cit++)
	{
		RsGxsNetService::pointer ns = RsGxsNetService::pointer(
				new RsGxsNetService(
				mTestScenario->getServiceType(),
				mTestScenario->getDataService(*cit),
				mTestScenario->getDummyNetManager(*cit),
				new NotifyWithPeerId(*cit, *this),
				mTestScenario->getServiceInfo(),
				mTestScenario->getDummyReputations(*cit),
				mTestScenario->getDummyCircles(*cit), true
				)
		);

		NxsTestHubConnection *connection =
				new NxsTestHubConnection(*cit, this);
		ns->setServiceServer(connection);

		mPeerNxsMap.insert(std::make_pair(*cit, ns));
	}
}


rs_nxs_test::NxsTestHub::~NxsTestHub() {
}


bool rs_nxs_test::NxsTestHub::testsPassed()
{
	return mTestScenario->checkTestPassed();
}


void rs_nxs_test::NxsTestHub::run()
{
	bool running = isRunning();
	double timeDelta = .2;
	while(running)
	{
#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

        tick();

		running = isRunning();
	}

}


void rs_nxs_test::NxsTestHub::StartTest()
{
	// get all services up and running
	PeerNxsMap::iterator mit = mPeerNxsMap.begin();
	for(; mit != mPeerNxsMap.end(); mit++)
	{
		createThread(*(mit->second));
	}

	createThread(*this);
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

	std::map<RsNxsMsg*, RsGxsMsgMetaData*> toStore;
	std::vector<RsNxsMsg*>::iterator it = messages.begin();
	for(; it != messages.end(); it++)
	{
		RsNxsMsg* msg = *it;
		RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();
		bool ok = meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len));
		toStore.insert(std::make_pair(msg, meta));
	}

	RsGeneralDataService* ds = mTestScenario->getDataService(pid);
	ds->storeMessage(toStore);
}


void rs_nxs_test::NxsTestHub::notifyNewGroups(const RsPeerId& pid, std::vector<RsNxsGrp*>& groups)
{
	std::map<RsNxsGrp*, RsGxsGrpMetaData*> toStore;
	std::vector<RsNxsGrp*>::iterator it = groups.begin();
	for(; it != groups.end(); it++)
	{
		RsNxsGrp* grp = *it;
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		bool ok = meta->deserialise(grp->meta.bin_data, grp->meta.bin_len);
		toStore.insert(std::make_pair(grp, meta));
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
	PayLoad p(peerFrom, item);
	mPayLoad.push(p);
	return true;
}

void rs_nxs_test::NxsTestHub::tick()
{
	// for each nxs instance pull out all items from each and then move to destination peer

	PeerNxsMap::iterator it = mPeerNxsMap.begin();

	// deliver payloads to peer's net services
	while(!mPayLoad.empty())
	{
		PayLoad& p = mPayLoad.front();

		RsRawItem* item = p.second;
		RsPeerId peerFrom = p.first;
		RsPeerId peerTo = item->PeerId();
		item->PeerId(peerFrom);
		mPeerNxsMap[peerTo]->recv(item); //
		mPayLoad.pop();
	}

	// then tick net services
	for(; it != mPeerNxsMap.end(); it++)
	{
		RsGxsNetService::pointer s = it->second;
		s->tick();

	}

}


