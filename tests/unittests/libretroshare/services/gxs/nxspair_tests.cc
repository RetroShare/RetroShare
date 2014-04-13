
#include <gtest/gtest.h>

// from librssimulator

// from libretroshare
#include "serialiser/rsnxsitems.h"

// local
#include "GxsPairServiceTester.h"
#include "gxstestservice.h"

/* Here we want to test the most basic NXS interactions.
 * we have a GXS service, with no AuthorIDs or Circles.
 *
 */

/*
 * Test 1: nothing in Groups.... only sync packets.
 *
 * This test is rather slow - should speed it up.
 */

TEST(libretroshare_services, GxsNxsPairExchange1)
{
	RsPeerId p1 = RsPeerId::random();
	RsPeerId p2 = RsPeerId::random();
	int testMode = 0;

	GxsPairServiceTester tester(p1, p2, testMode);

	// we only care about the transaction going one way ...
	// so drop SyncGrp packets from p2 -> p1.

	SetFilter &dropFilter = tester.getDropFilter();
	dropFilter.setFilterMode(SetFilter::FILTER_PARAMS);
	dropFilter.setUseSource(true);
	dropFilter.addSource(p2);
	{
		RsNxsSyncGrp *syncGrp = new RsNxsSyncGrp(RS_SERVICE_GXS_TYPE_TEST);
		dropFilter.setUseFullTypes(true);
		dropFilter.addFullType(syncGrp->PacketId());
	}

	// these are currently slow operations.
	tester.createGroup(p2, "group1");
	tester.createGroup(p2, "group2");

	int counter = 0;
	while((counter < 60))
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::cerr << "==========================================================================================";
	std::cerr << std::endl;
	std::cerr << "#Packets: " << tester.getPacketCount();
	std::cerr << std::endl;

	for(int i = 0; i < tester.getPacketCount(); i++)
	{
		SetPacket &pkt = tester.examinePacket(i);

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


