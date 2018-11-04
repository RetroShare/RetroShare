/*******************************************************************************
 * libretroshare/src/pqi: pqinetstatebox.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 Retroshare Team <retroshare.team@gmail.com>                  *
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

#include "retroshare/rsconfig.h"
#include "util/rsnet.h"
#include "pqi/pqinetstatebox.h"
#include "util/rstime.h"

#ifdef RS_USE_BITDHT
#include "bitdht/bdiface.h"
#endif

// External Interface.

void pqiNetStateBox::setAddressStunDht(const struct sockaddr_storage &addr, bool stable)
{
	if ((!mStunDhtSet) || (mStunDhtStable != stable) || 
			(!sockaddr_storage_same(addr, mStunDhtAddr)))
	{
		mStunDhtSet = true;
		mStunDhtStable = stable;
		mStunDhtAddr = addr;

		mStatusOkay = false;
	}
	mStunDhtTS = time(NULL);
}


void pqiNetStateBox::setAddressStunProxy(const struct sockaddr_storage &addr, bool stable)
{
	if ((!mStunProxySet) || (mStunProxyStable != stable) || 
		(!sockaddr_storage_same(addr, mStunProxyAddr)))
	{

		if (sockaddr_storage_sameip(addr,mStunProxyAddr)) 
		{
			if (mStunProxyStable != stable) 
			{
				mStunProxySemiStable = true;	
			}
		}
		else
		{
			mStunProxySemiStable = false; // change of address - must trigger this again!
		}

		mStunProxySet = true;
		mStunProxyStable = stable;
		mStunProxyAddr = addr;
		mStatusOkay = false;
	}
	mStunProxyTS = time(NULL);
}


void pqiNetStateBox::setAddressUPnP(bool active, const struct sockaddr_storage &addr)
{
	if ((!mUPnPSet) || (mUPnPActive != active) || 
		(!sockaddr_storage_same(addr, mUPnPAddr)))
	{
		mUPnPSet = true;
		mUPnPAddr = addr;
		mUPnPActive = active;

		mStatusOkay = false;
	}
	mUPnPTS = time(NULL);
}


void pqiNetStateBox::setAddressNatPMP(bool active, const struct sockaddr_storage &addr)
{
	if ((!mNatPMPSet) || (mNatPMPActive != active) || 
		(!sockaddr_storage_same(addr, mNatPMPAddr)))
	{
		mNatPMPSet = true;
		mNatPMPAddr = addr;
		mNatPMPActive = active;

		mStatusOkay = false;
	}
	mNatPMPTS = time(NULL);
}



void pqiNetStateBox::setAddressWebIP(bool active, const struct sockaddr_storage &addr)
{
	if ((!mWebIPSet) || (mWebIPActive != active) || 
		(!sockaddr_storage_same(addr, mWebIPAddr)))
	{
		mWebIPSet = true;
		mWebIPAddr = addr;
		mWebIPActive = active;

		mStatusOkay = false;
	}
	mWebIPTS = time(NULL);
}


void pqiNetStateBox::setPortForwarded(bool /*active*/, uint16_t port)
{
	if ((!mPortForwardSet) || (mPortForwarded != port))

	{
		mPortForwardSet = true;
		mPortForwarded = port;

		mStatusOkay = false;
	}
}


void pqiNetStateBox::setDhtState(bool on, bool active)
{
	if ((!mDhtSet) || (mDhtActive != active) || (mDhtOn != on))
	{
		mDhtSet = true;
		mDhtActive = active;
		mDhtOn = on;

		mStatusOkay = false;
	}
	mDhtTS = time(NULL);
}


/* Extract Net State */
uint32_t pqiNetStateBox::getNetworkMode()
{
	updateNetState();
	return mNetworkMode;
}

uint32_t pqiNetStateBox::getNatTypeMode()
{
	updateNetState();
	return mNatTypeMode;
}

uint32_t pqiNetStateBox::getNatHoleMode()
{
	updateNetState();
	return mNatHoleMode;
}

uint32_t pqiNetStateBox::getConnectModes()
{
	updateNetState();
	return mConnectModes;
}


uint32_t pqiNetStateBox::getNetStateMode()
{
	updateNetState();
	return mNetStateMode;
}



/******************************** Internal Workings *******************************/
pqiNetStateBox::pqiNetStateBox()
{
	reset();
}


