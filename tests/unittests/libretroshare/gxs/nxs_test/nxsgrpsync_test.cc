/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsgrpsync_test.cc                     *
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

#include "nxsgrpsync_test.h"
#include "retroshare/rstypes.h"
#include "gxs/rsdataservice.h"
#include "nxsdummyservices.h"
#include "../common/data_support.h"

using namespace rs_nxs_test;



NxsGrpSync::NxsGrpSync(RsGcxs* circle, RsGixsReputation* reputation):
    mServType(0)
{

	int numPeers = 2;

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

	if(!reputation)
		mRep = new RsNxsSimpleDummyReputation(reMap, true);
	else
	{
		mRep = reputation;
	}

	if(!circle)
		mCircles = new RsNxsSimpleDummyCircles();
	else
	{
		mCircles = circle;
	}


	mPgpUtils = new RsDummyPgpUtils();

	// lets create some a group each for all peers
	DataMap::iterator mit = mDataServices.begin();
	for(; mit != mDataServices.end(); mit++)
	{
		RsNxsGrp* grp = new RsNxsGrp(mServType);

		init_item(*grp);
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		init_item(meta);
		grp->metaData = meta;
		meta->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED;

		if(circle)
		{
			meta->mCircleType = GXS_CIRCLE_TYPE_EXTERNAL;
            meta->mCircleId = RsGxsCircleId::random();
		}

		RsGxsGroupId grpId = grp->grpId;

		RsNxsGrpDataTemporaryList gsp;
		gsp.push_back(grp);
		mit->second->storeGroup(gsp);

		// the expected result is that each peer has the group of the others
		it = mPeerIds.begin();
		for(; it != mPeerIds.end(); it++)
		{
			mExpectedResult[*it].push_back(grpId);
		}
	}

}


void NxsGrpSync::getPeers(std::list<RsPeerId>& peerIds)
{
	peerIds = mPeerIds;
}



RsGeneralDataService* NxsGrpSync::getDataService(const RsPeerId& peerId)
{
	return mDataServices[peerId];
}

RsNxsNetMgr* NxsGrpSync::getDummyNetManager(const RsPeerId& peerId)
{
	return mNxsNetMgrs[peerId];
}

RsGcxs* NxsGrpSync::getDummyCircles(const RsPeerId& /*peerId*/)
{
	return mCircles;
}

RsGixsReputation* NxsGrpSync::getDummyReputations(const RsPeerId& /*peerId*/)
{
	return mRep;
}

uint16_t NxsGrpSync::getServiceType()
{
	return mServType;
}

rs_nxs_test::NxsGrpSync::~NxsGrpSync()
{
	// clean up data stores
	DataMap::iterator mit = mDataServices.begin();
	for(; mit != mDataServices.end(); mit++)
	{
		RsGxsGroupId::std_vector grpIds;
		mit->second->resetDataStore();
		std::string dbFile = "grp_store_" + mit->first.toStdString();
		remove(dbFile.c_str());
	}
}

RsServiceInfo rs_nxs_test::NxsGrpSync::getServiceInfo() {
	return mServInfo;
}

PgpAuxUtils* rs_nxs_test::NxsGrpSync::getDummyPgpUtils()
{
	return mPgpUtils;
}

const NxsGrpTestScenario::ExpectedMap& rs_nxs_test::NxsGrpSync::getExpectedMap() {
	return mExpectedResult;
}


