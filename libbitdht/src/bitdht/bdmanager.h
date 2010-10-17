#ifndef BITDHT_MANAGER_H
#define BITDHT_MANAGER_H

/*
 * bitdht/bdmanager.h
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */



/*******
 * Node Manager.
 ******/

/******************************************
 * 1) Maintains a list of ids to search for.
 * 2) Sets up initial search for own node.
 * 3) Checks on status of queries.
 * 4) Callback on successful searches.
 *
 * This is pretty specific to RS requirements.
 ****/

#define BITDHT_PS_MASK_ACTIONS  	(0x000000ff)
#define BITDHT_PS_MASK_STATE    	(0x0000ff00)

#define BITDHT_PS_ACTION_SEARCHING 	(0x00000001)
#define BITDHT_PS_ACTION_WAITING	(0x00000002)
#define BITDHT_PS_ACTION_PINGING	(0x00000004)

#define BITDHT_PS_STATE_UNKNOWN		(0x00000100)
#define BITDHT_PS_STATE_OFFLINE		(0x00000200)
#define BITDHT_PS_STATE_ONLINE		(0x00000400)
#define BITDHT_PS_STATE_CONNECTED	(0x00000800)

#include "bdiface.h"
#include "bdnode.h"



class bdQueryPeer
{
	public:
	bdId mId;
	uint32_t mStatus;
	uint32_t mQFlags;
	time_t mLastQuery;
	time_t mLastFound;
	struct sockaddr_in mDhtAddr;
};


#define BITDHT_MGR_STATE_OFF		0
#define BITDHT_MGR_STATE_STARTUP	1
#define BITDHT_MGR_STATE_FINDSELF	2
#define BITDHT_MGR_STATE_ACTIVE 	3
#define BITDHT_MGR_STATE_REFRESH 	4
#define BITDHT_MGR_STATE_QUIET		5
#define BITDHT_MGR_STATE_FAILED		6

#define MAX_STARTUP_TIME 10
#define MAX_REFRESH_TIME 10

#define BITDHT_MGR_QUERY_FAILURE		1
#define BITDHT_MGR_QUERY_PEER_OFFLINE		2
#define BITDHT_MGR_QUERY_PEER_UNREACHABLE	3
#define BITDHT_MGR_QUERY_PEER_ONLINE		4


/*** NB: Nothing in here is protected by mutexes 
 * must be done at a higher level!
 ***/

class bdNodeManager: public bdNode, public BitDhtInterface
{
	public:
        bdNodeManager(bdNodeId *id, std::string dhtVersion, std::string bootfile, bdDhtFunctions *fns);


void 	iteration();

        /***** Functions to Call down to bdNodeManager ****/
        /* Request DHT Peer Lookup */
        /* Request Keyword Lookup */
virtual void addFindNode(bdNodeId *id, uint32_t mode);
virtual void removeFindNode(bdNodeId *id);
virtual void findDhtValue(bdNodeId *id, std::string key, uint32_t mode);

        /***** Add / Remove Callback Clients *****/
virtual void addCallback(BitDhtCallback *cb);
virtual void removeCallback(BitDhtCallback *cb);

        /***** Get Results Details *****/
virtual int getDhtPeerAddress(const bdNodeId *id, struct sockaddr_in &from);
virtual int getDhtValue(const bdNodeId *id, std::string key, std::string &value);

	/* stats and Dht state */
virtual int startDht();
virtual int stopDht();
virtual int stateDht(); /* STOPPED, STARTING, ACTIVE, FAILED */
virtual uint32_t statsNetworkSize();
virtual uint32_t statsBDVersionSize(); /* same version as us! */
        /******************* Internals *************************/

	// Overloaded from bdnode for external node callback.
virtual void addPeer(const bdId *id, uint32_t peerflags);


int 	isBitDhtPacket(char *data, int size, struct sockaddr_in &from);
	private:


void 	doNodeCallback(const bdId *id, uint32_t peerflags);
void 	doPeerCallback(const bdNodeId *id, uint32_t status);
void 	doValueCallback(const bdNodeId *id, std::string key, uint32_t status);

int	status();
int	checkStatus();
int 	checkPingStatus();
int 	SearchOutOfDate();
void	startQueries();

	std::map<bdNodeId, bdQueryPeer>	mActivePeers;
        std::list<BitDhtCallback *> mCallbacks;

	uint32_t mMode;
	time_t   mModeTS;

        bdDhtFunctions *mFns;

	uint32_t mNetworkSize;
	uint32_t mBdNetworkSize;

	/* future node functions */
	//addPeerPing(foundId);
	//clearPing(it->first);
	//PingStatus(it->first);
};

class bdDebugCallback: public BitDhtCallback
{
        public:
        ~bdDebugCallback();
virtual int dhtPeerCallback(const bdNodeId *id, uint32_t status);
virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status);

};


#endif
