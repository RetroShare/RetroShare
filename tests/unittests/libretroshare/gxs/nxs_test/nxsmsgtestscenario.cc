/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsmsgtestscenario.cc                  *
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

#include "nxsmsgtestscenario.h"

namespace rs_nxs_test {

NxsMsgTestScenario::NxsMsgTestScenario() {
	// TODO Auto-generated constructor stub

}

NxsMsgTestScenario::~NxsMsgTestScenario() {
	// TODO Auto-generated destructor stub
}

bool NxsMsgTestScenario::checkTestPassed() {

	// so we expect all peers to all messages for each group
	const ExpectedMap& exMap = getExpectedMap();

	ExpectedMap::const_iterator mit = exMap.begin();

	bool passed = true;


	for(; mit != exMap.end(); mit++)
	{
		const RsPeerId& pid = mit->first;

		const ExpectedMsgs& exMsgs = mit->second;
		ExpectedMsgs::const_iterator cit = exMsgs.begin();
		RsGeneralDataService* ds = getDataService(pid);

		for(; cit != exMsgs.end(); cit++)
		{
			const RsGxsGroupId& grpId = cit->first;
			RsGxsMessageId::std_vector expMsgIds = cit->second;
			RsGxsMessageId::std_vector msgIds;

			ds->retrieveMsgIds(grpId, msgIds);

			RsGxsMessageId::std_vector result(expMsgIds.size()+msgIds.size());

	        std::sort(msgIds.begin(), msgIds.end());
	        std::sort(expMsgIds.begin(), expMsgIds.end());

	        RsGxsMessageId::std_vector::iterator it = std::set_difference(msgIds.begin(), msgIds.end(),
					expMsgIds.begin(), expMsgIds.end(), result.begin());

			result.resize(it - result.begin());

			passed &= result.size() == 0;
		}

	}

	return passed;

}

bool NxsMsgTestScenario::checkDeepTestPassed() {
	return false;
}

void NxsMsgTestScenario::cleanTestScenario()
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
		std::string tableFile = "msg_store_" + cit->toStdString();
		remove(tableFile.c_str());
	}

}

RsGeneralDataService* NxsMsgTestScenario::createDataStore(
		const RsPeerId& peerId, uint16_t servType) {

	RsGeneralDataService* ds = new RsDataService("./", "msg_store_" +
					peerId.toStdString(), servType, NULL, "key");

	mDataPeerMap.insert(std::make_pair(peerId, ds));
	return ds;
}

}



 /* namespace rs_nxs_test */