void pqiNetStateBox::reset()
{

	mStatusOkay = false;
	//rstime_t mStatusTS;
	
	mNetworkMode = RSNET_NETWORK_UNKNOWN;
	mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
	mNatHoleMode = RSNET_NATHOLE_UNKNOWN;
	mConnectModes = RSNET_CONNECT_NONE;
	mNetStateMode = RSNET_NETSTATE_BAD_UNKNOWN;
	
	/* Parameters set externally */
	
	mStunDhtSet = false;
	mStunDhtTS = 0;
	mStunDhtStable = false;
	sockaddr_storage_clear(mStunDhtAddr);
	
	mStunProxySet = false;
	mStunProxySemiStable = false; 
	mStunProxyTS = 0;
	mStunProxyStable = false;
	sockaddr_storage_clear(mStunProxyAddr);
	
	mUPnPSet = false;
	mUPnPActive = false;
	sockaddr_storage_clear(mUPnPAddr);

	mNatPMPSet = false;
	mNatPMPActive = false;
	sockaddr_storage_clear(mNatPMPAddr);

	mWebIPSet = false;
	mWebIPActive = false;
	sockaddr_storage_clear(mWebIPAddr);

	mPortForwardSet = false;
	mPortForwarded = false;
	
	mDhtSet = false;
	mDhtActive = false;
	mDhtOn = false;

}

//#define NETSTATE_PARAM_TIMEOUT		600
#define NETSTATE_PARAM_TIMEOUT			900  // Change to 15 minutes -> see if it has effect on reconnect time
#define NETSTATE_TIMEOUT			60

	/* check/update Net State */
int pqiNetStateBox::statusOkay()
{
	if (!mStatusOkay)
	{
		return 0;
	}
	rstime_t now = time(NULL);
	if (now - mStatusTS > NETSTATE_TIMEOUT)
	{
		return 0;
	}
	return 1;
}

int pqiNetStateBox::updateNetState()
{
	if (!statusOkay())
	{
		determineNetworkState();
	}
	return 1;
}




void pqiNetStateBox::clearOldNetworkData()
{
#ifdef RS_USE_DHT_STUNNER
	/* check if any measurements are too old to consider */
	rstime_t now = time(NULL);
	if (now - mStunProxyTS > NETSTATE_PARAM_TIMEOUT)
	{
		mStunProxySet = false;
	}

	if (now - mStunDhtTS > NETSTATE_PARAM_TIMEOUT)
	{
		mStunDhtSet = false;
	}
#else
	//Set values, as they are not updated.
	mStunProxySet = true;
	mStunDhtSet = true;
	mStunProxyStable = true;
	mStunDhtStable = true;
#endif

}


