/*
 * libretroshare/src/chat: distantchat.h
 *
 * Services for RetroShare.
 *
 * Copyright 2014 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#pragma once

#include <turtle/turtleclientservice.h>
#include <chat/rschatitems.h>
#include <retroshare/rsmsgs.h>


class p3IdService ;

static const uint32_t DISTANT_CHAT_AES_KEY_SIZE = 16 ;

class DistantChatService: public RsTurtleClientService
{
public:
    DistantChatService(p3IdService *pids)
            : mIdService(pids), mDistantChatMtx("distant chat")
    {
        mTurtle = NULL ;
    }

    void flush() ;

    virtual void connectToTurtleRouter(p3turtle *) ;

    // Creates the invite if the public key of the distant peer is available.
    // Om success, stores the invite in the map above, so that we can respond to tunnel requests.
    //
    bool initiateDistantChatConnexion(const RsGxsId& to_gxs_id,const RsGxsId &from_gxs_id, uint32_t &error_code) ;
    bool closeDistantChatConnexion(const RsGxsId& pid) ;
    virtual bool getDistantChatStatus(const RsGxsId &gxs_id,uint32_t &status, RsGxsId *from_gxs_id=NULL) ;

    // derived in p3ChatService
    virtual void handleIncomingItem(RsItem *) = 0;
    virtual bool handleRecvChatMsgItem(RsChatMsgItem *ci)=0 ;

    bool handleOutgoingItem(RsChatItem *) ;
    bool handleRecvItem(RsChatItem *) ;
    void handleRecvChatStatusItem(RsChatStatusItem *cs) ;

private:
    class DistantChatPeerInfo
    {
    public:
        DistantChatPeerInfo() {}

        time_t last_contact ; 		// used to keep track of working connexion
    time_t last_keep_alive_sent ;	// last time we sent a keep alive packet.

        unsigned char aes_key[DISTANT_CHAT_AES_KEY_SIZE] ;

        uint32_t status ;		// info: do we have a tunnel ?
        RsPeerId virtual_peer_id;  	// given by the turtle router. Identifies the tunnel.
        RsGxsId own_gxs_id ;         	// gxs id we're using to talk.
        RsTurtleGenericTunnelItem::Direction direction ; // specifiec wether we are client(managing the tunnel) or server.
    };

    class DistantChatDHInfo
    {
    public:
        DH *dh ;
        RsGxsId gxs_id ;
        RsTurtleGenericTunnelItem::Direction direction ;
    };

    // This maps contains the current peers to talk to with distant chat.
    //
    std::map<RsGxsId, DistantChatPeerInfo> 	_distant_chat_contacts ;		// current peers we can talk to
    std::map<RsPeerId,DistantChatDHInfo>    	_distant_chat_virtual_peer_ids ;	// current virtual peers. Used to figure out tunnels, etc.

    // List of items to be sent asap. Used to store items that we cannot pass directly to
    // sendTurtleData(), because of Mutex protection.

    std::list<RsChatItem*> pendingDistantChatItems ;

    // Overloaded from RsTurtleClientService

    virtual bool handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id) ;
    virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
    void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
    void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;
    void markDistantChatAsClosed(const RsGxsId &gxs_id) ;
    void startClientDistantChatConnection(const RsGxsId &to_gxs_id,const RsGxsId& from_gxs_id) ;
    //bool getHashFromVirtualPeerId(const TurtleVirtualPeerId& pid,RsFileHash& hash) ;

    static TurtleFileHash hashFromGxsId(const RsGxsId& pid) ;
    static RsGxsId gxsIdFromHash(const TurtleFileHash& pid) ;

    void handleRecvDHPublicKey(RsChatDHPublicKeyItem *item) ;
    bool locked_sendDHPublicKey(const DH *dh, const RsGxsId &own_gxs_id, const RsPeerId &virtual_peer_id) ;
    bool locked_initDHSessionKey(DH *&dh);
    DistantChatPeerId virtualPeerIdFromHash(const TurtleFileHash& hash  ) ;	// ... and to a hash for p3turtle


    // Utility functions

    void sendTurtleData(RsChatItem *) ;

    static TurtleFileHash hashFromVirtualPeerId(const DistantChatPeerId& peerId) ;	// converts IDs so that we can talk to RsPeerId from outside

    p3turtle *mTurtle ;
    p3IdService *mIdService ;

    RsMutex mDistantChatMtx ;
};
