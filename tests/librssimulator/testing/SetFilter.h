/*******************************************************************************
 * librssimulator/testing/: SetFilter.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
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
#pragma once

#include <list>
#include "retroshare/rsids.h"
#include "SetPacket.h"

struct RsItem;

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


