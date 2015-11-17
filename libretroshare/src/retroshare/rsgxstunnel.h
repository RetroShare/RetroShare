/*
 * libretroshare/src/services: rsgrouter.h
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
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

#include "util/rsdir.h"
#include "retroshare/rsids.h"
#include "retroshare/rsgxsifacetypes.h"

class RsGxsTunnelService
{
public:
    class RsGxsTunnelClientService
    {
    public:
        // The client should derive this in order to handle notifications from the tunnel service.
        // This cannot be ignored because the client needs to know when the tunnel is active.
        
        virtual void notifyTunnelStatus(const RsGxsId& id,uint32_t tunnel_status) =0;
        
        // Data obtained from the corresponding GXS id. The memory ownership is transferred to the client, which
        // is responsible to free it using free() once used.
        
        virtual void receiveData(const RsGxsId& id,unsigned char *data,uint32_t data_size) =0;
    };
    
    class GxsTunnelInfo
    {
    public:
        RsGxsId  gxs_id ;		// GXS Id to which we're talking
        uint32_t tunnel_status ;	// active, requested, DH pending, etc.
        uint32_t pending_data_packets; // number of packets not acknowledged by other side, still on their way.
        uint32_t total_size_sent ;	// total number bytes sent through that tunnel since openned.
        uint32_t total_packets_sent ;	// total number of packets sent and acknowledged by other side
        
        std::vector<RsGxsTunnelClientService*> client_services ;
    };
    
    // This is the interface file for the secured tunnel service
    //
    //===================================================//
    //                  Debugging info                   //
    //===================================================//

    virtual bool getGxsTunnelsInfo(std::vector<GxsTunnelInfo>& infos) =0;

    // retrieve the routing probabilities

    //===================================================//
    //         Communication to other services.          //
    //===================================================//

    // Asks for a tunnel. The service will request it to turtle router, and exchange a AES key using DH.
    // When the tunnel is secured, the client---here supplied as argument---will be notified. He can
    // then send data into the tunnel. The same tunnel may be used by different clients.
    
    virtual bool requestSecuredTunnel(const RsGxsId& to_id,RsGxsTunnelClientService *client) =0 ;
    
    // Data is sent through the established tunnel, possibly multiple times, until reception is acknowledged
    
    virtual bool sendData(const RsGxsId& destination, const GRouterServiceId& client_id, const uint8_t *data, uint32_t data_size, const RsGxsId& signing_id, GRouterMsgPropagationId& id) =0;
    
    // Removes any established tunnel to this GXS id. This makes the tunnel refuse further data, but the tunnel will be however kept alive
    // until all pending data is flushed. All clients attached to the tunnel will be notified that the tunnel gets closed.
    
    virtual bool removeExistingTunnel(const RsGxsId& to_id) =0;

    //===================================================//
    //         Routage feedback from other services      //
    //===================================================//

};

// To access the GRouter from anywhere
//
extern RsGxsTunnelService *rsGxsTunnel ;
