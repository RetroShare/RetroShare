#ifndef NXSTESTHUB_H
#define NXSTESTHUB_H

#include "util/rsthreads.h"
#include "gxs/rsgxsnetservice.h"
#include "nxstestscenario.h"

// it would probably be useful if the test scenario
// provided the net dummy managers
// hence one could envision synchronising between an arbitrary number
// of peers


class NxsNetDummyMgr1 : public RsNxsNetMgr
{

public:

	NxsNetDummyMgr1() : mOwnId("peerA") {

		mPeers.insert("peerB");
	}

    std::string getOwnId()  { return mOwnId; }
    void getOnlineList(std::set<std::string>& ssl_peers) { ssl_peers = mPeers; }

private:

    std::string mOwnId;
    std::set<std::string> mPeers;

};

class NxsNetDummyMgr2 : public RsNxsNetMgr
{

public:

	NxsNetDummyMgr2() : mOwnId("peerB") {

		mPeers.insert("peerA");

	}

    std::string getOwnId()  { return mOwnId; }
    void getOnlineList(std::set<std::string>& ssl_peers) { ssl_peers = mPeers; }

private:

    std::string mOwnId;
    std::set<std::string> mPeers;
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
    NxsTestHub(NxsTestScenario*);


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

    std::pair<p3Service*, p3Service*> mServicePairs;
    std::pair<RsGxsNetService*, RsGxsNetService*> netServicePairs;
    NxsTestScenario *mTestScenario;

    NxsNetDummyMgr1 netMgr1;
    NxsNetDummyMgr2 netMgr2;

};

#endif // NXSTESTHUB_H
