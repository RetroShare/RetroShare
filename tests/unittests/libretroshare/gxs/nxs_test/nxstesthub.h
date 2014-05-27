#ifndef NXSTESTHUB_H
#define NXSTESTHUB_H

#include "util/rsthreads.h"
#include "gxs/rsgxsnetservice.h"
#include "nxstestscenario.h"
#include <queue>

// it would probably be useful if the test scenario
// provided the net dummy managers
// hence one could envision synchronising between an arbitrary number
// of peers



namespace rs_nxs_test
{

	class RecvPeerItemIface
	{
	public:
		virtual ~RecvPeerItemIface(){}
		virtual bool	recvItem(RsRawItem *item, const RsPeerId& peerFrom) =0 ;
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
	class NxsTestHub : public RsThread, public RecvPeerItemIface
	{
	public:


		/*!
		 * This constructs the test hub
		 * for a give scenario in mind
		 */
		NxsTestHub(NxsTestScenario::pointer testScenario);

		/*!
		 * This cleans up what ever testing resources are left
		 * including the test scenario
		 */
		virtual ~NxsTestHub();

		/*!
		 * To be called only after end test is called
		 * otherwise undefined
		 */
		bool testsPassed();

		/*!
		 *  This simulates the p3Service ticker and calls both gxs net services tick methods
		 *  Also enables transport of messages between both services
		 */
		void run();

		/*!
		 * Begings test, equivalent to CreateThread(this)
		 */
		void StartTest();

		/*!
		 * Gracefully ends the test
		 */
		void EndTest();

		/*!
		 * Clean up test environment
		 */
		void CleanUpTest();
	    /*!
	     * @param messages messages are deleted after function returns
	     */
	    void notifyNewMessages(const RsPeerId&, std::vector<RsNxsMsg*>& messages);

	    /*!
	     * @param messages messages are deleted after function returns
	     */
	    void notifyNewGroups(const RsPeerId&, std::vector<RsNxsGrp*>& groups);

	    static void Wait(int seconds);


	public:

		bool	recvItem(RsRawItem *item, const RsPeerId& peerFrom);

	private:

	    void tick();

	private:

	    typedef std::pair<RsPeerId, RsRawItem*> PayLoad;

	    typedef std::map<RsPeerId, RsGxsNetService::pointer > PeerNxsMap ;
		PeerNxsMap mPeerNxsMap;
		NxsTestScenario::pointer mTestScenario;
		std::queue<PayLoad> mPayLoad;

	};
}
#endif // NXSTESTHUB_H
