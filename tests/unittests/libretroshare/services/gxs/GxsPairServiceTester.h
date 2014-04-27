#pragma once

// from librssimulator
#include "testing/SetServiceTester.h"
#include "gxstestservice.h"

class GxsPeerNode;

class GxsPairServiceTester: public SetServiceTester
{
public:

	GxsPairServiceTester(const RsPeerId &peerId1, const RsPeerId &peerId2, int testMode, bool useIdentityService);
	~GxsPairServiceTester();

	// Make 4 peer version.
	GxsPairServiceTester(
                        const RsPeerId &peerId1, 
                        const RsPeerId &peerId2, 
                        const RsPeerId &peerId3,
                        const RsPeerId &peerId4,
                        int testMode, 
                        bool useIdentityService);

        GxsPeerNode *getGxsPeerNode(const RsPeerId &id);
	void PrintCapturedPackets();
};





