/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsIsolatedServiceTester.h             *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
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

#pragma once

// from librssimulator
#include "testing/IsolatedServiceTester.h"

struct  RsGxsIdExchange;
class	RsGxsCircleExchange;
class	GxsTestService;
class 	RsGeneralDataService;
class   RsGxsNetService;

class GxsIsolatedServiceTester: public IsolatedServiceTester
{
public:

	GxsIsolatedServiceTester(const RsPeerId &ownId, const RsPeerId &friendId, std::list<RsPeerId> peers, int testMode);
	~GxsIsolatedServiceTester();

	uint32_t mTestMode;
	std::string mGxsDir;

	// Id and Circle Interfaces. (NULL for now).
	RsGxsIdExchange *mGxsIdService;
	RsGxsCircleExchange *mGxsCircles;

	GxsTestService *mTestService;
        RsGeneralDataService* mTestDs;
        RsGxsNetService* mTestNs;
};





