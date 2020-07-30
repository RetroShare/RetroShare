/*******************************************************************************
 * libretroshare/src/pqi: p3netmgr.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#pragma once

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"
#include "pqi/pqiassist.h"
#include "pqi/pqinetstatebox.h"
#include "pqi/p3cfgmgr.h"
#include "util/rsthreads.h"
#include "util/rsdebug.h"

class ExtAddrFinder ;
class DNSResolver ;

	/* RS_VIS_STATE_XXXX
	 * determines how public this peer wants to be...
	 *
	 * STD = advertise to Peers / DHT checking etc 
	 * GRAY = share with friends / but not DHT 
	 * DARK = hidden from all 
	 * BROWN? = hidden from friends / but on DHT
	 */



struct pqiNetStatus
{
	pqiNetStatus();

        bool mExtAddrOk;       // have external address.
        bool mExtAddrStableOk; // stable external address.
        bool mUpnpOk;          // upnp is ok.
        bool mDhtOk;           // dht is ok.

	uint32_t mDhtNetworkSize;
	uint32_t mDhtRsNetworkSize;

	struct sockaddr_storage mLocalAddr; // percieved ext addr.
	struct sockaddr_storage mExtAddr; // percieved ext addr.

	bool mResetReq; // Not Used yet!.

	void print(std::ostream &out);
};

class p3PeerMgr;
class p3LinkMgr;

class p3PeerMgrIMPL;
class p3LinkMgrIMPL;

class rsUdpStack;
class UdpStunner;
class p3BitDht;
class UdpRelayReceiver;


#define NETMGR_DHT_FEEDBACK_CONNECTED	0x0001
#define NETMGR_DHT_FEEDBACK_CONN_FAILED 0x0002
#define NETMGR_DHT_FEEDBACK_CONN_CLOSED	0x0003


/**********
 * p3NetMgr Interface....
 * This allows a drop-in replacement for testing.
 */

class p3NetMgr
{
public:

	/*************** External Control ****************/

	// Setup Network State.
virtual bool 	setNetworkMode(uint32_t netMode) = 0;
virtual bool 	setVisState(uint16_t vs_disc, uint16_t vs_dht) = 0;

	// Switch DHT On/Off.
virtual bool netAssistFriend(const RsPeerId &id, bool on) = 0;
virtual bool netAssistKnownPeer(const RsPeerId &id, const struct sockaddr_storage &addr, uint32_t flags) = 0;
virtual bool netAssistBadPeer(const struct sockaddr_storage &addr, uint32_t reason, uint32_t flags, uint32_t age) = 0;
virtual bool netAssistStatusUpdate(const RsPeerId &id, int mode) = 0;

	/* Get Network State */
virtual uint32_t getNetStateMode() = 0;
virtual uint32_t getNetworkMode() = 0;
virtual uint32_t getNatTypeMode() = 0;
virtual uint32_t getNatHoleMode() = 0;
virtual uint32_t getConnectModes() = 0;

	/* Shut It Down! */
virtual bool	shutdown() = 0; /* blocking shutdown call */

        /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

	// THESE SHOULD BE MOVED TO p3PeerMgr (as it controls the config).
	// The functional object should be transformed into a NetAssistFirewall object.
	// ONLY USED by p3peers.cc & p3peermgr.cc
virtual bool  getIPServersEnabled() = 0;
virtual void  setIPServersEnabled(bool b)  = 0;
virtual void  getIPServersList(std::list<std::string>& ip_servers)  = 0;

	// ONLY USED by p3face-config.cc WHICH WILL BE REMOVED.
virtual void 	getNetStatus(pqiNetStatus &status) = 0;
virtual bool    getUPnPState() = 0;
virtual bool	getUPnPEnabled() = 0;
virtual bool	getDHTEnabled() = 0;

	virtual ~p3NetMgr();
};


class p3NetMgrIMPL: public p3NetMgr
{
public:

        p3NetMgrIMPL();

/************************************************************************************************/
/* EXTERNAL INTERFACE */
/************************************************************************************************/

	/*************** External Control ****************/

	// Setup Network State.
virtual bool 	setNetworkMode(uint32_t netMode);
virtual bool 	setVisState(uint16_t vs_disc, uint16_t vs_dht);

	// Switch DHT On/Off.
virtual bool netAssistFriend(const RsPeerId &id, bool on);
virtual bool netAssistKnownPeer(const RsPeerId &id, const struct sockaddr_storage &addr, uint32_t flags);
virtual bool netAssistBadPeer(const struct sockaddr_storage &addr, uint32_t reason, uint32_t flags, uint32_t age);
virtual bool netAssistStatusUpdate(const RsPeerId &id, int mode);

	/* Get Network State */
virtual uint32_t getNetStateMode();
virtual uint32_t getNetworkMode();
virtual uint32_t getNatTypeMode();
virtual uint32_t getNatHoleMode();
virtual uint32_t getConnectModes();

	/* Shut It Down! */
virtual bool	shutdown(); /* blocking shutdown call */

        /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

	// THESE SHOULD BE MOVED TO p3PeerMgr (as it controls the config).
	// The functional object should be transformed into a NetAssistFirewall object.
	// ONLY USED by p3peers.cc & p3peermgr.cc
virtual bool  getIPServersEnabled();
virtual void  setIPServersEnabled(bool b);
virtual void  getIPServersList(std::list<std::string>& ip_servers);

