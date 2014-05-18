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
	void cleanTestScenario();

protected:

	RsGeneralDataService* createDataStore(const RsPeerId& peerId, uint16_t servType);
	virtual const ExpectedMap& getExpectedMap() = 0;

private:

	typedef std::map<RsPeerId, RsGeneralDataService*> DataPeerMap;
	DataPeerMap mDataPeerMap;
};

} /* namespace rs_nxs_test */
#endif /* NXSMSGTESTSCENARIO_H_ */
