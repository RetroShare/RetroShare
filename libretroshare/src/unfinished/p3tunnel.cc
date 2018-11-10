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


//#include "retroshare/rsiface.h"
//#include "retroshare/rsinit.h" /* for PGPSSL flag */
//#include "retroshare/rspeers.h"
#include "services/p3tunnel.h"
#include "pqi/pqissltunnel.h"

#include "pqi/authssl.h"
#include "pqi/p3connmgr.h"

#include <errno.h>

#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsversion.h"

#define TUNNEL_HANDSHAKE_INIT		1
#define TUNNEL_HANDSHAKE_ACK    	2
#define TUNNEL_HANDSHAKE_REFUSE    	0

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
	return handleIncoming();
}

int p3tunnel::handleIncoming()
{
	RsItem *item = NULL;

#ifdef P3TUNNEL_DEBUG
	//std::cerr << "p3tunnel::handleIncoming() called." << std::endl;
#endif

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		if (!mConnMgr->getTunnelConnection()) {
			//no tunnel allowed, just drop the packet
			continue;
		}

		RsTunnelDataItem *tdi = NULL;
		RsTunnelHandshakeItem *thi = NULL;

		{
#ifdef P3TUNNEL_DEBUG
			std::string out = "p3tunnel::handleIncoming() Received Message!\n";
			item -> print_string(out);
			std::cerr << out;
#endif
		}

		if (NULL != (tdi = dynamic_cast<RsTunnelDataItem *> (item))) {
			recvTunnelData(tdi);
			nhandled++;
		} else if (NULL != (thi = dynamic_cast<RsTunnelHandshakeItem *> (item))) {
			recvTunnelHandshake(thi);
			nhandled++;
		}
		delete item;
	}
	return nhandled;
}

/*************************************************************************************/
/*				Output Network Msgs				     */
/*************************************************************************************/
void p3tunnel::sendTunnelData(std::string destPeerId, std::string relayPeerId, void *data, int data_length) {
	sendTunnelDataPrivate(relayPeerId, ownId,relayPeerId, destPeerId, data, data_length);
}


void p3tunnel::sendTunnelDataPrivate(std::string to, std::string sourcePeerId, std::string relayPeerId, std::string destPeerId, void *data, int data_length) {
	if (!mConnMgr->getTunnelConnection()) {
		//no tunnel allowed, just drop the request
		return;
	}

	RsStackMutex stack(mTunnelMtx); /********** STACK LOCKED MTX ******/

	// Then send message.
	{
#ifdef P3TUNNEL_DEBUG
		std::string out = "p3tunnel::sendTunnelDataPrivate() Constructing a RsTunnelItem Message!\n";
		out += "Sending to: " + to;
		std::cerr << out << std::endl;
#endif
	}

	// Construct a message
	RsTunnelDataItem *rdi = new RsTunnelDataItem();
	rdi->destPeerId = destPeerId;
	rdi->sourcePeerId = sourcePeerId;
	rdi->relayPeerId = relayPeerId;
	rdi->encoded_data_len = data_length;

	if(data_length > 0)
	{
		rdi->encoded_data = (void*)malloc(data_length);
		memcpy(rdi->encoded_data, data, data_length);
	}

	rdi->PeerId(to);

#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::sendTunnelDataPrivate()  data_length : "<<  data_length << std::endl;
#endif
	/* send msg */
	sendItem(rdi);
}

void p3tunnel::pingTunnelConnection(std::string relayPeerId, std::string destPeerId) {
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::pingTunnelConnection() sending ping with relay id : " << relayPeerId << std::endl;
	std::cerr << "ownId : " << ownId << std::endl;
	std::cerr << "destPeerId : " << destPeerId << std::endl;
#endif
	this->sendTunnelDataPrivate(relayPeerId, ownId, relayPeerId, destPeerId, NULL, 0);
}

void p3tunnel::initiateHandshake(std::string relayPeerId, std::string destPeerId) {
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::initiateHandshake() initiating handshake with relay id : " << relayPeerId << std::endl;
	std::cerr << "ownId : " << ownId << std::endl;
	std::cerr << "destPeerId : " << destPeerId << std::endl;
#endif
	// Construct a message
	RsTunnelHandshakeItem *rhi = new RsTunnelHandshakeItem();
	rhi->destPeerId = destPeerId;
	rhi->sourcePeerId = ownId;
	rhi->relayPeerId = relayPeerId;
	rhi->connection_accepted = TUNNEL_HANDSHAKE_INIT;
	rhi->sslCertPEM = AuthSSL::getAuthSSL()->SaveOwnCertificateToString();

	rhi->PeerId(relayPeerId);

	/* send msg */
	sendItem(rhi);
}

/*************************************************************************************/
/*				Input Network Msgs				     */
/*************************************************************************************/
void p3tunnel::recvTunnelHandshake(RsTunnelHandshakeItem *item)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::recvTunnelHandshake() From: " << item->PeerId() << std::endl;
#endif

	RsPeerDetails pd;
	if (!AuthSSL::getAuthSSL()->LoadDetailsFromStringCert(item->sslCertPEM, pd)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::recvTunnelHandshake() cert is not valid. This might be a intrusion attempt." << std::endl;
#endif
		return;
	}

	if (item->sourcePeerId != pd.id) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::recvTunnelHandshake() cert is not issued from the source id of the tunnel. This might be a intrusion attempt." << std::endl;
