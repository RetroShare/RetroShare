/*******************************************************************************
 * librssimulator/testing/: SetPacket.h                                        *
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

#include "pqi/p3linkmgr.h" // for RsPeerId ??

#include <list>
#include "retroshare/rsids.h"

struct RsItem;

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

