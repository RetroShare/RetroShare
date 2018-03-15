/*
 * libretroshare/src/gxs: rsgxsnettunnel.cc
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2018-2018 by Cyril Soler
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
 * Please report all bugs and problems to "retroshare.project@gmail.com"
 *
 */

#include "rsgxsnettunnel.h"

#define DEBUG_RSGXSNETTUNNEL 1

#define NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl; }

RsGxsNetTunnelService::RsGxsNetTunnelService(): mGxsNetTunnelMtx("GxsNetTunnel") {}

bool RsGxsNetTunnelService::manage(const RsGxsGroupId& group_id)
{
	RsFileHash hash = calculateGroupHash(group_id) ;

	RsStackMutex stack(mGxsNetTunnelMtx); /********** STACK LOCKED MTX ******/

    RsGxsNetTunnelGroupInfo& info(mClientGroups[group_id]) ;

    time_t now = time(NULL) ;

	if(info.group_status == RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE)
		return true;

    info.hash = hash ;
    info.last_contact = now ;
    info.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "Starting distant chat to " << to_gxs_id << ", hash = " << hash << ", from " << from_gxs_id << std::endl;
    std::cerr << "Asking turtle router to monitor tunnels for hash " << hash << std::endl;
#endif

    // Now ask the turtle router to manage a tunnel for that hash.

    mTurtle->monitorTunnels(hash,this,false) ;

	return true;
}

bool RsGxsNetTunnelService::release(const RsGxsGroupId& group_id)
{
	RsStackMutex stack(mGxsNetTunnelMtx); /********** STACK LOCKED MTX ******/

	// Here we need to clean the stuff that was created by this group id.

	auto it = mClientGroups.find(group_id) ;

	if(it == mClientGroups.end())
	{
		std::cerr << "RsGxsNetTunnelService::release(): Weird. Cannot release client group " << group_id << " that is not known." << std::endl;
		return false ;
	}

	mClientGroups.erase(it) ;
	return true ;
}

bool RsGxsNetTunnelService::sendData(const unsigned char *data,uint32_t size,const RsGxsNetTunnelVirtualPeerId& virtual_peer)
{
	// The data is encrypted using chacha20+SHA256 and sent to the turtle router.

	NOT_IMPLEMENTED();
	return false ;
}

bool RsGxsNetTunnelService::getVirtualPeers(const RsGxsGroupId&, std::list<RsPeerId>& peers)
{
	// returns the virtual peers for this group
	NOT_IMPLEMENTED();
	return false ;
}

RsGxsNetTunnelVirtualPeerInfo RsGxsNetTunnelService::makeVirtualPeerIdForGroup(const RsGxsGroupId&) const
{
	NOT_IMPLEMENTED();
	return RsGxsNetTunnelVirtualPeerInfo();
}
void RsGxsNetTunnelService::dump() const
{
	NOT_IMPLEMENTED();
}

//===========================================================================================================================================//
//                                                         Interaction with Turtle Router                                                    //
//===========================================================================================================================================//

void RsGxsNetTunnelService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	mTurtle->registerTunnelService(this) ;
}

bool RsGxsNetTunnelService::handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id)
{
	NOT_IMPLEMENTED();
	return false ;
}
void RsGxsNetTunnelService::receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
{
	NOT_IMPLEMENTED();
}
void RsGxsNetTunnelService::addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir)
{
	NOT_IMPLEMENTED();
}
void RsGxsNetTunnelService::removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&)
{
	NOT_IMPLEMENTED();
}

RsFileHash RsGxsNetTunnelService::calculateGroupHash(const RsGxsGroupId&) const
{
	NOT_IMPLEMENTED();
	return RsFileHash() ;
}

//===========================================================================================================================================//
//                                                               Service parts                                                               //
//===========================================================================================================================================//

#ifdef TODO
void RsGxsNetTunnelService::handleIncomingItem(const RsGxsTunnelId& tunnel_id,RsGxsTunnelItem *item)
{
    if(item == NULL)
	    return ;

    // We have 3 things to do:
    //
    // 1 - if it's a data item, send an ACK
    // 2 - if it's an ack item, mark the item as properly received, and remove it from the queue
    // 3 - if it's a status item, act accordingly.

    switch(item->PacketSubType())
    {

    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA:		handleRecvTunnelDataItem(tunnel_id,dynamic_cast<RsGxsTunnelDataItem*>(item)) ;
	    break ;

    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK:	handleRecvTunnelDataAckItem(tunnel_id,dynamic_cast<RsGxsTunnelDataAckItem*>(item)) ;
	    break ;

    case RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS:		handleRecvStatusItem(tunnel_id,dynamic_cast<RsGxsTunnelStatusItem*>(item)) ;
	    break ;

    default:
	    std::cerr << "(EE) impossible situation. DH items should be handled at the service level" << std::endl;
    }

    delete item ;
}
#endif

