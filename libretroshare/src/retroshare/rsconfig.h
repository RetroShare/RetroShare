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
#include <string>
#include <list>

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
		maxDownloadDataRate = 0;
		maxUploadDataRate = 0;
		maxIndivDataRate = 0;
	}

	int	maxDownloadDataRate;     /* kb */
	int	maxUploadDataRate;     /* kb */
	int	maxIndivDataRate; /* kb */
};


class RsConfigNetStatus
{
	public:
	RsConfigNetStatus()
	{
		localPort = extPort = 0 ;
		firewalled = forwardPort = false ;
		DHTActive = uPnPActive = netLocalOk = netUpnpOk = netDhtOk = netStunOk = netExtraAddressOk = false ;
		uPnPState = DHTPeers = 0 ;
	}

	std::string		ownId;
	std::string		ownName;

	std::string		localAddr;
	int			localPort;
	std::string		extAddr;
	int			extPort;
	std::string		extName;

	bool			firewalled;
	bool			forwardPort;

	/* older data types */
	bool			DHTActive;
	bool			uPnPActive;

	int			uPnPState;
	int			DHTPeers;

	/* Flags for Network Status */
	bool 			netLocalOk;     /* That we've talked to someone! */
	bool			netUpnpOk; /* upnp is enabled and active */
	bool			netDhtOk;  /* response from dht */
	bool			netStunOk;  /* recvd stun / udp packets */
	bool			netExtraAddressOk;  /* recvd ip address with external finder*/

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

	RsServerConfig()  { return; }
virtual ~RsServerConfig() { return; }

	/* From RsIface::RsConfig */
	// Implemented Only this one!
virtual int 	getConfigNetStatus(RsConfigNetStatus &status) = 0;

// NOT IMPLEMENTED YET!
//virtual int 	getConfigStartup(RsConfigStartup &params) = 0;
//virtual int 	getConfigDataRates(RsConfigDataRates &params) = 0;

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


};

#endif
