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
#include "pqi/pqipersongrp.h"

class p3ConnectMgr;

#include "pqi/pqimonitor.h"
#include "serialiser/rsdiscitems.h"
#include "services/p3service.h"
#include "pqi/authgpg.h"

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

class p3ConnectMgr;


class p3disc: public p3Service, public pqiMonitor, public p3Config, public AuthGPGService
{
	public:


        p3disc(p3ConnectMgr *cm, pqipersongrp *persGrp);

	/************* from pqiMonitor *******************/
virtual void statusChange(const std::list<pqipeer> &plist);
	/************* from pqiMonitor *******************/

int	tick();

	/* GUI requires access */
bool 	potentialproxies(std::string id, std::list<std::string> &proxyIds);
void 	getversions(std::map<std::string, std::string> &versions);

	/************* from AuthGPService ****************/
virtual AuthGPGOperation *getGPGOperation();
virtual void setGPGOperation(AuthGPGOperation *operation);

        protected:
/*****************************************************************/
/***********************  p3config  ******************************/
/* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);
/*****************************************************************/

    private:


void sendAllInfoToJustConnectedPeer(const std::string &id);
void sendJustConnectedPeerInfoToAllPeer(const std::string &id);

	/* Network Output */
//void sendOwnDetails(std::string to);
void sendOwnVersion(std::string to);
RsDiscReply *createDiscReply(const std::string &to, const std::string &about);
//void sendPeerIssuer(std::string to, std::string about);
void sendHeartbeat(std::string to);
void askInfoToAllPeers(std::string about);

	/* Network Input */
int  handleIncoming();
void recvAskInfo(RsDiscAskInfo *item);
void recvPeerDetails(RsDiscReply *item, const std::string &certGpgId);
//void recvPeerIssuerMsg(RsDiscIssuer *item);
void recvPeerVersionMsg(RsDiscVersion *item);
void recvHeartbeatMsg(RsDiscHeartbeat *item);

void removeFriend(std::string ssl_id); //keep tracks of removed friend so we're not gonna add them again immediately

/* handle network shape */
int     addDiscoveryData(std::string fromId, std::string aboutId,
		struct sockaddr_in laddr, struct sockaddr_in raddr,
		uint32_t flags, time_t ts);

int 	idServers();

	private:

	p3ConnectMgr *mConnMgr;
        pqipersongrp *mPqiPersonGrp;
        time_t lastSentHeartbeatTime;

	/* data */
	RsMutex mDiscMtx;

        std::map<std::string, time_t> deletedSSLFriendsIds;

        std::map<std::string, autoneighbour> neighbours;
	std::map<std::string, std::string> versions;

	std::map<std::string, std::list<std::string> > sendIdList;
	std::list<RsDiscReply*> pendingDiscReplyInList;
};




#endif // MRK_PQI_AUTODISC_H
