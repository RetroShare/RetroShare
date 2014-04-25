/*
 * nxsgrptestscenario.cpp
 *
 *  Created on: 23 Apr 2014
 *      Author: crispy
 */

#include "nxsgrptestscenario.h"
#include <algorithm>

namespace rs_nxs_test {

NxsGrpTestScenario::NxsGrpTestScenario() {
}

NxsGrpTestScenario::~NxsGrpTestScenario() {
}

bool NxsGrpTestScenario::checkTestPassed()
{
	const ExpectedMap& exMap = getExpectedMap();

	ExpectedMap::const_iterator mit = exMap.begin();

	bool passed = true;

	for(; mit != exMap.end(); mit++)
	{
		const RsPeerId& pid = mit->first;
		RsGxsGroupId::std_vector expGrpIds = mit->second;
		RsGeneralDataService* ds = getDataService(pid);
		RsGxsGroupId::std_vector grpIds;
		ds->retrieveGroupIds(grpIds);

		RsGxsGroupId::std_vector result(expGrpIds.size()+grpIds.size());
        std::sort(grpIds.begin(), grpIds.end());
        std::sort(expGrpIds.begin(), expGrpIds.end());
		RsGxsGroupId::std_vector::iterator it = std::set_difference(grpIds.begin(), grpIds.end(),
				expGrpIds.begin(), expGrpIds.end(), result.begin());

		result.resize(it - result.begin());

                passed &= result.size() == 0;
	}

	return passed;
}

bool NxsGrpTestScenario::checkDeepTestPassed()
{
	return false;
}

}

 /* namespace rs_nxs_test */
