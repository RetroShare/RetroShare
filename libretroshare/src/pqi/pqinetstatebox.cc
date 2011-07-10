
#include "util/rsnet.h"
#include "pqi/pqinetstatebox.h"
#include "time.h"

// External Interface.

void pqiNetStateBox::setAddressStunDht(struct sockaddr_in *addr, bool stable)
{
	if ((!mStunDhtSet) || (mStunDhtStable != stable) || 
			(addr->sin_addr.s_addr != mStunDhtAddr.sin_addr.s_addr) ||
			(addr->sin_port != mStunDhtAddr.sin_port))
	{
		mStunDhtSet = true;
		mStunDhtStable = stable;
		mStunDhtAddr = *addr;

		mStatusOkay = false;
	}
	mStunDhtTS = time(NULL);
}


void pqiNetStateBox::setAddressStunProxy(struct sockaddr_in *addr, bool stable)
{
	if ((!mStunProxySet) || (mStunProxyStable != stable) || 
		(addr->sin_addr.s_addr != mStunProxyAddr.sin_addr.s_addr) ||
		(addr->sin_port != mStunProxyAddr.sin_port))

	{
		mStunProxySet = true;
		mStunProxyStable = stable;
		mStunProxyAddr = *addr;

		mStatusOkay = false;
	}
	mStunProxyTS = time(NULL);
}


void pqiNetStateBox::setAddressUPnP(bool active, struct sockaddr_in *addr)
{
	if ((!mUPnPSet) || (mUPnPActive != active) || 
		(addr->sin_addr.s_addr != mUPnPAddr.sin_addr.s_addr) ||
		(addr->sin_port != mUPnPAddr.sin_port))

	{
		mUPnPSet = true;
		mUPnPAddr = *addr;
		mUPnPActive = active;

		mStatusOkay = false;
	}
	mUPnPTS = time(NULL);
}


void pqiNetStateBox::setAddressNatPMP(bool active, struct sockaddr_in *addr)
{
	if ((!mNatPMPSet) || (mNatPMPActive != active) || 
		(addr->sin_addr.s_addr != mNatPMPAddr.sin_addr.s_addr) ||
		(addr->sin_port != mNatPMPAddr.sin_port))

	{
		mNatPMPSet = true;
		mNatPMPAddr = *addr;
		mNatPMPActive = active;

		mStatusOkay = false;
	}
	mNatPMPTS = time(NULL);
}



void pqiNetStateBox::setAddressWebIP(bool active, struct sockaddr_in *addr)
{
	if ((!mWebIPSet) || (mWebIPActive != active) || 
		(addr->sin_addr.s_addr != mWebIPAddr.sin_addr.s_addr) ||
		(addr->sin_port != mWebIPAddr.sin_port))

	{
		mWebIPSet = true;
		mWebIPAddr = *addr;
		mWebIPActive = active;

		mStatusOkay = false;
	}
	mWebIPTS = time(NULL);
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
	mStatusOkay = false;
	//time_t mStatusTS;
	
	mNetworkMode = PNSB_NETWORK_UNKNOWN;
	mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
	mNatHoleMode = PNSB_NATHOLE_UNKNOWN;
	mConnectModes = PNSB_CONNECT_NONE;
	mNetStateMode = PNSB_NETSTATE_BAD_UNKNOWN;
	
	/* Parameters set externally */
	
	mStunDhtSet = false;
	time_t mStunDhtTS = 0;
	bool mStunDhtStable = false;
	//struct sockaddr_in mStunDhtAddr;
	
	mStunProxySet = false;
	time_t mStunProxyTS = 0;
	bool mStunProxyStable = false;
	//struct sockaddr_in mStunProxyAddr;
	
	mUPnPSet = false;
	mUPnPActive = false;
	//struct sockaddr_in mUPnPAddr;

	mNatPMPSet = false;
	mNatPMPActive = false;
	//struct sockaddr_in mNatPMPAddr;

	mWebIPSet = false;
	mWebIPActive = false;
	//struct sockaddr_in mWebIPAddr;

	mPortForwardedSet = false;
	mPortForwarded = 0;
	
	mDhtSet = false;
	mDhtActive = false;
	mDhtOn = false;

}

