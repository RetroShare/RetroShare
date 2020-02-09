/*******************************************************************************
 * libretroshare/src/dht: connectstatebox.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <drbob@lunamutt.com>                   *
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

#include "dht/connectstatebox.h"

#include "util/rsrandom.h"
#include "util/rsstring.h"

#include <iostream>

/**
 *
 * #define TESTING_PERIODS	1
 * #define DEBUG_CONNECTBOX	1
 *
 **/


/* Have made the PROXY Attempts + MAX_TIME much larger, 
 * have have potential for this to take a while.
 */

#ifdef TESTING_PERIODS
	#define FAILED_WAIT_TIME	(1800)  // 5 minutes.
	#define TCP_WAIT_TIME		(10)   // 1/6 minutes.
	#define DIRECT_MAX_WAIT_TIME	(30)   // 1/6 minutes.
	#define PROXY_BASE_WAIT_TIME	(10)   // 30 // 1/6 minutes.
	#define PROXY_MAX_WAIT_TIME	(30)  // 120 // 1/6 minutes.
	#define RELAY_MAX_WAIT_TIME	(30)   // 1/6 minutes.
	#define REVERSE_WAIT_TIME	(30)   // 1/2 minutes.

	#define MAX_DIRECT_ATTEMPTS	(3)
	#define MAX_PROXY_ATTEMPTS	(3)
	#define MAX_RELAY_ATTEMPTS	(3)

	#define MAX_DIRECT_FAILED_ATTEMPTS	(1)
	#define MAX_PROXY_FAILED_ATTEMPTS	(2)
	#define MAX_RELAY_FAILED_ATTEMPTS	(2)
#else
	#define FAILED_WAIT_TIME	(1800) // 30 minutes.
	#define TCP_WAIT_TIME		(60)   // 1 minutes.
	#define DIRECT_MAX_WAIT_TIME	(60)   // 1 minutes.

	#define PROXY_BASE_WAIT_TIME	(30)   // 1/2 minutes.
	#define PROXY_MAX_WAIT_TIME	(120)   // 1 minutes.

	#define RELAY_MAX_WAIT_TIME	(60)   // 1 minutes.
	#define REVERSE_WAIT_TIME	(300)  // 5 minutes.

	#define MAX_DIRECT_ATTEMPTS	(5)
	#define MAX_PROXY_ATTEMPTS	(10)
	#define MAX_RELAY_ATTEMPTS	(5)

	#define MAX_DIRECT_FAILED_ATTEMPTS	(3)
	#define MAX_PROXY_FAILED_ATTEMPTS	(3)
	#define MAX_RELAY_FAILED_ATTEMPTS	(3)

#endif

PeerConnectStateBox::PeerConnectStateBox()
{
	//mPeerId = id;
	rstime_t now = time(NULL);
	mState = CSB_START;
	mNetState = CSB_NETSTATE_UNKNOWN;
	mStateTS = now;
	mNoAttempts = 0;
        mNoFailedAttempts = 0;
        mNextAttemptTS = now;
        mAttemptLength = 0;
	mProxyPortFlags = 0;
	mProxyPortChoice = false;
	mProxyPortTS = 0;


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
			str = "Start:";
			break;

		case CSB_TCP_WAIT:
			str = "TCP Wait:";
			break;
			
		case CSB_DIRECT_ATTEMPT:
			str = "Direct Attempt:";
			break;
			
		case CSB_DIRECT_WAIT:
			str = "Direct Wait:";
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
			str = "Unknown State:";
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
	rstime_t now = time(NULL);
	std::string out;
	rs_sprintf(out, "%s(%lu/%lu) for %ld secs", str.c_str(), mNoAttempts, mNoFailedAttempts, now - mStateTS);
	if ( (mState == CSB_CONNECTED) || (mState == CSB_DIRECT_ATTEMPT) ||
		(mState == CSB_PROXY_ATTEMPT) || (mState == CSB_RELAY_ATTEMPT) ||
		(mState == CSB_FAILED_WAIT) )
	{
		rs_sprintf_append(out, " Last Attempt: %ld", mAttemptLength);
	}
	else
	{
		rs_sprintf_append(out, " LA: %ld", mAttemptLength);
		rs_sprintf_append(out, " NextAttempt: %ld", mNextAttemptTS - now);
	}

	return out;
}


