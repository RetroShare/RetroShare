/*******************************************************************************
 * librssimulator/peer/: PeerNode.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <list>
#include <string.h>
#include <retroshare/rsids.h>

#include <retroshare/rspeers.h>
#include <turtle/p3turtle.h>
#include <serialiser/rsserial.h>
#include <pqi/p3linkmgr.h>
#include <pqi/p3peermgr.h>
#include <ft/ftserver.h>
#include <ft/ftcontroller.h>
#include <services/p3service.h>

#include "PeerNode.h"

#include "FakeLinkMgr.h"
#include "FakePeerMgr.h"
#include "FakeNetMgr.h"
#include "FakePublisher.h"
#include "FakeServiceControl.h"
#include "FakeNxsNetMgr.h"


PeerNode::PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends, bool online)
	: _id(id)
{
	// add a service server.
	mLinkMgr = new FakeLinkMgr(id, friends, online) ;
	mPeerMgr = new FakePeerMgr(id, friends) ;
	mPublisher = new FakePublisher() ;
	mNxsNetMgr = new FakeNxsNetMgr(mLinkMgr);

	_service_control = new FakeServiceControl(mLinkMgr) ;
	_service_server = new p3ServiceServer(mPublisher,_service_control);
}

PeerNode::~PeerNode()
{
	delete _service_server ;
	delete mPublisher;
	delete mPeerMgr;
	delete mLinkMgr;
}

void PeerNode::AddService(pqiService *service)
{
	_service_server->addService(service, true);
}

void PeerNode::AddPqiMonitor(pqiMonitor *service)
{
	mPqiMonitors.push_back(service);
}

void PeerNode::AddPqiServiceMonitor(pqiServiceMonitor *service)
{
	mPqiServiceMonitors.push_back(service);
}

p3NetMgr *PeerNode::getNetMgr()
{ 
	return mNetMgr; 
}

p3LinkMgr *PeerNode::getLinkMgr()
{ 
	return mLinkMgr; 
}


p3PeerMgr *PeerNode::getPeerMgr()
{ 
	return mPeerMgr; 
}


RsNxsNetMgr *PeerNode::getNxsNetMgr()
{ 
	return mNxsNetMgr; 
}

void PeerNode::notifyOfFriends()
{
	std::list<RsPeerId> friendList;
	mLinkMgr->getFriendList(friendList);

	std::list<RsPeerId>::iterator it;
	for(it = friendList.begin(); it != friendList.end(); it++)
	{
		pqipeer peer;
		peer.id = *it;
		peer.state = RS_PEER_S_FRIEND;
		peer.actions = RS_PEER_NEW;
	}
}

void PeerNode::bringOnline(std::list<RsPeerId> &onlineList)
{
	std::list<RsPeerId>::iterator it;
	std::list<pqipeer> pqiMonitorChanges;
	std::list<pqiServicePeer> pqiServiceMonitorChanges;

	for(it = onlineList.begin(); it != onlineList.end(); it++)
	{
		mLinkMgr->setOnlineStatus(*it, true);

		pqipeer peer;
		peer.id = *it;
		peer.state = RS_PEER_S_FRIEND | RS_PEER_S_CONNECTED;
		peer.actions = RS_PEER_CONNECTED;

		pqiMonitorChanges.push_back(peer);

		pqiServicePeer sp;
		sp.id = *it;
		sp.actions = RS_SERVICE_PEER_CONNECTED;

		pqiServiceMonitorChanges.push_back(sp);
	}

        std::list<pqiMonitor *>::iterator pit;
	for(pit = mPqiMonitors.begin(); pit != mPqiMonitors.end(); pit++)
	{
		(*pit)->statusChange(pqiMonitorChanges);
	}

        std::list<pqiServiceMonitor *>::iterator spit;
	for(spit = mPqiServiceMonitors.begin(); spit != mPqiServiceMonitors.end(); spit++)
	{
		(*spit)->statusChange(pqiServiceMonitorChanges);
	}
}


void PeerNode::takeOffline(std::list<RsPeerId> &offlineList)
{
	std::list<RsPeerId>::iterator it;
	std::list<pqipeer> pqiMonitorChanges;
	std::list<pqiServicePeer> pqiServiceMonitorChanges;

	for(it = offlineList.begin(); it != offlineList.end(); it++)
	{
		mLinkMgr->setOnlineStatus(*it, false);

		pqipeer peer;
		peer.id = *it;
		peer.state = RS_PEER_S_FRIEND;
		peer.actions = RS_PEER_DISCONNECTED;

		pqiMonitorChanges.push_back(peer);

		pqiServicePeer sp;
		sp.id = *it;
		sp.actions = RS_SERVICE_PEER_DISCONNECTED;

		pqiServiceMonitorChanges.push_back(sp);
	}

        std::list<pqiMonitor *>::iterator pit;
	for(pit = mPqiMonitors.begin(); pit != mPqiMonitors.end(); pit++)
	{
		(*pit)->statusChange(pqiMonitorChanges);
	}

        std::list<pqiServiceMonitor *>::iterator spit;
	for(spit = mPqiServiceMonitors.begin(); spit != mPqiServiceMonitors.end(); spit++)
	{
		(*spit)->statusChange(pqiServiceMonitorChanges);
	}
}






void PeerNode::tick()
{
#ifdef DEBUG
	std::cerr << "  ticking peer node " << _id << std::endl;
#endif
	_service_server->tick() ;
}

void PeerNode::incoming(RsRawItem *item)
{
#ifdef DEBUG
	std::cerr << "PeerNode::incoming()" << std::endl;
#endif
	_service_server->recvItem(item) ;
}

RsRawItem *PeerNode::outgoing()
{
	return mPublisher->outgoing() ;
}

bool PeerNode::haveOutgoingPackets()
{
	return (!(mPublisher->outgoingEmpty()));
}

