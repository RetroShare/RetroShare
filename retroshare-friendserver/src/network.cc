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

#ifdef WINDOWS_SYS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#endif

#include "util/rsnet.h"
#include "util/rsprint.h"
#include "util/rsdebug.h"

#include "pqi/pqithreadstreamer.h"
#include "pqi/pqifdbin.h"

#include "network.h"
#include "friend_server/fsitem.h"

FsNetworkInterface::FsNetworkInterface(const std::string& listening_address,uint16_t listening_port)
    : PQInterface(RsPeerId()),mFsNiMtx(std::string("FsNetworkInterface")),mListeningAddress(listening_address),mListeningPort(listening_port)
{
    RS_STACK_MUTEX(mFsNiMtx);

    mClintListn = 0;
    mClintListn = socket(AF_INET, SOCK_STREAM, 0); // creating socket

    int flags=1;
    setsockopt(mClintListn,SOL_SOCKET,TCP_NODELAY,(char*)&flags,sizeof(flags));

    unix_fcntl_nonblock(mClintListn);

    struct sockaddr_in ipOfServer;
    memset(&ipOfServer, '0', sizeof(ipOfServer));

    assert(mListeningPort > 1024);

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(mListeningPort); // this is the port number of running server

    int addr[4];
    if(sscanf(listening_address.c_str(),"%d.%d.%d.%d",&addr[0],&addr[1],&addr[2],&addr[3]) != 4)
        throw std::runtime_error("Cannot parse a proper IPv4 address in \""+listening_address+"\"");

    for(int i=0;i<4;++i)
        if(addr[i] < 0 || addr[i] > 255)
            throw std::runtime_error("Cannot parse a proper IPv4 address in \""+listening_address+"\"");

    ipOfServer.sin_addr.s_addr = htonl( (addr[0] << 24) + (addr[1] << 16) + (addr[2] << 8) + addr[3] );

    if(bind(mClintListn, (struct sockaddr*)&ipOfServer , sizeof(ipOfServer)) < 0)
    {
        RsErr() << "Error while binding: errno=" << errno ;
        return;
    }

    if(listen(mClintListn , 40) < 0)
    {
        RsErr() << "Error while calling listen: errno=" << errno ;
        return;
    }

    RsDbg() << "Network interface now listening for TCP on " << sockaddr_storage_tostring( *(sockaddr_storage*)&ipOfServer) ;
}

