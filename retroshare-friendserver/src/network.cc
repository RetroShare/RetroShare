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
    : mFsNiMtx(std::string("FsNetworkInterface"))
{
    RS_STACK_MUTEX(mFsNiMtx);

    mClintListn = 0;
    mTotalReadBytes = 0;
    mTotalBufferBytes = 0;

    struct sockaddr_in ipOfServer;

    mClintListn = socket(AF_INET, SOCK_STREAM, 0); // creating socket

    memset(&ipOfServer, '0', sizeof(ipOfServer));

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_addr.s_addr = htonl(INADDR_ANY);
    ipOfServer.sin_port = htons(2017); // this is the port number of running server

    bind(mClintListn, (struct sockaddr*)&ipOfServer , sizeof(ipOfServer));
    listen(mClintListn , 40);

    RsDbg() << "Network interface now listening for TCP on " << sockaddr_storage_tostring( *(sockaddr_storage*)&ipOfServer) ;
}

int FsNetworkInterface::close()
{
    RsDbg() << "Stopping network interface" << std::endl;
    return 1;
}

void FsNetworkInterface::threadTick()
{
    tick();
}

int FsNetworkInterface::tick()
{
    std::cerr << "ticking FsNetworkInterface" << std::endl;

    int clintConnt = accept(mClintListn, (struct sockaddr*)NULL, NULL); // accept is a blocking call!

    char inBuffer[1025];
    memset(inBuffer,0,1025);

    int readbytes = read(clintConnt, inBuffer, sizeof(inBuffer));

    if(readbytes < 0)
        RsErr() << "read() failed. Errno=" << errno ;

    std::cerr << "clintConnt: " << clintConnt << ", readbytes: " << readbytes << std::endl;

    ::close(clintConnt);

    // display some debug info

    if(readbytes > 0)
    {
        RsDbg() << "Received the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(inBuffer),readbytes,50) << std::endl;
        RsDbg() << "Received the following bytes: " << std::string(inBuffer,readbytes) << std::endl;

        void *ptr = malloc(readbytes);

        if(!ptr)
            throw std::runtime_error("Cannot allocate memory! Go buy some RAM!");

        memcpy(ptr,inBuffer,readbytes);

        RS_STACK_MUTEX(mFsNiMtx);
        in_buffer.push_back(std::make_pair(ptr,readbytes));

        mTotalBufferBytes += readbytes;
        mTotalReadBytes += readbytes;

        RsDbg() << "InBuffer: " << in_buffer.size() << " elements. Total size: " << mTotalBufferBytes << ". Total read: " << mTotalReadBytes ;
    }

    return true;
}

int FsNetworkInterface::senddata(void *, int len)
{
    RsErr() << "Trying to send data through FsNetworkInterface although it's not implemented yet!"<< std::endl;
    return false;
}
int FsNetworkInterface::readdata(void *data, int len)
{
    RS_STACK_MUTEX(mFsNiMtx);

    // read incoming bytes in the buffer

    int total_len = 0;

    while(total_len < len)
    {
        if(in_buffer.empty())
        {
            mTotalBufferBytes -= total_len;
            return total_len;
        }

        // If the remaining buffer is too large, chop of the beginning of it.

        if(total_len + in_buffer.front().second > len)
        {
            memcpy(&(static_cast<unsigned char *>(data)[total_len]),in_buffer.front().first,len - total_len);

            void *ptr = malloc(in_buffer.front().second - (len - total_len));
            memcpy(ptr,&(static_cast<unsigned char*>(in_buffer.front().first)[len - total_len]),in_buffer.front().second - (len - total_len));

            free(in_buffer.front().first);
            in_buffer.front().first = ptr;
            in_buffer.front().second -= len-total_len;

            mTotalBufferBytes -= len;
            return len;
        }
        else // copy everything
        {
            memcpy(&(static_cast<unsigned char *>(data)[total_len]),in_buffer.front().first,in_buffer.front().second);

            total_len += in_buffer.front().second;

            free(in_buffer.front().first);
            in_buffer.pop_front();
        }
    }
    mTotalBufferBytes -= len;
    return len;
}

int FsNetworkInterface::netstatus()
{
    return 1; // dummy response.
}
int FsNetworkInterface::isactive()
{
    RS_STACK_MUTEX(mFsNiMtx);
    return mClintListn > 0;
}
bool FsNetworkInterface::moretoread(uint32_t /* usec */)
{
    RS_STACK_MUTEX(mFsNiMtx);
    return mTotalBufferBytes > 0;
}
bool FsNetworkInterface::cansend(uint32_t)
{
    return false;
}





