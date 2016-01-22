/*
 * libretroshare/src/chat: distantchat.h
 *
 * Services for RetroShare.
 *
 * Copyright 2015 by Cyril Soler
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

// Generic tunnel service
//
// Preconditions:
//	* the secured tunnel service takes care of:
//		- tunnel health: tunnels are kept alive using special items, re-openned when necessary, etc.
//		- transport: items are ACK-ed and re-sent if never received
//		- encryption: items are all encrypted and authenticated using PFS(DH)+HMAC(sha1)+AES(128)
//	* each tunnel is associated to a specific GXS id on both sides. Consequently, services that request tunnels from different IDs to a 
//		server for the same GXS id need to be handled correctly.
//	* client services must register to the secured tunnel service if they want to use it.
//	* multiple services can use the same tunnel. Items contain a service Id that is obtained when registering to the secured tunnel service.
//
// GUI
//	* the GUI should show for each tunnel:
//		- starting and ending GXS ids
//		- tunnel status (DH ok, closed from distant peer, locally closed, etc)
//		- amount of data that is transferred in the tunnel
//		- number of pending items (and total size)
//		- number ACKed items both ways.
//
//	  We can use an additional tab "Authenticated tunnels" in the statistics->turtle window for that purpose.
//
// Interaction with services:
//
//	Services request tunnels from a given GXS id and to a given GXS id. When ready, they get a handle (type = RsGxsTunnelId)
//
//	Services send data in the tunnel using the virtual peer id
//
//      Data is send to a service ID (could be any existing service ID). The endpoint of the tunnel must register each service, in order to
//	allow the data to be transmitted/sent from/to that service. Otherwise an error is issued.
//
// Encryption
//	* the whole tunnel traffic is encrypted using AES-128 with random IV
//	* a random key is established using DH key exchange for each connection (establishment of a new virtual peer)
//	* encrypted items are authenticated with HMAC(sha1). 
//	* DH public keys are the only chunks of data that travel un-encrypted along the tunnel. They are 
//        signed to avoid any MITM interactions. No time-stamp is used in DH exchange since a replay attack would not work.
//
// Algorithms
//
//	* we need two layers: the turtle layer, and the GXS id layer.
//		- for each pair of GXS ids talking, a single turtle tunnel is used
//		- that tunnel can be shared by multiple services using it.
//		- services are responsoble for asking tunnels and also droppping them when unused.
//		- at the turtle layer, the tunnel will be effectively closed only when no service uses it.
//	* IDs
//		TurtleVirtualPeerId:	
//			- Used by tunnel service for each turtle tunnel
//			- one virtual peer ID per GXS tunnel
//
//		GxsTunnelId:		
//			- one GxsTunnelId per couple of GXS ids. But we also need to allow multiple services to use the tunnel.
//			
//	* at the turtle layer:
//		- accept virtual peers from turtle tunnel service. The hash for that VP only depends on the server GXS id at server side, which is our
//			own ID at server side, and destination ID at client side. What happens if two different clients request to talk to the same GXS id? (same hash)
//			They should use different virtual peers, so it should be ok. 
//
//                 Turtle hash:   [ 0 ---------------15 16---19 ]
//                                      Destination      Random
//
//		    We Use 16 bytes to target the exact destination of the hash. The source part is just 4 arbitrary bytes that need to be different for all source
//                 IDs that come from the same peer, which is quite likely to be sufficient. The real source of the tunnel will make itself known when sending the 
//                 DH key.
//
//	* at the GXS layer
//		- we should be able to have as many tunnels as they are different couples of GXS ids to interact. That means the tunnel should be determined
//		 by a mix between our own GXS id and the GXS id we're talking to. That is what the TunnelVirtualPeer is.
//
//      
//	RequestTunnel(source_own_id,destination_id)                                                                              -
//                            |                                                                                                  |
//                            +---------------------------> p3Turtle::monitorTunnels(  hash(destination_id)   )                  |
//                                                                            |                                                  |
//                                                                   [Turtle async work] -------------------+                    |   Turtle layer: one virtual peer id
//                                                                            |                             |                    |
//      handleTunnelRequest() <-----------------------------------------------+                             |                    |
//                |                                                                                         |                    |
//                +---------------- keep record in _gxs_tunnel_virtual_peer_id, initiate DH exchange        |                    -
//                                                                                                          |                    |
//      handleDHPublicKey()   <-----------------------------------------------------------------------------+                    |
//                |                                                                                                              |
//                +---------------- update _gxs_tunnel_contacts[ tunnel_hash = hash(own_id, destination_id) ]                    |   GxsTunnelId level
//                |                                                                                                              |
//                +---------------- notify client service that Peer(destination_id, tunnel_hash) is ready to talk to             |
//                                                                                                                               -

#include <turtle/turtleclientservice.h>
#include <retroshare/rsgxstunnel.h>
#include <services/p3service.h>
#include <gxstunnel/rsgxstunnelitems.h>

class RsGixs ;

static const uint32_t GXS_TUNNEL_AES_KEY_SIZE = 16 ;

class p3GxsTunnelService: public RsGxsTunnelService, public RsTurtleClientService, public p3Service
{
public:
    p3GxsTunnelService(RsGixs *pids) ;
    virtual void connectToTurtleRouter(p3turtle *) ;

    // Creates the invite if the public key of the distant peer is available.
    // Om success, stores the invite in the map above, so that we can respond to tunnel requests.
    //
    virtual bool requestSecuredTunnel(const RsGxsId& to_id,const RsGxsId& from_id,RsGxsTunnelId& tunnel_id,uint32_t service_id,uint32_t& error_code) ;
    
    virtual bool closeExistingTunnel(const RsGxsTunnelId &tunnel_id,uint32_t service_id) ;
    virtual bool getTunnelsInfo(std::vector<GxsTunnelInfo>& infos);
    virtual bool getTunnelInfo(const RsGxsTunnelId& tunnel_id,GxsTunnelInfo& info);
    virtual bool sendData(const RsGxsTunnelId& tunnel_id,uint32_t service_id,const uint8_t *data,uint32_t size) ;
    
    virtual bool registerClientService(uint32_t service_id,RsGxsTunnelClientService *service) ;

    // derived from p3service
    
    virtual int tick();
    virtual RsServiceInfo getServiceInfo();
    
private:
    void flush() ;
    virtual void handleIncomingItem(const RsGxsTunnelId &tunnel_id, RsGxsTunnelItem *) ;
    
    class GxsTunnelPeerInfo
    {
    public:
        GxsTunnelPeerInfo() : last_contact(0), last_keep_alive_sent(0), status(0), direction(0)
        {
            memset(aes_key, 0, GXS_TUNNEL_AES_KEY_SIZE);
            
            total_sent = 0 ;
            total_received = 0 ;
        }

        time_t last_contact ; 		// used to keep track of working connexion
	time_t last_keep_alive_sent ;	// last time we sent a keep alive packet.

        unsigned char aes_key[GXS_TUNNEL_AES_KEY_SIZE] ;

        uint32_t status ;		// info: do we have a tunnel ?
        RsPeerId virtual_peer_id;  	// given by the turtle router. Identifies the tunnel.
        RsGxsId to_gxs_id;  		// gxs id we're talking to
        RsGxsId own_gxs_id ;         	// gxs id we're using to talk.
        RsTurtleGenericTunnelItem::Direction direction ; // specifiec wether we are client(managing the tunnel) or server.
        TurtleFileHash hash ;		// hash that is last used. This is necessary for handling tunnel establishment
        std::set<uint32_t> client_services ;// services that used this tunnel
        std::map<uint64_t,time_t> received_data_prints ; // list of recently received messages, to avoid duplicates. Kept for 20 mins at most.
        uint32_t total_sent ;
        uint32_t total_received ;
    };

    class GxsTunnelDHInfo
    {
    public:
        GxsTunnelDHInfo() : dh(0), direction(0), status(0) {}

        DH *dh ;
        RsGxsId gxs_id ;
        RsGxsId own_gxs_id ;
        RsGxsTunnelId tunnel_id ; // this is a proxy, since we cna always recompute that from the two previous values.
        RsTurtleGenericTunnelItem::Direction direction ;
	uint32_t status ;
	TurtleFileHash hash ;
    };

    struct GxsTunnelData
    {
        RsGxsTunnelDataItem *data_item ;
        time_t    last_sending_attempt ;
    };
    
    // This maps contains the current peers to talk to with distant chat.
    //
    std::map<RsGxsTunnelId,GxsTunnelPeerInfo> 		_gxs_tunnel_contacts ;		// current peers we can talk to
    std::map<TurtleVirtualPeerId,GxsTunnelDHInfo>    	_gxs_tunnel_virtual_peer_ids ;	// current virtual peers. Used to figure out tunnels, etc.

    // List of items to be sent asap. Used to store items that we cannot pass directly to
    // sendTurtleData(), because of Mutex protection.

    std::map<uint64_t,GxsTunnelData> 		pendingGxsTunnelDataItems ;	// items that need provable transport and encryption
    std::list<RsGxsTunnelItem*> 		pendingGxsTunnelItems ;		// items that do not need provable transport, yet need encryption
    std::list<RsGxsTunnelDHPublicKeyItem*> 	pendingDHItems ;		

    // Overloaded from RsTurtleClientService

    virtual bool handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id) ;
    virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
    void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
    void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;
    
    // session handling handles
    
    void startClientGxsTunnelConnection(const RsGxsId &to_gxs_id, const RsGxsId& from_gxs_id, uint32_t service_id, RsGxsTunnelId &tunnel_id) ;
    void locked_restartDHSession(const RsPeerId &virtual_peer_id, const RsGxsId &own_gxs_id) ;

    // utility functions

    static TurtleFileHash randomHashFromDestinationGxsId(const RsGxsId& destination) ;
    static RsGxsId destinationGxsIdFromHash(const TurtleFileHash& sum) ;

    // Cryptography management
    
    void handleRecvDHPublicKey(RsGxsTunnelDHPublicKeyItem *item) ;
    bool locked_sendDHPublicKey(const DH *dh, const RsGxsId& own_gxs_id, const RsPeerId& virtual_peer_id) ;
    bool locked_initDHSessionKey(DH *&dh);
    
    TurtleVirtualPeerId virtualPeerIdFromHash(const TurtleFileHash& hash) ;	// ... and to a hash for p3turtle
    RsGxsTunnelId makeGxsTunnelId(const RsGxsId &own_id, const RsGxsId &distant_id) const;	// creates a unique ID from two GXS ids.

    // item handling
    
    void handleRecvStatusItem(const RsGxsTunnelId& id,RsGxsTunnelStatusItem *item) ;
    void handleRecvTunnelDataItem(const RsGxsTunnelId& id,RsGxsTunnelDataItem *item) ;
    void handleRecvTunnelDataAckItem(const RsGxsTunnelId &id, RsGxsTunnelDataAckItem *item);
    
    // Comunication with Turtle service

    bool locked_sendEncryptedTunnelData(RsGxsTunnelItem *item) ;
    bool locked_sendClearTunnelData(RsGxsTunnelDHPublicKeyItem *item);	// this limits the usage to DH items. Others should be encrypted!
    
    bool handleEncryptedData(const uint8_t *data_bytes,uint32_t data_size,const TurtleFileHash& hash,const RsPeerId& virtual_peer_id) ;

    // local data
    
    p3turtle 	*mTurtle ;
    RsGixs 	*mGixs ;
    RsMutex  	 mGxsTunnelMtx ;
    
    std::map<uint32_t,RsGxsTunnelClientService*> mRegisteredServices ;
    
    void debug_dump();
};

