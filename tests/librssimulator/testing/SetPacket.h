#pragma once

#include "pqi/p3linkmgr.h" // for RsPeerId ??

#include <list>
#include "retroshare/rsids.h"

class RsItem;

class SetPacket
{
public:
	SetPacket(double t, const RsPeerId &srcId, 
		const RsPeerId &destId, RsItem *item)
	:mTime(t), mSrcId(srcId), mDestId(destId), mItem(item)
	{
		return;
	}
		
	SetPacket() :mTime(), mItem(NULL) { return; }
		
	double mTime; // relative to test creation;
	RsPeerId mSrcId;
	RsPeerId mDestId;
	RsItem  *mItem;
};

