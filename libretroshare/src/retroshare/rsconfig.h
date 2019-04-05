#ifndef RETROSHARE_CONFIG_GUI_INTERFACE_H
#define RETROSHARE_CONFIG_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsconfig.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#include <inttypes.h>
#include <retroshare/rstypes.h>
#include <string>
#include <list>
#include <map>

/* The New Config Interface Class */
class RsServerConfig;
extern RsServerConfig *rsConfig;

#define RSNET_NETWORK_UNKNOWN		1
#define RSNET_NETWORK_RESTARTING	2
#define RSNET_NETWORK_OFFLINE		3
#define RSNET_NETWORK_LOCALNET		4
#define RSNET_NETWORK_BEHINDNAT		5
#define RSNET_NETWORK_EXTERNALIP	6

// WHAT TYPE OF FIREWALL?
#define RSNET_NATTYPE_NONE		1
#define RSNET_NATTYPE_UNKNOWN		2
#define RSNET_NATTYPE_SYMMETRIC 	3
#define RSNET_NATTYPE_DETERM_SYM 	4
#define RSNET_NATTYPE_RESTRICTED_CONE	5
#define RSNET_NATTYPE_FULL_CONE		6
#define RSNET_NATTYPE_OTHER		7

// WHAT TYPE OF HOLE?
#define RSNET_NATHOLE_UNKNOWN		0		
#define RSNET_NATHOLE_NONE		1		
#define RSNET_NATHOLE_UPNP		2	
#define RSNET_NATHOLE_NATPMP		3
#define RSNET_NATHOLE_FORWARDED		4

// Types of Connections.
#define RSNET_CONNECT_NONE		0x0000
#define RSNET_CONNECT_ACCEPT_TCP	0x0001
#define RSNET_CONNECT_OUTGOING_TCP	0x0002
#define RSNET_CONNECT_DIRECT_UDP	0x0100
#define RSNET_CONNECT_PROXY_UDP		0x0200
#define RSNET_CONNECT_RELAY_UDP		0x0400

// net state (good, okay, bad)
// BAD. (RED)
#define RSNET_NETSTATE_BAD_UNKNOWN	1
#define RSNET_NETSTATE_BAD_OFFLINE	2
#define RSNET_NETSTATE_BAD_NATSYM	3
#define RSNET_NETSTATE_BAD_NODHT_NAT	4

// CAUTION. (ORANGE)
#define RSNET_NETSTATE_WARNING_RESTART	5
#define RSNET_NETSTATE_WARNING_NATTED	6
#define RSNET_NETSTATE_WARNING_NODHT	7

// GOOD (GREEN)
// NAT with forwarded port, or EXT port.
#define RSNET_NETSTATE_GOOD		8

// ADVANCED MODE (BLUE)
// If the user knows what they are doing... we cannot confirm this.
#define RSNET_NETSTATE_ADV_FORWARD	9
#define RSNET_NETSTATE_ADV_DARK_FORWARD	10


/* matched to the uPnP states */
#define UPNP_STATE_UNINITIALISED  0
#define UPNP_STATE_UNAVAILABILE   1
#define UPNP_STATE_READY          2
#define UPNP_STATE_FAILED_TCP     3
#define UPNP_STATE_FAILED_UDP     4
#define UPNP_STATE_ACTIVE         5




/************************** Indicate How experienced the RsUser is... based on Friends / Firewall status ******/

#define RSCONFIG_USER_LEVEL_NEW		0x0001		/* no friends */
#define RSCONFIG_USER_LEVEL_BASIC	0x0002		/* no connections */
#define RSCONFIG_USER_LEVEL_CASUAL	0x0003		/* firewalled */
#define RSCONFIG_USER_LEVEL_POWER	0x0004		/* good! */
#define RSCONFIG_USER_LEVEL_OVERRIDE	0x0005		/* forced to POWER level */



// Must Match up with strings internal to Retroshare.
#define RS_CONFIG_ADVANCED		0x0101


#define RS_OPMODE_FULL		0x0001
#define RS_OPMODE_NOTURTLE	0x0002
#define RS_OPMODE_GAMING	0x0003
#define RS_OPMODE_MINIMAL	0x0004


class RsConfigStartup
{
	public:
	RsConfigStartup()
	{
		promptAtBoot = 1;
	}


int 		promptAtBoot; /* popup the password prompt */
};


class RsConfigDataRates
{
	public:
	RsConfigDataRates()
	{
		mRateIn = 0;
		mRateMaxIn = 0;
		mAllocIn = 0;

		mAllocTs = 0;

		mRateOut = 0;
		mRateMaxOut = 0;
		mAllowedOut = 0;

		mAllowedTs = 0;

		mQueueIn = 0;
		mQueueOut = 0;
	}

