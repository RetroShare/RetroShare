/*
 * libretroshare/src/services: p3tunnel.h
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

#ifndef MRK_PQI_TUNNEL_H
#define MRK_PQI_TUNNEL_H

// The AutoDiscovery Class

#include "pqi/pqipersongrp.h"

// system specific network headers
#include "pqi/pqi.h"

class p3ConnectMgr;

#include "pqi/pqimonitor.h"
#include "services/p3service.h"
#include "serialiser/rstunnelitems.h"
#include "pqi/authssl.h"

class p3tunnel: public p3Service, public pqiMonitor
{
	public:

virtual void	statusChange(const std::list<pqipeer> &plist);

        p3tunnel(p3ConnectMgr *cm, pqipersongrp *persGrp);

int	tick();

void sendTunnelData(std::string destPeerId, std::string relayPeerId, void *data, int data_length);

void pingTunnelConnection(std::string relayPeerId, std::string destPeerId);
void initiateHandshake(std::string relayPeerId, std::string destPeerId);

	private:

void sendTunnelDataPrivate(std::string to, std::string sourcePeerId, std::string relayPeerId, std::string destPeerId, void *data, int data_length);

void privateRecvTunnelDataRelaying(RsTunnelDataItem *item); //invoked when I am relaying
void privateRecvTunnelDataDestination(RsTunnelDataItem *item); //invoked when I am the destination of the tunnel

	/* Network Input */
int  handleIncoming();
void recvTunnelData(RsTunnelDataItem *item);
void recvTunnelHandshake(RsTunnelHandshakeItem *item);


	private:

	p3ConnectMgr *mConnMgr;
	pqipersongrp *mPqiPersonGrp;
	std::string ownId;


	/* data */
	RsMutex mTunnelMtx;
};

#endif // MRK_PQI_TUNNEL_H
