/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxstesthub.h                           *
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

class NotifyWithPeerId;
class NxsTestHubConnection ;

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
    class NxsTestHub : public RsTickingThread, public RecvPeerItemIface
	{
	public:


		/*!
		 * This constructs the test hub
		 * for a give scenario in mind
		 */
		NxsTestHub(NxsTestScenario* testScenario);

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

        /*!
         *  This simulates the p3Service ticker and calls both gxs net services tick methods
         *  Also enables transport of messages between both services
         */
        virtual void data_tick();

	private:

	    typedef std::pair<RsPeerId, RsRawItem*> PayLoad;

	    typedef std::map<RsPeerId, RsGxsNetService* > PeerNxsMap ;

		NxsTestScenario *mTestScenario;
		RsMutex mMtx;
		PeerNxsMap mPeerNxsMap;
		std::queue<PayLoad> mPayLoad;
		std::list<NotifyWithPeerId*> mNotifys;
		std::list<NxsTestHubConnection *> mConnections;
	};
}
#endif // NXSTESTHUB_H
