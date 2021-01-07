/*******************************************************************************
 * bitdht/bdmanager.h                                                          *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef BITDHT_MANAGER_H
#define BITDHT_MANAGER_H

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

#include "bitdht/bdiface.h"
#include "bitdht/bdnode.h"
#include "util/bdbloom.h"



class bdQueryPeer
{
	public:
	bdId mId;
	uint32_t mStatus;
	uint32_t mQFlags;
	//time_t mLastQuery;
	//time_t mLastFound;
	struct sockaddr_in mDhtAddr;
	time_t mCallbackTS;   // for UPDATES flag.
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
		bdNodeManager(bdNodeId *id, std::string dhtVersion, std::string bootfile, std::string bootfilebak, const std::string &filterfile, bdDhtFunctions *fns);


void 	iteration();

        /***** Functions to Call down to bdNodeManager ****/


        /* Friend Tracking */
virtual void addBadPeer(const struct sockaddr_in &addr, uint32_t source, uint32_t reason, uint32_t age);
virtual void updateKnownPeer(const bdId *id, uint32_t type, uint32_t flags);

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
virtual int getDhtBucket(const int idx, bdBucket &bucket);

virtual int getDhtQueries(std::map<bdNodeId, bdQueryStatus> &queries);
virtual int getDhtQueryStatus(const bdNodeId *id, bdQuerySummary &query);

	/***** Connection Interface ****/
virtual bool ConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start);
virtual void ConnectionAuth(bdId *srcId, bdId *proxyId, bdId *destId, 
						uint32_t mode, uint32_t loc, uint32_t bandwidth, uint32_t delay, uint32_t answer);
virtual void ConnectionOptions(uint32_t allowedModes, uint32_t flags);

virtual bool setAttachMode(bool on);

	/* stats and Dht state */
virtual int startDht();
virtual int stopDht();
virtual int stateDht(); /* STOPPED, STARTING, ACTIVE, FAILED */
virtual uint32_t statsNetworkSize();
virtual uint32_t statsBDVersionSize(); /* same version as us! */

virtual uint32_t setDhtMode(uint32_t dhtFlags);

        /******************* Internals *************************/

	// Overloaded from bdnode for external node callback.
virtual void addPeer(const bdId *id, uint32_t peerflags);
        // Overloaded from bdnode for external node callback.
virtual void callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId,
                                        int mode, int point, int param, int cbtype, int errcode); 

int 	isBitDhtPacket(char *data, int size, struct sockaddr_in &from);

	// this function is used by bdFilter (must be public!)
void	doIsBannedCallback(const sockaddr_in *addr, bool *isAvailable, bool* isBanned);

private:


void 	doNodeCallback(const bdId *id, uint32_t peerflags);
void 	doPeerCallback(const bdId *id, uint32_t status);
void 	doValueCallback(const bdNodeId *id, std::string key, uint32_t status);
void    doInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info);

int	status();
int	checkStatus();
int 	checkPingStatus();
int	checkBadPeerStatus();
int 	SearchOutOfDate();
void	startQueries();

int     QueryRandomLocalNet();
void    SearchForLocalNet();

	std::map<bdNodeId, bdQueryPeer>	mActivePeers;
        std::list<BitDhtCallback *> mCallbacks;

	uint32_t mMode;
	time_t   mModeTS;

        time_t mStartTS;
        time_t mSearchTS;
        bool mSearchingDone;

	bdDhtFunctions *mDhtFns;

	uint32_t mNetworkSize;
	uint32_t mBdNetworkSize;

	bdBloom mBloomFilter;

        bool mLocalNetEnhancements;

	/* future node functions */
	//addPeerPing(foundId);
	//clearPing(it->first);
	//PingStatus(it->first);
};

class bdDebugCallback: public BitDhtCallback
{
        public:
        ~bdDebugCallback();
virtual int dhtPeerCallback(const bdId *id, uint32_t status);
virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status);
virtual int dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,             
                         uint32_t mode, uint32_t point, uint32_t param, uint32_t cbtype, uint32_t errcode);
virtual int dhtInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info);

};


#endif