#endif
		return;
	}

	//compare the peer id from the item sender to the ids in the item.
	if (item->PeerId() == item->sourcePeerId && ownId == item->relayPeerId) {
		if (mConnMgr->isOnline(item->destPeerId)) {
#ifdef P3TUNNEL_DEBUG
			std::cerr << "p3tunnel::recvTunnelHandshake() relaying packet." << std::endl;
#endif
			//relaying the handshake
			RsTunnelHandshakeItem* forwardItem = new RsTunnelHandshakeItem();
			forwardItem->sourcePeerId   = item->sourcePeerId;
			forwardItem->relayPeerId    = item->relayPeerId;
			forwardItem->destPeerId     = item->destPeerId;
			forwardItem->connection_accepted = item->connection_accepted;
			forwardItem->sslCertPEM     = item->sslCertPEM;
			forwardItem->PeerId(item->destPeerId);
			sendItem(forwardItem);
		} else {
			//sending back refuse
			//not implemented
#ifdef P3TUNNEL_DEBUG
			std::cerr << "p3tunnel::recvTunnelHandshake() not relaying packet because destination is offline." << std::endl;
#endif
		}
	} else if (item->PeerId() == item->relayPeerId && ownId == item->destPeerId) {
		if (item->connection_accepted == TUNNEL_HANDSHAKE_INIT || item->connection_accepted == TUNNEL_HANDSHAKE_ACK) {
			//check if we accept connection
			if (!mConnMgr->isFriend(pd.id)) {
				//send back a refuse
				// not implemented
			} else {
				if (item->connection_accepted == TUNNEL_HANDSHAKE_INIT) {
#ifdef P3TUNNEL_DEBUG
					std::cerr << "p3tunnel::recvTunnelHandshake() sending back acknowledgement to " << item->sourcePeerId << std::endl;
#endif
					//send back acknowledgement
					RsTunnelHandshakeItem* ack = new RsTunnelHandshakeItem();
					ack->sourcePeerId = ownId;
					ack->relayPeerId = item->relayPeerId;
					ack->destPeerId = item->sourcePeerId;
					ack->connection_accepted = TUNNEL_HANDSHAKE_ACK;
					ack->sslCertPEM = AuthSSL::getAuthSSL()->SaveOwnCertificateToString();
					ack->PeerId(item->relayPeerId);
					sendItem(ack);
				}

				//open the local tunnel connection
#ifdef P3TUNNEL_DEBUG
				std::cerr << "p3tunnel::recvTunnelHandshake() opening localy the tunnel connection emulation." << std::endl;
#endif
				pqiperson *pers = mPqiPersonGrp->getPeer(item->sourcePeerId);
				pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
				pqicon->IncommingHanshakePacket(item->relayPeerId);
			}
		}
	}
}


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
	}
}

void p3tunnel::privateRecvTunnelDataRelaying(RsTunnelDataItem *item) {
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() I am relaying, let's see if it's possible to send the packet to destination." << std::endl;
#endif
	if (!mConnMgr->isFriend(item->sourcePeerId) || !mConnMgr->isFriend(item->destPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() not trusting relay or dest peer. Aborting." << std::endl;
#endif
		return;
	}
	if (mConnMgr->isOnline(item->destPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataRelaying() I am relaying, relay the packet to destination." << std::endl;
#endif
		sendTunnelDataPrivate(item->destPeerId, item->sourcePeerId, ownId, item->destPeerId, item->encoded_data, item->encoded_data_len);
		return;
	}
}

void p3tunnel::privateRecvTunnelDataDestination(RsTunnelDataItem *item) {
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel::privateRecvTunnelDataDestination() I am the destination Id, let's make some checks and read the packet." << std::endl;
#endif

	if (!mConnMgr->isFriend(item->sourcePeerId) || !mConnMgr->isFriend(item->relayPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() not trusting relay or source peer. Aborting." << std::endl;
#endif
		return;
	}

	if (!mConnMgr->isOnline(item->relayPeerId)) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() relay peer is not connected, connection impossible. Aborting." << std::endl;
#endif
		return;
	}

	pqiperson *pers = mPqiPersonGrp->getPeer(item->sourcePeerId);
	if (pers == NULL) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() tunnel connection not found. Aborting." << std::endl;
#endif
		return;
	}

	pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
	if (pqicon == NULL || !pqicon->isactive()) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() tunnel connection not found. Aborting." << std::endl;
#endif
		return;
	}

	//send the packet to the net emulation layer
	if (item->encoded_data_len == 0) {
#ifdef P3TUNNEL_DEBUG
		std::cerr << "p3tunnel::privateRecvTunnelDataDestination() receiving a ping packet, activating connection and sending back acknowlegment." << std::endl;
#endif
		pqissltunnel *pqicon = (pqissltunnel *)(((pqiconnect *) pers->getKid(PQI_CONNECT_TUNNEL))->ni);
		pqicon->IncommingPingPacket();
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
