/*******************************************************************************
 * libretroshare/src/pqi: p3dhtmgr.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie.                                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/

#ifndef MRK_P3_DHT_MANAGER_HEADER
#define MRK_P3_DHT_MANAGER_HEADER

/* Interface class for DHT data */

#include <string>
#include <map>
#include "pqi/pqinetwork.h"

#include "util/rsthreads.h"
#include "pqi/pqimonitor.h"

#include "pqi/pqiassist.h"

/* All other #defs are in .cc */
#define DHT_ADDR_INVALID        0xff
#define DHT_ADDR_TCP            0x01
#define DHT_ADDR_UDP            0x02


/* for DHT peer STATE */
#define DHT_PEER_OFF            0
#define DHT_PEER_INIT           1
#define DHT_PEER_SEARCH         2
#define DHT_PEER_FOUND          3

/* for DHT peer STATE (ownEntry) */
#define DHT_PEER_ADDR_KNOWN     4
#define DHT_PEER_PUBLISHED      5

/* Interface with Real DHT Implementation */
#define DHT_MODE_SEARCH         1
#define DHT_MODE_PUBLISH        1
#define DHT_MODE_NOTIFY         2
#define DHT_MODE_BOOTSTRAP	3


/* TIMEOUTS: Reference Values are set here... */

#define DHT_SEARCH_PERIOD       1800 /* PeerKeys: if we haven't found them: 30 min */
#define DHT_CHECK_PERIOD        1800 /* PeerKeys: re-lookup peer: 30 min */
#define DHT_PUBLISH_PERIOD      1800 /* OwnKey: 30 min */
#define DHT_NOTIFY_PERIOD       300  /* 5 min - Notify Check period */

/* TTLs for DHTs posts */
#define DHT_TTL_PUBLISH         (DHT_PUBLISH_PERIOD + 120)  // for a little overlap.
#define DHT_TTL_NOTIFY          (DHT_NOTIFY_PERIOD  + 60)   // for time to find it...
#define DHT_TTL_BOOTSTRAP       (DHT_PUBLISH_PERIOD)        // To start with.

class dhtPeerEntry
{
        public:
	dhtPeerEntry();

        RsPeerId id;
        uint32_t state;
        rstime_t lastTS;

	uint32_t notifyPending;
	rstime_t   notifyTS;

        struct sockaddr_in laddr, raddr;
	uint32_t type;  /* ADDR_TYPE as defined above */

	std::string hash1; /* SHA1 Hash of id */
	std::string hash2; /* SHA1 Hash of reverse Id */
};

class p3DhtMgr: public pqiNetAssistConnect, public RsThread
{
	/* 
	 */
	public:
	p3DhtMgr(RsPeerId id, pqiConnectCb *cb);

	/********** External DHT Interface ************************
	 * These Functions are the external interface
	 * for the DHT, and must be non-blocking and return quickly
	 */

	/* OVERLOADED From pqiNetAssistConnect. */

virtual void enable(bool on);
virtual void shutdown();
virtual void restart();

virtual bool getEnabled(); /* on */
virtual bool getActive();  /* actually working */

virtual void	setBootstrapAllowed(bool on);
virtual bool 	getBootstrapAllowed();

	/* set key data */
virtual bool 	setExternalInterface(struct sockaddr_in laddr,
			struct sockaddr_in raddr, uint32_t type);

	/* add / remove peers */
virtual bool findPeer(const RsPeerId& id);
virtual bool dropPeer(const RsPeerId& id);

	/* post DHT key saying we should connect (callback when done) */
virtual bool notifyPeer(const RsPeerId& id);

	/* extract current peer status */
virtual bool getPeerStatus(const RsPeerId& id,
			struct sockaddr_storage &laddr, struct sockaddr_storage &raddr,
			uint32_t &type, uint32_t &mode);

	/* stun */
virtual bool 	enableStun(bool on);
virtual bool 	addStun(std::string id);
	//doneStun();

	/********** Higher Level DHT Work Functions ************************
	 * These functions translate from the strings/addresss to 
	 * key/value pairs.
	 */
	public:

	/* results from DHT proper */
virtual bool dhtResultNotify(std::string id);
virtual bool dhtResultSearch(std::string id,
		struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
				uint32_t type, std::string sign);

virtual bool dhtResultBootstrap(std::string idhash);

	protected:

	/* can block briefly (called only from thread) */
virtual bool dhtPublish(std::string idhash,
		struct sockaddr_in &laddr, 
		struct sockaddr_in &raddr, 
				uint32_t type, std::string sign);

virtual bool dhtNotify(std::string idhash, std::string ownIdHash,
		std::string sign);

virtual	bool dhtSearch(std::string idhash, uint32_t mode);

virtual bool dhtBootstrap(std::string idhash, std::string ownIdHash,
		std::string sign); /* to publish bootstrap */



	/********** Actual DHT Work Functions ************************
	 * These involve a very simple LOW-LEVEL interface ... 
	 *
	 * publish
	 * search
	 * result
	 *
	 */

	public:

	/* Feedback callback (handled here) */
virtual bool resultDHT(std::string key, std::string value);

	protected:

virtual bool    dhtInit();
virtual bool	dhtShutdown();
virtual bool    dhtActive();
virtual int     status(std::ostream &out);

virtual bool publishDHT(std::string key, std::string value, uint32_t ttl);
virtual	bool searchDHT(std::string key);



	/********** Internal DHT Threading ************************
	 *
	 */

	public:

virtual void run();

	private:

	/* search scheduling */
void 	checkDHTStatus();
int 	checkStunState();
int 	checkStunState_Active(); /* when in active state */
int 	doStun();
int 	checkPeerDHTKeys();
int 	checkOwnDHTKeys();
int 	checkNotifyDHT();

void 	clearDhtData();

	/* IP Bootstrap */
bool 	getDhtBootstrapList();
std::string BootstrapId(uint32_t bin);
std::string randomBootstrapId();

	/* other feedback through callback */
	// use pqiNetAssistConnect.. version pqiConnectCb *connCb;

	/* protected by Mutex */
	RsMutex dhtMtx;

	bool 	 mDhtOn; /* User desired state */
	bool     mDhtModifications; /* any user requests? */

	dhtPeerEntry ownEntry;
	rstime_t ownNotifyTS;
	std::map<RsPeerId, dhtPeerEntry> peers;

	std::list<std::string> stunIds;
	bool     mStunRequired;

	uint32_t mDhtState;
	rstime_t   mDhtActiveTS;

	bool   mBootstrapAllowed;
	rstime_t mLastBootstrapListTS;
};


#endif // MRK_P3_DHT_MANAGER_HEADER


