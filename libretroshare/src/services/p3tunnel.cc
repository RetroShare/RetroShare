/*
 * libretroshare/src/services: p3tunnel.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "rsiface/rsiface.h"
#include "rsiface/rsinit.h" /* for PGPSSL flag */
#include "rsiface/rspeers.h"
#include "services/p3tunnel.h"
#include "pqi/pqissltunnel.h"

#include "pqi/authssl.h"
#include "pqi/p3connmgr.h"

#include <errno.h>

#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsversion.h"

p3tunnel::p3tunnel(p3ConnectMgr *cm, pqipersongrp *perGrp)
        :p3Service(RS_SERVICE_TYPE_TUNNEL), mConnMgr(cm), mPqiPersonGrp(perGrp)
{
	RsStackMutex stack(mTunnelMtx); /********** STACK LOCKED MTX ******/

	ownId = mConnMgr->getOwnId();
        std::cerr << "ownId : " << mConnMgr->getOwnId() << std::endl;
	addSerialType(new RsTunnelSerialiser());

	return;
}

void p3tunnel::statusChange(const std::list<pqipeer> &plist) {
}

int p3tunnel::tick()
{
        if (!mConnMgr->getTunnelConnection()) {
            //no tunnel allowed, just drop the packet
            return -1;
        }

	return handleIncoming();
}

int p3tunnel::handleIncoming()
{
        if (!mConnMgr->getTunnelConnection()) {
            //no tunnel allowed, just drop the packet
            return -1;
        }

	RsItem *item = NULL;

#ifdef P3TUNNEL_DEBUG
        std::cerr << "p3tunnel::handleIncoming() called";
	std::cerr << std::endl;
#endif

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsTunnelDataItem *tdi = NULL;

		{
#ifdef P3TUNNEL_DEBUG
			std::ostringstream out;
			out << "p3tunnel::handleIncoming()";
			out << " Received Message!" << std::endl;
                        item -> print(out);
                        out << std::endl;
                        std::cerr << out.str();
#endif
		}

		if (NULL != (tdi = dynamic_cast<RsTunnelDataItem *> (item))) {
#ifdef P3TUNNEL_DEBUG
                        std::cerr << "p3tunnel::handleIncoming() getRsItemSize(tdi->encoded_data) : " << getRsItemSize(tdi->encoded_data) << std::endl;
#endif
                        recvTunnelData(tdi);
			nhandled++;
		}
		delete item;
	}
	return nhandled;
}

/*************************************************************************************/
/*				Output Network Msgs				     */
/*************************************************************************************/
void p3tunnel::sendTunnelData(std::string destPeerId, std::string relayPeerId, void *data, int data_length)
{
    sendTunnelDataPrivate(1, relayPeerId, ownId,relayPeerId, destPeerId, data, data_length);
}

void p3tunnel::sendTunnelDataPrivate(int accept, std::string to, std::string sourcePeerId, std::string relayPeerId, std::string destPeerId, void *data, int data_length)
{
        if (!mConnMgr->getTunnelConnection()) {
            //no tunnel allowed, just drop the request
            return;
        }

        RsStackMutex stack(mTunnelMtx); /********** STACK LOCKED MTX ******/

	// Then send message.
	{
#ifdef P3TUNNEL_DEBUG
		  std::ostringstream out;
                  out << "p3tunnel::sendTunnelDataPrivate() Constructing a RsTunnelItem Message!" << std::endl;
		  out << "Sending to: " << to;
		  std::cerr << out.str() << std::endl;
#endif
	}

	// Construct a message
	RsTunnelDataItem *rdi = new RsTunnelDataItem();
	rdi->destPeerId = destPeerId;
	rdi->sourcePeerId = sourcePeerId;
	rdi->relayPeerId = relayPeerId;
	rdi->connection_accepted = accept;
	rdi->encoded_data_len = data_length;

        rdi->encoded_data = (void*)malloc(data_length);
        memcpy(rdi->encoded_data, data, data_length);

#ifdef P3TUNNEL_DEBUG
                  std::cerr << "p3tunnel::sendTunnelDataPrivate()  getRsItemSize(rdi->encoded_data) : "<<  getRsItemSize(rdi->encoded_data) << std::endl;
#endif

	rdi->PeerId(to);

	/* send msg */
	sendItem(rdi);
}