void pqiNetStateBox::determineNetworkState()
{
	clearOldNetworkData();
	rstime_t now = time(NULL);

	/* now we use the remaining valid input to determine network state */

	/* Most important Factor is whether we have DHT(STUN) info to ID connection */
	if (mDhtActive)
	{
	   /* firstly lets try to identify OFFLINE / UNKNOWN */
	   if ((!mStunProxySet) || (!mStunDhtSet))
	   {
		mNetworkMode = RSNET_NETWORK_UNKNOWN;
		// Assume these.
		mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
		mNatHoleMode = RSNET_NATHOLE_NONE;
		mNetStateMode = RSNET_NETSTATE_BAD_UNKNOWN;

		//mExtAddress = .... unknown;
		//mExtAddrStable = false;
	   }
	   else // Both Are Set!
	   {
		if (!mStunDhtStable)
		{
			//mExtAddress = mStunDhtExtAddress;
			//mExtAddrStable = false;

			if (mStunProxySemiStable)
			{
				/* I'm guessing this will be a common mode for modern NAT/Firewalls.
				 * a DETERMINISTIC SYMMETRIC NAT.... This is likely to be the 
				 * next iteration on the RESTRICTED CONE firewall described below.
				 * If you Stun fast, it looks like a SYMMETRIC NAT, but if you let
				 * the NAT timeout, you get back your original port so it looks like
				 * a RESTRICTED CONE nat...
				 *
				 * This kind of NAT is passable, if you only attempt one connection at
				 * a time, and are careful about it!
				 *
				 * NB: The StunDht port will never get this mode.
				 * It has unsolicited traffic which triggers SYM mode
				 *
				 */

				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_DETERM_SYM;
				mNatHoleMode = RSNET_NATHOLE_NONE;
				mNetStateMode = RSNET_NETSTATE_WARNING_NATTED;

			}
			else if (!mStunProxyStable)
			{
				/* both unstable, Symmetric NAT, Firewalled, No UDP Hole */
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_SYMMETRIC;
				mNatHoleMode = RSNET_NATHOLE_NONE;
				mNetStateMode = RSNET_NETSTATE_BAD_NATSYM;
			}
			else
			{
				/* if DhtStun Unstable, but ProxyStable, then we have
				 * an interesting case. This is close to a Debian Firewall
				 * I tested in the past....
				 *
				 * The big difference between DhtStun and ProxyStun is
				 * that Dht Port receives unsolicated packets, 
				 * while Proxy Port always sends an outgoing one first.
				 *
				 * In the case of the debian firewall, the unsolicated pkts
				 * caused the outgoing port to change.
				 *
				 * We will label this difference RESTRICTED vs FULL CONE,
				 * but that label is really fully accurate. (gray area).
				 */

				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_RESTRICTED_CONE;
				mNatHoleMode = RSNET_NATHOLE_NONE;
				mNetStateMode = RSNET_NETSTATE_WARNING_NATTED;
			}
		}
		else // Dht Stable.
		{
				/* DHT Stable port can be caused by:
				 * 1) Forwarded Port (UPNP, NATPMP, FORWARDED)
				 * 2) FULL CONE NAT.
				 * 3) EXT Port.
				 * Must look at Proxy Stability.
				 *    - if Proxy Unstable, then must be forwarded port.
				 *    - if Proxy Stable, then we cannot tell.
				 * 	 -> Must use User Info (Upnp, PMP, Forwarded Flag).
				 *	 -> Also possible to be EXT Port (Check against iface)
				 */

			//mExtAddress = mStunDhtExtAddress;
			//mExtAddrStable = true;

			// Initial Fallback Guess at firewall state.
			if (mStunProxySemiStable)
			{
				/* must be a forwarded port/ext or something similar */
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_DETERM_SYM;
				mNatHoleMode = RSNET_NATHOLE_FORWARDED;
				mNetStateMode = RSNET_NETSTATE_GOOD;
			}
			else if (!mStunProxyStable)
			{
				/* must be a forwarded port/ext or something similar */
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_SYMMETRIC;
				mNatHoleMode = RSNET_NATHOLE_FORWARDED;
				mNetStateMode = RSNET_NETSTATE_GOOD;
			}
			else
			{
				/* fallback is FULL CONE NAT */
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_FULL_CONE;
				mNatHoleMode = RSNET_NATHOLE_NONE;
				mNetStateMode = RSNET_NETSTATE_WARNING_NATTED;
			}

			if (mUPnPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
				mNatHoleMode = RSNET_NATHOLE_UPNP;
				mNetStateMode = RSNET_NETSTATE_GOOD;
				//mExtAddress = ... from UPnP, should match StunDht.
				//mExtAddrStable = true;
			}
			else if (mNatPMPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
				mNatHoleMode = RSNET_NATHOLE_NATPMP;
				mNetStateMode = RSNET_NETSTATE_GOOD;
				//mExtAddress = ... from NatPMP, should match NatPMP
				//mExtAddrStable = true;
			}
			else 
			{
				bool isExtAddress = false;
	
				if (isExtAddress)
				{
					mNetworkMode = RSNET_NETWORK_EXTERNALIP;
					mNatTypeMode = RSNET_NATTYPE_NONE;
					mNatHoleMode = RSNET_NATHOLE_NONE;
					mNetStateMode = RSNET_NETSTATE_GOOD;
	
					//mExtAddrStable = true;
				}
				else if (mPortForwardSet)
				{
					mNetworkMode = RSNET_NETWORK_BEHINDNAT;
					// Use Fallback Guess.
					//mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
					mNatHoleMode = RSNET_NATHOLE_FORWARDED;
					mNetStateMode = RSNET_NETSTATE_ADV_FORWARD;
	
					//mExtAddrStable = true; // Probably, makin assumption.
				}
				else
				{
					/* At this point, we go with the fallback guesses */
				}
			}

		}
	   }
	}
	else // DHT Inactive, must use other means...
	{
		/* If we get here we are dealing with a silly peer in "DarkMode".
		 * We have to primarily rely on the feedback from UPnP, PMP or WebSite "WhatsMyIp".
		 * This is in the "Advanced" Settings and liable to be set wrong.
		 * but thats the users fault!
		 */

		if (mUPnPActive)
		{
			// This Mode is OKAY.
			mNetworkMode = RSNET_NETWORK_BEHINDNAT;
			mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
			mNatHoleMode = RSNET_NATHOLE_UPNP;
			//mExtAddress = ... from UPnP.
			//mExtAddrStable = true;
			mNetStateMode = RSNET_NETSTATE_WARNING_NODHT;
		}
		else if (mNatPMPActive)
		{
			// This Mode is OKAY.
			mNetworkMode = RSNET_NETWORK_BEHINDNAT;
			mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
			mNatHoleMode = RSNET_NATHOLE_NATPMP;
			//mExtAddress = ... from NatPMP.
			//mExtAddrStable = true;
			mNetStateMode = RSNET_NETSTATE_WARNING_NODHT;
		}
		else 
		{
			/* if we reach this point, we really need a Web "WhatsMyIp" Check of our Ext Ip Address. */
			/* Check for the possibility of an EXT address ... */
			bool isExtAddress = false;

			//mExtAddress = ... from WhatsMyIp.

			if (isExtAddress)
			{
				mNetworkMode = RSNET_NETWORK_EXTERNALIP;
				mNatTypeMode = RSNET_NATTYPE_NONE;
				mNatHoleMode = RSNET_NATHOLE_NONE;

				//mExtAddrStable = true;
				mNetStateMode = RSNET_NETSTATE_WARNING_NODHT;
			}
			else if (mPortForwardSet)
			{
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
				mNatHoleMode = RSNET_NATHOLE_FORWARDED;

				//mExtAddrStable = true; // Probably, makin assumption.
				mNetStateMode = RSNET_NETSTATE_WARNING_NODHT;
			}
			else
			{
				/* At this point we must assume firewalled.
				 * These people have destroyed the possibility of making connections ;(
				 * Should WARN about this.
				 */ 
				mNetworkMode = RSNET_NETWORK_BEHINDNAT;
				mNatTypeMode = RSNET_NATTYPE_UNKNOWN;
				mNatHoleMode = RSNET_NATHOLE_NONE;
				mNetStateMode = RSNET_NETSTATE_BAD_NODHT_NAT;

				//mExtAddrStable = false; // Unlikely to be stable.
			}
		}
	}

	workoutNetworkMode();

	/* say that things are okay */
	mStatusOkay = true;
	mStatusTS = now;
}

	

