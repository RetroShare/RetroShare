/*******************************************************************************
 * libretroshare/src/retroshare: rsgxstunnel.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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
 *******************************************************************************/
#pragma once

#include "util/rsdir.h"
#include "retroshare/rsids.h"
#include "retroshare/rsturtle.h"
#include "retroshare/rsgxsifacetypes.h"

class RsGxsTunnelService
{
public:
    enum {
	RS_GXS_TUNNEL_ERROR_NO_ERROR          = 0x0000,
	RS_GXS_TUNNEL_ERROR_UNKNOWN_GXS_ID    = 0x0001  
    };

    enum {
	RS_GXS_TUNNEL_STATUS_UNKNOWN          = 0x00,
	RS_GXS_TUNNEL_STATUS_TUNNEL_DN        = 0x01,
	RS_GXS_TUNNEL_STATUS_CAN_TALK         = 0x02,
	RS_GXS_TUNNEL_STATUS_REMOTELY_CLOSED  = 0x03  
    };
    
    class RsGxsTunnelClientService
    {
    public:
        // The client should derive this in order to handle notifications from the tunnel service.
        // This cannot be ignored because the client needs to know when the tunnel is active.
        
        virtual void notifyTunnelStatus(const RsGxsTunnelId& tunnel_id,uint32_t tunnel_status) =0;
        
        // Data obtained from the corresponding GXS id. The memory ownership is transferred to the client, which
        // is responsible to free it using free() once used.
        
        virtual void receiveData(const RsGxsTunnelId& id,unsigned char *data,uint32_t data_size) =0;
        
        // Used by the creator of the service to supply a pointer to the GXS tunnel service for it to be able to send data etc.
        
        virtual void connectToGxsTunnelService(RsGxsTunnelService *tunnel_service) =0;
        
        // Gives feedback about type of data that is allowed in. For security reasons, this always needs to be re-derived (Clients can return true on default)
        
        virtual bool acceptDataFromPeer(const RsGxsId& gxs_id,const RsGxsTunnelId& tunnel_id,bool am_I_client_side) = 0 ;
    };
    
    class GxsTunnelInfo
    {
    public:
	    // Tunnel information

	    RsGxsTunnelId tunnel_id ;
	    RsGxsId  destination_gxs_id ;        // GXS Id we're talking to
	    RsGxsId  source_gxs_id ;	          // GXS Id we're using to talk
	    uint32_t tunnel_status ;	          // active, requested, DH pending, etc.
	    uint32_t total_size_sent ;	          // total bytes sent through that tunnel since openned (including management). 
	    uint32_t total_size_received ;	  // total bytes received through that tunnel since openned (including management). 
            bool is_client_side ; 		  // specifiec wether we are client(managing the tunnel) or server.

	    // Data packets

	    uint32_t pending_data_packets;         // number of packets not acknowledged by other side, still on their way. Should be 0 unless something bad happens.
	    uint32_t total_data_packets_sent ;     // total number of data packets sent (does not include tunnel management)
	    uint32_t total_data_packets_received ; // total number of data packets received (does not include tunnel management)
    };
    
    // This is the interface file for the secured tunnel service
    //
    //===================================================//
    //                  Debugging info                   //
    //===================================================//

    virtual bool getTunnelsInfo(std::vector<GxsTunnelInfo>& infos) =0;
    virtual bool getTunnelInfo(const RsGxsTunnelId& tunnel_id,GxsTunnelInfo& info) =0;

    // retrieve the routing probabilities

    //===================================================//
    //         Communication to other services.          //
    //===================================================//

    // Register a new client service. The service ID needs to be unique, and it's the coder's resonsibility to use an ID that is not used elsewhere
    // for the same purpose. 
    
    virtual bool registerClientService(uint32_t service_id,RsGxsTunnelClientService *service) =0;
    
    // Asks for a tunnel. The service will request it to turtle router, and exchange a AES key using DH.
    // When the tunnel is secured, the client---here supplied as argument---will be notified. He can
    // then send data into the tunnel. The same tunnel may be used by different clients.
    // The service id is passed on so that the client is notified when the tunnel is up.
    
    virtual bool requestSecuredTunnel(const RsGxsId& to_id,const RsGxsId& from_id,RsGxsTunnelId& tunnel_id,uint32_t service_id,uint32_t& error_code) =0 ;
    
    // Data is sent through the established tunnel, possibly multiple times, until reception is acknowledged. If the tunnel does not exist, the item is rejected and 
    // an error is issued. In any case, the memory ownership of the data is *not* transferred to the tunnel service, so the client should delete it afterwards, if needed.
    
    virtual bool sendData(const RsGxsTunnelId& tunnel_id, uint32_t client_service_id, const uint8_t *data, uint32_t data_size) =0;
    
    // Removes any established tunnel to this GXS id. This makes the tunnel refuse further data, but the tunnel will be however kept alive
    // until all pending data is flushed. All clients attached to the tunnel will be notified that the tunnel gets closed.
    
    virtual bool closeExistingTunnel(const RsGxsTunnelId& to_id,uint32_t service_id) =0;

    //===================================================//
    //         Routage feedback from other services      //
    //===================================================//

};

// To access the GRouter from anywhere
//
extern RsGxsTunnelService *rsGxsTunnel ;
