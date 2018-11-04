/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsmsgsync_test.cc                     *
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

#include "nxsmsgsync_test.h"
#include "retroshare/rstypes.h"
#include "gxs/rsdataservice.h"
#include "nxsdummyservices.h"
#include "../common/data_support.h"
//#include <auto_ptr.h> //Already include in memory, a better way.
#include <memory>

using namespace rs_nxs_test;

rs_nxs_test::NxsMsgSync::~NxsMsgSync()
{
	for(std::map<RsPeerId,RsNxsNetMgr*>::const_iterator it(mNxsNetMgrs.begin());it!=mNxsNetMgrs.end();++it)
		delete it->second ;

	for(DataMap::const_iterator it(mDataServices.begin());it!=mDataServices.end();++it)
		delete it->second ;

	delete mRep ;
	delete mCircles;
	delete mPgpUtils;
}


rs_nxs_test::NxsMsgSync::NxsMsgSync()
 : mPgpUtils(NULL), mServType(0) {
	int numPeers = 2;

	// create 2 peers
	for(int i =0; i < numPeers; i++)
	{
		RsPeerId id = RsPeerId::random();
		mPeerIds.push_back(id);
	}


	std::list<RsPeerId>::iterator it = mPeerIds.begin();
	for(; it != mPeerIds.end(); it++)
	{
		// data stores
		RsGeneralDataService* ds = createDataStore(*it, mServType);
		mDataServices.insert(std::make_pair(*it, ds));

		// net managers
		std::list<RsPeerId> otherPeers;
		copy_all_but<RsPeerId>(*it, mPeerIds, otherPeers);
		RsNxsNetMgr* mgr = new rs_nxs_test::RsNxsNetDummyMgr(*it, otherPeers);
		mNxsNetMgrs.insert(std::make_pair(*it, mgr));
	}

	RsNxsSimpleDummyReputation::RepMap reMap;
	// now reputation service
	mRep = new RsNxsSimpleDummyReputation(reMap, true);
	mCircles = new RsNxsSimpleDummyCircles();

	// lets create 2 groups and all peers will have them
	int nGrps = 2;

	NxsMsgTestScenario::ExpectedMap& expMap = mExpectedResult;
	for(int i=0; i < nGrps; i++)
	{
		std::auto_ptr<RsNxsGrp> grp = std::auto_ptr<RsNxsGrp>(new RsNxsGrp(mServType));

		init_item(*grp);
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		init_item(meta);
		meta->mReputationCutOff = 0;
		meta->mGroupId = grp->grpId;
                grp->metaData = meta;
		meta->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED;
		meta->mCircleType = GXS_CIRCLE_TYPE_PUBLIC;

		RsGxsGroupId grpId = grp->grpId;

		// the expected result is that each peer has all messages
		it = mPeerIds.begin();

		DataMap::iterator mit = mDataServices.begin();

		// add a clone of group into the peer's service
		// then create 2 msgs for each peer for each group
		for(; mit != mDataServices.end(); mit++)
		{
			// first store grp
			RsGeneralDataService* ds = mit->second;
			RsNxsGrp* grp_clone = grp->clone();
			RsNxsGrpDataTemporaryList gsp;
			gsp.push_back(grp_clone);
			ds->storeGroup(gsp);

			RsGxsGroupId grpId = grp->grpId;
			RsPeerId peerId = mit->first;

			NxsMsgTestScenario::ExpectedMsgs expMsgs;
			int nMsgs = 2; // and each grp for each peer gets a unique message
			for(int j=0; j < nMsgs; j++)
			{
				RsNxsMsg* msg = new RsNxsMsg(mServType);
				init_item(*msg);
				msg->grpId = grp->grpId;
				RsGxsMsgMetaData* msgMeta = new RsGxsMsgMetaData();
				init_item(msgMeta);
				msg->metaData = msgMeta;
				msgMeta->mGroupId = grp->grpId;
				msgMeta->mMsgId = msg->msgId;

				RsNxsMsgDataTemporaryList msm;
				msm.push_back(msg);
				RsGxsMessageId msgId = msg->msgId;
				ds->storeMessage(msm);

				it = mPeerIds.begin();

				// the expectation is that all peers have the same messages
				for(; it != mPeerIds.end(); it++)
				{
					NxsMsgTestScenario::ExpectedMsgs& expMsgs = expMap[peerId];
					expMsgs[grpId].push_back(msgId);
				}
			}
		}
	}
}

void rs_nxs_test::NxsMsgSync::getPeers(std::list<RsPeerId>& peerIds) {
	peerIds = mPeerIds;
}

RsGeneralDataService* rs_nxs_test::NxsMsgSync::getDataService(
		const RsPeerId& peerId) {
	return mDataServices[peerId];
}

RsNxsNetMgr* rs_nxs_test::NxsMsgSync::getDummyNetManager(
		const RsPeerId& peerId) {
	return mNxsNetMgrs[peerId];
}

RsGcxs* rs_nxs_test::NxsMsgSync::getDummyCircles(const RsPeerId& /*peerId*/) {
	return mCircles;
}

RsGixsReputation* rs_nxs_test::NxsMsgSync::getDummyReputations(
		const RsPeerId& /*peerId*/) {
	return mRep;
}

uint16_t rs_nxs_test::NxsMsgSync::getServiceType() {
	return mServType;
}

RsServiceInfo rs_nxs_test::NxsMsgSync::getServiceInfo() {
	return mServInfo;
}

PgpAuxUtils* rs_nxs_test::NxsMsgSync::getDummyPgpUtils()
{
	return mPgpUtils;
}

const NxsMsgTestScenario::ExpectedMap& rs_nxs_test::NxsMsgSync::getExpectedMap() {
	return mExpectedResult;
}
