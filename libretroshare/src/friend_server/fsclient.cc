/*******************************************************************************
 * libretroshare/src/file_sharing: fsclient.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2021 by retroshare team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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

#include "pqi/pqithreadstreamer.h"
#include "retroshare/rspeers.h"

#include "fsclient.h"
#include "fsbio.h"

bool FsClient::requestFriends(const std::string& address,uint16_t port,uint32_t reqs,std::map<std::string,bool>& friend_certificates)
{
    // send our own certificate to publish and expects response frmo the server , decrypts it and reutnrs friend list

    RsFriendServerClientPublishItem *pitem = new RsFriendServerClientPublishItem();

    pitem->n_requested_friends = reqs;
    pitem->long_invite = rsPeers->GetRetroshareInvite();

    std::list<RsItem*> response;

    sendItem(address,port,pitem,response);

    // now decode the response

    friend_certificates.clear();

    for(auto item:response)
    {
        auto *encrypted_response_item = dynamic_cast<RsFriendServerEncryptedServerResponseItem*>(item);

        if(!encrypted_response_item)
        {
            delete item;
            continue;
        }

        // For now, also handle unencrypted response items. Will be disabled in production

        auto *response_item = dynamic_cast<RsFriendServerServerResponseItem*>(item);

        if(!response_item)
        {
            delete item;
            continue;
        }

        for(const auto& it:response_item->friend_invites)
            friend_certificates.insert(it);
    }
    return friend_certificates.size();
}

bool FsClient::sendItem(const std::string& address,uint16_t port,RsItem *item,std::list<RsItem*>& response)
{
    // open a connection

    RsDbg() << "Sending item to friend server at \"" << address << ":" << port ;

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
    ipOfServer.sin_port = htons(port);
    ipOfServer.sin_addr.s_addr = inet_addr(address.c_str());

    if(connect(CreateSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection failed due to port and ip problems, or server is not available\n");
        return false;
    }

    // Serialise the item and send it.

    uint32_t size = RsSerialiser::MAX_SERIAL_SIZE;
    RsTemporaryMemory data(size);

    if(!data)
    {
        RsErr() << "Cannot allocate memory to send item!" << std::endl;
        return false;
    }

    FsSerializer *fss = new FsSerializer;
    RsSerialiser *rss = new RsSerialiser();	// deleted by ~pqistreamer()
    rss->addSerialType(fss);

    FsSerializer().serialise(item,data,&size);
    write(CreateSocket,data,size);				// shouldn't we use the pqistreamer in R/W mode instead?

    RsDbg() << "Item sent. Waiting for response..." ;

    // Now attempt to read and deserialize anything that comes back from that connexion until it gets closed by the server.

    FsBioInterface *bio = new FsBioInterface(CreateSocket);	// deleted by ~pqistreamer()

    pqithreadstreamer p(this,rss,RsPeerId(),bio,BIN_FLAGS_READABLE | BIN_FLAGS_NO_DELETE | BIN_FLAGS_NO_CLOSE);
    p.start();

    uint32_t ss;
    p.SendItem(item,ss);

    while(true)
    {
        p.tick();

        RsItem *item = GetItem();

        if(item)
        {
            response.push_back(item);
            std::cerr << "Got a response item: " << std::endl;
            std::cerr << *item << std::endl;
        }

        if(!bio->isactive())	// socket has probably closed
        {
            RsDbg() << "(client side) Socket has been closed by server.";
            RsDbg() << "  Stopping/killing pqistreamer" ;
            p.fullstop();

            RsDbg() << "  Closing socket." ;
            close(CreateSocket);
            CreateSocket=0;

            RsDbg() << "  Exiting loop." ;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return true;
}

bool FsClient::RecvItem(RsItem *item)
{
    mIncomingItems.push_back(item);
    return true;
}

RsItem *FsClient::GetItem()
{
    if(mIncomingItems.empty())
        return nullptr;

    RsItem *item = mIncomingItems.front();
    mIncomingItems.pop_front();

    return item;
}
