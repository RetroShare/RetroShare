/*
 * libretroshare/src/dht: connectstatebox.cc
 *
 * RetroShare DHT C++ Interface.
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

#include "dht/connectstatebox.h"
#include "retroshare/rsconfig.h"

#include <iostream>
#include <sstream>

#define FAILED_WAIT_TIME	(300) //(1800) // 30 minutes.
#define TCP_WAIT_TIME		(60)   // 1 minutes.
#define DIRECT_WAIT_TIME	(60)   // 1 minutes.
#define PROXY_WAIT_TIME		(60)   // 1 minutes.
#define RELAY_WAIT_TIME		(60)   // 1 minutes.
#define REVERSE_WAIT_TIME	(300)  // 5 minutes.

#define MAX_DIRECT_ATTEMPTS	(3)
#define MAX_PROXY_ATTEMPTS	(3)
#define MAX_RELAY_ATTEMPTS	(3)



PeerConnectStateBox::PeerConnectStateBox()
{
	//mPeerId = id;
	mState = CSB_START;
	mNetState = CSB_NETSTATE_UNKNOWN;
	mStateTS = 0;
	mNoAttempts = 0;
}


std::string NetStateAsString(uint32_t netstate)
{
	std::string str;
	switch(netstate)
	{
		case CSB_NETSTATE_FORWARD:
			str = "Forwarded";
			break;
			
		case CSB_NETSTATE_STABLENAT:
			str = "StableNat";
			break;
			
		case CSB_NETSTATE_EXCLUSIVENAT:
			str = "ExclusiveNat";
			break;
			
		case CSB_NETSTATE_FIREWALLED:
			str = "Firewalled";
			break;
		default:
			str = "Unknown NetState";
			break;
	}
	return str;
}

std::string StateAsString(uint32_t state)
{
	std::string str;
	switch(state)
	{
		case CSB_START:
			str = "Start";
			break;

		case CSB_TCP_WAIT:
			str = "TCP Wait";
			break;
			
		case CSB_DIRECT_ATTEMPT:
			str = "Direct Attempt";
			break;
			
		case CSB_DIRECT_WAIT:
			str = "Direct Wait";
			break;
			
		case CSB_PROXY_ATTEMPT:
			str = "Proxy Attempt:";
			break;

		case CSB_PROXY_WAIT:
			str = "Proxy Wait:";
			break;

		case CSB_RELAY_ATTEMPT:
			str = "Relay Attempt:";
			break;

		case CSB_RELAY_WAIT:
			str = "Relay Wait:";
			break;

		case CSB_REVERSE_WAIT:
			str = "Reverse Wait:";
			break;

		case CSB_FAILED_WAIT:
			str = "Failed Wait:";
			break;

		case CSB_CONNECTED:
			str = "Connected:";
			break;

		default:
			str = "Unknown State";
			break;
	}

	return str;
}


std::string UpdateAsString(uint32_t update)
{
	std::string str;
	switch(update)
	{
		case CSB_UPDATE_NONE:
			str = "none";
			break;
			
		case CSB_UPDATE_CONNECTED:
			str = "Connected";
			break;
			
		case CSB_UPDATE_DISCONNECTED:
			str = "Disconnected";
			break;

		case CSB_UPDATE_AUTH_DENIED:
			str = "Auth Denied";
			break;

		case CSB_UPDATE_FAILED_ATTEMPT:
			str = "Failed Attempt:";
			break;

		case CSB_UPDATE_MODE_UNAVAILABLE:
			str = "Mode Unavailable:";
			break;

		default:
			str = "Unknown Update";
			break;
	}
	return str;
}

void PeerConnectStateBox::errorMsg(std::ostream &out, std::string msg, uint32_t updateParam)
{
	out << "PeerConnectStateBox::ERROR " << msg;
	out << " NetState: " << NetStateAsString(mNetState);
	out << " State: " << StateAsString(mState);
	out << " Update: " << UpdateAsString(updateParam);
	out << " for peer: " << mPeerId;
	out << std::endl;
}


void PeerConnectStateBox::stateMsg(std::ostream &out, std::string msg, uint32_t updateParam)
{
	out << "PeerConnectStateBox::MSG " << msg;
	out << " NetState: " << NetStateAsString(mNetState);
	out << " State: " << StateAsString(mState);
	out << " Update: " << UpdateAsString(updateParam);
	out << " for peer: " << mPeerId;
	out << std::endl;
}

std::string PeerConnectStateBox::connectState() const
{
	std::string str = StateAsString(mState);
	std::ostringstream out;
	time_t now = time(NULL);
	out << str << "(" << mNoAttempts << ") for " << now - mStateTS << " secs";
	return out.str();
}


uint32_t convertNetStateToInternal(uint32_t netmode, uint32_t nattype)
{
	uint32_t connNet = CSB_NETSTATE_UNKNOWN;
		
	if (netmode == RSNET_NETWORK_EXTERNALIP)
	{
		connNet = CSB_NETSTATE_FORWARD;
	}
	else if (netmode == RSNET_NETWORK_BEHINDNAT)
	{
		if ((nattype == RSNET_NATTYPE_RESTRICTED_CONE) ||
			(nattype == RSNET_NATTYPE_FULL_CONE))
		{
			connNet = CSB_NETSTATE_STABLENAT;
		}
		else if (nattype == RSNET_NATTYPE_DETERM_SYM)
		{
			connNet = CSB_NETSTATE_EXCLUSIVENAT; 
		}
		else
		{
			connNet = CSB_NETSTATE_FIREWALLED;
		}
	}
	return connNet;
}

bool shouldUseProxyPortInternal(uint32_t netstate)
{
	if (netstate == CSB_NETSTATE_FORWARD)
	{
		return false;
	}
	return true;
}

bool PeerConnectStateBox::shouldUseProxyPort(uint32_t netmode, uint32_t nattype)
{
	uint32_t netstate = convertNetStateToInternal(netmode, nattype);
	return shouldUseProxyPortInternal(netstate);
}

uint32_t PeerConnectStateBox::connectCb(uint32_t cbtype, uint32_t netmode, uint32_t nattype)
{
	uint32_t netstate = convertNetStateToInternal(netmode, nattype);

	std::cerr << "PeerConnectStateBox::connectCb(";
	if (cbtype == CSB_CONNECT_DIRECT)
	{
		std::cerr << "DIRECT";
	}
	else
	{
		std::cerr << "UNREACHABLE";
	}
	std::cerr << "," << NetStateAsString(netstate) << ")";
	std::cerr << std::endl;

	if (netstate != mNetState)
	{
		std::cerr << "PeerConnectStateBox::connectCb() WARNING Changing NetState from: ";
		std::cerr << " from: " << NetStateAsString(mNetState);
		std::cerr << " to: " << NetStateAsString(netstate);
		std::cerr << " for peer: " << mPeerId;
		std::cerr << std::endl;

		mNetState = netstate;
	}

	if (cbtype == CSB_CONNECT_DIRECT)
	{
		return connectCb_direct();
	}
	else
	{
		return connectCb_unreachable();
	}
}

	
uint32_t PeerConnectStateBox::connectCb_direct()
{
	uint32_t retval = 0;
	time_t now = time(NULL);

	switch(mState)
	{

		case CSB_DIRECT_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == DIRECT_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_PROXY_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == PROXY_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_RELAY_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == RELAY_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_FAILED_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < FAILED_WAIT_TIME) 
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
				break;
			}
		}	/* FALLTHROUGH TO START CASE */
		default:
		case CSB_REVERSE_WAIT:
		case CSB_PROXY_WAIT:
		case CSB_RELAY_WAIT:
		{
			if (mState != CSB_FAILED_WAIT)
			{
				/* ERROR */
				errorMsg(std::cerr, "mState != FAILED_WAIT", 0);

			}

		}	/* FALLTHROUGH TO START CASE */
		case CSB_START:
		{
			/* starting up the connection */
			mState = CSB_TCP_WAIT;
			retval = CSB_ACTION_TCP_CONN;
			mStateTS = now;
			mNoAttempts = 0;

		}
			break;
		case CSB_TCP_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < TCP_WAIT_TIME)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
			}
			else
			{
				/* try again */
				mState = CSB_DIRECT_ATTEMPT;
				retval = CSB_ACTION_DIRECT_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts = 0;
			}
		}
			break;

		case CSB_DIRECT_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < DIRECT_WAIT_TIME)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
			}
			else if (mNoAttempts >= MAX_DIRECT_ATTEMPTS) /* if too many attempts */
			{
				/* no RELAY attempt => FAILED_WAIT */
				mState = CSB_FAILED_WAIT;
				retval = CSB_ACTION_WAIT;
				mStateTS = now;
				mNoAttempts = 0;
			}
			else
			{
				/* try again */
				mState = CSB_DIRECT_ATTEMPT;
				retval = CSB_ACTION_DIRECT_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts++;
			}
		}
			break;

		case CSB_CONNECTED:
		{
				retval = CSB_ACTION_WAIT;
		}
			break;
	}

	return retval;

}


