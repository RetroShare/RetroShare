/*
 * libretroshare/src/pqi: p3netmgr.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef MRK_PQI_NET_MANAGER_HEADER
#define MRK_PQI_NET_MANAGER_HEADER

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"

//#include "pqi/p3dhtmgr.h"
//#include "pqi/p3upnpmgr.h"
#include "pqi/pqiassist.h"

#include "pqi/pqinetstatebox.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

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



class pqiNetStatus
{
	public:

	pqiNetStatus();

        bool mLocalAddrOk;     // Local address is not loopback.
        bool mExtAddrOk;       // have external address.
        bool mExtAddrStableOk; // stable external address.
        bool mUpnpOk;          // upnp is ok.
        bool mDhtOk;           // dht is ok.

	uint32_t mDhtNetworkSize;
	uint32_t mDhtRsNetworkSize;

	struct sockaddr_in mLocalAddr; // percieved ext addr.
	struct sockaddr_in mExtAddr; // percieved ext addr.

	bool mResetReq; // Not Used yet!.

	void print(std::ostream &out);

	bool NetOk() // minimum to believe network is okay.`
	{
		return (mLocalAddrOk && mExtAddrOk);
	}
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

        p3NetMgr() { return; }
virtual ~p3NetMgr() { return; }


	/*************** External Control ****************/

	// Setup Network State.
virtual bool 	setNetworkMode(uint32_t netMode) = 0;
virtual bool 	setVisState(uint32_t visState) = 0;

	// Switch DHT On/Off.
virtual bool netAssistFriend(std::string id, bool on) = 0;
virtual bool netAssistStatusUpdate(std::string id, int mode) = 0;

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


/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/

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
virtual bool 	setVisState(uint32_t visState);

	// Switch DHT On/Off.
virtual bool netAssistFriend(std::string id, bool on);
virtual bool netAssistStatusUpdate(std::string id, int mode);

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
void	setAddrAssist(pqiAddrAssist *dhtStun, pqiAddrAssist *proxyStun);

void 	tick();

	// THESE MIGHT BE ADDED TO INTERFACE.
bool 	setLocalAddress(struct sockaddr_in addr);
bool 	setExtAddress(struct sockaddr_in addr);

	/*************** Setup ***************************/
void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

void    addNetListener(pqiNetListener *listener);

	// SHOULD MAKE THIS PROTECTED.
bool	checkNetAddress(); /* check our address is sensible */


protected:

void 	slowTick();

	/* THESE FUNCTIONS ARE ON_LONGER EXTERNAL - CAN THEY BE REMOVED? */
//bool    getDHTStats(uint32_t &netsize, uint32_t &localnetsize);

//bool	getNetStatusLocalOk();
//bool	getNetStatusUpnpOk();
//bool	getNetStatusDhtOk();
//bool	getNetStatusStunOk();
//bool	getNetStatusExtraAddressCheckOk();

//bool 	getUpnpExtAddress(struct sockaddr_in &addr);
//bool 	getExtFinderAddress(struct sockaddr_in &addr);

//void 	setOwnNetConfig(uint32_t netMode, uint32_t visState);


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
void 		netAssistConnectTick();

/* Assist Firewall */
bool netAssistExtAddress(struct sockaddr_in &extAddr);
bool netAssistFirewallPorts(uint16_t iport, uint16_t eport);

		/* Assist Connect */
//virtual bool netAssistFriend(std::string id, bool on); (PUBLIC)
bool netAssistSetAddress( struct sockaddr_in &laddr,
                                        struct sockaddr_in &eaddr,
					uint32_t mode);


	/* Internal Functions */
void 	netReset();

void 	statusTick();
void 	netTick();
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
void    updateNatSetting();

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
	pqiAddrAssist *mDhtStunner;
	pqiAddrAssist *mProxyStunner;

	RsMutex mNetMtx; /* protects below */

void 	netStatusReset_locked();

	struct sockaddr_in mLocalAddr;
	struct sockaddr_in mExtAddr;

	uint32_t mNetMode;
	uint32_t mVisState;

	time_t   mNetInitTS;
	uint32_t mNetStatus;

	bool     mStatusChanged;

	bool mUseExtAddrFinder;

	/* network status flags (read by rsiface) */
	pqiNetStatus mNetFlags;
	pqiNetStatus mOldNetFlags;


	// Improved NetStatusBox, which uses the Stunners!
	pqiNetStateBox mNetStateBox;

	time_t mLastSlowTickTime;
	uint32_t mOldNatType;
	uint32_t mOldNatHole;

};

#endif // MRK_PQI_NET_MANAGER_HEADER