#define NETSTATE_PARAM_TIMEOUT		600
#define NETSTATE_TIMEOUT			60

	/* check/update Net State */
int pqiNetStateBox::statusOkay()
{
	if (!mStatusOkay)
	{
		return 0;
	}
	time_t now = time(NULL);
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
	/* check if any measurements are too old to consider */
	time_t now = time(NULL);
	if (now - mStunProxyTS > NETSTATE_PARAM_TIMEOUT)
	{
		mStunProxySet = false;
	}

	if (now - mStunDhtTS > NETSTATE_PARAM_TIMEOUT)
	{
		mStunDhtSet = false;
	}
}


void pqiNetStateBox::determineNetworkState()
{
	clearOldNetworkData();
	time_t now = time(NULL);

	/* now we use the remaining valid input to determine network state */

	/* Most important Factor is whether we have DHT(STUN) info to ID connection */
	if (mDhtActive)
	{
	   /* firstly lets try to identify OFFLINE / UNKNOWN */
	   if ((!mStunProxySet) || (!mStunDhtSet))
	   {
		mNetworkMode = PNSB_NETWORK_UNKNOWN;
		// Assume these.
		mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
		mNatHoleMode = PNSB_NATHOLE_NONE;
		mNetStateMode = PNSB_NETSTATE_BAD_UNKNOWN;

		//mExtAddress = .... unknown;
		//mExtAddrStable = false;
	   }
	   else // Both Are Set!
	   {
		if (!mStunDhtStable)
		{
			//mExtAddress = mStunDhtExtAddress;
			//mExtAddrStable = false;

			if (!mStunProxyStable)
			{
				/* both unstable, Symmetric NAT, Firewalled, No UDP Hole */
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_SYMMETRIC;
				mNatHoleMode = PNSB_NATHOLE_NONE;
				mNetStateMode = PNSB_NETSTATE_BAD_NATSYM;
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

				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_RESTRICTED_CONE;
				mNatHoleMode = PNSB_NATHOLE_NONE;
				mNetStateMode = PNSB_NETSTATE_WARNING_NATTED;
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
			if (!mStunProxyStable)
			{
				/* must be a forwarded port/ext or something similar */
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_SYMMETRIC;
				mNatHoleMode = PNSB_NATHOLE_FORWARDED;
				mNetStateMode = PNSB_NETSTATE_GOOD;
			}
			else
			{
				/* fallback is FULL CONE NAT */
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_FULL_CONE;
				mNatHoleMode = PNSB_NATHOLE_NONE;
				mNetStateMode = PNSB_NETSTATE_WARNING_NATTED;
			}

			if (mUPnPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
				mNatHoleMode = PNSB_NATHOLE_UPNP;
				mNetStateMode = PNSB_NETSTATE_GOOD;
				//mExtAddress = ... from UPnP, should match StunDht.
				//mExtAddrStable = true;
			}
			else if (mNatPMPActive)
			{
				// This Mode is OKAY.
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				// Use Fallback Guess.
				//mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
				mNatHoleMode = PNSB_NATHOLE_NATPMP;
				mNetStateMode = PNSB_NETSTATE_GOOD;
				//mExtAddress = ... from NatPMP, should match NatPMP
				//mExtAddrStable = true;
			}
			else 
			{
				bool isExtAddress = false;
	
				if (isExtAddress)
				{
					mNetworkMode = PNSB_NETWORK_EXTERNALIP;
					mNatTypeMode = PNSB_NATTYPE_NONE;
					mNatHoleMode = PNSB_NATHOLE_NONE;
					mNetStateMode = PNSB_NETSTATE_GOOD;
	
					//mExtAddrStable = true;
				}
				else if (mPortForwardedSet)
				{
					mNetworkMode = PNSB_NETWORK_BEHINDNAT;
					// Use Fallback Guess.
					//mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
					mNatHoleMode = PNSB_NATHOLE_FORWARDED;
					mNetStateMode = PNSB_NETSTATE_ADV_FORWARD;
	
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
			mNetworkMode = PNSB_NETWORK_BEHINDNAT;
			mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
			mNatHoleMode = PNSB_NATHOLE_UPNP;
			//mExtAddress = ... from UPnP.
			//mExtAddrStable = true;
			mNetStateMode = PNSB_NETSTATE_WARNING_NODHT;
		}
		else if (mNatPMPActive)
		{
			// This Mode is OKAY.
			mNetworkMode = PNSB_NETWORK_BEHINDNAT;
			mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
			mNatHoleMode = PNSB_NATHOLE_NATPMP;
			//mExtAddress = ... from NatPMP.
			//mExtAddrStable = true;
			mNetStateMode = PNSB_NETSTATE_WARNING_NODHT;
		}
		else 
		{
			/* if we reach this point, we really need a Web "WhatsMyIp" Check of our Ext Ip Address. */
			/* Check for the possibility of an EXT address ... */
			bool isExtAddress = false;

			//mExtAddress = ... from WhatsMyIp.

			if (isExtAddress)
			{
				mNetworkMode = PNSB_NETWORK_EXTERNALIP;
				mNatTypeMode = PNSB_NATTYPE_NONE;
				mNatHoleMode = PNSB_NATHOLE_NONE;

				//mExtAddrStable = true;
				mNetStateMode = PNSB_NETSTATE_WARNING_NODHT;
			}
			else if (mPortForwardedSet)
			{
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
				mNatHoleMode = PNSB_NATHOLE_FORWARDED;

				//mExtAddrStable = true; // Probably, makin assumption.
				mNetStateMode = PNSB_NETSTATE_WARNING_NODHT;
			}
			else
			{
				/* At this point we must assume firewalled.
				 * These people have destroyed the possibility of making connections ;(
				 * Should WARN about this.
				 */ 
				mNetworkMode = PNSB_NETWORK_BEHINDNAT;
				mNatTypeMode = PNSB_NATTYPE_UNKNOWN;
				mNatHoleMode = PNSB_NATHOLE_NONE;
				mNetStateMode = PNSB_NETSTATE_BAD_NODHT_NAT;

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
	mConnectModes = PNSB_CONNECT_NONE;
	switch(mNetworkMode)
	{
		case PNSB_NETWORK_UNKNOWN:
		case PNSB_NETWORK_OFFLINE:
		case PNSB_NETWORK_LOCALNET:
			/* nothing here */
			break;
		case PNSB_NETWORK_EXTERNALIP:
			mConnectModes = PNSB_CONNECT_OUTGOING_TCP;
			mConnectModes |= PNSB_CONNECT_ACCEPT_TCP;

			if (mDhtActive)
			{
				mConnectModes |= PNSB_CONNECT_DIRECT_UDP;
				/* if open port. don't want PROXY or RELAY connect 
				 * because we should be able to do direct with EVERYONE.
				 * Ability to do Proxy is dependent on FIREWALL status.
				 * Technically could do RELAY, but disable both.
				 */
				//mConnectModes |= PNSB_CONNECT_PROXY_UDP;
				//mConnectModes |= PNSB_CONNECT_RELAY_UDP;
			}
			break;
		case PNSB_NETWORK_BEHINDNAT:
			mConnectModes = PNSB_CONNECT_OUTGOING_TCP;

			/* we're okay if there's a NAT HOLE */
			if ((mNatHoleMode == PNSB_NATHOLE_UPNP) ||
				(mNatHoleMode == PNSB_NATHOLE_NATPMP) ||
				(mNatHoleMode == PNSB_NATHOLE_FORWARDED))
			{
				mConnectModes |= PNSB_CONNECT_ACCEPT_TCP;
				if (mDhtActive)
				{
					mConnectModes |= PNSB_CONNECT_DIRECT_UDP;
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
				mConnectModes |= PNSB_CONNECT_DIRECT_UDP;
				mConnectModes |= PNSB_CONNECT_RELAY_UDP;

				if ((mNatTypeMode == PNSB_NATTYPE_RESTRICTED_CONE) ||
				    (mNatTypeMode == PNSB_NATTYPE_FULL_CONE))
				{
					mConnectModes |= PNSB_CONNECT_PROXY_UDP;
				}
			   }
			}
			break;
	}
}

