/*******************************************************************************
 * libretroshare/src/pqi: pqinetstatebox.cc                                    *
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
RsNetworkMode pqiNetStateBox::getNetworkMode()
{
	updateNetState();
	return mNetworkMode;
}

RsNatTypeMode pqiNetStateBox::getNatTypeMode()
{
	updateNetState();
	return mNatTypeMode;
}

RsNatHoleMode pqiNetStateBox::getNatHoleMode()
{
	updateNetState();
	return mNatHoleMode;
}

RsConnectModes pqiNetStateBox::getConnectModes()
{
	updateNetState();
	return mConnectModes;
}


RsNetState pqiNetStateBox::getNetStateMode()
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
	
	mNetworkMode = RsNetworkMode::UNKNOWN;
	mNatTypeMode = RsNatTypeMode::UNKNOWN;
	mNatHoleMode = RsNatHoleMode::UNKNOWN;
	mConnectModes = RsConnectModes::NONE;
	mNetStateMode = RsNetState::BAD_UNKNOWN;
	
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
		mNetworkMode = RsNetworkMode::UNKNOWN;
		// Assume these.
		mNatTypeMode = RsNatTypeMode::UNKNOWN;
		mNatHoleMode = RsNatHoleMode::NONE;
		mNetStateMode = RsNetState::BAD_UNKNOWN;

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

				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::DETERM_SYM;
				mNatHoleMode = RsNatHoleMode::NONE;
				mNetStateMode = RsNetState::WARNING_NATTED;

			}
			else if (!mStunProxyStable)
			{
				/* both unstable, Symmetric NAT, Firewalled, No UDP Hole */
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::SYMMETRIC;
				mNatHoleMode = RsNatHoleMode::NONE;
				mNetStateMode = RsNetState::BAD_NATSYM;
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

				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::RESTRICTED_CONE;
				mNatHoleMode = RsNatHoleMode::NONE;
				mNetStateMode = RsNetState::WARNING_NATTED;
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
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::DETERM_SYM;
				mNatHoleMode = RsNatHoleMode::FORWARDED;
				mNetStateMode = RsNetState::GOOD;
			}
			else if (!mStunProxyStable)
			{
				/* must be a forwarded port/ext or something similar */
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::SYMMETRIC;
				mNatHoleMode = RsNatHoleMode::FORWARDED;
				mNetStateMode = RsNetState::GOOD;
			}
			else
			{
				/* fallback is FULL CONE NAT */
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::FULL_CONE;
				mNatHoleMode = RsNatHoleMode::NONE;
				mNetStateMode = RsNetState::WARNING_NATTED;
			}

			if (mUPnPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = RsNatTypeMode::UNKNOWN;
				mNatHoleMode = RsNatHoleMode::UPNP;
				mNetStateMode = RsNetState::GOOD;
				//mExtAddress = ... from UPnP, should match StunDht.
				//mExtAddrStable = true;
			}
			else if (mNatPMPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = RsNatTypeMode::UNKNOWN;
				mNatHoleMode = RsNatHoleMode::NATPMP;
				mNetStateMode = RsNetState::GOOD;
				//mExtAddress = ... from NatPMP, should match NatPMP
				//mExtAddrStable = true;
			}
			else 
			{
				bool isExtAddress = false;
	
				if (isExtAddress)
				{
					mNetworkMode = RsNetworkMode::EXTERNALIP;
					mNatTypeMode = RsNatTypeMode::NONE;
					mNatHoleMode = RsNatHoleMode::NONE;
					mNetStateMode = RsNetState::GOOD;
	
					//mExtAddrStable = true;
				}
				else if (mPortForwardSet)
				{
					mNetworkMode = RsNetworkMode::BEHINDNAT;
					// Use Fallback Guess.
					//mNatTypeMode = RsNatTypeMode::UNKNOWN;
					mNatHoleMode = RsNatHoleMode::FORWARDED;
					mNetStateMode = RsNetState::ADV_FORWARD;
	
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
			mNetworkMode = RsNetworkMode::BEHINDNAT;
			mNatTypeMode = RsNatTypeMode::UNKNOWN;
			mNatHoleMode = RsNatHoleMode::UPNP;
			//mExtAddress = ... from UPnP.
			//mExtAddrStable = true;
			mNetStateMode = RsNetState::WARNING_NODHT;
		}
		else if (mNatPMPActive)
		{
			// This Mode is OKAY.
			mNetworkMode = RsNetworkMode::BEHINDNAT;
			mNatTypeMode = RsNatTypeMode::UNKNOWN;
			mNatHoleMode = RsNatHoleMode::NATPMP;
			//mExtAddress = ... from NatPMP.
			//mExtAddrStable = true;
			mNetStateMode = RsNetState::WARNING_NODHT;
		}
		else 
		{
			/* if we reach this point, we really need a Web "WhatsMyIp" Check of our Ext Ip Address. */
			/* Check for the possibility of an EXT address ... */
			bool isExtAddress = false;

			//mExtAddress = ... from WhatsMyIp.

			if (isExtAddress)
			{
				mNetworkMode = RsNetworkMode::EXTERNALIP;
				mNatTypeMode = RsNatTypeMode::NONE;
				mNatHoleMode = RsNatHoleMode::NONE;

				//mExtAddrStable = true;
				mNetStateMode = RsNetState::WARNING_NODHT;
			}
			else if (mPortForwardSet)
			{
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::UNKNOWN;
				mNatHoleMode = RsNatHoleMode::FORWARDED;

				//mExtAddrStable = true; // Probably, makin assumption.
				mNetStateMode = RsNetState::WARNING_NODHT;
			}
			else
			{
				/* At this point we must assume firewalled.
				 * These people have destroyed the possibility of making connections ;(
				 * Should WARN about this.
				 */ 
				mNetworkMode = RsNetworkMode::BEHINDNAT;
				mNatTypeMode = RsNatTypeMode::UNKNOWN;
				mNatHoleMode = RsNatHoleMode::NONE;
				mNetStateMode = RsNetState::BAD_NODHT_NAT;

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
	mConnectModes = RsConnectModes::NONE;
	switch(mNetworkMode)
	{
	    case RsNetworkMode::UNKNOWN:
	    case RsNetworkMode::OFFLINE:
	    case RsNetworkMode::LOCALNET:
	    case RsNetworkMode::RESTARTING:
			/* nothing here */
			break;
	    case RsNetworkMode::EXTERNALIP:
			mConnectModes = RsConnectModes::OUTGOING_TCP;
			mConnectModes |= RsConnectModes::ACCEPT_TCP;

			if (mDhtActive)
			{
				mConnectModes |= RsConnectModes::DIRECT_UDP;
				/* if open port. don't want PROXY or RELAY connect 
				 * because we should be able to do direct with EVERYONE.
				 * Ability to do Proxy is dependent on FIREWALL status.
				 * Technically could do RELAY, but disable both.
				 */
				//mConnectModes |= RsConnectModes::PROXY_UDP;
				//mConnectModes |= RsConnectModes::RELAY_UDP;
			}
			break;
	    case RsNetworkMode::BEHINDNAT:
			mConnectModes = RsConnectModes::OUTGOING_TCP;

			/* we're okay if there's a NAT HOLE */
			if ((mNatHoleMode == RsNatHoleMode::UPNP) ||
			    (mNatHoleMode == RsNatHoleMode::NATPMP) ||
			    (mNatHoleMode == RsNatHoleMode::FORWARDED))
			{
				mConnectModes |= RsConnectModes::ACCEPT_TCP;
				if (mDhtActive)
				{
					mConnectModes |= RsConnectModes::DIRECT_UDP;
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
				mConnectModes |= RsConnectModes::DIRECT_UDP;
				mConnectModes |= RsConnectModes::RELAY_UDP;

				if ((mNatTypeMode == RsNatTypeMode::RESTRICTED_CONE) ||
				    (mNatTypeMode == RsNatTypeMode::FULL_CONE) ||
				    (mNatTypeMode == RsNatTypeMode::DETERM_SYM))
				{
					mConnectModes |= RsConnectModes::PROXY_UDP;
				}
			   }
			}
			break;
	}
}