	/* all in kB/s */
	float   mRateIn;
	float   mRateMaxIn;
	float   mAllocIn;

	time_t	mAllocTs;

	float   mRateOut;
	float   mRateMaxOut;
	float   mAllowedOut;

	time_t	mAllowedTs;

	int 	mQueueIn;
	int	mQueueOut;
};

class RSTrafficClue
{
public:
    time_t     TS ;
    uint32_t   size ;
    uint8_t    priority ;
    uint16_t   service_id ;
    uint8_t    service_sub_id ;
    RsPeerId   peer_id ;
    uint32_t   count ;

    RSTrafficClue() { TS=0;size=0;service_id=0;service_sub_id=0; count=0; }
    RSTrafficClue& operator+=(const RSTrafficClue& tc) { size += tc.size; count += tc.count ; return *this ;}
};

class RsConfigNetStatus
{
	public:
	RsConfigNetStatus()
	{
		localPort = extPort = 0 ;
		firewalled = forwardPort = false ;
		DHTActive = uPnPActive = netLocalOk = netUpnpOk = netDhtOk = netStunOk = netExtAddressOk = false ;
		uPnPState = 0 ;
		//DHTPeers = 0 ;
        netDhtNetSize = netDhtRsNetSize = 0;
	}

	RsPeerId		ownId;
	std::string		ownName;

	std::string		localAddr;
	int			localPort;
	std::string		extAddr;
	int			extPort;
	std::string		extDynDns;

	bool			firewalled;
	bool			forwardPort;

	/* older data types */
	bool			DHTActive;
	bool			uPnPActive;

	int			uPnPState;

	/* Flags for Network Status */
	bool 			netLocalOk;     /* That we've talked to someone! */
	bool			netUpnpOk; /* upnp is enabled and active */
	bool			netDhtOk;  /* response from dht */
	bool			netStunOk;  /* recvd stun / udp packets */
	bool			netExtAddressOk;  /* from Dht/Stun or External IP Finder */

	uint32_t		netDhtNetSize;  /* response from dht */
	uint32_t		netDhtRsNetSize;  /* response from dht */
};


/*********
 * This is a new style RsConfig Interface.
 * It should contain much of the information for the Options/Config Window.
 *
 * To start with, I'm going to move the stuff from RsIface::RsConfig into here.
 *
 */


class RsServerConfig
{
public:

    RsServerConfig()  {}
    virtual ~RsServerConfig() {}

    /* From RsIface::RsConfig */
    // Implemented Only this one!
    virtual int 	getConfigNetStatus(RsConfigNetStatus &status) = 0;

    // NOT IMPLEMENTED YET!
    //virtual int 	getConfigStartup(RsConfigStartup &params) = 0;

    virtual int getTotalBandwidthRates(RsConfigDataRates &rates) = 0;
    virtual int getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap) = 0;
    virtual int getTrafficInfo(std::list<RSTrafficClue>& out_lst,std::list<RSTrafficClue>& in_lst) = 0 ;

    /* From RsInit */

    // NOT IMPLEMENTED YET!
    //virtual std::string      RsConfigDirectory() = 0;
    //virtual std::string      RsConfigKeysDirectory() = 0;

    //virtual std::string  RsProfileConfigDirectory() = 0;
    //virtual bool         getStartMinimised() = 0;
    //virtual std::string  getRetroShareLink() = 0;

    //virtual bool getAutoLogin() = 0;
    //virtual void setAutoLogin(bool autoLogin) = 0;
    //virtual bool RsClearAutoLogin() = 0;

    //virtual std::string getRetroshareDataDirectory() = 0;

    /* New Stuff */

    virtual uint32_t getUserLevel() = 0;

    virtual uint32_t getNetState() = 0;
    virtual uint32_t getNetworkMode() = 0;
    virtual uint32_t getNatTypeMode() = 0;
    virtual uint32_t getNatHoleMode() = 0;
    virtual uint32_t getConnectModes() = 0;

    virtual bool getConfigurationOption(uint32_t key, std::string &opt) = 0;
    virtual bool setConfigurationOption(uint32_t key, const std::string &opt) = 0;


    /* Operating Mode */
    virtual uint32_t getOperatingMode() = 0;
    virtual bool     setOperatingMode(uint32_t opMode) = 0;
    virtual bool     setOperatingMode(const std::string &opModeStr) = 0;

    /* Data Rate Control - to be moved here */
    virtual int SetMaxDataRates( int downKb, int upKb ) = 0;
    virtual int GetMaxDataRates( int &inKb, int &outKb ) = 0;
    
    virtual int GetCurrentDataRates( float &inKb, float &outKb ) = 0;
    virtual int GetTrafficSum( uint64_t &inb, uint64_t &outb ) = 0;
};

#endif
