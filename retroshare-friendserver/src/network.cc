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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "util/rsnet.h"
#include "util/rsprint.h"
#include "util/rsdebug.h"

#include "network.h"

FsNetworkInterface::FsNetworkInterface()
{
    mClintListn = 0;
    start();
}
void FsNetworkInterface::start()
{
    struct sockaddr_in ipOfServer;

    mClintListn = socket(AF_INET, SOCK_STREAM, 0); // creating socket

    memset(&ipOfServer, '0', sizeof(ipOfServer));

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_addr.s_addr = htonl(INADDR_ANY);
    ipOfServer.sin_port = htons(2017); // this is the port number of running server

    bind(mClintListn, (struct sockaddr*)&ipOfServer , sizeof(ipOfServer));
    listen(mClintListn , 20);

    RsDbg() << "Network interface now listening for TCP on " << sockaddr_storage_tostring( *(sockaddr_storage*)&ipOfServer) << std::endl;
}

int FsNetworkInterface::close()
{
    RsDbg() << "Stopping network interface" << std::endl;
    return 1;
}

int FsNetworkInterface::tick()
{
    int clintConnt = accept(mClintListn, (struct sockaddr*)NULL, NULL);

    char inBuffer[1025];
    int readbytes = read(clintConnt, inBuffer, strlen(inBuffer));

    ::close(clintConnt);

    // display some debug info

    if(readbytes > 0)
    {
        RsDbg() << "Received the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(inBuffer),readbytes,50) << std::endl;
        RsDbg() << "Received the following bytes: " << std::string(inBuffer,readbytes) << std::endl;
    }
    else
        std::this_thread::sleep_for(std::chrono::seconds(1));

    return true;
}

