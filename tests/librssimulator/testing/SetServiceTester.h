#pragma once

#include <list>
#include "retroshare/rsids.h"
#include "rsitems/rsitem.h"
#include "pqi/p3linkmgr.h"

#include "SetPacket.h"
#include "SetFilter.h"

struct RsItem;
class PeerNode;
class RsSerialType;
class RsSerialiser;

class SetServiceTester
{
public:
	SetServiceTester();
	~SetServiceTester();

	enum EventType {
		UNTIL_CAPTURE = 0,
		UNTIL_FINISH,
		UNTIL_NONE
	};

	bool addNode(const RsPeerId &peerId, std::list<RsPeerId> friendIds);
	bool addNode(const RsPeerId &peerId, PeerNode *node);

	void addSerialType(RsSerialType *st);
	bool startup();

	bool bringOnline(const RsPeerId &peerId, std::list<RsPeerId> peers);

	bool tick();
	bool tickUntilCapturedPacket(int max_ticks, uint32_t &idx);
	bool tickUntilFinish(int max_ticks);

	// return true, if we not transmit it.
	virtual bool filter(const SetPacket& packet);

	// return true, if we should save it.
	virtual bool capture(const SetPacket& packet);
	
	// return true to finish.
	virtual bool finish(const SetPacket& packet);

	uint32_t  getPacketCount();
	SetPacket &examinePacket(uint32_t idx);
	bool injectPacket(const SetPacket &pkt);

	uint32_t  getNodeCount();
	PeerNode *getPeerNode(const RsPeerId &id);

	SetFilter &getDropFilter() { return mDropFilter; }
	SetFilter &getCaptureFilter() { return mCaptureFilter; }
	SetFilter &getFinishFilter() { return mFinishFilter; }

private:

	bool tickUntilEvent(int max_ticks, EventType eventType);

	RsItem *convertToRsItem(RsRawItem *rawitem, bool toDelete);
	RsRawItem *convertToRsRawItem(RsItem *item, bool toDelete);


	time_t mRefTime;	
	std::map<RsPeerId, PeerNode *> mNodes;
	RsSerialiser *mRsSerialiser;
	std::vector<SetPacket> mPackets;

	SetFilter mDropFilter;
	SetFilter mCaptureFilter;
	SetFilter mFinishFilter;
};



