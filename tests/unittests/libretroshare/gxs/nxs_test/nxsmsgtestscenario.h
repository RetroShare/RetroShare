/*
 * nxsmsgtestscenario.h
 *
 *  Created on: 26 Apr 2014
 *      Author: crispy
 */

#ifndef NXSMSGTESTSCENARIO_H_
#define NXSMSGTESTSCENARIO_H_

#include "nxstestscenario.h"

namespace rs_nxs_test {

class NxsMsgTestScenario : public NxsTestScenario {
public:
	NxsMsgTestScenario();
	virtual ~NxsMsgTestScenario();

	typedef std::map<RsGxsGroupId, RsGxsMessageId::std_vector> ExpectedMsgs;
	typedef std::map<RsPeerId, ExpectedMsgs> ExpectedMap;

	bool checkTestPassed();
	bool checkDeepTestPassed();

protected:

	RsDataService* createDataStore(const RsPeerId& peerId);
	virtual const ExpectedMap& getExpectedMap() = 0;
};

} /* namespace rs_nxs_test */
#endif /* NXSMSGTESTSCENARIO_H_ */