/* based on calculated settings, what is the network mode
 */

void pqiNetStateBox::workoutNetworkMode()
{
	/* connectModes are dependent on the other modes */
	mConnectModes = RSNET_CONNECT_NONE;
	switch(mNetworkMode)
	{
		case RSNET_NETWORK_UNKNOWN:
		case RSNET_NETWORK_OFFLINE:
		case RSNET_NETWORK_LOCALNET:
			/* nothing here */
			break;
		case RSNET_NETWORK_EXTERNALIP:
			mConnectModes = RSNET_CONNECT_OUTGOING_TCP;
			mConnectModes |= RSNET_CONNECT_ACCEPT_TCP;

			if (mDhtActive)
			{
				mConnectModes |= RSNET_CONNECT_DIRECT_UDP;
				/* if open port. don't want PROXY or RELAY connect 
				 * because we should be able to do direct with EVERYONE.
				 * Ability to do Proxy is dependent on FIREWALL status.
				 * Technically could do RELAY, but disable both.
				 */
				//mConnectModes |= RSNET_CONNECT_PROXY_UDP;
				//mConnectModes |= RSNET_CONNECT_RELAY_UDP;
			}
			break;
		case RSNET_NETWORK_BEHINDNAT:
			mConnectModes = RSNET_CONNECT_OUTGOING_TCP;

			/* we're okay if there's a NAT HOLE */
			if ((mNatHoleMode == RSNET_NATHOLE_UPNP) ||
				(mNatHoleMode == RSNET_NATHOLE_NATPMP) ||
				(mNatHoleMode == RSNET_NATHOLE_FORWARDED))
			{
				mConnectModes |= RSNET_CONNECT_ACCEPT_TCP;
				if (mDhtActive)
				{
					mConnectModes |= RSNET_CONNECT_DIRECT_UDP;
					/* dont want PROXY | RELAY with open ports */
				}
			}
			else
			{
			   /* If behind NAT without NATHOLE, this is where RELAY | PROXY
			    * are useful. We Flag DIRECT connections, cos we can do these
			    * with peers with Open Ports. (but not with other NATted peers).
			    */
			   if (mDhtActive)
			   {
				mConnectModes |= RSNET_CONNECT_DIRECT_UDP;
				mConnectModes |= RSNET_CONNECT_RELAY_UDP;

				if ((mNatTypeMode == RSNET_NATTYPE_RESTRICTED_CONE) ||
				    (mNatTypeMode == RSNET_NATTYPE_FULL_CONE) ||
				    (mNatTypeMode == RSNET_NATTYPE_DETERM_SYM))
				{
					mConnectModes |= RSNET_CONNECT_PROXY_UDP;
				}
			   }
			}
			break;
	}
}




