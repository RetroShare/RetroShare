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

class rsUdpStack;
class UdpStunner;
class p3BitDht;
class UdpRelayReceiver;


class p3NetMgr
{
	public:

        p3NetMgr();

void	setManagers(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr);
void	setAddrAssist(pqiAddrAssist *dhtStun, pqiAddrAssist *proxyStun);

void 	tick();

	/*************** Setup ***************************/
void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

void    addNetListener(pqiNetListener *listener);

bool	checkNetAddress(); /* check our address is sensible */

	/*************** External Control ****************/
bool	shutdown(); /* blocking shutdown call */

	/* a nice simple network configuration */
uint32_t getNetStateMode();
uint32_t getNetworkMode();
uint32_t getNatTypeMode();
uint32_t getNatHoleMode();
uint32_t getConnectModes();




bool    getUPnPState();
bool	getUPnPEnabled();
bool	getDHTEnabled();
bool    getDHTStats(uint32_t &netsize, uint32_t &localnetsize);

bool  getIPServersEnabled();
void  setIPServersEnabled(bool b) ;
void  getIPServersList(std::list<std::string>& ip_servers) ;

bool	getNetStatusLocalOk();
bool	getNetStatusUpnpOk();
bool	getNetStatusDhtOk();
bool	getNetStatusStunOk();
bool	getNetStatusExtraAddressCheckOk();

bool 	getUpnpExtAddress(struct sockaddr_in &addr);
bool 	getExtFinderAddress(struct sockaddr_in &addr);
void 	getNetStatus(pqiNetStatus &status);

void 	setOwnNetConfig(uint32_t netMode, uint32_t visState);
bool 	setLocalAddress(struct sockaddr_in addr);
bool 	setExtAddress(struct sockaddr_in addr);
bool 	setNetworkMode(uint32_t netMode);
bool 	setVisState(uint32_t visState);


virtual bool netAssistFriend(std::string id, bool on);

	/*************** External Control ****************/

	/* access to network details (called through Monitor) */

protected:
	/****************** Internal Interface *******************/
virtual bool enableNetAssistFirewall(bool on);
virtual bool netAssistFirewallEnabled();
virtual bool netAssistFirewallActive();
virtual bool netAssistFirewallShutdown();

virtual bool enableNetAssistConnect(bool on);
virtual bool netAssistConnectEnabled();
virtual bool netAssistConnectActive();
virtual bool netAssistConnectShutdown();
virtual bool netAssistConnectStats(uint32_t &netsize, uint32_t &localnetsize);


/* Assist Firewall */
bool netAssistExtAddress(struct sockaddr_in &extAddr);
bool netAssistFirewallPorts(uint16_t iport, uint16_t eport);

		/* Assist Connect */
//virtual bool netAssistFriend(std::string id, bool on);
virtual bool netAssistSetAddress( struct sockaddr_in &laddr,
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

void 	networkConsistencyCheck();


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
};

#endif // MRK_PQI_NET_MANAGER_HEADER
