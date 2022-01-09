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
#include "pqi/pqifdbin.h"
#include "pqi/pqiproxy.h"

bool FsClient::requestFriends(const std::string& address,uint16_t port,
                              const std::string& proxy_address,uint16_t proxy_port,
                              uint32_t reqs,std::map<std::string,bool>& friend_certificates)
{
    // send our own certificate to publish and expects response frmo the server , decrypts it and reutnrs friend list

    RsFriendServerClientPublishItem *pitem = new RsFriendServerClientPublishItem();

    pitem->n_requested_friends = reqs;

    std::string pgp_base64_string,pgp_base64_checksum,short_invite;
    rsPeers->GetPGPBase64StringAndCheckSum(rsPeers->getGPGOwnId(),pgp_base64_string,pgp_base64_checksum);

    if(!rsPeers->getShortInvite(short_invite,RsPeerId(),RetroshareInviteFlags::RADIX_FORMAT | RetroshareInviteFlags::DNS))
    {
        RsErr() << "Cannot request own short invite! Something's very wrong." ;
        return false;
    }

    pitem->pgp_public_key_b64 = pgp_base64_string;
    pitem->short_invite = short_invite;

    std::list<RsItem*> response;
    sendItem(address,port,proxy_address,proxy_port,pitem,response);

    // now decode the response

    friend_certificates.clear();

    for(auto item:response)
    {
        // auto *encrypted_response_item = dynamic_cast<RsFriendServerEncryptedServerResponseItem*>(item);

        // if(!encrypted_response_item)
        // {
        //     delete item;
        //     continue;
        // }

        // For now, also handle unencrypted response items. Will be disabled in production

        auto *response_item = dynamic_cast<RsFriendServerServerResponseItem*>(item);

        if(response_item)
            handleServerResponse(response_item);

        delete item;
    }
    return friend_certificates.size();
}

void FsClient::handleServerResponse(RsFriendServerServerResponseItem *item)
{
    std::cerr << "Received a response item from server: " << std::endl;
    std::cerr << *item << std::endl;

    //     for(const auto& it:response_item->friend_invites)
    //        friend_certificates.insert(it);
}

bool FsClient::sendItem(const std::string& server_address,uint16_t server_port,
                        const std::string& proxy_address,uint16_t proxy_port,
                        RsItem *item,std::list<RsItem*>& response)
{
    // open a connection

    RsDbg() << "Sending item to friend server at \"" << server_address << ":" << server_port << " through proxy " << proxy_address << ":" << proxy_port;

    int CreateSocket = 0;
    char dataReceived[1024];
    struct sockaddr_in ipOfServer;

    memset(dataReceived, '0' ,sizeof(dataReceived));

    if((CreateSocket = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("Socket not created \n");
        return 1;
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(proxy_port);
    ipOfServer.sin_addr.s_addr = inet_addr(proxy_address.c_str());

    if(connect(CreateSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection to proxy failed due to port and ip problems, or proxy is not available\n");
        return false;
    }

    // Now connect to the proxy

    int ret=0;
    pqiproxyconnection proxy;
    proxy.setRemoteAddress(server_address);
    proxy.setRemotePort(server_port);

    while(1 != (ret = proxy.proxy_negociate_connection(CreateSocket)))
        if(ret < 0)
        {
            RsErr() << "FriendServer client: Connection problem to the proxy!" ;
            return false;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Serialise the item and send it.

    FsSerializer *fss = new FsSerializer;
    RsSerialiser *rss = new RsSerialiser();	// deleted by ~pqistreamer()
    rss->addSerialType(fss);

    RsFdBinInterface *bio = new RsFdBinInterface(CreateSocket,true);	// deleted by ~pqistreamer()

    pqithreadstreamer p(this,rss,RsPeerId(),bio,BIN_FLAGS_READABLE | BIN_FLAGS_WRITEABLE | BIN_FLAGS_NO_CLOSE);
    p.start();

    uint32_t ss;
    p.SendItem(item,ss);

    RsDbg() << "Item sent. Waiting for response..." ;

    // Now attempt to read and deserialize anything that comes back from that connexion until it gets closed by the server.

    while(true)
    {
        p.tick(); // ticks bio

        RsItem *item = GetItem();
#ifdef DEBUG_FSCLIENT
        RsDbg() << "Ticking for response...";
#endif
        if(item)
        {
            response.push_back(item);
            std::cerr << "Got a response item: " << std::endl;
            std::cerr << *item << std::endl;

            RsDbg() << "End of transmission. " ;
            break;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    RsDbg() << "  Stopping/killing pqistreamer" ;
    p.fullstop();

    RsDbg() << "  Closing socket." ;
    close(CreateSocket);
    CreateSocket=0;

    RsDbg() << "  Exiting loop." ;

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
