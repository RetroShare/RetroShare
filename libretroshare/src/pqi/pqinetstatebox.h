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
	void setAddressStunDht(const struct sockaddr_storage &addr, bool stable);
	void setAddressStunProxy(const struct sockaddr_storage &addr, bool stable);

	void setAddressUPnP(bool active, const struct sockaddr_storage &addr);
	void setAddressNatPMP(bool active, const struct sockaddr_storage &addr);
	void setAddressWebIP(bool active, const struct sockaddr_storage &addr);

	void setPortForwarded(bool active, uint16_t port);

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
	struct sockaddr_storage mStunDhtAddr;

	bool mStunProxySet;
	time_t mStunProxyTS;
	bool mStunProxyStable;
	bool mStunProxySemiStable;
	struct sockaddr_storage mStunProxyAddr;

	bool mDhtSet;
	time_t mDhtTS;
	bool mDhtOn;
	bool mDhtActive;

	bool mUPnPSet;
	struct sockaddr_storage mUPnPAddr;
	bool mUPnPActive;
	time_t mUPnPTS;

	bool mNatPMPSet;
	struct sockaddr_storage mNatPMPAddr;
	bool mNatPMPActive;
	time_t mNatPMPTS;

	bool mWebIPSet;
	struct sockaddr_storage mWebIPAddr;
	bool mWebIPActive;
	time_t mWebIPTS;

	bool mPortForwardSet;
	uint16_t  mPortForwarded;
};



std::string NetStateNetStateString(uint32_t netstate);
std::string NetStateConnectModesString(uint32_t connect);
std::string NetStateNatHoleString(uint32_t natHole);
std::string NetStateNatTypeString(uint32_t natType);
std::string NetStateNetworkModeString(uint32_t netMode);


#endif
