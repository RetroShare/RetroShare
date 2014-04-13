/*
 * nxsgrpsync_test.cc
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#include "nxsgrpsync_test.h"
#include "retroshare/rstypes.h"
#include "gxs/rsdataservice.h"
#include "nxsdummyservices.h"

using namespace rs_nxs_test;

template<typename T>
void copy_all_but(T& ex, const std::list<T>& s, std::list<T> d)
{
	std::list<T>::const_iterator cit = s.begin();
	for(; cit != s.end(); cit++)
		if(*cit != ex)
			d.push_back(*cit);
}

NxsGrpSync::NxsGrpSync()
{
	// first choose ids

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
		RsGeneralDataService* ds = new RsDataService("./", "grp_store" +
				it->toStdString(), mServType, NULL, "key");
		mDataServices.insert(*it, ds);

		// net managers
		std::list<RsPeerId> otherPeers;
		copy_all_but<RsPeerId>(*it, mPeerIds, otherPeers);
		RsNxsNetMgr* mgr = new RsNxsNetDummyMgr(*it, otherPeers);
		mNxsNetMgrs.insert(std::make_pair(*it, mgr));

		// now reputation service
		mRep = new RsNxsSimpleDummyReputation();
		mCircles = new RsNxsSimpleDummyCircles();

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

bool NxsGrpSync::checkTestPassed()
{
	// look at data store of peer1 an compare to peer 2
	return true;
}

RsNxsNetMgr* NxsGrpSync::getDummyNetManager(const RsPeerId& peerId)
{
	return mNxsNetMgrs[peerId];
}

RsGcxs* NxsGrpSync::getDummyCircles(const RsPeerId& peerId)
{
	return mCircles;
}

RsGixsReputation* NxsGrpSync::getDummyReputations(const RsPeerId& peerId)
{
	return mRep;
}

uint16_t NxsGrpSync::getServiceType()
{
}