uint32_t convertNetStateToInternal(RsNetworkMode netmode, uint32_t nathole, uint32_t nattype)
{
	uint32_t connNet = CSB_NETSTATE_UNKNOWN;
		
	if (netmode == RsNetworkMode::EXTERNALIP)
	{
		connNet = CSB_NETSTATE_FORWARD;
	}
	else if ((nathole != RSNET_NATHOLE_UNKNOWN) && (nathole != RSNET_NATHOLE_NONE))
	{
		connNet = CSB_NETSTATE_FORWARD;
	}
	else if (netmode == RsNetworkMode::BEHINDNAT)
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

bool PeerConnectStateBox::shouldUseProxyPort(RsNetworkMode netmode, uint32_t nathole, uint32_t nattype)
{
	uint32_t netstate = convertNetStateToInternal(netmode, nathole, nattype);
	return shouldUseProxyPortInternal(netstate);
}

uint32_t PeerConnectStateBox::calcNetState(RsNetworkMode netmode, uint32_t nathole, uint32_t nattype)
{
	uint32_t netstate = convertNetStateToInternal(netmode, nathole, nattype);
	return netstate;
}


uint32_t PeerConnectStateBox::connectCb(uint32_t cbtype, RsNetworkMode netmode, uint32_t nathole, uint32_t nattype)
{
	uint32_t netstate = convertNetStateToInternal(netmode, nathole, nattype);

#ifdef 	DEBUG_CONNECTBOX
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
#endif

	if (netstate != mNetState)
	{
#ifdef 	DEBUG_CONNECTBOX
		std::cerr << "PeerConnectStateBox::connectCb() WARNING Changing NetState from: ";
		std::cerr << " from: " << NetStateAsString(mNetState);
		std::cerr << " to: " << NetStateAsString(netstate);
		std::cerr << " for peer: " << mPeerId;
		std::cerr << std::endl;
#endif

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
	rstime_t now = time(NULL);

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
			//if (now - mStateTS < FAILED_WAIT_TIME) 
			if (mNextAttemptTS > now)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
				break;
			}
		}	/* FALLTHROUGH TO START CASE */
		/* fallthrough */
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
		/* fallthrough */
		case CSB_START:
		{
			/* starting up the connection */
			mState = CSB_TCP_WAIT;
			retval = CSB_ACTION_TCP_CONN;
			mStateTS = now;
			mNoAttempts = 0;
			mNoFailedAttempts = 0;
			mNextAttemptTS = now + TCP_WAIT_TIME;

		}
			break;
		case CSB_TCP_WAIT:
		{
			/* if too soon */
			//if (now - mStateTS < TCP_WAIT_TIME)
			if (mNextAttemptTS > now)
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
				mNoFailedAttempts = 0;
			}
		}
			break;

		case CSB_DIRECT_WAIT:
		{
			/* if too soon */
			//if (now - mStateTS < DIRECT_WAIT_TIME)
			if (mNextAttemptTS > now)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
			}
			else if ((mNoAttempts >= MAX_DIRECT_ATTEMPTS) ||
				 (mNoFailedAttempts >= MAX_DIRECT_FAILED_ATTEMPTS))
			{
				/* if too many attempts */
				/* no RELAY attempt => FAILED_WAIT */
				mState = CSB_FAILED_WAIT;
				retval = CSB_ACTION_WAIT;
				mStateTS = now;
				mNoAttempts = 0;
				mNoFailedAttempts = 0;
				mNextAttemptTS = now + FAILED_WAIT_TIME;
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

	rstime_t now = time(NULL);

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
			//if (now - mStateTS < FAILED_WAIT_TIME)
			if (mNextAttemptTS > now)
			{
				/* same state */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too soon, no action", 0);
#endif
				retval = CSB_ACTION_WAIT;
				break;
			}
		}	/* FALLTHROUGH TO START CASE */
		/* fallthrough */
		default:
		case CSB_DIRECT_WAIT:
		{
			if (mState != CSB_FAILED_WAIT)
			{
				/* ERROR */
				errorMsg(std::cerr, "mState != FAILED_WAIT", 0);

			}

		}	/* FALLTHROUGH TO START CASE */
		/* fallthrough */
		case CSB_START:
		{
			/* starting up the connection */
			mState = CSB_TCP_WAIT;
			retval = CSB_ACTION_WAIT;  /* NO POINT TRYING A TCP_CONN */
			mStateTS = now;
			mNoAttempts = 0;
			mNoFailedAttempts = 0;
			mNextAttemptTS = now + TCP_WAIT_TIME;

		}
			break;
		case CSB_TCP_WAIT:
		{
			/* if too soon */
			if (mNextAttemptTS > now)
			{
				/* same state */
				retval = CSB_ACTION_WAIT;
			}
			else
			{
				/* starting up the connection */
				if (mState != CSB_NETSTATE_FIREWALLED)
				{
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "not Firewalled => PROXY_ATTEMPT", 0);
#endif
					mState = CSB_PROXY_ATTEMPT;
					retval = CSB_ACTION_PROXY_CONN | proxyPortMode;
				}
				else
				{
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "Firewalled => RELAY_ATTEMPT", 0);
#endif
					mState = CSB_RELAY_ATTEMPT;
					retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;

				}

				mStateTS = now;
				mNoAttempts = 0;
				mNoFailedAttempts = 0;
			}

		}
			break;
		case CSB_PROXY_WAIT:
		{
			/* if too soon */
			if (mNextAttemptTS > now)
			{
				/* same state */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too soon, no action", 0);
#endif
				retval = CSB_ACTION_WAIT;
			}
			else if ((mNoAttempts >= MAX_PROXY_ATTEMPTS) ||
				 (mNoFailedAttempts >= MAX_PROXY_FAILED_ATTEMPTS))
			{
				/* if too many attempts */
				/* switch to RELAY attempt */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too many PROXY => RELAY_ATTEMPT", 0);
#endif
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts = 0;
				mNoFailedAttempts = 0;
			}
			else
			{
				/* try again */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "PROXY_ATTEMPT try again", 0);
#endif
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
			//if (now - mStateTS < REVERSE_WAIT_TIME)
			if (mNextAttemptTS > now)
			{
				/* same state */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too soon, no action", 0);
#endif
				retval = CSB_ACTION_WAIT;
			}
			else 
			{
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "timeout => RELAY_ATTEMPT", 0);
#endif
				/* switch to RELAY attempt */
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts = 0;
				mNoFailedAttempts = 0;
			}
		}
			break;

		case CSB_RELAY_WAIT:
		{
			/* if too soon */
			if (mNextAttemptTS > now)
			{
				/* same state */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too soon, no action", 0);
#endif
				retval = CSB_ACTION_WAIT;
			}
			else if ((mNoAttempts >= MAX_RELAY_ATTEMPTS) ||
				 (mNoFailedAttempts >= MAX_RELAY_FAILED_ATTEMPTS))
			{
				/* if too many attempts */
				/* switch to RELAY attempt */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "too many RELAY => FAILED_WAIT", 0);
#endif
				mState = CSB_FAILED_WAIT;
				retval = CSB_ACTION_WAIT;
				mStateTS = now;
				mNoAttempts = 0;
				mNoFailedAttempts = 0;
				mNextAttemptTS = now + FAILED_WAIT_TIME;
			}
			else
			{
				/* try again */
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "RELAY_ATTEMPT try again", 0);
#endif
				mState = CSB_RELAY_ATTEMPT;
				retval = CSB_ACTION_RELAY_CONN | CSB_ACTION_DHT_PORT;
				mStateTS = now;
				mNoAttempts++;
			}
		}
			break;

		case CSB_CONNECTED:
		{
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "connected => no action", 0);
#endif
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
	 * 3) RETRY ATTEMPT
	 * 4) FAILED ATTEMPT
	 * 5) CONNECTION
	 * 6) DISCONNECTED.
	 *
	 * Fitting these into the states:
		case CSB_START:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
		case CSB_CONNECTED:
			CONNECTION => CSB_CONNECTED
			DISCONNECTED => CSB_START
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT

		case CSB_FAILED_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
		case CSB_REVERSE_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
		case CSB_DIRECT_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			RETRY ATTEMPT => stay here
			MODE UNAVAILABLE - probably error => CSB_FAILED_WAIT
			error if: MODE UNAVAILABLE, DISCONNECTED

		case CSB_PROXY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			RETRY ATTEMPT => stay here
			MODE_UNAVAILABLE => CSB_REVERSE_WAIT | CSB_RELAY_ATTEMPT
			error if: DISCONNECTED

		case CSB_RELAY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			AUTH DENIED => CSB_FAILED_WAIT
			FAILED ATTEMPT => stay here.
			RETRY ATTEMPT => stay here
			MODE_UNAVAILABLE => CSB_FAILED_WAIT
			error if: DISCONNECTED

		case CSB_DIRECT_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
		case CSB_PROXY_WAIT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH_DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
		case CSB_RELAY_ATTEMPT:
			CONNECTION => CSB_CONNECTED
			error if: AUTH_DENIED, MODE UNAVAILABLE, FAILED ATTEMPT, RETRY ATTEMPT, DISCONNECTED
	 */

	/* DO Connect / Disconnect Updates ... very specific! */
	rstime_t now = time(NULL);
	switch(update)
	{
		case CSB_UPDATE_CONNECTED:
		{
			if ((mState == CSB_DIRECT_ATTEMPT) || (mState == CSB_PROXY_ATTEMPT) || (mState == CSB_RELAY_ATTEMPT))
			{
				mAttemptLength = now - mStateTS;
			}
			else
			{
				mAttemptLength = 0;
			}
	
#ifdef 	DEBUG_CONNECTBOX
			stateMsg(std::cerr, "=> CONNECTED", update);
#endif
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
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "=> START", update);
#endif
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
#ifdef 	DEBUG_CONNECTBOX
			stateMsg(std::cerr, "=> FAILED WAIT", update);
#endif
			mState = CSB_FAILED_WAIT;
			mStateTS = now;
			mAttemptLength = now - mStateTS;
			mNextAttemptTS = now + FAILED_WAIT_TIME;

			return 1;
		}
			break;
		/* if standard FAIL => stay where we are */
		case CSB_UPDATE_RETRY_ATTEMPT:
		{
#ifdef 	DEBUG_CONNECTBOX
			stateMsg(std::cerr, "RETRY FAIL => switch to wait state", update);
#endif
			mAttemptLength = now - mStateTS;
			switch(mState)
			{
				case CSB_DIRECT_ATTEMPT:
					mState = CSB_DIRECT_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + RSRandom::random_u32() % DIRECT_MAX_WAIT_TIME;
					break;
				case CSB_PROXY_ATTEMPT:
					mState = CSB_PROXY_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + PROXY_BASE_WAIT_TIME + (RSRandom::random_u32() % (PROXY_MAX_WAIT_TIME - PROXY_BASE_WAIT_TIME));
					break;
				case CSB_RELAY_ATTEMPT:
					mState = CSB_RELAY_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + RSRandom::random_u32() % RELAY_MAX_WAIT_TIME;
					break;
				default:
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "RETRY FAIL, but unusual state", update);
#endif
					break;
			}
					
			return 1;
		}
		/* if standard FAIL => stay where we are */
		case CSB_UPDATE_FAILED_ATTEMPT:
		{
#ifdef 	DEBUG_CONNECTBOX
			stateMsg(std::cerr, "STANDARD FAIL => switch to wait state", update);
#endif
			mNoFailedAttempts++;
			mAttemptLength = now - mStateTS;
			switch(mState)
			{
				case CSB_DIRECT_ATTEMPT:
					mState = CSB_DIRECT_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + RSRandom::random_u32() % DIRECT_MAX_WAIT_TIME;
					break;
				case CSB_PROXY_ATTEMPT:
					mState = CSB_PROXY_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + PROXY_BASE_WAIT_TIME + (RSRandom::random_u32() % (PROXY_MAX_WAIT_TIME - PROXY_BASE_WAIT_TIME));
					break;
				case CSB_RELAY_ATTEMPT:
					mState = CSB_RELAY_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + RSRandom::random_u32() % RELAY_MAX_WAIT_TIME;
					break;
				default:
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "STANDARD FAIL, but unusual state", update);
#endif
					break;
			}
					
			return 1;
		}
			break;

		/* finally MODE_UNAVAILABLE */
		case CSB_UPDATE_MODE_UNAVAILABLE:
		{
			mAttemptLength = now - mStateTS;
			if (mState == CSB_PROXY_ATTEMPT)
			{
				if (mNetState == CSB_NETSTATE_FORWARD)
				{
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "as FORWARDED => REVERSE_WAIT", update);
#endif
					mState = CSB_REVERSE_WAIT;
					mStateTS = now;
					mNextAttemptTS = now + REVERSE_WAIT_TIME;
				}
				else
				{
#ifdef 	DEBUG_CONNECTBOX
					stateMsg(std::cerr, "as !FORWARDED => RELAY_ATTEMPT", update);
#endif
					mState = CSB_RELAY_WAIT;
					mNoAttempts = 0;
					mStateTS = now;
					mNextAttemptTS = now + RSRandom::random_u32() % RELAY_MAX_WAIT_TIME;
				}
				return 1;
			}
			else
			{
#ifdef 	DEBUG_CONNECTBOX
				stateMsg(std::cerr, "MODE UNAVAIL => FAILED_WAIT", update);
#endif
				mState = CSB_FAILED_WAIT;
				mStateTS = now;
				mNextAttemptTS = now + FAILED_WAIT_TIME;

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
#ifdef 	DEBUG_CONNECTBOX
	rstime_t now = time(NULL);

	std::cerr << "PeerConnectStateBox::getProxyPortChoice() Using ConnectLogic Info from: ";
	std::cerr << now-mProxyPortTS << " ago. Flags: " << mProxyPortFlags;
	std::cerr << " UseProxyPort? " << mProxyPortChoice;
	std::cerr << std::endl;
#endif

	return mProxyPortChoice;
}



