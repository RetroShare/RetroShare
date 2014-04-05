#include "nxstesthub.h"

NxsTestHub::NxsTestHub(NxsTestScenario * nts, std::set<RsPeerId> &peers) : mTestScenario(nts)
{

    std::set<RsPeerId>::iterator sit = peers.begin();

    for(; sit != peers.end(); sit++)
    {
        std::set<RsPeerId> msgPeers = peers;

        // add peers all peers except one iterator currently points to
        msgPeers.erase(*sit);
        NxsNetDummyMgr* dummyMgr = new NxsNetDummyMgr(*sit, msgPeers);
        RsGeneralDataService* ds = mTestScenario->getDataService(*sit);
        NxsMessageTestObserver* obs = new NxsMessageTestObserver(ds);

	RsServiceInfo info;
        RsGxsNetService* netService =
                new RsGxsNetService(mTestScenario->getServiceType(),
                                                 ds, dummyMgr, obs, info);


        mNetServices.insert(std::make_pair(*sit, netService));
        mObservers.insert(std::make_pair(*sit, obs));
    }

    sit = peers.begin();

    // launch net services
    for(; sit != peers.end(); sit++)
    {
        RsGxsNetService* n = mNetServices[*sit];
        createThread(*n);
        mServices.insert(std::make_pair(*sit, n));
    }
}

NxsTestHub::~NxsTestHub()
{
    std::map<RsPeerId, RsGxsNetService*>::iterator mit = mNetServices.begin();

    for(; mit != mNetServices.end(); mit++)
        delete mit->second;
}


void NxsTestHub::run()
{
        double timeDelta = .2;

	while(isRunning()){

                // make thread sleep for a bit
        #ifndef WINDOWS_SYS
                usleep((int) (timeDelta * 1000000));
        #else
                Sleep((int) (timeDelta * 1000));
        #endif


                std::map<RsPeerId, p3Service*>::iterator mit = mServices.begin();

                for(; mit != mServices.end(); mit++)
                {
                    p3Service* s = mit->second;
                    s->tick();
                }

                mit = mServices.begin();

                // collect msgs to send to peers from peers
                for(; mit != mServices.end(); mit++)
                {
                    const RsPeerId& peer = mit->first;
                    p3Service* s = mit->second;

                    // first store all the sends from all services
                    RsItem* item = NULL;

                    while((item =  s->send()) != NULL){

                        const RsPeerId peerToReceive = item->PeerId();

                        // set the peer this item comes from
                        item->PeerId(peer);
                        mPeerQueues[peerToReceive].push_back(item);
                    }


                }

                // now route items to peers
                std::map<RsPeerId, std::vector<RsItem*> >::iterator mit_queue = mPeerQueues.begin();

                for(; mit_queue != mPeerQueues.end(); mit_queue++)
                {
                    std::vector<RsItem*>& queueV = mit_queue->second;
                    std::vector<RsItem*>::iterator vit = queueV.begin();
                    const RsPeerId peerToReceive = mit_queue->first;
                    for(; vit != queueV.end(); vit++)
                    {

                        RsItem* item = *vit;
                        p3Service* service = mServices[peerToReceive];

                        service->receive(dynamic_cast<RsRawItem*>(item));
                    }
                    queueV.clear();
                }
	}
}

void NxsTestHub::cleanUp()
{
    std::map<RsPeerId, RsGxsNetService*>::iterator mit = mNetServices.begin();
    for(; mit != mNetServices.end(); mit++)
    {
        RsGxsNetService* n = mit->second;
        n->join();
    }

    // also shut down this net service peers if this goes down
    mTestScenario->cleanUp();
}

bool NxsTestHub::testsPassed()
{
	return false;
}
