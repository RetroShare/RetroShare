#pragma once

#include <list>
#include "retroshare/rsids.h"
#include "SetPacket.h"

class RsItem;

class SetFilter
{
	public:

	enum FilterMode {
		FILTER_NONE,
		FILTER_ALL,
		FILTER_PARAMS
	};

	SetFilter()
	:mFilterMode(FILTER_ALL), 
	 mUseSource(false), mUseDest(false),
	 mUseFullTypes(false), mUseServiceTypes(false) { return; }

	SetFilter(FilterMode mode)
	:mFilterMode(mode), 
	 mUseSource(false), mUseDest(false),
	 mUseFullTypes(false), mUseServiceTypes(false) { return; }

	void setFilterMode(FilterMode mode) { mFilterMode = mode; }
	void setUseSource(bool toUse) { mUseSource = toUse; }
	void setUseDest(bool toUse)   { mUseDest   = toUse; }
	void setUseFullTypes(bool toUse)  { mUseFullTypes  = toUse; }
	void setUseServiceTypes(bool toUse)  { mUseServiceTypes  = toUse; }
	void addSource(const RsPeerId &id) { mSources.insert(id); }
	void addDest(const RsPeerId &id)   { mDests.insert(id);   }
	void addFullType(const uint32_t t)     { mFullTypes.insert(t);    }
	void addServiceType(const uint16_t t)     { mServiceTypes.insert(t);    }

	virtual bool filter(const SetPacket &pkt);

	private:

        // capture parameters.
	FilterMode mFilterMode;
        bool mUseSource;
        std::set<RsPeerId> mSources;
        bool mUseDest;
        std::set<RsPeerId> mDests;
        bool mUseFullTypes;
        std::set<uint32_t> mFullTypes;
        bool mUseServiceTypes;
        std::set<uint16_t> mServiceTypes;
};


