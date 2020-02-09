/*******************************************************************************
 * libretroshare/src/pqi: pqinetstatebox.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 Retroshare Team <retroshare.project@gmail.com>               *
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
#ifndef PQI_NET_STATUS_BOX_H
#define PQI_NET_STATUS_BOX_H

/* a little state box to determine network status */

#include <string>
#include <list>

#include <util/rstime.h>
#include <retroshare/rsconfig.h>

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
	RsNetworkMode getNetworkMode();
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
	rstime_t mStatusTS;

	RsNetworkMode mNetworkMode;
	uint32_t mNatTypeMode;
	uint32_t mNatHoleMode;
	uint32_t mConnectModes;
	uint32_t mNetStateMode;

	/* Parameters set externally */

	bool mStunDhtSet;
	rstime_t mStunDhtTS;
	bool mStunDhtStable;
	struct sockaddr_storage mStunDhtAddr;

	bool mStunProxySet;
	rstime_t mStunProxyTS;
	bool mStunProxyStable;
	bool mStunProxySemiStable;
	struct sockaddr_storage mStunProxyAddr;

	bool mDhtSet;
	rstime_t mDhtTS;
	bool mDhtOn;
	bool mDhtActive;

	bool mUPnPSet;
	struct sockaddr_storage mUPnPAddr;
	bool mUPnPActive;
	rstime_t mUPnPTS;

	bool mNatPMPSet;
	struct sockaddr_storage mNatPMPAddr;
	bool mNatPMPActive;
	rstime_t mNatPMPTS;

	bool mWebIPSet;
	struct sockaddr_storage mWebIPAddr;
	bool mWebIPActive;
	rstime_t mWebIPTS;

	bool mPortForwardSet;
	uint16_t  mPortForwarded;
};



std::string NetStateNetStateString(uint32_t netstate);
std::string NetStateConnectModesString(uint32_t connect);
std::string NetStateNatHoleString(uint32_t natHole);
std::string NetStateNatTypeString(uint32_t natType);


#endif
