#pragma once

#include <list>
#include "retroshare/rsids.h"
#include "pqi/p3linkmgr.h"

struct RsItem;
class PeerNode;
class RsSerialType;
class RsSerialiser;

class IsolatedServiceTester
{
public:
	IsolatedServiceTester(RsPeerId ownId, std::list<RsPeerId> peers);
	~IsolatedServiceTester();

	void addSerialType(RsSerialType *st);
	bool startup();

	void notifyPeers();
	bool bringOnline(std::list<RsPeerId> peers);

	bool tick();
	bool tickUntilPacket(int max_ticks);

	RsItem *getPacket();
	bool sendPacket(RsItem *);

	PeerNode *getPeerNode() { return mNode; }

private:
	PeerNode *mNode;
	RsSerialiser *mRsSerialiser;
};



