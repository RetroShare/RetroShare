/*******************************************************************************
 * librssimulator/peer/: PeerNode.h                                            *
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
#pragma once

#include <list>

#include "retroshare/rsids.h"

class RsRawItem ;
class p3ServiceServer ;
class FakePublisher;
class p3ServiceControl;
class FakeLinkMgr;
class FakePeerMgr;
class FakeNetMgr;
class RsNxsNetMgr;
class FakeNxsNetMgr;
class pqiMonitor;
class pqiServiceMonitor;
class pqiService;

class PeerNode
{
	public:
		PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends, bool online) ;
		~PeerNode() ;

		RsRawItem *outgoing() ;
		void incoming(RsRawItem *) ;
		bool haveOutgoingPackets();

		const RsPeerId& id() const { return _id ;}

		void tick() ;

		p3ServiceControl *getServiceControl() const { return _service_control; }
		p3LinkMgr *getLinkMgr();
		p3PeerMgr *getPeerMgr();
		p3NetMgr *getNetMgr();
		RsNxsNetMgr *getNxsNetMgr();

		void AddService(pqiService *service);
		void AddPqiMonitor(pqiMonitor *service);
		void AddPqiServiceMonitor(pqiServiceMonitor *service);

		// 
		void notifyOfFriends();
		void bringOnline(std::list<RsPeerId> &friends);
		void takeOffline(std::list<RsPeerId> &friends);

	private:
		RsPeerId _id;

		p3ServiceServer *_service_server ;
		FakePublisher *mPublisher ;
		p3ServiceControl *_service_control; 
		FakeLinkMgr *mLinkMgr;
		FakePeerMgr *mPeerMgr;
		FakeNetMgr *mNetMgr;
		FakeNxsNetMgr *mNxsNetMgr;

		/* for monitors */
		std::list<pqiMonitor *> mPqiMonitors;
		std::list<pqiServiceMonitor *> mPqiServiceMonitors;
};





