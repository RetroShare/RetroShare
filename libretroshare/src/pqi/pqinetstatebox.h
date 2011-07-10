#ifndef PQI_NET_STATUS_BOX_H
#define PQI_NET_STATUS_BOX_H

/* a little state box to determine network status */

#include <string>
#include <list>

#include "bitdht/bdiface.h"

/*** Network state 
 * Want this to be all encompassing.
 *
 */

#define PNSB_NETWORK_UNKNOWN		1
#define PNSB_NETWORK_RESTARTING		2
#define PNSB_NETWORK_OFFLINE		3
#define PNSB_NETWORK_LOCALNET		4
#define PNSB_NETWORK_BEHINDNAT		5
#define PNSB_NETWORK_EXTERNALIP		6

// WHAT TYPE OF FIREWALL?
#define PNSB_NATTYPE_NONE			1
#define PNSB_NATTYPE_UNKNOWN			2
#define PNSB_NATTYPE_SYMMETRIC 			3
#define PNSB_NATTYPE_RESTRICTED_CONE		4
#define PNSB_NATTYPE_FULL_CONE			5
#define PNSB_NATTYPE_OTHER			6

// WHAT TYPE OF HOLE?
#define PNSB_NATHOLE_UNKNOWN		0		
#define PNSB_NATHOLE_NONE		1		
#define PNSB_NATHOLE_UPNP		2	
#define PNSB_NATHOLE_NATPMP		3
#define PNSB_NATHOLE_FORWARDED		4

// Types of Connections.
#define PNSB_CONNECT_NONE		0x0000
#define PNSB_CONNECT_ACCEPT_TCP		0x0001
#define PNSB_CONNECT_OUTGOING_TCP	0x0002
#define PNSB_CONNECT_DIRECT_UDP		0x0100
#define PNSB_CONNECT_PROXY_UDP		0x0200
#define PNSB_CONNECT_RELAY_UDP		0x0400

// net state (good, okay, bad)
// BAD. (RED)
#define PNSB_NETSTATE_BAD_UNKNOWN	1
#define PNSB_NETSTATE_BAD_OFFLINE	2
#define PNSB_NETSTATE_BAD_NATSYM	3
#define PNSB_NETSTATE_BAD_NODHT_NAT	4

// CAUTION. (ORANGE)
#define PNSB_NETSTATE_WARNING_RESTART	5
#define PNSB_NETSTATE_WARNING_NATTED	6
#define PNSB_NETSTATE_WARNING_NODHT	7

// GOOD (GREEN)
// NAT with forwarded port, or EXT port.
#define PNSB_NETSTATE_GOOD		8

// ADVANCED MODE (BLUE)
// If the user knows what they are doing... we cannot confirm this.
#define PNSB_NETSTATE_ADV_FORWARD	9
#define PNSB_NETSTATE_ADV_DARK_FORWARD	10

class pqiNetStateBox
{
	public:
	pqiNetStateBox();

	/* input network bits */
	void setAddressStunDht(struct sockaddr_in *, bool stable);
	void setAddressStunProxy(struct sockaddr_in *, bool stable);

	void setAddressUPnP(bool active, struct sockaddr_in *addr);
	void setAddressNatPMP(bool active, struct sockaddr_in *addr);
	void setAddressWebIP(bool active, struct sockaddr_in *addr);

	void setDhtState(bool dhtOn, bool dhtActive);

	uint32_t getNetStateMode();
	uint32_t getNetworkMode();
	uint32_t getNatTypeMode();
	uint32_t getNatHoleMode();
	uint32_t getConnectModes();

	private:

	/* calculate network state */
	void clearOldNetworkData();
	void determineNetworkState();
	int statusOkay();		
	int updateNetState();

	/* more internal fns */
	void workoutNetworkMode();

	bool mStatusOkay;
	time_t mStatusTS;

	uint32_t mNetworkMode;
	uint32_t mNatTypeMode;
	uint32_t mNatHoleMode;
	uint32_t mConnectModes;
	uint32_t mNetStateMode;

	/* Parameters set externally */

	bool mStunDhtSet;
	time_t mStunDhtTS;
	bool mStunDhtStable;
	struct sockaddr_in mStunDhtAddr;

	bool mStunProxySet;
	time_t mStunProxyTS;
	bool mStunProxyStable;
	struct sockaddr_in mStunProxyAddr;

	bool mDhtSet;
	time_t mDhtTS;
	bool mDhtOn;
	bool mDhtActive;

	bool mUPnPSet;
	struct sockaddr_in mUPnPAddr;
	bool mUPnPActive;
	time_t mUPnPTS;

	bool mNatPMPSet;
	struct sockaddr_in mNatPMPAddr;
	bool mNatPMPActive;
	time_t mNatPMPTS;

	bool mWebIPSet;
	struct sockaddr_in mWebIPAddr;
	bool mWebIPActive;
	time_t mWebIPTS;

	bool mPortForwardedSet;
	uint16_t  mPortForwarded;
};

#endif
