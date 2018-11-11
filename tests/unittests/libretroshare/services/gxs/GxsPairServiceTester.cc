/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsPairServiceTester.cc                *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
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

// local
#include "GxsPairServiceTester.h"
#include "GxsPeerNode.h"
#include "gxstestservice.h"

// libretroshare
#include "rsitems/rsnxsitems.h"

GxsPairServiceTester::GxsPairServiceTester(const RsPeerId &peerId1, const RsPeerId &peerId2, int testMode, bool useIdentityService)
	:SetServiceTester()
{ 
	// setup stuff.
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSID));
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSCIRCLE));
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_TEST));

	std::list<RsPeerId> friendList1, friendList2;
	friendList1.push_back(peerId2);
	friendList2.push_back(peerId1);
	
	GxsPeerNode *n1 = new GxsPeerNode(peerId1, friendList1, testMode, useIdentityService);
	GxsPeerNode *n2 = new GxsPeerNode(peerId2, friendList2, testMode, useIdentityService);

	addNode(peerId1, n1);
	addNode(peerId2, n2);

	startup();
	tick();

	bringOnline(peerId1, friendList1);
	bringOnline(peerId2, friendList2);
}


GxsPairServiceTester::GxsPairServiceTester(
			const RsPeerId &peerId1, 
			const RsPeerId &peerId2, 
			const RsPeerId &peerId3, 
			const RsPeerId &peerId4, 
			int testMode, 
			bool useIdentityService)

	:SetServiceTester()
{ 
	// setup stuff.
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSID));
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_GXSCIRCLE));
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_TEST));

	std::list<RsPeerId> friendList1, friendList2, friendList3, friendList4;
	friendList1.push_back(peerId2);
	friendList1.push_back(peerId3);
	friendList1.push_back(peerId4);

	friendList2.push_back(peerId1);
	friendList2.push_back(peerId3);
	friendList2.push_back(peerId4);

	friendList3.push_back(peerId1);
	friendList3.push_back(peerId2);
	friendList3.push_back(peerId4);

	friendList4.push_back(peerId1);
	friendList4.push_back(peerId2);
	friendList4.push_back(peerId3);
	
	GxsPeerNode *n1 = new GxsPeerNode(peerId1, friendList1, testMode, useIdentityService);
	GxsPeerNode *n2 = new GxsPeerNode(peerId2, friendList2, testMode, useIdentityService);
	GxsPeerNode *n3 = new GxsPeerNode(peerId3, friendList3, testMode, useIdentityService);
	GxsPeerNode *n4 = new GxsPeerNode(peerId4, friendList4, testMode, useIdentityService);

	addNode(peerId1, n1);
	addNode(peerId2, n2);
	addNode(peerId3, n3);
	addNode(peerId4, n4);

	startup();
	tick();

	bringOnline(peerId1, friendList1);
	bringOnline(peerId2, friendList2);
	bringOnline(peerId3, friendList3);
	bringOnline(peerId4, friendList4);
}


GxsPairServiceTester::~GxsPairServiceTester()
{
	return;
}
	

GxsPeerNode *GxsPairServiceTester::getGxsPeerNode(const RsPeerId &id)
{
	return (GxsPeerNode *) getPeerNode(id);
}


void GxsPairServiceTester::PrintCapturedPackets()
{
	std::cerr << "==========================================================================================";
	std::cerr << std::endl;
	std::cerr << "#Packets: " << getPacketCount();
	std::cerr << std::endl;

	for(uint32_t i = 0; i < getPacketCount(); i++)
	{
		SetPacket &pkt = examinePacket(i);

		std::cerr << "==========================================================================================";
		std::cerr << std::endl;
		std::cerr << "Time: " << pkt.mTime;
		std::cerr << " From: " << pkt.mSrcId.toStdString() << " To: " << pkt.mDestId.toStdString();
		std::cerr << std::endl;
		std::cerr << "-----------------------------------------------------------------------------------------";
		std::cerr << std::endl;
		pkt.mItem->print(std::cerr);
		std::cerr << "==========================================================================================";
		std::cerr << std::endl;
	}
}

