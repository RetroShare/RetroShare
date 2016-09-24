
#include <gtest/gtest.h>

// from librssimulator

// from libretroshare
#include "serialiser/rsnxsitems.h"

// local
#include "GxsPairServiceTester.h"
#include "GxsPeerNode.h"
#include "gxstestservice.h"

/* 
 * Fancy Circle / ID tests.
 * 
 * 1) Have to create a GxsID-PgpLinked for external Circle tests... 
 * - These take some time to move + be hash checked.
 *
 * 2) Have to create Forums with Signed Msgs to make the Authors Move.
 *
 * 3) Then we create a bunch of Circles.
 *
 *
 */

/*
 * Test 1: nothing in Groups.... only sync packets.
 *
 * This test is rather slow - should speed it up.
 */

// test is currently broken, it does not go further than "Create Identities"
// probably because it does not return from peerNode1->createIdentity
// TODO: fix test
TEST(libretroshare_services, DISABLED_GxsCircles1)
//TEST(libretroshare_services, GxsCircles1)
{
	time_t starttime = time(NULL);
	RsGxsCircleId nullCircleId;
	RsGxsId	      nullAuthorId;
        std::set<RsPgpId> nullLocalMembers;
        std::list<RsGxsId> nullExtMembers;

	RsPeerId p1 = RsPeerId::random();
	RsPeerId p2 = RsPeerId::random();
	RsPeerId p3 = RsPeerId::random();
	RsPeerId p4 = RsPeerId::random();

	/* tweak ids - so that they are easy to ID. */
	((unsigned char *) p1.toByteArray())[0] = 1;
	((unsigned char *) p2.toByteArray())[0] = 2;
	((unsigned char *) p3.toByteArray())[0] = 3;
	((unsigned char *) p4.toByteArray())[0] = 4;

	std::cerr << "TEST(libretroshare_services, GxsCircles1) PeerIds";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;
	std::cerr << "Peer1 : " << p1.toStdString();
	std::cerr << std::endl;
	std::cerr << "Peer2 : " << p2.toStdString();
	std::cerr << std::endl;
	std::cerr << "Peer3 : " << p3.toStdString();
	std::cerr << std::endl;
	std::cerr << "Peer4 : " << p4.toStdString();
	std::cerr << std::endl;

	int testMode = 0;

	GxsPairServiceTester tester(p1, p2, p3, p4, testMode, true);
	GxsPeerNode *peerNode1 = tester.getGxsPeerNode(p1);
	GxsPeerNode *peerNode2 = tester.getGxsPeerNode(p2);
	GxsPeerNode *peerNode3 = tester.getGxsPeerNode(p3);
	GxsPeerNode *peerNode4 = tester.getGxsPeerNode(p4);

	// Standard Filters.

	// create Identities.
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Create Identities";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

        RsGxsId gxsId1, gxsId2, gxsId3, gxsId4;
	EXPECT_TRUE(peerNode1->createIdentity("gxsid1", true, GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, gxsId1));
	EXPECT_TRUE(peerNode2->createIdentity("gxsid2", true, GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, gxsId2));
	EXPECT_TRUE(peerNode3->createIdentity("gxsid3", true, GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, gxsId3));
	EXPECT_TRUE(peerNode4->createIdentity("gxsid4", true, GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, gxsId4));

        std::map<RsPeerId, RsGxsId> NodeIdMap;
	NodeIdMap[p1] = gxsId1;
	NodeIdMap[p2] = gxsId2;
	NodeIdMap[p3] = gxsId3;
	NodeIdMap[p4] = gxsId4;

	// create Group / Msg (for Id exchange).
	RsGxsGroupId p1GroupId1;
	RsGxsMessageId   p1g1MsgId1;
	EXPECT_TRUE(peerNode1->createGroup("p1group1", GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, nullAuthorId, p1GroupId1));
	EXPECT_TRUE(peerNode1->createMsg("p1g1msg1", p1GroupId1, gxsId1, p1g1MsgId1));
	std::cerr << "p1->group1 id: " << p1GroupId1;
	std::cerr << std::endl;
	std::cerr << "p1->group1, msgId: " << p1g1MsgId1;
	std::cerr << std::endl;

	// let Group Exchange.
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Let Groups Exchange";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	int counter = 0;
	while(counter < 30)
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::cerr << "TEST(libretroshare_services, GxsCircles1) Checking Group Exchange";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	{
	
		std::list<RsGxsGroupId> p1GroupList;
		std::list<RsGxsGroupId> p2GroupList;
		EXPECT_TRUE(peerNode1->getGroupList(p1GroupList));
		EXPECT_TRUE(peerNode2->getGroupList(p2GroupList));
		EXPECT_TRUE(p1GroupList.size() == 1);
		EXPECT_TRUE(p2GroupList.size() == 1);
		EXPECT_TRUE(p1GroupList.end() != std::find(p1GroupList.begin(), p1GroupList.end(), p1GroupId1));
		EXPECT_TRUE(p2GroupList.end() != std::find(p2GroupList.begin(), p2GroupList.end(), p1GroupId1));
	
#if 0
		std::vector<RsTestGroup> p1Groups;
		std::vector<RsTestGroup> p2Groups;
		EXPECT_TRUE(peerNode1->getGroups(p1Groups));
		EXPECT_TRUE(peerNode2->getGroups(p2Groups));
		EXPECT_TRUE(p1Groups.size() == 2);
		EXPECT_TRUE(p2Groups.size() == 2);
#endif
	
	}

	// Subscribe to Groups - so that we can get a msgs, and reply msg for 
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Subscribing to Groups";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	EXPECT_TRUE(peerNode2->subscribeToGroup(p1GroupId1, true));
	EXPECT_TRUE(peerNode3->subscribeToGroup(p1GroupId1, true));
	EXPECT_TRUE(peerNode4->subscribeToGroup(p1GroupId1, true));
	RsGxsMessageId   p2g1MsgId2;
	RsGxsMessageId   p3g1MsgId3;
	RsGxsMessageId   p4g1MsgId4;
	EXPECT_TRUE(peerNode2->createMsg("p2g1msg2", p1GroupId1, gxsId2, p2g1MsgId2));
	EXPECT_TRUE(peerNode3->createMsg("p3g1msg3", p1GroupId1, gxsId3, p3g1MsgId3));
	EXPECT_TRUE(peerNode4->createMsg("p4g1msg4", p1GroupId1, gxsId4, p4g1MsgId4));

	// let Group Exchange.
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Let Groups Exchange";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	counter = 0;
	while(counter < 30)
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	// Now we have setup the basics... GxsIds have been exchanged.
	// now create some circles....
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Creating Circles";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	std::string circleName1 = "p1c1-EC-public-p1p2p3p4";
	// Ext Group, containing everyone, shared publicly.
        RsGxsGroupId p1c1_circleId;
        std::set<RsGxsId> p1c1_members;
    p1c1_members.insert(gxsId1);
    p1c1_members.insert(gxsId2);
    p1c1_members.insert(gxsId3);
    p1c1_members.insert(gxsId4);
	EXPECT_TRUE(peerNode1->createCircle(circleName1, GXS_CIRCLE_TYPE_PUBLIC, 
			nullCircleId, nullAuthorId, nullLocalMembers, p1c1_members, p1c1_circleId));

		
	// Ext Group containing p1,p2, shared publicly.
	std::string circleName2 = "p1c2-EC-public-p1p2";
        RsGxsGroupId p1c2_circleId;
        std::set<RsGxsId> p1c2_members;
    p1c2_members.insert(gxsId1);
    p1c2_members.insert(gxsId2);
	EXPECT_TRUE(peerNode1->createCircle(circleName2, GXS_CIRCLE_TYPE_PUBLIC, 
			nullCircleId, nullAuthorId, nullLocalMembers, p1c2_members, p1c2_circleId));

	// Ext Group containing p2 (missing creator!) shared publicly.
	std::string circleName3 = "p1c3-EC-public-p2";
        RsGxsGroupId p1c3_circleId;
        std::set<RsGxsId> p1c3_members;
    p1c3_members.insert(gxsId2);
	EXPECT_TRUE(peerNode1->createCircle(circleName3, GXS_CIRCLE_TYPE_PUBLIC, 
			nullCircleId, nullAuthorId, nullLocalMembers, p1c3_members, p1c3_circleId));

	// Ext Group containing p1,p2,p3 shared SELF-REF.
	std::string circleName4 = "p1c4-EC-self-p1p2p3";
        RsGxsGroupId p1c4_circleId;
        std::set<RsGxsId> p1c4_members;
    p1c4_members.insert(gxsId1);
    p1c4_members.insert(gxsId2);
    p1c4_members.insert(gxsId3);
	EXPECT_TRUE(peerNode1->createCircle(circleName4, GXS_CIRCLE_TYPE_EXT_SELF, 
			nullCircleId, nullAuthorId, nullLocalMembers, p1c4_members, p1c4_circleId));
		
	// Ext Group containing p1,p2 shared EXT  p1c4. (p1,p2,p3).
        RsGxsCircleId constrain_circleId(p1c4_circleId.toStdString());
	std::string circleName5 = "p1c5-EC-ext-p1p2";
        RsGxsGroupId p1c5_circleId;
        std::set<RsGxsId> p1c5_members;
    p1c5_members.insert(gxsId1);
    p1c5_members.insert(gxsId2);
	EXPECT_TRUE(peerNode1->createCircle(circleName5, GXS_CIRCLE_TYPE_EXTERNAL, 
			constrain_circleId, nullAuthorId, nullLocalMembers, p1c5_members, p1c5_circleId));
		

	// Ext Group containing p1,p4 shared EXT  p1c4. (p1,p2,p3).
	// (does p4 get stuff).
	std::string circleName6 = "p1c6-EC-ext-p1p4";
        RsGxsGroupId p1c6_circleId;
        std::set<RsGxsId> p1c6_members;
    p1c6_members.insert(gxsId1);
    p1c6_members.insert(gxsId4);
	EXPECT_TRUE(peerNode1->createCircle(circleName6, GXS_CIRCLE_TYPE_EXTERNAL, 
			constrain_circleId, nullAuthorId, nullLocalMembers, p1c6_members, p1c6_circleId));
		
	std::cerr << "Circle1: " << circleName1 << " CircleId: " << p1c1_circleId.toStdString() << std::endl;
	std::cerr << "Circle2: " << circleName2 << " CircleId: " << p1c2_circleId.toStdString() << std::endl;
	std::cerr << "Circle3: " << circleName3 << " CircleId: " << p1c3_circleId.toStdString() << std::endl;
	std::cerr << "Circle4: " << circleName4 << " CircleId: " << p1c4_circleId.toStdString() << std::endl;
	std::cerr << "Circle5: " << circleName5 << " CircleId: " << p1c5_circleId.toStdString() << std::endl;
	std::cerr << "Circle6: " << circleName6 << " CircleId: " << p1c6_circleId.toStdString() << std::endl;

	counter = 0;
	while(counter < 60)
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::cerr << "TEST(libretroshare_services, GxsCircles1) Creating Groups";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

        RsGxsCircleId circleId1(p1c1_circleId.toStdString());
        RsGxsCircleId circleId2(p1c2_circleId.toStdString());
        RsGxsCircleId circleId3(p1c3_circleId.toStdString());
        RsGxsCircleId circleId4(p1c4_circleId.toStdString());
        RsGxsCircleId circleId5(p1c5_circleId.toStdString());
        RsGxsCircleId circleId6(p1c6_circleId.toStdString());

	RsGxsGroupId cgId1;
	RsGxsMessageId   cmId1;
	RsGxsGroupId cgId2;
	RsGxsMessageId   cmId2;
	RsGxsGroupId cgId3;
	RsGxsMessageId   cmId3;
	RsGxsGroupId cgId4;
	RsGxsMessageId   cmId4;
	RsGxsGroupId cgId5;
	RsGxsMessageId   cmId5;
	RsGxsGroupId cgId6;
	RsGxsMessageId   cmId6;
	EXPECT_TRUE(peerNode1->createGroup("cgId1", GXS_CIRCLE_TYPE_EXTERNAL, circleId1, nullAuthorId, cgId1));
	EXPECT_TRUE(peerNode1->createMsg("cmId1", cgId1, gxsId1, cmId1));
	EXPECT_TRUE(peerNode1->createGroup("cgId2", GXS_CIRCLE_TYPE_EXTERNAL, circleId2, nullAuthorId, cgId2));
	EXPECT_TRUE(peerNode1->createMsg("cmId2", cgId2, gxsId1, cmId2));
	EXPECT_TRUE(peerNode1->createGroup("cgId3", GXS_CIRCLE_TYPE_EXTERNAL, circleId3, nullAuthorId, cgId3));
	EXPECT_TRUE(peerNode1->createMsg("cmId3", cgId3, gxsId1, cmId3));
	EXPECT_TRUE(peerNode1->createGroup("cgId4", GXS_CIRCLE_TYPE_EXTERNAL, circleId4, nullAuthorId, cgId4));
	EXPECT_TRUE(peerNode1->createMsg("cmId4", cgId4, gxsId1, cmId4));
	EXPECT_TRUE(peerNode1->createGroup("cgId5", GXS_CIRCLE_TYPE_EXTERNAL, circleId5, nullAuthorId, cgId5));
	EXPECT_TRUE(peerNode1->createMsg("cmId5", cgId5, gxsId1, cmId5));
	EXPECT_TRUE(peerNode1->createGroup("cgId6", GXS_CIRCLE_TYPE_EXTERNAL, circleId6, nullAuthorId, cgId6));
	EXPECT_TRUE(peerNode1->createMsg("cmId6", cgId6, gxsId1, cmId6));

	// Expected Results.
	std::map<RsPeerId, std::map<RsGxsGroupId, uint32_t> > mExpectedPeerMsgs;
	std::list<RsGxsMessageId> emptyList;
	uint32_t expectedMsgCount = 0;

	// First Group - everyone is subscribed. 4 msgs + 1.
	expectedMsgCount = 5;
	mExpectedPeerMsgs[p1][cgId1] = expectedMsgCount;
	mExpectedPeerMsgs[p2][cgId1] = expectedMsgCount;
	mExpectedPeerMsgs[p3][cgId1] = expectedMsgCount;
	mExpectedPeerMsgs[p4][cgId1] = expectedMsgCount;

	// Group 2, p1 & p2. 2 msgs + 1.
	expectedMsgCount = 3;
	mExpectedPeerMsgs[p1][cgId2] = expectedMsgCount;
	mExpectedPeerMsgs[p2][cgId2] = expectedMsgCount;
	mExpectedPeerMsgs[p3][cgId2] = 0;
	mExpectedPeerMsgs[p4][cgId2] = 0;

	// Group 3, circle only has p2. but p1 creates. different counts.
	// i.e. orig not in circle.
	mExpectedPeerMsgs[p1][cgId3] = 2; // orig + own. (shouldn't receive from p2)
	mExpectedPeerMsgs[p2][cgId3] = 1; // own (shouldn't accept from p1)
	mExpectedPeerMsgs[p3][cgId3] = 0;
	mExpectedPeerMsgs[p4][cgId3] = 0;

	// Group 4, p1,p2,p3: 3 msgs + 1
	expectedMsgCount = 4;
	mExpectedPeerMsgs[p1][cgId4] = expectedMsgCount;
	mExpectedPeerMsgs[p2][cgId4] = expectedMsgCount;
	mExpectedPeerMsgs[p3][cgId4] = expectedMsgCount;
	mExpectedPeerMsgs[p4][cgId4] = 0;

	// Group 5, p1 & p2. 2 msgs + 1.
	expectedMsgCount = 3;
	mExpectedPeerMsgs[p1][cgId5] = expectedMsgCount;
	mExpectedPeerMsgs[p2][cgId5] = expectedMsgCount;
	mExpectedPeerMsgs[p3][cgId5] = 0;
	mExpectedPeerMsgs[p4][cgId5] = 0;

	// Group 6, circle has p1,p4, but onle shared to p1,p2,p3;
	// i.e p4 group has unknown circle.
	//
	// This is an interesting case...
	// unknown group - what do we do?
	mExpectedPeerMsgs[p1][cgId6] = 2; // orig + own. (shouldn't receive from p4)
	mExpectedPeerMsgs[p2][cgId6] = 0; 
	mExpectedPeerMsgs[p3][cgId6] = 0;
	mExpectedPeerMsgs[p4][cgId6] = 1; // own



	// let Group Exchange.
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Let Restricted Groups Exchange";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	counter = 0;
	while(counter < 60)
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::cerr << "TEST(libretroshare_services, GxsCircles1) Subscribe / Create Msgs.";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;
	{
		std::list<RsGxsGroupId> p2GroupList;
		std::list<RsGxsGroupId> p3GroupList;
		std::list<RsGxsGroupId> p4GroupList;
		EXPECT_TRUE(peerNode2->getGroupList(p2GroupList));
		EXPECT_TRUE(peerNode3->getGroupList(p3GroupList));
		EXPECT_TRUE(peerNode4->getGroupList(p4GroupList));

		std::cerr << "GroupId cgId1: " << cgId1.toStdString();
		std::cerr << std::endl;
		std::cerr << "GroupId cgId2: " << cgId2.toStdString();
		std::cerr << std::endl;
		std::cerr << "GroupId cgId3: " << cgId3.toStdString();
		std::cerr << std::endl;
		std::cerr << "GroupId cgId4: " << cgId4.toStdString();
		std::cerr << std::endl;
		std::cerr << "GroupId cgId5: " << cgId5.toStdString();
		std::cerr << std::endl;
		std::cerr << "GroupId cgId6: " << cgId6.toStdString();
		std::cerr << std::endl;
	}

	{
        	std::map<RsPeerId, RsGxsId>::iterator nit;
		for(nit = NodeIdMap.begin(); nit != NodeIdMap.end(); nit++)
		{
			GxsPeerNode *peerNode = tester.getGxsPeerNode(nit->first);

			std::list<RsGxsGroupId> groupList;
			std::list<RsGxsGroupId>::iterator it;
			EXPECT_TRUE(peerNode->getGroupList(groupList));

			std::cerr << "peer: " << nit->first << " has #Groups: " << groupList.size();
			std::cerr << std::endl;
			for(it = groupList.begin(); it != groupList.end(); it++)
			{
				std::cerr << "\t groupId: " << *it;
				std::cerr << std::endl;
			}
		}



		// Now we Subscribe / Create a Msg per Group that they have
		for(nit = NodeIdMap.begin(); nit != NodeIdMap.end(); nit++)
		{
			GxsPeerNode *peerNode = tester.getGxsPeerNode(nit->first);

			std::list<RsGxsGroupId> groupList;
			std::list<RsGxsGroupId>::iterator it;
			EXPECT_TRUE(peerNode->getGroupList(groupList));

			for(it = groupList.begin(); it != groupList.end(); it++)
			{
				RsGxsMessageId   msgId;
				EXPECT_TRUE(peerNode->subscribeToGroup(*it, true));
				EXPECT_TRUE(peerNode->createMsg("msg", *it, nit->second, msgId));

				std::cerr << std::endl;
				std::cerr << "Created Msg peer: " << nit->first;
				std::cerr << " GxsId: "  << nit->second; 
				std::cerr << " GroupId: " << *it;
				std::cerr << " MsgId: "   << msgId;
				std::cerr << std::endl;
			}

		}
	}

	// let Group Exchange.
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Exchange Restricted Group Messages";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	counter = 0;
	while(counter < 60)
	{
		counter++;
		tester.tick();
		sleep(1);
	}


	{
		std::cerr << "TEST(libretroshare_services, GxsCircles1) Checking Msg Count";
		std::cerr << std::endl;
		// Check messages.
		std::map<RsPeerId, std::map<RsGxsGroupId, uint32_t> >::iterator nit;
		for(nit = mExpectedPeerMsgs.begin(); nit != mExpectedPeerMsgs.end(); nit++)
		{
			std::cerr << "Node Id: " << nit->first;
			std::cerr << std::endl;
			GxsPeerNode *peerNode = tester.getGxsPeerNode(nit->first);
			
			std::map<RsGxsGroupId, uint32_t> &groupMap = nit->second;
			std::map<RsGxsGroupId, uint32_t>::iterator git;
			std::list<RsGxsGroupId> groupList;
			std::list<RsGxsGroupId>::iterator lit;
			EXPECT_TRUE(peerNode->getGroupList(groupList));

			for(git = groupMap.begin(); git != groupMap.end(); git++)
			{
				if (git->second)
				{
					/* expect to find it in groupList */
					lit = std::find(groupList.begin(), groupList.end(), git->first);
					EXPECT_TRUE(lit != groupList.end());
					if (lit != groupList.end())
					{
						/* check message count */
						std::list<RsGxsMessageId> msgList;
						peerNode->getMsgList(git->first, msgList);
						EXPECT_TRUE(msgList.size() == git->second);

						std::cerr << "\t GroupId: " << git->first;
						std::cerr << " Expected Msg Count: " << git->second;
						std::cerr << " Actual Msg Count: " << msgList.size();
						std::cerr << std::endl;

						std::list<RsGxsMessageId>::iterator mit;
						for(mit = msgList.begin(); mit != msgList.end(); mit++)
						{
							std::cerr << "\t\tMsgId: " << *mit;
							std::cerr << std::endl;
						}
					}
					else
					{
						std::cerr << "\t GroupId: " << git->first;
						std::cerr << " Error Expected but missing";
						std::cerr << std::endl;
					}
				}
				else
				{
					lit = std::find(groupList.begin(), groupList.end(), git->first);
					EXPECT_TRUE(lit == groupList.end());

					if (lit == groupList.end())
					{
						std::cerr << "\t GroupId: " << git->first;
						std::cerr << " Not present - as expected";
						std::cerr << std::endl;
					}
					else
					{
						std::cerr << "\t GroupId: " << git->first;
						std::cerr << " ERROR - not expected";
						std::cerr << std::endl;
					}
				}
			}
		}
	}

	std::cerr << "TEST(libretroshare_services, GxsCircles1) Test Finished";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

// Below here is extra checks - which require rsgxsnetservice to be overloaded by RsGxsNetServiceTester
// needs private->protected.
#if 0
	std::cerr << "TEST(libretroshare_services, GxsCircles1) Checking Grp Availability";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;

	{
        	std::map<RsPeerId, RsGxsId>::const_iterator nit;
		for(nit = NodeIdMap.begin(); nit != NodeIdMap.end(); nit++)
		{
			GxsPeerNode *peerNode = tester.getGxsPeerNode(nit->first);
			std::cerr << "=============================";
			std::cerr << " PeerId: " << nit->first;
			std::cerr << " & GxsId: " << nit->second;
			std::cerr << std::endl;

			std::map<RsPeerId, RsGxsId>::const_iterator nit2;
			for(nit2 = NodeIdMap.begin(); nit2 != NodeIdMap.end(); nit2++)
			{
				if (nit->first == nit2->first)
				{
					continue;
				}

				std::cerr << "-----------------------";
				std::cerr << "PeerId: " << nit->first;
				std::cerr << " TestService checkAllowed(" << nit2->first;
				std::cerr << ")";
				std::cerr << std::endl;

				peerNode->checkTestServiceAllowedGroups(nit2->first);

				std::cerr << "-----------------------";
				std::cerr << "PeerId: " << nit->first;
				std::cerr << " CircleService checkAllowed(" << nit2->first;
				std::cerr << ")";
				std::cerr << std::endl;

				peerNode->checkCircleServiceAllowedGroups(nit2->first);
			}
		}
	}

	std::cerr << "TEST(libretroshare_services, GxsCircles1) Finished - Printing Captured Packets";
	std::cerr << std::endl;
	std::cerr << "TIME TO GET HERE: " << time(NULL) - starttime;
	std::cerr << std::endl;
#endif

	// no need to print packets.
	//tester.PrintCapturedPackets();

}