uint32_t PeerConnectStateBox::connectCb_unreachable()
{
	uint32_t retval = 0;

	uint32_t proxyPortMode = CSB_ACTION_PROXY_PORT;
	if (!shouldUseProxyPortInternal(mNetState))
	{
		proxyPortMode = CSB_ACTION_DHT_PORT;
	}

	time_t now = time(NULL);

	switch(mState)
	{
		case CSB_DIRECT_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == DIRECT_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_PROXY_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == PROXY_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_RELAY_ATTEMPT:
		{
			errorMsg(std::cerr, "mState == RELAY_ATTEMPT", 0);
			retval = CSB_ACTION_WAIT;
		}
			break;

		case CSB_FAILED_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < FAILED_WAIT_TIME)
			{
				/* same state */
				stateMsg(std::cerr, "too soon, no action", 0);
				retval = CSB_ACTION_WAIT;
				break;
			}
		}	/* FALLTHROUGH TO START CASE */
		default:
		case CSB_DIRECT_WAIT:
		{
			if (mState != CSB_FAILED_WAIT)
			{
				/* ERROR */
				errorMsg(std::cerr, "mState != FAILED_WAIT", 0);

			}

		}	/* FALLTHROUGH TO START CASE */
		case CSB_START:
		{
			/* starting up the connection */
			mState = CSB_TCP_WAIT;
			retval = CSB_ACTION_WAIT;  /* NO POINT TRYING A TCP_CONN */
			mStateTS = now;
			mNoAttempts = 0;

		}
			break;
		case CSB_TCP_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < TCP_WAIT_TIME)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
			}
			else
			{
				/* starting up the connection */
				if (mState != CSB_NETSTATE_FIREWALLED)
				{
					stateMsg(std::cerr, "not Firewalled => PROXY_ATTEMPT", 0);
					mState = CSB_PROXY_ATTEMPT;
					retval = CSB_ACTION_PROXY_CONN | proxyPortMode;
				}
				else
				{
					stateMsg(std::cerr, "Firewalled => RELAY_ATTEMPT", 0);
					mState = CSB_RELAY_ATTEMPT;
					retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;

				}

				mStateTS = now;
				mNoAttempts = 0;
			}

		}
			break;
		case CSB_PROXY_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < PROXY_WAIT_TIME)
			{
				/* same state */
				stateMsg(std::cerr, "too soon, no action", 0);
				retval = CSB_ACTION_WAIT;
			}
			else if (mNoAttempts >= MAX_PROXY_ATTEMPTS) /* if too many attempts */
			{
				/* switch to RELAY attempt */
				stateMsg(std::cerr, "too many PROXY => RELAY_ATTEMPT", 0);
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts = 0;
			}
			else
			{
				/* try again */
				stateMsg(std::cerr, "PROXY_ATTEMPT try again", 0);
				mState = CSB_PROXY_ATTEMPT;
				retval = CSB_ACTION_PROXY_CONN | proxyPortMode;
				mStateTS = now;
				mNoAttempts++;
			}
		}
			break;

		case CSB_REVERSE_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < REVERSE_WAIT_TIME)
			{
				/* same state */
				stateMsg(std::cerr, "too soon, no action", 0);
				retval = CSB_ACTION_WAIT;
			}
			else 
			{
				stateMsg(std::cerr, "timeout => RELAY_ATTEMPT", 0);
				/* switch to RELAY attempt */
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts = 0;
			}
		}
			break;

		case CSB_RELAY_WAIT:
		{
			/* if too soon */
			if (now - mStateTS < RELAY_WAIT_TIME)
			{
				/* same state */
				stateMsg(std::cerr, "too soon, no action", 0);
				retval = CSB_ACTION_WAIT;
			}
			else if (mNoAttempts >= MAX_RELAY_ATTEMPTS) /* if too many attempts */
			{
				/* switch to RELAY attempt */
				stateMsg(std::cerr, "too many RELAY => FAILED_WAIT", 0);
				mState = CSB_FAILED_WAIT;
				retval = CSB_ACTION_WAIT;
				mStateTS = now;
				mNoAttempts = 0;
			}
			else
			{
				/* try again */
				stateMsg(std::cerr, "RELAY_ATTEMPT try again", 0);
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts++;
			}
		}
			break;

		case CSB_CONNECTED:
		{
				stateMsg(std::cerr, "connected => no action", 0);
				retval = CSB_ACTION_WAIT;
		}
			break;
	}

	return retval;
}



