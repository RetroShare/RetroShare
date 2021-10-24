/*
 * RetroShare Friend Server
 * Copyright (C) 2021-2021  retroshare team <retroshare.project@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "util/rsthreads.h"
#include "pqi/pqi_base.h"

class pqistreamer;

struct ConnectionData
{
    sockaddr client_address;
    int socket;
    pqistreamer *pqi;
};

// This class handles multiple connections to the server and supplies RsItem elements

class FsNetworkInterface: public RsTickingThread
{
public:
    FsNetworkInterface() ;
    virtual ~FsNetworkInterface() ;

    // basic functionality

    RsItem *GetItem();

    // Implements RsTickingThread

    void threadTick() override;

protected:
    bool checkForNewConnections();

private:
    RsMutex mFsNiMtx;

    void initListening();
    void stopListening();

    int mClintListn ;	// listening socket
    std::map<RsPeerId,ConnectionData> mConnections;
};