	// ONLY USED by p3face-config.cc WHICH WILL BE REMOVED.
virtual void 	getNetStatus(pqiNetStatus &status);
virtual bool    getUPnPState();
virtual bool	getUPnPEnabled();
virtual bool	getDHTEnabled();

/************************************************************************************************/
/* Extra IMPL Functions (used by p3PeerMgr, p3NetMgr + Setup) */
/************************************************************************************************/

void	setManagers(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr);
#ifdef RS_USE_DHT_STUNNER
void	setAddrAssist(pqiAddrAssist *dhtStun, pqiAddrAssist *proxyStun);
#endif // RS_USE_DHT_STUNNER

void 	tick();

	// THESE MIGHT BE ADDED TO INTERFACE.
bool 	setLocalAddress(const struct sockaddr_storage &addr);
bool 	setExtAddress(const struct sockaddr_storage &addr);
bool 	getExtAddress(sockaddr_storage &addr);

	/*************** Setup ***************************/
void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

void    addNetListener(pqiNetListener *listener);

	// SHOULD MAKE THIS PROTECTED.
bool	checkNetAddress(); /* check our address is sensible */

protected:
	/****************** Internal Interface *******************/
bool enableNetAssistFirewall(bool on);
bool netAssistFirewallEnabled();
bool netAssistFirewallActive();
bool netAssistFirewallShutdown();

bool enableNetAssistConnect(bool on);
bool netAssistConnectEnabled();
bool netAssistConnectActive();
bool netAssistConnectShutdown();
bool netAssistConnectStats(uint32_t &netsize, uint32_t &localnetsize);

void netAssistTick();

/* Assist Firewall */
bool netAssistExtAddress(struct sockaddr_storage &extAddr);
bool netAssistFirewallPorts(uint16_t iport, uint16_t eport);

		/* Assist Connect */
//virtual bool netAssistFriend(std::string id, bool on); (PUBLIC)
bool netAssistSetAddress(const struct sockaddr_storage &laddr,
                                        const struct sockaddr_storage &eaddr,
					uint32_t mode);

bool netAssistAttach(bool on);


	/* Internal Functions */
void 	netReset();

void 	statusTick();
void 	netStatusTick();
void 	netStartup();

	/* startup the bits */
void 	netDhtInit();
void 	netUdpInit();
void 	netStunInit();



void	netInit();

void 	netExtInit();
void 	netExtCheck();

void 	netUpnpInit();
void 	netUpnpCheck();

void    netUnreachableCheck();


	/* net state via NetStateBox */
void 	updateNetStateBox_temporal();
void 	updateNetStateBox_startup();
void 	updateNetStateBox_reset();
    void updateNatSetting();

	/** Conservatively guess new external port, previous approach (aka always
	 * reset it to local port) break setups where external manually
	 * forwarded port is different then local port. A common case is having
	 * SSLH listening on port 80 on the router with public IP forwanding
	 * plain HTTP connections to a web server and --anyprot connections to
	 * retroshare to make censor/BOFH/bad firewall life a little more
	 * difficult */
	uint16_t guessNewExtPort()
	{
		uint16_t newExtPort = sockaddr_storage_port(mExtAddr);
		if(!newExtPort) newExtPort = sockaddr_storage_port(mLocalAddr);
		return newExtPort;
	}

private:
	// These should have there own Mutex Protection,
	ExtAddrFinder *mExtAddrFinder ;

	/* These are considered static from a MUTEX perspective */
	std::map<uint32_t, pqiNetAssistFirewall *> mFwAgents;
	std::map<uint32_t, pqiNetAssistConnect  *> mDhts;

        std::list<pqiNetListener *> mNetListeners;

	p3PeerMgr *mPeerMgr; 
	p3LinkMgr *mLinkMgr; 

	//p3BitDht   *mBitDht;
#ifdef RS_USE_DHT_STUNNER
	pqiAddrAssist *mDhtStunner = nullptr;
	pqiAddrAssist *mProxyStunner = nullptr;
#endif // RS_USE_DHT_STUNNER

	RsMutex mNetMtx; /* protects below */

void 	netStatusReset_locked();

	// TODO: Sat Oct 24 15:51:24 CEST 2015 The fact of having just two possible address is a flawed assumption, this should be redesigned soon.
	struct sockaddr_storage mLocalAddr;
	struct sockaddr_storage mExtAddr;

	uint32_t mNetMode;
	uint16_t mVsDisc;
	uint16_t mVsDht;

	rstime_t   mNetInitTS;
	uint32_t mNetStatus;

	bool     mStatusChanged;

	bool mUseExtAddrFinder;

	/* network status flags (read by rsiface) */
	pqiNetStatus mNetFlags;
	pqiNetStatus mOldNetFlags;


	// Improved NetStatusBox, which uses the Stunners!
	pqiNetStateBox mNetStateBox;

	rstime_t mDoNotNetCheckUntilTs;
	uint32_t mOldNatType;
	uint32_t mOldNatHole;

	RS_SET_CONTEXT_DEBUG_LEVEL(2)
};
