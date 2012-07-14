#include "nxstesthub.h"

NxsTestHub::NxsTestHub(NxsTestScenario* nts) : mTestScenario(nts)
{

	netServicePairs.first = new RsGxsNetService(0, mTestScenario->dummyDataService1(), &netMgr1, mTestScenario);
	netServicePairs.second = new RsGxsNetService(0, mTestScenario->dummyDataService2(), &netMgr2, mTestScenario);

	mServicePairs.first = netServicePairs.first;
	mServicePairs.second = netServicePairs.second;

	createThread(*(netServicePairs.first));
	createThread(*(netServicePairs.second));
}

NxsTestHub::~NxsTestHub()
{
	delete netServicePairs.first;
	delete netServicePairs.second;
}


void NxsTestHub::run()
{

	std::list<RsItem*> send_queue_s1, send_queue_s2;

	while(isRunning()){

		// make thread sleep for a couple secs
		usleep(300);

		p3Service* s1 = mServicePairs.first;
		p3Service* s2 = mServicePairs.second;

		RsItem* item = NULL;
		while((item = s1->send()) != NULL)
		{
			send_queue_s1.push_back(item);
		}

		while((item = s2->send()) != NULL)
		{
			send_queue_s2.push_back(item);
		}

		while(!send_queue_s1.empty()){
			item = send_queue_s1.front();
			s2->receive(dynamic_cast<RsRawItem*>(item));
			send_queue_s1.pop_front();
		}

		while(!send_queue_s2.empty()){
			item = send_queue_s2.front();
			s1->receive(dynamic_cast<RsRawItem*>(item));
			send_queue_s2.pop_front();
		}

		// tick services so nxs net services processe items
		s1->tick();
		s2->tick();
	}

	// also shut down this net service peers if this goes down
	netServicePairs.first->join();
	netServicePairs.second->join();
}

void NxsTestHub::cleanUp()
{
	mTestScenario->cleanUp();
}

bool NxsTestHub::testsPassed()
{
	return false;
}
