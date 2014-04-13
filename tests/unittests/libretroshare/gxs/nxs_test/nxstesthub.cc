#include "nxstesthub.h"



class NotifyWithPeerId : public RsNxsObserver
{
public:

	NotifyWithPeerId(RsPeerId val, NxsTestHub& hub)
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

rs_nxs_test::NxsTestHub::NxsTestHub(NxsTestScenario* testScenario)
 : mTestScenario(testScenario)
{
	std::list<RsPeerId> peers;
	mTestScenario->getPeers(peers);

	// for each peer get initialise a nxs net instance
	// and pass this to the simulator

	std::list<RsPeerId>::const_iterator cit = peers.begin();

	for(; cit != peers.end(); cit++)
	{
		RsGxsNetService* ns = new RsGxsNetService(mTestScenario->getServiceType(),
				mTestScenario->getDataService(*cit), mTestScenario->getDummyNetManager(*cit),
				new NotifyWithPeerId(*cit, *this),
				mTestScenario->getServiceInfo(), mTestScenario->getDummyReputations(*cit),
				mTestScenario->getDummyCircles(*cit), true);

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
		bool ok = meta->deserialise(msg->meta.bin_data, msg->meta.bin_len);
		toStore.insert(std::make_pair(msg, meta));
	}

	RsDataService* ds = mTestScenario->getDataService(pid);
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

	RsDataService* ds = mTestScenario->getDataService(pid);
	ds->storeGroup(toStore);
}

void rs_nxs_test::NxsTestHub::Wait(int seconds) {

#ifndef WINDOWS_SYS
        usleep((int) (1000000));
#else
        Sleep((int) (1000));
#endif
}

void rs_nxs_test::NxsTestHub::tick()
{
	// for each nxs instance pull out all items from each and then move to destination peer

	typedef std::map<RsPeerId, std::list<RsItem*> > DestMap;

	PeerNxsMap::iterator it = mPeerNxsMap.begin();
	DestMap destMap;
	for(; it != mPeerNxsMap.end(); it++)
	{
		RsGxsNetService* s = *it;
		s->tick();
		RsItem* item = s->recvItem();

		if(item != NULL)
			destMap[item->PeerId()].push_back(item);
	}

	DestMap::iterator dit = destMap.begin();

	for(; dit != destMap.end(); dit++ )
	{
		std::list<RsItem*> payload = dit->second;
		const RsPeerId& pid = dit->first;

		if(mPeerNxsMap.count(pid) > 0)
		{
			std::list<RsItem*>::iterator pit = payload.begin();
			for(; pit != payload.end(); pit)
				mPeerNxsMap[pid]->recvItem(*pit);
		}
		else
		{
			std::cerr << "Could not find peer: " << pid << std::endl;
		}
	}
}


