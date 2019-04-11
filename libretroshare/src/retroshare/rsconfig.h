/*******************************************************************************
 * libretroshare/src/retroshare: rsconfig.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RETROSHARE_CONFIG_GUI_INTERFACE_H
#define RETROSHARE_CONFIG_GUI_INTERFACE_H

#include <inttypes.h>
#include <retroshare/rstypes.h>
#include <string>
#include <list>
#include <map>

/* The New Config Interface Class */
class RsServerConfig;

/**
 * Pointer to global instance of RsServerConfig service implementation
 * @jsonapi{development}
 */
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


struct RsConfigDataRates : RsSerializable
{
	RsConfigDataRates() :
	    mRateIn(0), mRateMaxIn(0), mAllocIn(0),
	    mAllocTs(0),
	    mRateOut(0), mRateMaxOut(0), mAllowedOut(0),
	    mAllowedTs(0),
	    mQueueIn(0), mQueueOut(0)
	{}

	/* all in kB/s */
	float mRateIn;
	float mRateMaxIn;
	float mAllocIn;

	rstime_t mAllocTs;

	float mRateOut;
	float mRateMaxOut;
	float mAllowedOut;

	rstime_t mAllowedTs;

	int	mQueueIn;
	int	mQueueOut;

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(mRateIn);
		RS_SERIAL_PROCESS(mRateMaxIn);
		RS_SERIAL_PROCESS(mAllocIn);

		RS_SERIAL_PROCESS(mAllocTs);

		RS_SERIAL_PROCESS(mRateOut);
		RS_SERIAL_PROCESS(mRateMaxOut);
		RS_SERIAL_PROCESS(mAllowedOut);

		RS_SERIAL_PROCESS(mAllowedTs);

		RS_SERIAL_PROCESS(mQueueIn);
		RS_SERIAL_PROCESS(mQueueOut);
	}
};

struct RSTrafficClue : RsSerializable
{
    rstime_t     TS ;
    uint32_t   size ;
    uint8_t    priority ;
    uint16_t   service_id ;
    uint8_t    service_sub_id ;
    RsPeerId   peer_id ;
    uint32_t   count ;

    RSTrafficClue() { TS=0;size=0;service_id=0;service_sub_id=0; count=0; }
    RSTrafficClue& operator+=(const RSTrafficClue& tc) { size += tc.size; count += tc.count ; return *this ;}

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(TS);
		RS_SERIAL_PROCESS(size);
		RS_SERIAL_PROCESS(priority);
		RS_SERIAL_PROCESS(service_id);
		RS_SERIAL_PROCESS(service_sub_id);
		RS_SERIAL_PROCESS(peer_id);
		RS_SERIAL_PROCESS(count);
	}
};

struct RsConfigNetStatus : RsSerializable
{
	RsConfigNetStatus()
	{
		localPort = extPort = 0 ;
		firewalled = forwardPort = false ;
		DHTActive = uPnPActive = netLocalOk = netUpnpOk = netDhtOk = netStunOk = netExtAddressOk = false ;
		uPnPState = 0 ;
		//DHTPeers = 0 ;
        netDhtNetSize = netDhtRsNetSize = 0;
	}

	RsPeerId	ownId;
	std::string	ownName;

	std::string	localAddr;
	int			localPort;
	std::string	extAddr;
	int			extPort;
	std::string	extDynDns;

	bool		firewalled;
	bool		forwardPort;

	/* older data types */
	bool		DHTActive;
	bool		uPnPActive;

	int			uPnPState;

	/* Flags for Network Status */
	bool 		netLocalOk;		/* That we've talked to someone! */
	bool		netUpnpOk;		/* upnp is enabled and active */
	bool		netDhtOk;		/* response from dht */
	bool		netStunOk;		/* recvd stun / udp packets */
	bool		netExtAddressOk;/* from Dht/Stun or External IP Finder */

