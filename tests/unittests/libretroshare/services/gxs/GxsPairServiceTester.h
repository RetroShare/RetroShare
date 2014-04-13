#pragma once

// from librssimulator
#include "testing/SetServiceTester.h"

class GxsPeerNode;

class GxsPairServiceTester: public SetServiceTester
{
public:

	GxsPairServiceTester(const RsPeerId &peerId1, const RsPeerId &peerId2, int testMode);
	~GxsPairServiceTester();

	void createGroup(const RsPeerId &id, const std::string &name);

        GxsPeerNode *getGxsPeerNode(const RsPeerId &id);
};