void p3tunnel::pingTunnelConnection(std::string relayPeerId, std::string destPeerId) {
    std::cerr << "p3tunnel::pingTunnelConnection() sending ping with relay id : " << relayPeerId << std::endl;
    std::cerr << "ownId : " << ownId << std::endl;
    std::cerr << "destPeerId : " << destPeerId << std::endl;
    this->sendTunnelDataPrivate(1, relayPeerId, ownId, relayPeerId, destPeerId, NULL, 0);
}

/*************************************************************************************/
/*				Input Network Msgs				     */
/*************************************************************************************/
void p3tunnel::recvTunnelData(RsTunnelDataItem *item)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::recvPeerConnectRequest() From: " << item->PeerId() << std::endl;
#endif

	//compare the peer id from the item sender to the ids in the item.
	if (item->PeerId() == item->sourcePeerId && ownId == item->relayPeerId) {
		privateRecvTunnelDataRelaying(item);
	} else if (item->PeerId() == item->relayPeerId && ownId == item->destPeerId) {
		privateRecvTunnelDataDestination(item);
	} else if (item->PeerId() == item->destPeerId && ownId == item->relayPeerId) {
	    //it's a ping reply from a destination, I'm relaying. Just forward the packet to the source
	    if (item->connection_accepted && mConnMgr->isOnline(item->sourcePeerId)) {
		sendTunnelDataPrivate(1, item->sourcePeerId, item->sourcePeerId, ownId, item->destPeerId, NULL, 0);
		return;
	    }
	} else if (item->PeerId() == item->relayPeerId && ownId == item->sourcePeerId) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() it's a ping reply. Let's see if the tunnel is accepted." << std::endl;
#endif
	    if (item->connection_accepted) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() tunnel is accepted. activate the pqissltunnel connection." << std::endl;
#endif
		pqiperson *pers = mPqiPersonGrp->getPeer(item->destPeerId);
		pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
		pqicon->IncommingPingPacket(item->relayPeerId);
	    } else {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() tunnel is not accepted." << std::endl;
#endif
		return;
	    }
	}
}

void p3tunnel::privateRecvTunnelDataRelaying(RsTunnelDataItem *item) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() I am relaying, let's see if it's possible to send the packet to destination." << std::endl;
#endif
	    if (mConnMgr->isOnline(item->destPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() I am relaying, relay the packet to destination." << std::endl;
#endif
		sendTunnelDataPrivate(1, item->destPeerId, item->sourcePeerId, ownId, item->destPeerId, item->encoded_data, item->encoded_data_len);
		return;
	    } else {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() destination peer is not online, send back the request with a deny" << std::endl;
#endif
		sendTunnelDataPrivate(0, item->sourcePeerId, item->sourcePeerId, ownId, item->destPeerId, NULL, 0);
		return;
	    }
}

void p3tunnel::privateRecvTunnelDataDestination(RsTunnelDataItem *item) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() I am the destination Id, let's make some checks and read the packet." << std::endl;
#endif
	    if (!mConnMgr->isFriend(item->sourcePeerId) || !mConnMgr->isFriend(item->relayPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() not trusting rely or source peer. Aborting." << std::endl;
#endif
		return;
	    }

//peer is online when connected through a tunnel so we should not drop the packet
//	    if (mConnMgr->isOnline(item->sourcePeerId)) {
//#ifdef P3TUNNEL_DEBUG
//		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() no need to make tunnel connection, source peer is online. Aborting." << std::endl;
//#endif
//		return;
//	    }

	    if (!mConnMgr->isOnline(item->relayPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() relay peer is not connected, connection impossible. Aborting." << std::endl;
#endif
		return;
	    }
	    pqiperson *pers = mPqiPersonGrp->getPeer(item->sourcePeerId);
	    if (pers == NULL) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() internal source pqiperson peer not found. Aborting." << std::endl;
#endif
		return;
	    }

            //if data is empty, then it's a ping, send the packet to the net emulation layer
	    if (item->encoded_data_len == 0) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() receiving a ping packet, activating connection and sending back acknowlegment." << std::endl;
#endif
		pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
		pqicon->IncommingPingPacket(item->relayPeerId);
                //sendTunnelDataPrivate(1, item->relayPeerId, item->sourcePeerId, item->relayPeerId, ownId, NULL, 0);
	    } else {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() receiving a data packet, transfer it to the pqissltunnel connection." << std::endl;
                std::cerr << "p3tunnel::privateRecvTunnelDataDestination() getRsItemSize(item->encoded_data) : " << getRsItemSize(item->encoded_data) << std::endl;
#endif
		pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
		pqicon->addIncomingPacket(item->encoded_data, item->encoded_data_len);
	    }
	    return;
}
