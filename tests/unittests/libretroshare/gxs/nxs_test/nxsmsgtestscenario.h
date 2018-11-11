/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsmsgtestscenario.h                   *
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
