/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsmsgsync_test.h                      *
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