std::string NetStateNetworkModeString(uint32_t netMode)
{
	std::string str;
	switch(netMode)
	{
		default:
		case RSNET_NETWORK_UNKNOWN:
			str = "Unknown NetState";
		break;
		case RSNET_NETWORK_OFFLINE:
			str = "Offline";
		break;
		case RSNET_NETWORK_LOCALNET:
			str = "Local Net";
		break;
		case RSNET_NETWORK_BEHINDNAT:
			str = "Behind NAT";
		break;
		case RSNET_NETWORK_EXTERNALIP:
			str = "External IP";
		break;
	}
	return str;
}

std::string NetStateNatTypeString(uint32_t natType)
{
	std::string str;
	switch(natType)
	{
		default:
		case RSNET_NATTYPE_UNKNOWN:
			str = "UNKNOWN NAT STATE";
		break;
		case RSNET_NATTYPE_SYMMETRIC:
			str = "SYMMETRIC NAT";
		break;
		case RSNET_NATTYPE_DETERM_SYM:       
			str = "DETERMINISTIC SYM NAT";
		break;
		case RSNET_NATTYPE_RESTRICTED_CONE:
			str = "RESTRICTED CONE NAT";
		break;
		case RSNET_NATTYPE_FULL_CONE:
			str = "FULL CONE NAT";
		break;
		case RSNET_NATTYPE_OTHER:
			str = "OTHER NAT";
		break;
		case RSNET_NATTYPE_NONE:
			str = "NO NAT";
		break;
	}
	return str;
}

		
std::string NetStateNatHoleString(uint32_t natHole)
{
	std::string str;
	switch(natHole)
	{
		default:
		case RSNET_NATHOLE_UNKNOWN:
			str = "UNKNOWN NAT HOLE STATUS";
		break;
		case RSNET_NATHOLE_NONE:
			str = "NO NAT HOLE";
		break;
		case RSNET_NATHOLE_UPNP:
			str = "UPNP FORWARD";
		break;
		case RSNET_NATHOLE_NATPMP:
			str = "NATPMP FORWARD";
		break;
		case RSNET_NATHOLE_FORWARDED:
			str = "MANUAL FORWARD";
		break;
	}
	return str;
}


std::string NetStateConnectModesString(uint32_t connect)
{
	std::string	connOut;
	if (connect & RSNET_CONNECT_OUTGOING_TCP)
	{
		connOut += "TCP_OUT ";
	}
	if (connect & RSNET_CONNECT_ACCEPT_TCP)
	{
		connOut += "TCP_IN ";
	}
	if (connect & RSNET_CONNECT_DIRECT_UDP)
	{
		connOut += "DIRECT_UDP ";
	}
	if (connect & RSNET_CONNECT_PROXY_UDP)
	{
		connOut += "PROXY_UDP ";
	}
	if (connect & RSNET_CONNECT_RELAY_UDP)
	{
		connOut += "RELAY_UDP ";
	}
	return connOut;
}



std::string NetStateNetStateString(uint32_t netstate)
{
	std::string str;
	switch(netstate)
	{
		case RSNET_NETSTATE_BAD_UNKNOWN:
			str = "NET BAD: Unknown State";
		break;
		case RSNET_NETSTATE_BAD_OFFLINE:
			str = "NET BAD: Offline";
		break;
		case RSNET_NETSTATE_BAD_NATSYM:
			str = "NET BAD: Behind Symmetric NAT";
		break;
		case RSNET_NETSTATE_BAD_NODHT_NAT:
			str = "NET BAD: Behind NAT & No DHT";
		break;
		case RSNET_NETSTATE_WARNING_RESTART:
			str = "NET WARNING: NET Restart";
		break;
		case RSNET_NETSTATE_WARNING_NATTED:
			str = "NET WARNING: Behind NAT";
		break;
		case RSNET_NETSTATE_WARNING_NODHT:
			str = "NET WARNING: No DHT";
		break;
		case RSNET_NETSTATE_GOOD:
			str = "NET STATE GOOD!";
		break;
		case RSNET_NETSTATE_ADV_FORWARD:
			str = "CAUTION: UNVERIFABLE FORWARD!";
		break;
		case RSNET_NETSTATE_ADV_DARK_FORWARD:
			str = "CAUTION: UNVERIFABLE FORWARD & NO DHT";
		break;
	}
	return str;
}





