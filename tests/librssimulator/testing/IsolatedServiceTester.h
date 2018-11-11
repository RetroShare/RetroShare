/*******************************************************************************
 * librssimulator/testing/: IsolatedServiceTester.h                            *
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



