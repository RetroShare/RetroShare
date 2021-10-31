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
#include "pqi/pqistreamer.h"

#include "network.h"

class RsFriendServerClientRemoveItem;
class RsFriendServerClientPublishItem;

struct PeerInfo
{
    std::string short_certificate;
    rstime_t last_connection_TS;
};

class FriendServer : public RsTickingThread
{
public:
    FriendServer(const std::string& base_directory);

private:
    // overloads RsTickingThread

    virtual void threadTick() override;
    virtual void run() override;

    // Own algorithmics

    void handleClientRemove(const RsFriendServerClientRemoveItem *item);
    void handleClientPublish(const RsFriendServerClientPublishItem *item);

    void autoWash();
    void debugPrint();

    // Local members

    FsNetworkInterface *mni;

    std::string mBaseDirectory;

    std::map<RsPeerId, PeerInfo> mCurrentClientPeers;
};
