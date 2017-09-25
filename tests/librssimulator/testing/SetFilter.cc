
#include "SetFilter.h"

#include "rsitems/rsitem.h"

bool SetFilter::filter(const SetPacket &pkt)
{
	switch(mFilterMode)
	{
		case FILTER_NONE:
			return false;
		break;

		case FILTER_ALL:
			return true;
		break;

		case FILTER_PARAMS:
			// fall through to below.
		break;
	}

	// use params.
	if (mUseSource)
	{
        	std::set<RsPeerId>::const_iterator it;
		it = mSources.find(pkt.mSrcId);
		if (it == mSources.end())
		{
			return false;
		}
	}

	if (mUseDest)
	{
        	std::set<RsPeerId>::const_iterator it;
		it = mDests.find(pkt.mDestId);
		if (it == mDests.end())
		{
			return false;
		}
	}

	if (mUseFullTypes)
	{
        	std::set<uint32_t>::const_iterator it;
		it = mFullTypes.find(pkt.mItem->PacketId());
		if (it == mFullTypes.end())
		{
			return false;
		}
	}

	if (mUseServiceTypes)
	{
        	std::set<uint16_t>::const_iterator it;
		it = mServiceTypes.find(pkt.mItem->PacketService());
		if (it == mServiceTypes.end())
		{
			return false;
		}
	}
	return true;
}


		


