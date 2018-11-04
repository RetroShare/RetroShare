/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsgrptestscenario.h                   *
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
