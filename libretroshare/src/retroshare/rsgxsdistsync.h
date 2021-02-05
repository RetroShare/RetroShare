/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsdistsync.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Cyril Soler   <retroshare@lunamutt.com>                 *
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

#include "retroshare/rsfiles.h"
#include "retroshare/rsturtle.h"

typedef RsPeerId RsGxsNetTunnelVirtualPeerId ;

struct RsGxsNetTunnelVirtualPeerInfo
{
	enum {   RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   = 0x00,		// unknown status.
		     RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK = 0x01,		// tunnel has been established and we're waiting for virtual peer id
		     RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    = 0x02		// virtual peer id is known. Data can transfer.
	     };

	RsGxsNetTunnelVirtualPeerInfo() : vpid_status(RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN), last_contact(0),side(0) { memset(encryption_master_key,0,32) ; }
	virtual ~RsGxsNetTunnelVirtualPeerInfo(){}

	uint8_t vpid_status ;					// status of the peer
	rstime_t  last_contact ;					// last time some data was sent/recvd
	uint8_t side ;	                        // client/server
	uint8_t encryption_master_key[32];

	TurtleVirtualPeerId      turtle_virtual_peer_id ;  // turtle peer to use when sending data to this vpid.

	RsGxsGroupId group_id ;					// group that virtual peer is providing
	uint16_t service_id ; 					// this is used for checkng consistency of the incoming data
};

struct RsGxsNetTunnelGroupInfo
{
	enum GroupStatus {
		    RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
		    RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               = 0x01,	// no virtual peers requested, just waiting
		    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x02	// some virtual peers are available. Data can be read/written
	};

	enum GroupPolicy {
		    RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            = 0x00,	// nothing has been set
		    RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            = 0x01,	// group is available for server side tunnels, but does not explicitely request tunnels
		    RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             = 0x02,	// group will only explicitely request tunnels if none available
		    RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING         = 0x03,	// group explicitely requests tunnels
    };

	RsGxsNetTunnelGroupInfo() : group_policy(RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE),group_status(RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE),last_contact(0) {}

	GroupPolicy    group_policy ;
	GroupStatus    group_status ;
	rstime_t       last_contact ;
	RsFileHash     hash ;
	uint16_t       service_id ;

	std::set<TurtleVirtualPeerId> virtual_peers ; // list of which virtual peers provide this group. Can me more than 1.
};

// This class is here to provide statistics about GXS dist sync internals. It
//
class RsGxsDistSync
{
   public:
		virtual void getStatistics(
	        std::map<RsGxsGroupId,RsGxsNetTunnelGroupInfo>& groups,                                 // groups on the client and server side
	  		std::map<RsGxsNetTunnelVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo>& virtual_peers,    // current virtual peers, which group they provide, and how to talk to them through turtle
            std::map<TurtleVirtualPeerId,RsGxsNetTunnelVirtualPeerId>& turtle_vpid_to_net_tunnel_vpid,
		    Bias20Bytes& bias
		) const =0;
		virtual bool isGXSHash(RsFileHash hash) const =0;
};

extern RsGxsDistSync *rsGxsDistSync ;

