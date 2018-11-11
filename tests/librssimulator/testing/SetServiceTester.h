/*******************************************************************************
 * librssimulator/testing/: SetServiceTester.h                                 *
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