FsNetworkInterface::~FsNetworkInterface()
{
    RS_STACK_MUTEX(mFsNiMtx);
    for(auto& it:mConnections)
    {
        delete it.second.pqi_thread;
        std::cerr << "Releasing socket " << it.second.socket << std::endl;
        close(it.second.socket);
    }
    std::cerr << "Releasing listening socket " << mClintListn << std::endl;
    close(mClintListn);
}
void FsNetworkInterface::threadTick()
{
    // 1 - check for new connections

    checkForNewConnections();

    // 2 - tick all streamers

    std::list<RsPeerId> to_close;

    {
        RS_STACK_MUTEX(mFsNiMtx);
        for(auto& it:mConnections)
        {
            it.second.pqi_thread->tick();

            if(!it.second.bio->isactive() && !it.second.bio->moretoread(0))
                to_close.push_back(it.first);
        }

        for(const auto& pid:to_close)
            locked_closeConnection(pid);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

static RsPeerId makePeerId(int t)
{
    unsigned char s[RsPeerId::SIZE_IN_BYTES];
    memset(s,0,sizeof(s));

    *reinterpret_cast<int*>(&s) = t;
    return RsPeerId::fromBufferUnsafe(s);
}
bool FsNetworkInterface::checkForNewConnections()
{
    // look for incoming data

    struct sockaddr addr;
    socklen_t addr_len = sizeof(sockaddr);

    int clintConnt = accept(mClintListn, &addr, &addr_len); // accept is a blocking call!

    if(clintConnt < 0)
    {
        int err = rs_socket_error();

        if(err == EWOULDBLOCK || err == EAGAIN)
            ;//RsErr()<< "Incoming connection with nothing to read!" << std::endl;
        else
            RsErr()<< "Error when accepting connection." << std::endl;

        return false;
    }
    RsDbg() << "Got incoming connection from " << sockaddr_storage_tostring( *(sockaddr_storage*)&addr);

    // Make the socket non blocking so that we can read from it and return if nothing comes

    int flags=1;
    setsockopt(clintConnt,SOL_SOCKET,TCP_NODELAY,(char*)&flags,sizeof(flags));

    unix_fcntl_nonblock(clintConnt);

    // Create connection info

    RsDbg() << "  Creating connection data." ;

    ConnectionData c;
    c.socket = clintConnt;
    c.client_address = addr;
    RsPeerId pid = makePeerId(clintConnt);

    RsDbg() << "  socket: " << clintConnt;
    RsDbg() << "  client address: " <<  sockaddr_storage_tostring(*(sockaddr_storage*)&addr);
    RsDbg() << "  peer id: " << pid ;

    // Setup a pqistreamer to deserialize whatever comes from this connection

    RsSerialiser *rss = new RsSerialiser ;
    rss->addSerialType(new FsSerializer) ;

    RsFdBinInterface *bio = new RsFdBinInterface(clintConnt,true);

    auto pqi = new pqithreadstreamer(this,rss, pid, bio,BIN_FLAGS_READABLE | BIN_FLAGS_WRITEABLE);

    c.pqi_thread = pqi;
    c.bio = bio;

    {
        RS_STACK_MUTEX(mFsNiMtx);
        mConnections[pid] = c;

        pqi->start();
    }

    RsDbg() << "  streamer has properly started." ;
    return true;
}

bool FsNetworkInterface::RecvItem(RsItem *item)
{
    RS_STACK_MUTEX(mFsNiMtx);

    RsDbg() << "FsNetworkInterface: received item " << (void*)item;

    auto it = mConnections.find(item->PeerId());

    if(it == mConnections.end())
    {
        RsErr() << "Receiving an item for peer ID " << item->PeerId() << " but no connection is known for that peer." << std::endl;
        delete item;
        return false;
    }

    it->second.incoming_items.push_back(item);
    return true;
}

RsItem *FsNetworkInterface::GetItem()
{
    RS_STACK_MUTEX(mFsNiMtx);

    for(auto& it:mConnections)
    {
        if(!it.second.incoming_items.empty())
        {
            RsItem *item = it.second.incoming_items.front();
            it.second.incoming_items.pop_front();

            RsDbg() << "FsNetworkInterface: returning item " << (void*)item << " to caller.";
            return item;
        }
    }
    return nullptr;
}

int FsNetworkInterface::SendItem(RsItem *item)
{
    RS_STACK_MUTEX(mFsNiMtx);

    const auto& it = mConnections.find(item->PeerId());

    if(it == mConnections.end())
    {
        RsErr() << "Cannot send item to peer " << item->PeerId() << ": no pending sockets available." ;
        delete item;
        return 0;
    }

    uint32_t ss;
    return it->second.pqi_thread->SendItem(item,ss);
}

void FsNetworkInterface::closeConnection(const RsPeerId& peer_id)
{
    RS_STACK_MUTEX(mFsNiMtx);

    locked_closeConnection(peer_id);
}
void FsNetworkInterface::locked_closeConnection(const RsPeerId& peer_id)
{
    RsDbg() << "Closing connection to virtual peer " << peer_id ;

    const auto& it = mConnections.find(peer_id);

    if(it == mConnections.end())
    {
        RsErr() << "  Cannot close connection to peer " << peer_id << ": no pending sockets available." ;
        return;
    }

    if(!it->second.incoming_items.empty())
    {
        RsErr() << "  Trying to close an incoming connection with incoming items still pending! The items will be lost:" << std::endl;

        for(auto& item:it->second.incoming_items)
        {
            RsErr() << *item;
            delete item;
        }

        it->second.incoming_items.clear();
    }
    // Close the socket and delete everything.

    close(it->second.socket);
    it->second.pqi_thread->fullstop();
    it->second.bio->close();

    delete it->second.pqi_thread;

    mConnections.erase(it);
}

void FsNetworkInterface::debugPrint()
{
    RsDbg() << "    " << mClintListn ;	// listening socket
    RsDbg() << "    Connections: " << mConnections.size() ;

    for(auto& it:mConnections)
        RsDbg() << "      " << it.first << ": from \"" << sockaddr_storage_tostring(*(sockaddr_storage*)(&it.second.client_address)) << "\", socket=" << it.second.socket ;

    std::map<RsPeerId,ConnectionData> mConnections;
}














