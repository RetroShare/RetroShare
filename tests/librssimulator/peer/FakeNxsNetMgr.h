#pragma once

#include "gxs/rsgxsnetservice.h"
#include "FakeLinkMgr.h"

class FakeNxsNetMgr : public RsNxsNetMgr
{

public:

	FakeNxsNetMgr(FakeLinkMgr *linkMgr)
	:mLinkMgr(linkMgr) { return; }

	const RsPeerId& getOwnId()  
	{ 
		return mLinkMgr->getOwnId();
	}

	void getOnlineList(uint32_t serviceId, std::set<RsPeerId>& ssl_peers) 
	{ 
		(void) serviceId;

		std::list<RsPeerId> peerList;
		mLinkMgr->getOnlineList(peerList);

		std::list<RsPeerId>::const_iterator it;
		for(it = peerList.begin(); it != peerList.end(); it++)
		{
			ssl_peers.insert(*it);
		}
	}
private:

	FakeLinkMgr *mLinkMgr;
};

