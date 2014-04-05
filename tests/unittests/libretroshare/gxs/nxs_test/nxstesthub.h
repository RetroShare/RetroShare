#ifndef NXSTESTHUB_H
#define NXSTESTHUB_H

#include "util/rsthreads.h"
#include "gxs/rsgxsnetservice.h"
#include "nxstestscenario.h"

// it would probably be useful if the test scenario
// provided the net dummy managers
// hence one could envision synchronising between an arbitrary number
// of peers


class NxsNetDummyMgr : public RsNxsNetMgr
{

public:

    NxsNetDummyMgr(RsPeerId ownId, std::set<RsPeerId> peers) : mOwnId(ownId), mPeers(peers) {

	}

    const RsPeerId& getOwnId()  { return mOwnId; }
    void getOnlineList(uint32_t serviceId, std::set<RsPeerId>& ssl_peers) { ssl_peers = mPeers; }

private:

    RsPeerId mOwnId;
    std::set<RsPeerId> mPeers;

};


/*!
 * Testing of nxs services occurs through use of two services
 * When a service sends this class can interrogate the send and the receives of
 *
 * NxsScenario stores the type of synchronisation to be tested
 * Operation:
 * First NxsTestHub needs to be instantiated with a test scenario
 *   * The scenario contains two databases to be used on the communicating pair of RsGxsNetService instances (net instances)
 * The Test hub has a ticker service for the p3Services which allows the netservices to search what groups and messages they have
 * and synchronise according to their subscriptions. The default is to subscribe to all groups held by other peer
 * The threads for both net instances are started which begins their processing of transactions
 */
class NxsTestHub : public RsThread
{
public:


	/*!
	 * This construct the test hub
	 * for a give scenario in mind
	 */
    NxsTestHub(NxsTestScenario*, std::set<RsPeerId>& peers);


    /*!
     *
     */
    virtual ~NxsTestHub();

    /*!
     * To be called only after this thread has
     * been shutdown
     */
    bool testsPassed();

    /*!
     *  This simulates the p3Service ticker and calls both gxs net services tick methods
     *  Also enables transport of messages between both services
     */
    void run();


    void cleanUp();
private:

    std::map<RsPeerId, p3Service*> mServices;
    std::map<RsPeerId, RsGxsNetService*> mNetServices;
    std::map<RsPeerId, NxsMessageTestObserver*> mObservers;

    std::map<RsPeerId, std::vector<RsItem*> > mPeerQueues;

    NxsTestScenario *mTestScenario;

};

#endif // NXSTESTHUB_H
