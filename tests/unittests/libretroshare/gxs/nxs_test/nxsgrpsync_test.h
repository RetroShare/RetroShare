/*
 * nxsgrpsync_test.h
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#ifndef NXSGRPSYNC_TEST_H_
#define NXSGRPSYNC_TEST_H_


#include "nxstestscenario.h"

namespace rs_nxs_test
{

	class NxsGrpSync : public  NxsTestScenario
	{
	public:

		NxsGrpSync();
		~NxsGrpSync();

		void getPeers(std::list<RsPeerId>& peerIds);
		RsGeneralDataService* getDataService(const RsPeerId& peerId);
		bool checkTestPassed();
		RsNxsNetMgr* getDummyNetManager(const RsPeerId& peerId);
		RsGcxs* getDummyCircles(const RsPeerId& peerId);
		RsGixsReputation* getDummyReputations(const RsPeerId& peerId);
		uint16_t getServiceType();
		RsServiceInfo getServiceInfo();

	private:

		std::list<RsPeerId> mPeerIds;
		typedef std::map<RsPeerId, RsGeneralDataService*> DataMap;
		typedef std::map<RsPeerId, std::list<RsNxsGrp*> > ExpectedMap;

		DataMap mDataServices;
		std::map<RsPeerId, RsNxsNetMgr*> mNxsNetMgrs;
		RsGixsReputation* mRep;
		RsGcxs* mCircles;
		RsServiceInfo mServInfo;

		ExpectedMap mExpectedResult;

		uint16_t mServType;
	};

}
#endif /* NXSGRPSYNC_TEST_H_ */
