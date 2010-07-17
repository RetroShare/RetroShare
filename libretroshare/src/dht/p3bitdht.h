/*
 * libretroshare/src/dht: p3bitdht.h
 *
 * BitDht interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
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


#ifndef MRK_P3_BITDHT_H
#define MRK_P3_BITDHT_H

#include "pqi/pqiassist.h"

#include <string>
#include <map>
#include "pqi/pqinetwork.h"
#include "pqi/pqimonitor.h"
#include "util/rsthreads.h"

#include "udp/udpstack.h"
#include "udp/udpbitdht.h"
#include "bitdht/bdiface.h"

class p3BitDht: public pqiNetAssistConnect
{
	public:
	p3BitDht(std::string id, pqiConnectCb *cb, 
		UdpStack *udpstack, std::string bootstrapfile);

virtual	~p3BitDht();

	/* pqiNetAssist - external interface functions */
virtual void    enable(bool on);  
virtual void    shutdown(); /* blocking call */
virtual void	restart();

virtual bool    getEnabled();
virtual bool    getActive();

	/* pqiNetAssistConnect - external interface functions */

	/* add / remove peers */
virtual bool 	findPeer(std::string id);
virtual bool 	dropPeer(std::string id);

	/* extract current peer status */
virtual bool 	getPeerStatus(std::string id, struct sockaddr_in &raddr, 
					uint32_t &mode);

virtual bool 	getExternalInterface(struct sockaddr_in &raddr, 
					uint32_t &mode);

	/* Callback functions - from bitdht */
int 	NodeCallback(const bdId *id, uint32_t peerflags);
int 	PeerCallback(const bdNodeId *id, uint32_t status);
int 	ValueCallback(const bdNodeId *id, std::string key, uint32_t status);

	private:
	/* translation stuff */
	int calculateNodeId(const std::string pid, bdNodeId *id);

	int lookupNodeId(const std::string pid, bdNodeId *id);
	int lookupRsId(const bdNodeId *id, std::string &pid);
	int storeTranslation(const std::string pid);
	int removeTranslation(const std::string pid);

	UdpBitDht *mUdpBitDht; /* has own mutex */

	RsMutex dhtMtx;
	/* translation maps */
        std::map<std::string, bdNodeId> mTransToNodeId;
        std::map<bdNodeId, std::string> mTransToRsId;

};

#endif /* MRK_P3_BITDHT_H */