	uint32_t	netDhtNetSize;	/* response from dht */
	uint32_t	netDhtRsNetSize;/* response from dht */

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(ownId);
		RS_SERIAL_PROCESS(ownName);

		RS_SERIAL_PROCESS(localAddr);
		RS_SERIAL_PROCESS(localPort);
		RS_SERIAL_PROCESS(extAddr);
		RS_SERIAL_PROCESS(extPort);
		RS_SERIAL_PROCESS(extDynDns);

		RS_SERIAL_PROCESS(firewalled);
		RS_SERIAL_PROCESS(forwardPort);

		RS_SERIAL_PROCESS(DHTActive);
		RS_SERIAL_PROCESS(uPnPActive);

		RS_SERIAL_PROCESS(uPnPState);

		RS_SERIAL_PROCESS(netLocalOk);
		RS_SERIAL_PROCESS(netUpnpOk);
		RS_SERIAL_PROCESS(netDhtOk);
		RS_SERIAL_PROCESS(netStunOk);
		RS_SERIAL_PROCESS(netExtAddressOk);

		RS_SERIAL_PROCESS(netDhtNetSize);
		RS_SERIAL_PROCESS(netDhtRsNetSize);
	}
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
	/**
	 * @brief getConfigNetStatus return the net status
	 * @jsonapi{development}
	 * @param[out] status network status
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int 	getConfigNetStatus(RsConfigNetStatus &status) = 0;

    // NOT IMPLEMENTED YET!
    //virtual int 	getConfigStartup(RsConfigStartup &params) = 0;

	/**
	 * @brief getTotalBandwidthRates returns the current bandwidths rates
	 * @jsonapi{development}
	 * @param[out] rates
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int getTotalBandwidthRates(RsConfigDataRates &rates) = 0;

	/**
	 * @brief getAllBandwidthRates get the bandwidth rates for all peers
	 * @jsonapi{development}
	 * @param[out] ratemap map with peers->rates
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap) = 0;

	/**
	 * @brief getTrafficInfo returns a list of all tracked traffic clues
	 * @jsonapi{development}
	 * @param[out] out_lst outgoing traffic clues
	 * @param[out] in_lst incomming traffic clues
	 * @return returns 1 on succes and 0 otherwise
	 */
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
	/**
	 * @brief getOperatingMode get current operating mode
	 * @jsonapi{development}
	 * @return return the current operating mode
	 */
    virtual uint32_t getOperatingMode() = 0;

	/**
	 * @brief setOperatingMode set the current oprating mode
	 * @jsonapi{development}
	 * @param[in] opMode new opearting mode
	 * @return
	 */
    virtual bool     setOperatingMode(uint32_t opMode) = 0;

	/**
	 * @brief setOperatingMode set the current operating mode from string
	 * @param[in] opModeStr new operating mode as string
	 * @return
	 */
    virtual bool     setOperatingMode(const std::string &opModeStr) = 0;

    /* Data Rate Control - to be moved here */
	/**
	 * @brief SetMaxDataRates set maximum upload and download rates
	 * @jsonapi{development}
	 * @param[in] downKb download rate in kB
	 * @param[in] upKb upload rate in kB
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int SetMaxDataRates( int downKb, int upKb ) = 0;

	/**
	 * @brief GetMaxDataRates get maximum upload and download rates
	 * @jsonapi{development}
	 * @param[out] inKb download rate in kB
	 * @param[out] outKb upload rate in kB
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int GetMaxDataRates( int &inKb, int &outKb ) = 0;
    
	/**
	 * @brief GetCurrentDataRates get current upload and download rates
	 * @jsonapi{development}
	 * @param[out] inKb download rate in kB
	 * @param[out] outKb upload rate in kB
	 * @return returns 1 on succes and 0 otherwise
	 */
    virtual int GetCurrentDataRates( float &inKb, float &outKb ) = 0;
    virtual int GetTrafficSum( uint64_t &inb, uint64_t &outb ) = 0;
};

#endif
