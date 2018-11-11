/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/gxspublishmsgtest.h                *
 *                                                                             *
 * Copyright (C) 2013, Crispy <retroshare.team@gmailcom>                       *
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

#ifndef GXSPUBLISHMSGTEST_H_
#define GXSPUBLISHMSGTEST_H_

#include "genexchangetester.h"

class GxsPublishMsgTest: public GenExchangeTest {
public:
	GxsPublishMsgTest(GenExchangeTestService* const testService,
			RsGeneralDataService* dataService);
	virtual ~GxsPublishMsgTest();

	void runTests();

    // message tests
    bool testMsgSubmissionRetrieval();
//    bool testMsgIdRetrieval();
//    bool testMsgIdRetrieval_OptParents();
//    bool testMsgIdRetrieval_OptOrigMsgId();
//    bool testMsgIdRetrieval_OptLatest();
//    bool testSpecificMsgMetaRetrieval();


};

#endif /* GXSPUBLISHMSGTEST_H_ */
