/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsPairServiceTester.h                 *
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
#include "testing/SetServiceTester.h"
#include "gxstestservice.h"

class GxsPeerNode;

class GxsPairServiceTester: public SetServiceTester
{
public:

	GxsPairServiceTester(const RsPeerId &peerId1, const RsPeerId &peerId2, int testMode, bool useIdentityService);
	~GxsPairServiceTester();

	// Make 4 peer version.
	GxsPairServiceTester(
                        const RsPeerId &peerId1, 
                        const RsPeerId &peerId2, 
                        const RsPeerId &peerId3,
                        const RsPeerId &peerId4,
                        int testMode, 
                        bool useIdentityService);

        GxsPeerNode *getGxsPeerNode(const RsPeerId &id);
	void PrintCapturedPackets();
};





