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

class pqiNetStateBox
{
	public:
	pqiNetStateBox();

	void reset();

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
	bool mStunProxySemiStable;
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



std::string NetStateNetStateString(uint32_t netstate);
std::string NetStateConnectModesString(uint32_t connect);
std::string NetStateNatHoleString(uint32_t natHole);
std::string NetStateNatTypeString(uint32_t natType);
std::string NetStateNetworkModeString(uint32_t netMode);


#endif
