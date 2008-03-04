/*
 * libretroshare/src/services: p3disc.h
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

#ifndef MRK_PQI_AUTODISC_H
#define MRK_PQI_AUTODISC_H

// The AutoDiscovery Class

#include <string>
#include <list>

// system specific network headers
#include "pqi/pqinetwork.h"

#include "pqi/pqi.h"

class p3ConnectMgr;
class p3AuthMgr;

#include "pqi/pqimonitor.h"
#include "serialiser/rsdiscitems.h"
#include "services/p3service.h"

class autoserver
{
	public:
		autoserver()
		:ts(0), discFlags(0) { return;}

		std::string id;
		struct sockaddr_in localAddr;
		struct sockaddr_in remoteAddr;

		time_t ts;
		uint32_t discFlags;
};


class autoneighbour: public autoserver
{
	public:
		autoneighbour()
		:autoserver(), authoritative(false) {}

		bool authoritative;
		bool validAddrs;

		std::map<std::string, autoserver> neighbour_of;

};

class p3AuthMgr;
class p3ConnectMgr;


class p3disc: public p3Service, public pqiMonitor
{
	public:


        p3disc(p3AuthMgr *am, p3ConnectMgr *cm);

	/************* from pqiMonitor *******************/
virtual void statusChange(const std::list<pqipeer> &plist);
	/************* from pqiMonitor *******************/

int	tick();

	/* GUI requires access */
bool 	potentialproxies(std::string id, std::list<std::string> &proxyIds);

	private:


void respondToPeer(std::string id);

	/* Network Output */
void sendOwnDetails(std::string to);
void sendPeerDetails(std::string to, std::string about);

	/* Network Input */
int  handleIncoming();
void recvPeerOwnMsg(RsDiscItem *item);
void recvPeerFriendMsg(RsDiscReply *item);

	/* handle network shape */
int     addDiscoveryData(std::string fromId, std::string aboutId,
		struct sockaddr_in laddr, struct sockaddr_in raddr, 
		uint32_t flags, time_t ts);

int 	idServers();


	private:

	p3AuthMgr *mAuthMgr;
	p3ConnectMgr *mConnMgr;


	/* data */
	RsMutex mDiscMtx;

	bool mRemoteDisc;
	bool mLocalDisc;

	std::map<std::string, autoneighbour> neighbours;

};




#endif // MRK_PQI_AUTODISC_H
