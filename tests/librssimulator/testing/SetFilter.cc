/*******************************************************************************
 * librssimulator/testing/: SetFilter.cc                                       *
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


		


