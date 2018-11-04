/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsgrptestscenario.cc                  *
 *                                                                             *
 * Copyright (C) 2014, Crispy <retroshare.team@gmailcom>                       *
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
		RsGxsGroupId::std_vector::iterator it = std::set_symmetric_difference(grpIds.begin(), grpIds.end(),
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

void NxsGrpTestScenario::cleanTestScenario()
{
	DataPeerMap::iterator mit = mDataPeerMap.begin();
	RsPeerId::std_vector peerIds;

	// remove grps and msg data
	for(; mit != mDataPeerMap.end(); mit++)
	{
		mit->second->resetDataStore();
		peerIds.push_back(mit->first);
	}

	// now delete tables
	RsPeerId::std_vector::const_iterator cit = peerIds.begin();

	for(; cit != peerIds.end(); cit++)
	{
		std::string tableFile = "grp_store_" + cit->toStdString();
		remove(tableFile.c_str());
	}
}

RsGeneralDataService* NxsGrpTestScenario::createDataStore(
		const RsPeerId& peerId, uint16_t servType)
{
	RsGeneralDataService* ds = new RsDataService("./", "grp_store_" +
					peerId.toStdString(), servType, NULL, "key");

	mDataPeerMap.insert(std::make_pair(peerId, ds));
	return ds;
}

}



 /* namespace rs_nxs_test */
