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

	bool checkTestPassed();
	bool checkDeepTestPassed();

protected:

	RsDataService* createDataStore(const RsPeerId& peerId);
	virtual const ExpectedMap& getExpectedMap() = 0;
};

} /* namespace rs_nxs_test */
#endif /* NXSGRPTESTSCENARIO_H_ */