uint32_t PeerConnectStateBox::updateCb(uint32_t update)
{
	/* The Error Callback doesn't trigger another connection.
	 * but can change the connection state
	 *
	 * Possible Errors:
	 * 1) AUTH DENIED.  (fatal) 
	 * 2) MODE UNAVILABLE 
	 * 3) FAILED ATTEMPT
	 * 4) CONNECTION
	 * 5) DISCONNECTED.
	 *
	 * Fitting these into the states:
		case CSB_START:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
		case CSB_CONNECTED:
			CONNECTION => CSB_CONNECTED
			DISCONNECTED => CSB_START
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT

		case CSB_FAILED_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
		case CSB_REVERSE_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
		case CSB_DIRECT_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			MODE UNAVAILABLE - probably error => CSB_FAILED_WAIT
			error if: MODE UNAVAILABLE, DISCONNECTED

		case CSB_PROXY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			MODE_UNAVAILABLE => CSB_REVERSE_WAIT | CSB_RELAY_ATTEMPT
			error if: DISCONNECTED

		case CSB_RELAY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			MODE_UNAVAILABLE => CSB_FAILED_WAIT
			error if: DISCONNECTED

		case CSB_DIRECT_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
		case CSB_PROXY_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH_DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
		case CSB_RELAY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH_DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, DISCONNECTED
	 */

	/* DO Connect / Disconnect Updates ... very specific! */
	time_t now = time(NULL);
	switch(update)
	{
		case CSB_UPDATE_CONNECTED:
		{
			stateMsg(std::cerr, "=> CONNECTED", update);
			mState = CSB_CONNECTED;
			mStateTS = now;
			return 0;
		}
			break;
		case CSB_UPDATE_DISCONNECTED:
		{
			if (mState != CSB_CONNECTED)
			{
				/* ERROR, ignore (as already in disconnected state) */
				errorMsg(std::cerr, "mState != CSB_CONNECTED", update);

			}
			else
			{
				stateMsg(std::cerr, "=> START", update);
				/* move to START state */
				mState = CSB_START;
				mStateTS = now;
				return 1;
			}
				
			return 0;
		}
			break;
	}


	/* Now catch errors for feedback when we should be WAITING */
	switch(mState)
	{
		default:
		case CSB_DIRECT_WAIT:
		case CSB_PROXY_WAIT:
		case CSB_RELAY_WAIT:
		case CSB_REVERSE_WAIT:
		case CSB_FAILED_WAIT:
		case CSB_START:
		case CSB_CONNECTED: /* impossible */
		{
			/* ERROR */
			/* shouldn't receive anything here! */
			errorMsg(std::cerr, "shouldnt get anything", update);

			return 0;
		}
			break;

		case CSB_DIRECT_ATTEMPT:
		case CSB_PROXY_ATTEMPT:
		case CSB_RELAY_ATTEMPT:
		{
			/* OKAY */

		}
			break;
	}


	switch(update)
	{
		/* if AUTH_DENIED ... => FAILED_WAIT */
		case CSB_UPDATE_AUTH_DENIED:
		{
			stateMsg(std::cerr, "=> FAILED WAIT", update);
			mState = CSB_FAILED_WAIT;
			mStateTS = now;

			return 1;
		}
			break;
		/* if standard FAIL => stay where we are */
		case CSB_UPDATE_FAILED_ATTEMPT:
		{
			stateMsg(std::cerr, "STANDARD FAIL => switch to wait state", update);
			switch(mState)
			{
				case CSB_DIRECT_ATTEMPT:
					mState = CSB_DIRECT_WAIT;
					mStateTS = now;
					break;
				case CSB_PROXY_ATTEMPT:
					mState = CSB_PROXY_WAIT;
					mStateTS = now;
					break;
				case CSB_RELAY_ATTEMPT:
					mState = CSB_RELAY_WAIT;
					mStateTS = now;
					break;
				default:
					stateMsg(std::cerr, "STANDARD FAIL, but unusual state", update);
					break;
			}
					
			return 1;
		}
			break;

		/* finally MODE_UNAVAILABLE */
		case CSB_UPDATE_MODE_UNAVAILABLE:
		{
			if (mState == CSB_PROXY_ATTEMPT)
			{
				if (mNetState == CSB_NETSTATE_FORWARD)
				{
					stateMsg(std::cerr, "as FORWARDED => REVERSE_WAIT", update);
					mState = CSB_REVERSE_WAIT;
					mStateTS = now;
				}
				else
				{
					stateMsg(std::cerr, "as !FORWARDED => RELAY_ATTEMPT", update);
					mState = CSB_RELAY_WAIT;
					mNoAttempts = 0;
					mStateTS = now;
				}
				return 1;
			}
			else
			{
				stateMsg(std::cerr, "MODE UNAVAIL => FAILED_WAIT", update);
				mState = CSB_FAILED_WAIT;
				mStateTS = now;

				if ((mState == CSB_DIRECT_ATTEMPT)
					|| (mState == CSB_PROXY_ATTEMPT))
				{
					/* OKAY */
					return 1;
				}
				else
				{
					/* ERROR */
					errorMsg(std::cerr, "strange MODE", update);

					return 0;
				}
			}
		}
			break;

		default:
		{
			/* ERROR */
			errorMsg(std::cerr, "impossible default", update);
		}
			break;
	}
				
	/* if we get here... ERROR */
	errorMsg(std::cerr, "if we get here => ERROR", update);


	return 0;
}










bool PeerConnectStateBox::storeProxyPortChoice(uint32_t flags, bool useProxyPort)
{
	mProxyPortFlags = flags;
	mProxyPortChoice = useProxyPort;
	mProxyPortTS = time(NULL);

	return useProxyPort;
}

bool PeerConnectStateBox::getProxyPortChoice()
{
	time_t now = time(NULL);

	std::cerr << "PeerConnectStateBox::getProxyPortChoice() Using ConnectLogic Info from: ";
	std::cerr << now-mProxyPortTS << " ago. Flags: " << mProxyPortFlags;
	std::cerr << " UseProxyPort? " << mProxyPortChoice;
	std::cerr << std::endl;

	return mProxyPortChoice;
}



