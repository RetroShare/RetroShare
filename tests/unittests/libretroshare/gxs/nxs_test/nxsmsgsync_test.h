/*
 * nxsmsgsync_test.h
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#ifndef NXSMSGSYNC_TEST_H_
#define NXSMSGSYNC_TEST_H_

#include "nxsmsgtestscenario.h"

namespace rs_nxs_test {

	class NxsMsgSync : public NxsMsgTestScenario
	{
	public:

		NxsMsgSync();
		virtual ~NxsMsgSync();
		void getPeers(std::list<RsPeerId>& peerIds);
		RsGeneralDataService* getDataService(const RsPeerId& peerId);
		RsNxsNetMgr* getDummyNetManager(const RsPeerId& peerId);
		RsGcxs* getDummyCircles(const RsPeerId& peerId);
		RsGixsReputation* getDummyReputations(const RsPeerId& peerId);
		uint16_t getServiceType();
		RsServiceInfo getServiceInfo();
		PgpAuxUtils* getDummyPgpUtils();

	protected:

		const ExpectedMap& getExpectedMap();

	private:

		std::list<RsPeerId> mPeerIds;
		typedef std::map<RsPeerId, RsGeneralDataService*> DataMap;

		DataMap mDataServices;
		std::map<RsPeerId, RsNxsNetMgr*> mNxsNetMgrs;
		RsGixsReputation* mRep;
		RsGcxs* mCircles;
		RsServiceInfo mServInfo;
		PgpAuxUtils* mPgpUtils;

		NxsMsgTestScenario::ExpectedMap mExpectedResult;

		uint16_t mServType;

	};

}


#endif /* NXSMSGSYNC_TEST_H_ */
