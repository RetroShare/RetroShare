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
#include "../common/data_support.h"

using namespace rs_nxs_test;

template<typename T>
void copy_all_but(T& ex, const std::list<T>& s, std::list<T>& d)
{
	typename std::list<T>::const_iterator cit = s.begin();
	for(; cit != s.end(); cit++)
		if(*cit != ex)
			d.push_back(*cit);
}

NxsGrpSync::NxsGrpSync()
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
		RsGeneralDataService* ds = new RsDataService("./", "grp_store_" +
				it->toStdString(), mServType, NULL, "key");
		mDataServices.insert(std::make_pair(*it, ds));

		// net managers
		std::list<RsPeerId> otherPeers;
		copy_all_but<RsPeerId>(*it, mPeerIds, otherPeers);
		RsNxsNetMgr* mgr = new rs_nxs_test::RsNxsNetDummyMgr(*it, otherPeers);
		mNxsNetMgrs.insert(std::make_pair(*it, mgr));

		RsNxsSimpleDummyReputation::RepMap reMap;
		std::list<RsNxsSimpleDummyCircles::Membership> membership;
		// now reputation service
		mRep = new RsNxsSimpleDummyReputation(reMap, true);
		mCircles = new RsNxsSimpleDummyCircles(membership, true);
	}

	// lets create some a group each for all peers
	DataMap::iterator mit = mDataServices.begin();
	for(; mit != mDataServices.end(); mit++)
	{
		RsNxsGrp* grp = new RsNxsGrp(mServType);

		init_item(*grp);
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		init_item(meta);
		grp->metaData = meta;
		RsNxsGrp* grp_copy = grp->clone();


		RsGeneralDataService::GrpStoreMap gsp;
		gsp.insert(std::make_pair(grp, meta));
		mit->second->storeGroup(gsp);

		// the expected result is that each peer has the group of the others
		it = mPeerIds.begin();
		for(; it != mPeerIds.end(); it++)
		{
			if(mit->first != *it)
				mExpectedResult[*it].push_back(grp_copy);
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

bool NxsGrpSync::checkTestPassed()
{
	// look at data store, as all peers should have same
	// number of groups
	DataMap::iterator mit = mDataServices.begin();
	std::list<int> vals;
	for(; mit != mDataServices.end(); mit++)
	{
		RsGxsGroupId::std_vector grpIds;
		mit->second->retrieveGroupIds(grpIds);
		vals.push_back(grpIds.size());
	}

	std::list<int>::iterator lit = vals.begin();
	int prev = *lit;
	bool passed = true;
	for(; lit != vals.end(); lit++)
	{
		passed &= *lit == prev;
		prev = *lit;
	}

	return passed;
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

