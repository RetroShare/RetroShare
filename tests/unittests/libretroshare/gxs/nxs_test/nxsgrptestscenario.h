/*
 * nxsgrptestscenario.h
 *
 *  Created on: 23 Apr 2014
 *      Author: crispy
 */

#ifndef NXSGRPTESTSCENARIO_H_
#define NXSGRPTESTSCENARIO_H_

#include "nxstestscenario.h"

namespace rs_nxs_test {

class NxsGrpTestScenario : public NxsTestScenario {
public:

	typedef std::map<RsPeerId, RsGxsGroupId::std_vector > ExpectedMap;

	NxsGrpTestScenario();
	virtual ~NxsGrpTestScenario();

	virtual bool checkTestPassed();
	virtual bool checkDeepTestPassed();
	void cleanTestScenario();

protected:

	virtual const ExpectedMap& getExpectedMap() = 0;
	RsGeneralDataService* createDataStore(const RsPeerId& peerId, uint16_t servType);

private:

	typedef std::map<RsPeerId, RsGeneralDataService*> DataPeerMap;
	DataPeerMap mDataPeerMap;
};

} /* namespace rs_nxs_test */
#endif /* NXSGRPTESTSCENARIO_H_ */
