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

#include "pqi/pqithreadstreamer.h"

#include "network.h"
#include "fsitem.h"

FsNetworkInterface::FsNetworkInterface()
    : mFsNiMtx(std::string("FsNetworkInterface"))
{
    RS_STACK_MUTEX(mFsNiMtx);

    mClintListn = 0;

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

void FsNetworkInterface::threadTick()
{
    // 1 - check for new connections

    checkForNewConnections();

    // 2 - tick all streamers

    for(auto& it:mConnections)
        it.second.pqi->tick();
}

static RsPeerId makePeerId(int t)
{
    unsigned char s[RsPeerId::SIZE_IN_BYTES];
    *reinterpret_cast<int*>(&s) = t;
    return RsPeerId::fromBufferUnsafe(s);
}
bool FsNetworkInterface::checkForNewConnections()
{
    // look for incoming data

//    fd_set ReadFDs, WriteFDs, ExceptFDs;
//    FD_ZERO(&ReadFDs);
//    FD_ZERO(&WriteFDs);
//    FD_ZERO(&ExceptFDs);
//
//    FD_SET(mClintListn, &ReadFDs);
//    FD_SET(mClintListn, &ExceptFDs);
//
//    struct timeval timeout;
//    timeout.tv_sec = 0;
//    timeout.tv_usec = 500000; // 200 ms timeout
//    int status = select(mClintListn+1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout);
//
//    if(status <= 0)		// if no incoming data, return. Each tick waits for 200 ms anyway.
//        return false;

    struct sockaddr addr;

    //socklen_t addr_len = sizeof(sockaddr);
    //int clintConnt = accept(mClintListn, &addr, &addr_len); // accept is a blocking call!
    int clintConnt = accept(mClintListn, nullptr, nullptr); // accept is a blocking call!

    if(clintConnt < 0)
    {
        RsErr()<< "Incoming connection with nothing to read!" << std::endl;
        return false;
    }
    RsDbg() << "Got incoming connection from " << sockaddr_storage_tostring( *(sockaddr_storage*)&addr);

    ConnectionData c;
    c.socket = clintConnt;
    c.client_address = addr;

    RsPeerId pid = makePeerId(clintConnt);

    RsSerialiser *rss = new RsSerialiser ;
    rss->addSerialType(new FsSerializer) ;

    FsBioInterface *bio = new FsBioInterface(clintConnt);

    auto p = new pqistreamer(rss, pid, bio,BIN_FLAGS_READABLE | BIN_FLAGS_WRITEABLE);
    auto pqi = new pqithreadstreamer(p,rss, pid, bio,BIN_FLAGS_READABLE | BIN_FLAGS_WRITEABLE);
    c.pqi = pqi;

    pqi->start();

    RS_STACK_MUTEX(mFsNiMtx);
    mConnections[makePeerId(clintConnt)] = c;

    return true;
}

RsItem *FsNetworkInterface::GetItem()
{
    RS_STACK_MUTEX(mFsNiMtx);

    for(auto& it:mConnections)
    {
        RsItem *item = it.second.pqi->GetItem();
        if(item)
            return item;
    }
    return nullptr;
}

FsBioInterface::FsBioInterface(int socket)
    : mCLintConnt(socket)
{
    mTotalReadBytes=0;
    mTotalBufferBytes=0;
}

int FsBioInterface::tick()
{
    std::cerr << "ticking FsNetworkInterface" << std::endl;

    // 2 - read incoming data pending on existing connections

    char inBuffer[1025];
    memset(inBuffer,0,1025);

    int readbytes = read(mCLintConnt, inBuffer, sizeof(inBuffer));

    if(readbytes < 0)
        RsErr() << "read() failed. Errno=" << errno ;

    std::cerr << "clintConnt: " << mCLintConnt << ", readbytes: " << readbytes << std::endl;

    //::close(clintConnt);

    // display some debug info

    if(readbytes > 0)
    {
        RsDbg() << "Received the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(inBuffer),readbytes,50) << std::endl;
        RsDbg() << "Received the following bytes: " << std::string(inBuffer,readbytes) << std::endl;

        void *ptr = malloc(readbytes);

        if(!ptr)
            throw std::runtime_error("Cannot allocate memory! Go buy some RAM!");

        memcpy(ptr,inBuffer,readbytes);

        in_buffer.push_back(std::make_pair(ptr,readbytes));
        mTotalBufferBytes += readbytes;

        mTotalReadBytes += readbytes;

        std::cerr << "Total bytes:" << mTotalReadBytes << std::endl;
        std::cerr << "Connections:" << std::endl;

        RsDbg() << "socket: " << mCLintConnt << ". Total read: " << mTotalReadBytes << ". Buffer size: " << mTotalBufferBytes << std::endl ;
    }

    return true;
}

int FsBioInterface::readdata(void *data, int len)
{
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

int FsBioInterface::senddata(void *data, int len)
{
    return isactive();
}
int FsBioInterface::netstatus()
{
    return 1; // dummy response.
}
int FsBioInterface::isactive()
{
    return mCLintConnt > 0;
}
bool FsBioInterface::moretoread(uint32_t /* usec */)
{
    return mTotalBufferBytes > 0;
}
bool FsBioInterface::cansend(uint32_t)
{
    return isactive();
}

int FsBioInterface::close()
{
    RsDbg() << "Stopping network interface" << std::endl;
    return 1;
}


FsClient::FsClient(const std::string& address)
    : mServerAddress(address)
{
}

bool FsClient::sendItem(RsItem *item)
{
    // request a connection
    int CreateSocket = 0,n = 0;
    char dataReceived[1024];
    struct sockaddr_in ipOfServer;

    memset(dataReceived, '0' ,sizeof(dataReceived));

    if((CreateSocket = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("Socket not created \n");
        return 1;
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(2017);
    ipOfServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(CreateSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection failed due to port and ip problems\n");
        return 1;
    }

    while((n = read(CreateSocket, dataReceived, sizeof(dataReceived)-1)) > 0)
    {
        dataReceived[n] = 0;
        if(fputs(dataReceived, stdout) == EOF)
        {
            printf("\nStandard output error");
        }

        printf("\n");
    }

    if( n < 0)
    {
        printf("Standard input error \n");
    }

    return 0;

    // if ok, stream the item through it
}
