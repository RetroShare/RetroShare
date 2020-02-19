/*******************************************************************************
 * libretroshare/src/dht: p3bitdht_peernet.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2011 by Robert Fernie <drbob@lunamutt.com>                   *
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

#include "dht/p3bitdht.h"

#include <stdio.h>
#include <iostream>


#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bitdht/bdmanager.h"

//#include "udp/udplayer.h"

#include "util/rsnet.h"
#include "tcponudp/tou.h"
#include "tcponudp/udpstunner.h"
#include "tcponudp/udprelay.h"

#include "pqi/p3netmgr.h"
#include "pqi/pqimonitor.h"

#define PEERNET_CONNECT_TIMEOUT 180   // Should be BIGGER than Higher level (but okay if not!)

#define MIN_DETERMINISTIC_SWITCH_PERIOD 	60

/***
 *
 * #define DEBUG_BITDHT_COMMON	1     // These are the things that are called regularly (annoying for debugging specifics)
 * #define DEBUG_PEERNET	1 
 *
 **/


/******************************************************************************************
 ************************************* Dht Callback ***************************************
 ******************************************************************************************/

/**** dht NodeCallback ****
 *
 *
 * In the old version, we used this to callback mConnCb->peerStatus()
 * We might want to drop this, and concentrate on the connection stuff.
 * 
 * -> if an new dht peer, then pass to Stunners.
 * -> do we care if we know them? not really!
 */

int p3BitDht::InfoCallback(const bdId *id, uint32_t /*type*/, uint32_t /*flags*/, std::string /*info*/)
{
	/* translate info */
	RsPeerId rsid;
	struct sockaddr_in addr = id->addr;
	int outtype = PNASS_TYPE_BADPEER;
	int outreason = PNASS_REASON_UNKNOWN;
	int outage = 0;

#ifdef DEBUG_BITDHT_COMMON
	std::cerr << "p3BitDht::InfoCallback() likely BAD_PEER: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

		DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(id->id), RsDhtPeerType::ANY);

		if (dpd)
		{
			rsid = dpd->mRsId;
		}
	}

	if (mPeerSharer)
	{
		struct sockaddr_storage tmpaddr;
		struct sockaddr_in *ap = (struct sockaddr_in *) &tmpaddr;
		ap->sin_family = AF_INET;
		ap->sin_addr = addr.sin_addr;
		ap->sin_port = addr.sin_port;
		
		mPeerSharer->updatePeer(rsid, tmpaddr, outtype, outreason, outage);
	}

#ifdef RS_USE_DHT_STUNNER
	/* call to the Stunners to drop the address as well */
	/* IDEALLY these addresses should all be filtered at UdpLayer level instead! */
	if (mDhtStunner)
	{
		mDhtStunner->dropStunPeer(addr);
	}
	if (mProxyStunner)
	{
		mProxyStunner->dropStunPeer(addr);
	}
#endif // RS_USE_DHT_STUNNER

	return 1;
}



/**** dht NodeCallback ****
 *
 *
 * In the old version, we used this to callback mConnCb->peerStatus()
 * We might want to drop this, and concentrate on the connection stuff.
 * 
 * -> if an new dht peer, then pass to Stunners.
 * -> do we care if we know them? not really!
 */

int p3BitDht::NodeCallback(const bdId *id, uint32_t peerflags)
{
	// No need for Mutex, and Stunners are self protected.

	// These are the flags that will be returned by p3BitDht & RS peers with the new DHT.
	// if (peerflags & BITDHT_PEER_STATUS_DHT_ENGINE_VERSION)  // Change to this later...
	if ((peerflags & BITDHT_PEER_STATUS_DHT_ENGINE) && 
		(peerflags & BITDHT_PEER_STATUS_DHT_APPL))
	{

		/* pass off to the Stunners
		 * but only if they need them.
		 * ideally don't pass to both peers... (XXX do later)
		 */

		{
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
			if (id->id == mOwnDhtId)
			{
#ifdef DEBUG_BITDHT_COMMON
				std::cerr << "p3BitDht::NodeCallback() Skipping own id";
				bdStdPrintId(std::cerr, id);
				std::cerr << std::endl;
#endif
				return 1;
			}
		}

#ifdef RS_USE_DHT_STUNNER
		if ((mProxyStunner) && (mProxyStunner->needStunPeers()))
		{
#ifdef DEBUG_BITDHT_COMMON
			std::cerr << "p3BitDht::NodeCallback() Passing BitDHT Peer to DhtStunner: ";
			bdStdPrintId(std::cerr, id);
			std::cerr << std::endl;
#endif
			mProxyStunner->addStunPeer(id->addr, NULL);
		}
		/* else */ // removed else until we have lots of peers.

		if ((mDhtStunner) && (mDhtStunner->needStunPeers()))
		{
#ifdef DEBUG_BITDHT_COMMON
			std::cerr << "p3BitDht::NodeCallback() Passing BitDHT Peer to DhtStunner: ";
			bdStdPrintId(std::cerr, id);
			std::cerr << std::endl;
#endif
			mDhtStunner->addStunPeer(id->addr, NULL);
		}
#endif // RS_USE_DHT_STUNNER
	}
	return 1;
}

/**** dht PeerCallback ****
 *
 *
 * In the old version, we used this to callback mConnCb->peerConnectRequest()
 * we need to continue doing this, to maintain compatibility.
 * 
 * -> update Dht Status, and trigger connect if ONLINE or UNREACHABLE
 */


int p3BitDht::PeerCallback(const bdId *id, uint32_t status)
{
	//std::string str;
	//bdStdPrintNodeId(str, &(id->id));

	//std::cerr << "p3BitDht::dhtPeerCallback()";
	//std::cerr << std::endl;

	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(id->id), RsDhtPeerType::FRIEND);

	if (!dpd)
	{
		/* ERROR */
		std::cerr << "p3BitDht::PeerCallback() ERROR Unknown Peer: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << " status: " << status;
		std::cerr << std::endl;

		return 0;
	}

	sockaddr_clear(&(dpd->mDhtId.addr)); 

	switch(status)
	{
		default:
			dpd->mDhtState = RSDHT_PEERDHT_NOT_ACTIVE;
			break;

		case BITDHT_MGR_QUERY_FAILURE:
			dpd->mDhtState = RSDHT_PEERDHT_FAILURE;
			break;

		case BITDHT_MGR_QUERY_PEER_OFFLINE:
			dpd->mDhtState = RSDHT_PEERDHT_OFFLINE;
			break;

		case BITDHT_MGR_QUERY_PEER_UNREACHABLE:
			dpd->mDhtState = RSDHT_PEERDHT_UNREACHABLE;
			dpd->mDhtId = *id; // set the IP:Port of the unreachable peer.
			UnreachablePeerCallback_locked(id, status, dpd);

			break;

		case BITDHT_MGR_QUERY_PEER_ONLINE:
			dpd->mDhtState = RSDHT_PEERDHT_ONLINE;
			dpd->mDhtId = *id; // set the IP:Port of the Online peer.
			OnlinePeerCallback_locked(id, status, dpd);

			break;

	}

	rstime_t now = time(NULL);
	dpd->mDhtUpdateTS = now;

	return 1;
}



int p3BitDht::OnlinePeerCallback_locked(const bdId *id, uint32_t /*status*/, DhtPeerDetails *dpd)
{
	/* remove unused parameter warnings */
	(void) id;

	if ((dpd->mPeerConnectState != RSDHT_PEERCONN_DISCONNECTED) ||
			(dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING))
	{

#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::OnlinePeerCallback_locked() WARNING Ignoring Callback: connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif

		return 1;
	}

	bool connectOk = true;
	bool doTCPCallback = false;

	/* work out network state */
	uint32_t connectFlags = dpd->mConnectLogic.connectCb(CSB_CONNECT_DIRECT,
					mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode());
//	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

	switch(connectFlags & CSB_ACTION_MASK_MODE)
	{
		default:
		case CSB_ACTION_WAIT:
		{
			connectOk = false;
		}
			break;
		case CSB_ACTION_TCP_CONN:
		{
			connectOk = false;
			doTCPCallback = true;
		}
			break;
		case CSB_ACTION_DIRECT_CONN:
		{

			connectOk = true;
		}
			break;
		case CSB_ACTION_PROXY_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned PROXY";
			std::cerr << std::endl;
			connectOk = false;
		}
			break;
		case CSB_ACTION_RELAY_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned RELAY";
			std::cerr << std::endl;
			connectOk = false;
		}
			break;
	}

	if (connectOk)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "dhtPeerCallback. Peer Online, triggering Direct Connection for: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif

		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_CONNECT;
		ca.mMode = BITDHT_CONNECT_MODE_DIRECT;
		ca.mDestId = *id;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;

		mActions.push_back(ca);
	}

	/* might need to make this an ACTION - leave for now */
	if (doTCPCallback)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "dhtPeerCallback. Peer Online, triggering TCP Connection for: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif

		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_TCPATTEMPT;
		ca.mMode = BITDHT_CONNECT_MODE_DIRECT;
		ca.mDestId = *id;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;

		mActions.push_back(ca);
		//ConnectCalloutTCPAttempt(dpd->mRsId, id->addr);
	}

	return 1;
}



int p3BitDht::UnreachablePeerCallback_locked(const bdId *id, uint32_t /*status*/, DhtPeerDetails *dpd)
{

	if ((dpd->mPeerConnectState != RSDHT_PEERCONN_DISCONNECTED) ||
			(dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING))
	{

#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::UnreachablePeerCallback_locked() WARNING Ignoring Callback: connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif

		return 1;
	}

#ifdef DEBUG_PEERNET
	std::cerr << "dhtPeerCallback. Peer Unreachable, triggering Proxy | Relay Connection for: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	bool proxyOk = false;
	bool connectOk = true;

	/* work out network state */
	uint32_t connectFlags = dpd->mConnectLogic.connectCb(CSB_CONNECT_UNREACHABLE,
					mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode());
	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

	switch(connectFlags & CSB_ACTION_MASK_MODE)
	{
		default:
		case CSB_ACTION_WAIT:
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtPeerCallback. Request to Wait ... so no Connection Attempt for ";
			bdStdPrintId(std::cerr, id);
			std::cerr << std::endl;
#endif
			connectOk = false;
		}
			break;
		case CSB_ACTION_DIRECT_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned DIRECT";
			std::cerr << std::endl;

			connectOk = false;
		}
			break;
		case CSB_ACTION_PROXY_CONN:
		{
			proxyOk = true;
			connectOk = true;
		}
			break;
		case CSB_ACTION_RELAY_CONN:
		{
			proxyOk = false;
			connectOk = true;
		}
			break;
	}

	if (connectOk)
	{
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_CONNECT;
		ca.mDestId = *id;

		dpd->mConnectLogic.storeProxyPortChoice(connectFlags, useProxyPort);

		if (proxyOk)
		{
			ca.mMode = BITDHT_CONNECT_MODE_PROXY;
#ifdef DEBUG_PEERNET
			std::cerr << "dhtPeerCallback. Trying Proxy Connection.";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtPeerCallback. Trying Relay Connection.";
			std::cerr << std::endl;
#endif
			ca.mMode = BITDHT_CONNECT_MODE_RELAY;
		}

		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		mActions.push_back(ca);
	}
	else
	{
#ifdef DEBUG_PEERNET
		std::cerr << "dhtPeerCallback. Cancelled Connection Attempt for ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif
	}

	return 1;
}



int p3BitDht::ValueCallback(const bdNodeId */*id*/, std::string /*key*/, uint32_t /*status*/)
{
	std::cerr << "p3BitDht::ValueCallback() ERROR Does nothing!";
	std::cerr << std::endl;

	return 1;
}

int p3BitDht::ConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
						uint32_t mode, uint32_t point, uint32_t param, uint32_t cbtype, uint32_t errcode)
{
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::ConnectCallback()";
	std::cerr << std::endl;
	std::cerr << "srcId: ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << std::endl;
	std::cerr << "proxyId: ";
	bdStdPrintId(std::cerr, proxyId);
	std::cerr << std::endl;
	std::cerr << "destId: ";
	bdStdPrintId(std::cerr, destId);
	std::cerr << std::endl;

	std::cerr << " mode: " << mode;
	std::cerr << " param: " << param;
	std::cerr << " point: " << point;
	std::cerr << " cbtype: " << cbtype;
	std::cerr << std::endl;
#endif


	/* we handle MID and START/END points differently... this is biggest difference.
	 * so handle first.
	 */

	bdId peerId;
	rstime_t now = time(NULL);

	switch(point)
	{
		default:
		case BD_PROXY_CONNECTION_UNKNOWN_POINT:
		{
			std::cerr << "p3BitDht::dhtConnectCallback() ERROR UNKNOWN point, ignoring Callback";
			std::cerr << std::endl;
			return 0;
		}
		case BD_PROXY_CONNECTION_START_POINT:
			peerId = *destId;
			break;
		case BD_PROXY_CONNECTION_END_POINT:
			peerId = *srcId;
			break;
		case BD_PROXY_CONNECTION_MID_POINT:
		{
			/* AS a mid point, we can receive.... AUTH,PENDING,PROXY,FAILED */

			switch(cbtype)
			{
				case BITDHT_CONNECT_CB_AUTH:
				{
#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback() Proxy Connection Requested Between:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
#endif
					/* if there is an error code - then it is just to inform us of a failed attempt */
					if (errcode)
					{
						RelayHandler_LogFailedProxyAttempt(srcId, destId, mode, errcode);
						/* END MID FAILED ATTEMPT */
						return 1;
					}
					
					uint32_t bandwidth = 0;

					int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
					if (checkProxyAllowed(srcId, destId, mode, bandwidth))
					{
						connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
#ifdef DEBUG_PEERNET
						std::cerr << "dhtConnectionCallback() Connection Allowed";
						std::cerr << std::endl;
#endif
					}
					else
					{
						connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
#ifdef DEBUG_PEERNET
						std::cerr << "dhtConnectionCallback() Connection Denied";
						std::cerr << std::endl;
#endif
					}
		
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					/* Push Back PeerAction */
					PeerAction ca;
					ca.mType = PEERNET_ACTION_TYPE_AUTHORISE;
					ca.mMode = mode;
					ca.mProxyId = *proxyId;
					ca.mSrcId = *srcId;
					ca.mDestId = *destId;
					ca.mPoint = point;
					ca.mAnswer = connectionAllowed;
					ca.mDelayOrBandwidth = bandwidth;
		
					mActions.push_back(ca);
				}
				break;
				case BITDHT_CONNECT_CB_PENDING:
				{
#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback() Proxy Connection Pending:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
#endif
				}
				break;
				case BITDHT_CONNECT_CB_PROXY:
				{
#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback() Proxy Connection Starting:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
#endif
				}
				break;
				case BITDHT_CONNECT_CB_FAILED:
				{
#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback() Proxy Connection Failed:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					
					std::cerr << " ErrorCode: " << errcode;
					int errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;
					int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
					
					std::cerr << " ErrorSrc: " << errsrc;
					std::cerr << " ErrorType: " << errtype;
					
					std::cerr << std::endl;
#endif

					if (mode == BITDHT_CONNECT_MODE_RELAY)
					{
						removeRelayConnection(srcId, destId);
					}
				}
				break;
				default:
				case BITDHT_CONNECT_CB_START:
				{
					std::cerr << "dhtConnectionCallback() ERROR unexpected Proxy ConnectionCallback:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
				}
				break;
			}
			/* End the MID Point stuff */
			return 1;
		}
	}

	/* if we get here, we are an endpoint (peer specified in peerId) */
	
	/* translate id into string for exclusive mode */
	std::string pid;
	bdStdPrintNodeId(pid, &(peerId.id), false);
	
	switch(cbtype)
	{
		case BITDHT_CONNECT_CB_AUTH:
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtConnectionCallback() Connection Requested By: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
#endif

			int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
			if (checkConnectionAllowed(&(peerId), mode))
			{
				connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
#ifdef DEBUG_PEERNET
				std::cerr << "dhtConnectionCallback() Connection Allowed";
				std::cerr << std::endl;
#endif
			}
			else
			{
				connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
#ifdef DEBUG_PEERNET
				std::cerr << "dhtConnectionCallback() Connection Denied";
				std::cerr << std::endl;
#endif
			}
	
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_AUTHORISE;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
			ca.mDelayOrBandwidth = 0;
	
			/* Check Proxy ExtAddress Status (but only if connection is Allowed) */	
			if ((connectionAllowed == BITDHT_CONNECT_ANSWER_OKAY) &&
			 		 (mode == BITDHT_CONNECT_MODE_PROXY))
			{
#ifdef DEBUG_PEERNET
				std::cerr << "dhtConnectionCallback() Checking Address for Proxy";
				std::cerr << std::endl;
#endif

				struct sockaddr_in extaddr;
				sockaddr_clear(&extaddr);

				bool connectOk = false;
				bool proxyPort = false; 
				bool exclusivePort = false;
#ifdef DEBUG_PEERNET
				std::cerr << "dhtConnectionCallback():  Proxy... deciding which port to use.";
				std::cerr << std::endl;
#endif
	
				DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RsDhtPeerType::FRIEND);
				if (dpd)
				{
					proxyPort = dpd->mConnectLogic.shouldUseProxyPort(
						mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode());

					dpd->mConnectLogic.storeProxyPortChoice(0, proxyPort);

#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback: Setting ProxyPort: ";
					std::cerr << " UseProxyPort? " << proxyPort;
					std::cerr << std::endl;

					std::cerr << "dhtConnectionCallback: Checking ConnectLogic.NetState: ";
					std::cerr << dpd->mConnectLogic.calcNetState(mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode());
					std::cerr << std::endl;
#endif

					if (proxyPort)
					{
						exclusivePort = (CSB_NETSTATE_EXCLUSIVENAT == dpd->mConnectLogic.calcNetState(
									mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode()));
					}

#ifdef DEBUG_PEERNET
					if (exclusivePort) std::cerr << "dhtConnectionCallback: we Require Exclusive Proxy Port for connection" << std::endl;
					else std::cerr << "dhtConnectionCallback: Dont need Exclusive Proxy Port for connection" << std::endl;
#endif

					connectOk = true;
				}
				else
				{
					std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
					std::cerr << std::endl;
				}

#ifdef RS_USE_DHT_STUNNER
				uint8_t extStable = 0;

				UdpStunner *stunner = mProxyStunner;
				if (!proxyPort)
				{
					stunner = mDhtStunner;
				}
				
				if ((connectOk) && (stunner) && (stunner->externalAddr(extaddr, extStable)))
				{
					if (extStable)
					{
#ifdef DEBUG_PEERNET
						std::cerr << "dhtConnectionCallback() Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(peerId));
						std::cerr << " is Ok as we have Stable Own External Proxy Address";
						std::cerr << std::endl;
#endif

						if (point == BD_PROXY_CONNECTION_END_POINT)
						{
							ca.mDestId.addr = extaddr;
						}
						else
						{
							ca.mSrcId.addr = extaddr;
							std::cerr << "dhtConnectionCallback() ERROR Proxy Auth as SrcId";
							std::cerr << std::endl;
						}

						/* check if we require exclusive use of the proxy port */
						if (exclusivePort)
						{
#ifdef DEBUG_PEERNET
							std::cerr << "dhtConnectionCallback: Attempting to Grab ExclusiveLock of UdpStunner";
							std::cerr << std::endl;
#endif
							
							int stun_age = mProxyStunner->grabExclusiveMode(pid);
							if (stun_age > 0)
							{
								int delay = 0;
								if (stun_age < MIN_DETERMINISTIC_SWITCH_PERIOD)
								{
									delay = MIN_DETERMINISTIC_SWITCH_PERIOD - stun_age;
								}

								/* great we got it! */
								ca.mDelayOrBandwidth = delay;

#ifdef DEBUG_PEERNET
								std::cerr << "dhtConnectionCallback: GotExclusiveLock With Delay: " << delay;
								std::cerr << " for stable port";
								std::cerr << std::endl;
#endif

								DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RsDhtPeerType::FRIEND);
								if (dpd)
								{
									dpd->mExclusiveProxyLock = true;

#ifdef DEBUG_PEERNET
									std::cerr << "dhtConnectionCallback: Success at grabbing ExclusiveLock of UdpStunner";
									std::cerr << std::endl;
#endif

								}
								else
								{
									std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
									std::cerr << std::endl;
									connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
									mProxyStunner->releaseExclusiveMode(pid,false);
								}
							}
							else
							{
								/* failed to get exclusive mode - must wait */
								connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;

#ifdef DEBUG_PEERNET
								std::cerr << "dhtConnectionCallback: Failed to Grab ExclusiveLock, Returning TEMPUNAVAIL";
								std::cerr << std::endl;
#endif
							}
						}

						
					}
					else
					{
						if (exclusivePort)
						{
							connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
#ifdef DEBUG_PEERNET
							std::cerr << "dhtConnectionCallback() Proxy Connection";
							std::cerr << " is Discarded, as External Proxy Address is Not Stable! (EXCLUSIVE MODE)";
							std::cerr << std::endl;
#endif
						}
						else
						{
							connectionAllowed = BITDHT_CONNECT_ERROR_UNREACHABLE;
#ifdef DEBUG_PEERNET
							std::cerr << "dhtConnectionCallback() Proxy Connection";
							std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
							std::cerr << std::endl;
#endif
						}
					}
				}
				else
				{
					connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
#ifdef DEBUG_PEERNET
					std::cerr << "dhtConnectionCallback() ERROR Proxy Connection ";
					std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
					std::cerr << std::endl;
#endif
				}
#else // RS_USE_DHT_STUNNER
				connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
				(void) connectOk;
				(void) exclusivePort;
#endif // RS_USE_DHT_STUNNER
			}

			ca.mMode = mode;
			ca.mPoint = point;
			ca.mAnswer = connectionAllowed;

			mActions.push_back(ca);
		}
		break;

		case BITDHT_CONNECT_CB_START:
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtConnectionCallback() Connection Starting with: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
#endif

			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_START;
			ca.mMode = mode;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
			ca.mPoint = point;
			ca.mDelayOrBandwidth = param;
			ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
			mActions.push_back(ca);

		}
		break;

		case BITDHT_CONNECT_CB_FAILED:
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtConnectionCallback() Connection Attempt Failed with:";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
			
			std::cerr << "dhtConnectionCallback() Proxy:";			
			bdStdPrintId(std::cerr, proxyId);
			std::cerr << std::endl;
				
			std::cerr << "dhtConnectionCallback() ";			
			std::cerr << " ErrorCode: " << errcode;
			int errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;
			int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
				
			std::cerr << " ErrorSrc: " << errsrc;
			std::cerr << " ErrorType: " << errtype;
			std::cerr << std::endl;
#endif
			
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RsDhtPeerType::FRIEND);
			if (dpd)
			{
				dpd->mPeerCbMsg = "ERROR : ";
				dpd->mPeerCbMsg += decodeConnectionError(errcode);
				dpd->mPeerCbMode = mode;
				dpd->mPeerCbPoint = point;
				dpd->mPeerCbProxyId = *proxyId;
				dpd->mPeerCbDestId = peerId;
				dpd->mPeerCbTS = now;
			}
			else
			{
				std::cerr << "dhtConnectionCallback() ";			
				std::cerr << "ERROR Unknown Peer";
				std::cerr << std::endl;
			}
		}
		break;

		case BITDHT_CONNECT_CB_REQUEST: 
		{
#ifdef DEBUG_PEERNET
			std::cerr << "dhtConnectionCallback() Local Connection Request Feedback:";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
			
			std::cerr << "dhtConnectionCallback() Proxy:";			
			bdStdPrintId(std::cerr, proxyId);
			std::cerr << std::endl;
#endif

			if (point != BD_PROXY_CONNECTION_START_POINT)
			{
				std::cerr << "dhtConnectionCallback() ERROR not START_POINT";
				std::cerr << std::endl;
				return 0;
			}

			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RsDhtPeerType::FRIEND);
			if (dpd)
			{
				if (errcode)
				{
					ReleaseProxyExclusiveMode_locked(dpd, false);

					dpd->mPeerReqStatusMsg = "STOPPED: ";
					dpd->mPeerReqStatusMsg += decodeConnectionError(errcode);
					dpd->mPeerReqState = RSDHT_PEERREQ_STOPPED;
					dpd->mPeerReqTS = now;

					int updatecode = CSB_UPDATE_FAILED_ATTEMPT;
					int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
					switch(errtype)
					{
						/* fatal errors */
						case BITDHT_CONNECT_ERROR_AUTH_DENIED:
						{
							updatecode = CSB_UPDATE_AUTH_DENIED;
						}
							break;

						/* move on errors */
						case BITDHT_CONNECT_ERROR_UNREACHABLE: // sym NAT.
						case BITDHT_CONNECT_ERROR_UNSUPPORTED: // mode unavailable.
						{
							updatecode = CSB_UPDATE_MODE_UNAVAILABLE;
						}
							break;


						/* retry attempts */
						case BITDHT_CONNECT_ERROR_TEMPUNAVAIL:
						case BITDHT_CONNECT_ERROR_DUPLICATE:
						{
							updatecode = CSB_UPDATE_RETRY_ATTEMPT;
						}
							break;
						/* standard failed attempts */
						case BITDHT_CONNECT_ERROR_GENERIC:
						case BITDHT_CONNECT_ERROR_PROTOCOL:
						case BITDHT_CONNECT_ERROR_TIMEOUT:
						case BITDHT_CONNECT_ERROR_NOADDRESS:
						case BITDHT_CONNECT_ERROR_OVERLOADED:
						/* CB_REQUEST errors */
						case BITDHT_CONNECT_ERROR_TOOMANYRETRY:
						case BITDHT_CONNECT_ERROR_OUTOFPROXY:
						{
							updatecode = CSB_UPDATE_FAILED_ATTEMPT;
						}
							break;

						/* user cancelled ... no update */
						case BITDHT_CONNECT_ERROR_USER:
						{
							updatecode = CSB_UPDATE_NONE;
						}
							break;
					}
					if (updatecode)
					{
						dpd->mConnectLogic.updateCb(updatecode);
					}
				}
				else // a new connection attempt.
				{
					dpd->mPeerReqStatusMsg = "Connect Attempt";
					dpd->mPeerReqState = RSDHT_PEERREQ_RUNNING;
					dpd->mPeerReqMode = mode;
					dpd->mPeerReqProxyId = *proxyId;
					dpd->mPeerReqTS = now;

					// This also is flagged into the instant Cb info.
					dpd->mPeerCbMsg = "Local Connect Attempt";
					dpd->mPeerCbMode = mode;
					dpd->mPeerCbPoint = point;
					dpd->mPeerCbProxyId = *proxyId;
					dpd->mPeerCbDestId = peerId;
					dpd->mPeerCbTS = now;
				}
			}
			else
			{
				std::cerr << "dhtConnectionCallback() ERROR Cannot find PeerStatus";
				std::cerr << std::endl;
			}
		}
		break;

		default:
		case BITDHT_CONNECT_CB_PENDING:
		case BITDHT_CONNECT_CB_PROXY:
		{
			std::cerr << "dhtConnectionCallback() ERROR unexpected ConnectionCallback:";
			std::cerr << std::endl;
			bdStdPrintId(std::cerr, srcId);
			std::cerr << " and ";
			bdStdPrintId(std::cerr, destId);
			std::cerr << std::endl;
		}
		break;
	}
	return 1;
}

/******************************************************************************************
 ******************************** Dht Actions / Monitoring ********************************
 ******************************************************************************************/


/* tick stuff that isn't in its own thread */

int p3BitDht::tick()
{
	doActions();
	monitorConnections();

	minuteTick();

#ifdef DEBUG_PEERNET_COMMON
	time_t now = time(NULL);  // Don't use rstime_t here or ctime break on windows
	std::cerr << "p3BitDht::tick() TIME: " << ctime(&now) << std::endl;
	std::cerr.flush();
#endif

	return 1;
}

#define MINUTE_IN_SECS	60
#define TEN_IN_SECS	10

int p3BitDht::minuteTick()
{
	rstime_t now = time(NULL);
	int deltaT = 0;
	
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/
		deltaT = now-mMinuteTS;
	}
	
	//if (deltaT > MINUTE_IN_SECS)
	if (deltaT > TEN_IN_SECS)
	{
		mRelay->checkRelays();

		updateDataRates();

		/* temp - testing - print dht & relay traffic */
		float dhtRead, dhtWrite;
		float relayRead, relayWrite, relayRelayed;

		getRelayRates(relayRead, relayWrite, relayRelayed);
		getDhtRates(dhtRead, dhtWrite);
		
#ifdef DEBUG_PEERNET_COMMON
		double denom = deltaT;

		std::cerr << "p3BitDht::minuteTick() ";
		std::cerr << "DhtRead: " << dhtRead / denom << " kB/s ";
		std::cerr << "DhtWrite: " << dhtWrite / denom << " kB/s ";
		std::cerr << std::endl;

		std::cerr << "p3BitDht::minuteTick() ";
		std::cerr << "RelayRead: " << relayRead / denom << " kB/s ";
		std::cerr << "RelayWrite: " << relayWrite / denom << " kB/s ";
		std::cerr << "RelayRelayed: " << relayRelayed / denom << " kB/s ";
		std::cerr << std::endl;
#endif


		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/
		mMinuteTS = now;
	}
	return 1;
}

int p3BitDht::doActions()
{
#ifdef DEBUG_PEERNET_COMMON
	std::cerr << "p3BitDht::doActions()" << std::endl;
#endif

	rstime_t now = time(NULL);

	while(mActions.size() > 0)
	{
		PeerAction action;

		{
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			if (mActions.size() < 1)
			{
				break;
			}

			action = mActions.front();
			mActions.pop_front();
		}

		switch(action.mType)
		{
			case PEERNET_ACTION_TYPE_CONNECT:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;
#endif

				bool doConnectionRequest = false;
				bool connectionReqFailed = false;
				bool grabbedExclusivePort = false;

				/* translate id into string for exclusive mode */
				std::string pid;
				bdStdPrintNodeId(pid, &(action.mDestId.id), false);
				
				
				// Parameters that will be used for the Connect Request.
				struct sockaddr_in connAddr; // We zero this address. (DHT Layer handles most cases)
				sockaddr_clear(&connAddr);
				uint32_t connStart = 1; /* > 0 indicates GO (number indicates required startup delay) */
				uint32_t connDelay = 0;

				uint32_t failReason = CSB_UPDATE_MODE_UNAVAILABLE;

				if ((action.mMode == BITDHT_CONNECT_MODE_DIRECT) ||
						(action.mMode == BITDHT_CONNECT_MODE_RELAY))
				{
					doConnectionRequest = true;
				}
				else if (action.mMode == BITDHT_CONNECT_MODE_PROXY)
				{
					struct sockaddr_in extaddr;
					sockaddr_clear(&extaddr);
					bool proxyPort = true;
					bool exclusivePort = false;
					bool connectOk = false;

#ifdef DEBUG_PEERNET
					std::cerr << "PeerAction:  Proxy... deciding which port to use.";
					std::cerr << std::endl;
#endif

					{
						RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	
						DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RsDhtPeerType::FRIEND);
						if (dpd)
						{
							connectOk = true;
							proxyPort = dpd->mConnectLogic.getProxyPortChoice();
							if (proxyPort)
							{
#ifdef DEBUG_PEERNET
								std::cerr << "PeerAction:  Using ProxyPort. Checking ConnectLogic.NetState: ";
								std::cerr << dpd->mConnectLogic.calcNetState(mNetMgr->getNetworkMode(), 
												mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode());
								std::cerr << std::endl;
#endif

								exclusivePort = (CSB_NETSTATE_EXCLUSIVENAT == dpd->mConnectLogic.calcNetState(
									mNetMgr->getNetworkMode(), mNetMgr->getNatHoleMode(), mNetMgr->getNatTypeMode()));

								if (exclusivePort)
								{
#ifdef DEBUG_PEERNET
									std::cerr << "PeerAction: NetState indicates Exclusive Mode";
									std::cerr << std::endl;
#endif
								}
							}
						}
						else
						{
							std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
							std::cerr << std::endl;
						}
					}
#ifdef RS_USE_DHT_STUNNER
					uint8_t extStable = 0;

					UdpStunner *stunner = mProxyStunner;
					if (!proxyPort)
					{
						stunner = mDhtStunner;
					}
					

					
					if ((connectOk) && (stunner) && (stunner->externalAddr(extaddr, extStable)))
					{
						if (extStable)
						{
#ifdef DEBUG_PEERNET
							std::cerr << "PeerAction: Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is OkGo as we have Stable Own External Proxy Address";
							std::cerr << std::endl;
#endif
						
							/* check if we require exclusive use of the proxy port */
							if (exclusivePort)
							{
								int stun_age = mProxyStunner->grabExclusiveMode(pid);
								if (stun_age > 0)
								{
									int delay = 0;
									if (stun_age < MIN_DETERMINISTIC_SWITCH_PERIOD)
									{
										delay = MIN_DETERMINISTIC_SWITCH_PERIOD - stun_age;
									}

#ifdef DEBUG_PEERNET
									std::cerr << "PeerAction: Stunner has indicated a Delay of " << delay;
									std::cerr << " to ensure a stable Port!";
									std::cerr << std::endl;
#endif
									
									/* great we got it! */
									connAddr = extaddr;
									connDelay = delay;
									doConnectionRequest = true;
									grabbedExclusivePort = true;
								}
								else
								{
									/* failed to get exclusive mode - must wait */
									connectionReqFailed = true;
									failReason = CSB_UPDATE_RETRY_ATTEMPT;
								}
							}
							else
							{
								/* stable and non-exclusive - go for it */
								connAddr = extaddr;
								connStart = 1;
								doConnectionRequest = true;
							}
						}
						else
						{
#ifdef DEBUG_PEERNET
							std::cerr << "PeerAction: WARNING Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
							std::cerr << std::endl;
#endif

							connectionReqFailed = true;
							if (exclusivePort)
							{
								failReason = CSB_UPDATE_RETRY_ATTEMPT;
#ifdef DEBUG_PEERNET
								std::cerr << "PeerAction: As Exclusive Mode, Port Will stabilise => RETRY";
								std::cerr << std::endl;
#endif

							}
							else
							{
								failReason = CSB_UPDATE_MODE_UNAVAILABLE;
#ifdef DEBUG_PEERNET
								std::cerr << "PeerAction: Not Exclusive Mode, => MODE UNAVAILABLE";
								std::cerr << std::endl;
#endif
							}
						}
					}
					else
					{
#ifdef DEBUG_PEERNET
						std::cerr << "PeerAction: ERROR Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(action.mDestId));
						std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
						std::cerr << std::endl;
#endif

						connectionReqFailed = true;
						failReason = CSB_UPDATE_RETRY_ATTEMPT;
					}
#else // RS_USE_DHT_STUNNER
					connectionReqFailed = true;
					failReason = CSB_UPDATE_RETRY_ATTEMPT;
					(void) connectOk;
#endif //RS_USE_DHT_STUNNER
				}

				if (doConnectionRequest)
				{
					if (mUdpBitDht->ConnectionRequest(&connAddr, &(action.mDestId.id), action.mMode, connDelay, connStart))
					{
						RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	
#ifdef DEBUG_PEERNET
						std::cerr << "PeerAction: Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(action.mDestId));
						std::cerr << " has gone ahead";
						std::cerr << std::endl;
#endif
	
	
						DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RsDhtPeerType::FRIEND);
						if (dpd)
						{
							dpd->mPeerReqStatusMsg = "Connect Request";
							dpd->mPeerReqState = RSDHT_PEERREQ_RUNNING;
							dpd->mPeerReqMode = action.mMode;
							dpd->mPeerReqTS = now;

							if (grabbedExclusivePort)
							{
								dpd->mExclusiveProxyLock = true;
							}
						}
						else
						{
							std::cerr << "PeerAction: Connect ERROR Cannot find PeerStatus";
							std::cerr << std::endl;
						}
					}
					else
					{
						connectionReqFailed = true;
						failReason = CSB_UPDATE_MODE_UNAVAILABLE;
					}
				}

				if (connectionReqFailed)
				{
#ifdef DEBUG_PEERNET
					std::cerr << "PeerAction: Connection Attempt to: ";
					bdStdPrintId(std::cerr, &(action.mDestId));
					std::cerr << " is Discarded, as Mode is Unavailable";
					std::cerr << std::endl;
#endif

					//if (grabbedExclusivePort)
					//{
					//	mProxyStunner->releaseExclusiveMode(pid,false);
					//}

					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RsDhtPeerType::FRIEND);
					if (dpd)
					{
						dpd->mConnectLogic.updateCb(failReason);

						dpd->mPeerReqStatusMsg = "Req Mode Unavailable";
						dpd->mPeerReqState = RSDHT_PEERREQ_STOPPED;
						dpd->mPeerReqMode = action.mMode;
						dpd->mPeerReqTS = now;

						if (grabbedExclusivePort)
						{
							ReleaseProxyExclusiveMode_locked(dpd, false);
						}
					}
					else
					{
						std::cerr << "PeerAction: ERROR Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(action.mDestId));
						std::cerr << " has no Internal Dht Peer!";
						std::cerr << std::endl;

						if (grabbedExclusivePort)
						{
							std::cerr << "PeerAction: ERROR ERROR, we grabd Exclusive Port to do this, trying emergency release";
							std::cerr << std::endl;
#ifdef RS_USE_DHT_STUNNER
							mProxyStunner->releaseExclusiveMode(pid,false);
#endif // RS_USE_DHT_STUNNER
						}
					}
				}

			}
			break;

			case PEERNET_ACTION_TYPE_AUTHORISE:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. Authorise Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << " delay/bandwidth: " << action.mDelayOrBandwidth;
				std::cerr << std::endl;
#endif

				int delay = 0;
				int bandwidth = 0;
				if (action.mMode == BITDHT_CONNECT_MODE_RELAY)
				{
					bandwidth = action.mDelayOrBandwidth;
				}
				else 
				{
					delay = action.mDelayOrBandwidth;
				}

				mUdpBitDht->ConnectionAuth(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, bandwidth, delay, action.mAnswer);

				// Only feedback to the gui if we are at END.
				if (action.mPoint == BD_PROXY_CONNECTION_END_POINT)
				{
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mSrcId.id), RsDhtPeerType::ANY);
					if (dpd)
					{	
						if (action.mAnswer)
						{
							dpd->mPeerCbMsg = "WE DENIED AUTH: ERROR : ";
							dpd->mPeerCbMsg += decodeConnectionError(action.mAnswer);
						}
						else
						{
							dpd->mPeerCbMsg = "We AUTHED";
						}
						dpd->mPeerCbMode = action.mMode;
						dpd->mPeerCbPoint = action.mPoint;
						dpd->mPeerCbProxyId = action.mProxyId;
						dpd->mPeerCbDestId = action.mSrcId;
						dpd->mPeerCbTS = now;
					}
					// Not an error if AUTH_DENIED - cos we don't know them! (so won't be in peerList).
					else if (action.mAnswer | BITDHT_CONNECT_ERROR_AUTH_DENIED)
					{
#ifdef DEBUG_PEERNET
						std::cerr << "PeerAction Authorise Connection ";			
						std::cerr << "Denied Unknown Peer";
						std::cerr << std::endl;
#endif
					}
					else 
					{
						std::cerr << "PeerAction Authorise Connection ";			
						std::cerr << "ERROR Unknown Peer & !DENIED ???";
						std::cerr << std::endl;
					}
				}
			}
			break;

			case PEERNET_ACTION_TYPE_START:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. Start Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << " delay/bandwidth: " << action.mDelayOrBandwidth;
				std::cerr << std::endl;
#endif

				initiateConnection(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, action.mDelayOrBandwidth);
			}
			break;

			case PEERNET_ACTION_TYPE_RESTARTREQ:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. Restart Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;
#endif

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 1;
				uint32_t delay = 0;

				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, delay, start);
			}
			break;

			case PEERNET_ACTION_TYPE_KILLREQ:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. Kill Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;
#endif

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 0;
				uint32_t delay = 0;
				
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, delay, start);
			}
			break;

			case PEERNET_ACTION_TYPE_TCPATTEMPT:
			{
				/* connect attempt */
#ifdef DEBUG_PEERNET
				std::cerr << "PeerAction. TCP Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;
#endif

				RsPeerId peerRsId;
				bool foundPeerId = false;
				{
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RsDhtPeerType::ANY);
					if (dpd)
					{
						peerRsId = dpd->mRsId;
						foundPeerId = true;
#ifdef DEBUG_PEERNET
						std::cerr << "PeerAction. TCP Connection Attempt. DoingCallback for RsID: ";
						std::cerr << peerRsId;
						std::cerr << std::endl;
#endif
					}
					else 
					{
						std::cerr << "PeerAction. TCP Connection Attempt. ERROR unknown peer";
						std::cerr << std::endl;
					}
				}

				if (foundPeerId)
				{
					ConnectCalloutTCPAttempt(peerRsId, action.mDestId.addr);
				}
			}
			break;
		}
	}
	return 1;
}





/****************************** Connection Logic ***************************/

/* Proxies!.
 *
 * We can allow all PROXY connections, as it is just a couple of messages,
 * however, we should be smart about how many RELAY connections are allowed.
 * 
 * e.g. Allow 20 Relay connections.
 * 15 for friends, 5 for randoms.
 *
 * Can also validate addresses with own secure connections.
 */

int p3BitDht::checkProxyAllowed(const bdId *srcId, const bdId *destId, int mode, uint32_t &bandwidth)
{
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::checkProxyAllowed()";
	std::cerr << std::endl;
#endif

	// Dont think that a mutex is required here! But might be so just lock to ensure that it is possible.
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	}

	if (mode == BITDHT_CONNECT_MODE_PROXY) 
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::checkProxyAllowed() Allowing all PROXY connections, OKAY";
		std::cerr << std::endl;
#endif

		bandwidth = 0; // unlimited as p2p.
		return 1;
		//return CONNECTION_OKAY;
	}

	if (mode != BITDHT_CONNECT_MODE_RELAY)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::checkProxyAllowed() unknown Connect Mode DENIED";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* will install the Relay Here... so that we reserve the Relay Space for later. 
	 * decide on relay bandwidth limitation as well
	 */
	if (RelayHandler_InstallRelayConnection(srcId, destId, mode, bandwidth))
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::checkProxyAllowed() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;
#endif

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::checkProxyAllowed() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;
#endif

		return 0;
		//return CONNECT_MODE_OVERLOADED;
	}

	return 0;
	//return CONNECT_MODE_NOTAVAILABLE;
	//return CONNECT_MODE_OVERLOADED;
}


int p3BitDht::checkConnectionAllowed(const bdId *peerId, int mode)
{
	(void) mode;

#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::checkConnectionAllowed() to: ";
	bdStdPrintId(std::cerr,peerId);
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;
#endif

	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	rstime_t now = time(NULL);

	/* check if they are in our friend list */
	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId->id), RsDhtPeerType::FRIEND);
	if (!dpd)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::checkConnectionAllowed() Peer Not Friend, DENIED";
		std::cerr << std::endl;
#endif

		/* store as failed connection attempt */
		std::map<bdNodeId, DhtPeerDetails>::iterator it;
		it = mFailedPeers.find(peerId->id);
		if (it == mFailedPeers.end())
		{
			mFailedPeers[peerId->id] = DhtPeerDetails();
			it = mFailedPeers.find(peerId->id);
		}

		/* flag as failed */
		it->second.mDhtId = *peerId;
		it->second.mDhtState = RSDHT_PEERDHT_NOT_ACTIVE;
		it->second.mDhtUpdateTS = now;

		it->second.mPeerType = RsDhtPeerType::OTHER;
		it->second.mPeerCbMsg = "Denied Non-Friend";

		it->second.mPeerReqStatusMsg = "Denied Non-Friend";
		it->second.mPeerReqState = RSDHT_PEERREQ_STOPPED;
		it->second.mPeerReqTS = now;
		it->second.mPeerReqMode = 0;
		//it->second.mPeerProxyId;
		it->second.mPeerReqTS = now;

		it->second.mPeerCbMsg = "Denied Non-Friend";

		it->second.mPeerConnectMsg = "Denied Non-Friend";
		it->second.mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;

		
		return 0;
		//return NOT_FRIEND;
	}

	/* are a friend */

	if (dpd->mPeerConnectState == RSDHT_PEERCONN_CONNECTED)
	{
		std::cerr << "p3BitDht::checkConnectionAllowed() ERROR Peer Already Connected, DENIED";
		std::cerr << std::endl;

		// STATUS UPDATE DONE IN ACTION.
		//it->second.mPeerStatusMsg = "2nd Connection Attempt!";	
		//it->second.mPeerUpdateTS = now;
		return 0;
		//return ALREADY_CONNECTED;
	}

	return 1;
	//return CONNECTION_OKAY;		
}





/* So initiateConnection has to call out to other bits of RS.
 * critical information is:
 *    Peer RsId, Peer Address, Connect Mode (includes Proxy/OrNot).

 *
 * What do we need: ACTIVE / PASSIVE / UNSPEC
 * + Min Delay Time, 
 */

 
void p3BitDht::ConnectCalloutTCPAttempt(const RsPeerId &peerId, struct sockaddr_in raddrv4)
{
	struct sockaddr_storage raddr;
	struct sockaddr_storage proxyaddr;
	struct sockaddr_storage srcaddr;
	
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(srcaddr);

	struct sockaddr_in *rap = (struct sockaddr_in *) &raddr;
    //struct sockaddr_in *pap = (struct sockaddr_in *) &proxyaddr;
    //struct sockaddr_in *sap = (struct sockaddr_in *) &srcaddr;
	
	// only one to translate
	rap->sin_family = AF_INET;
	rap->sin_addr = raddrv4.sin_addr;
	rap->sin_port = raddrv4.sin_port;
	
	uint32_t source = RS_CB_DHT;
	uint32_t connectFlags = RS_CB_FLAG_ORDER_UNSPEC | RS_CB_FLAG_MODE_TCP;
	uint32_t delay = 0;
	uint32_t bandwidth = 0;

	mConnCb->peerConnectRequest(peerId, raddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}

 
void p3BitDht::ConnectCalloutDirectOrProxy(const RsPeerId &peerId, struct sockaddr_in raddrv4, uint32_t connectFlags, uint32_t delay)
{
	struct sockaddr_storage raddr;
	struct sockaddr_storage proxyaddr;
	struct sockaddr_storage srcaddr;
	
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(srcaddr);
	
	struct sockaddr_in *rap = (struct sockaddr_in *) &raddr;
//	struct sockaddr_in *pap = (struct sockaddr_in *) &proxyaddr;
//	struct sockaddr_in *sap = (struct sockaddr_in *) &srcaddr;
	
	// only one to translate
	rap->sin_family = AF_INET;
	rap->sin_addr = raddrv4.sin_addr;
	rap->sin_port = raddrv4.sin_port;
	
	uint32_t source = RS_CB_DHT;
	uint32_t bandwidth = 0;

	mConnCb->peerConnectRequest(peerId, raddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}

void p3BitDht::ConnectCalloutRelay(const RsPeerId &peerId, 
		struct sockaddr_in srcaddrv4, struct sockaddr_in proxyaddrv4, struct sockaddr_in destaddrv4,
			uint32_t connectFlags, uint32_t bandwidth)
{
	struct sockaddr_storage destaddr;
	struct sockaddr_storage proxyaddr;
	struct sockaddr_storage srcaddr;
	
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(srcaddr);
	
	struct sockaddr_in *dap = (struct sockaddr_in *) &destaddr;
	struct sockaddr_in *pap = (struct sockaddr_in *) &proxyaddr;
	struct sockaddr_in *sap = (struct sockaddr_in *) &srcaddr;
	
	dap->sin_family = AF_INET;
	dap->sin_addr = destaddrv4.sin_addr;
	dap->sin_port = destaddrv4.sin_port;

	pap->sin_family = AF_INET;
	pap->sin_addr = proxyaddrv4.sin_addr;
	pap->sin_port = proxyaddrv4.sin_port;
	
	sap->sin_family = AF_INET;
	sap->sin_addr = srcaddrv4.sin_addr;
	sap->sin_port = srcaddrv4.sin_port;
	
	uint32_t source = RS_CB_DHT;
	uint32_t delay = 0;

	mConnCb->peerConnectRequest(peerId, destaddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}


	

//virtual void    peerConnectRequest(std::string id, struct sockaddr_in raddr,
//                        struct sockaddr_in proxyaddr,  struct sockaddr_in srcaddr,
//                        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth) = 0;
 

void p3BitDht::initiateConnection(const bdId *srcId, const bdId *proxyId, const bdId *destId, 
								  uint32_t mode, uint32_t loc, uint32_t delayOrBandwidth)
{
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::initiateConnection()";
	std::cerr << std::endl;
	std::cerr << "\t srcId: ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << std::endl;

	std::cerr << "\t proxyId: ";
	bdStdPrintId(std::cerr, proxyId);
	std::cerr << std::endl;

	std::cerr << "\t destId: ";
	bdStdPrintId(std::cerr, destId);
	std::cerr << std::endl;

	std::cerr << "\t Mode: " << mode << " loc: " << loc;
	std::cerr << std::endl;
	std::cerr << "\t DelayOrBandwidth: " << delayOrBandwidth;
	std::cerr << std::endl;
#endif

	bdId peerConnectId;

	uint32_t connectFlags = 0;
	
	uint32_t delay = 0;
	uint32_t bandwidth = 0;

	/* determine who the actual destination is.
	 * as we always specify the remote address, this is all we need. 
	 */
	if (loc == BD_PROXY_CONNECTION_START_POINT)
	{
		peerConnectId = *destId;
		/* lowest is active */
		if (srcId < destId)
		{
			connectFlags |= RS_CB_FLAG_ORDER_ACTIVE;
		}
		else
		{
			connectFlags |= RS_CB_FLAG_ORDER_PASSIVE;
		}
	}
	else if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		peerConnectId = *srcId;
		/* lowest is active (we are now dest - in this case) */
		if (destId < srcId)
		{
			connectFlags |= RS_CB_FLAG_ORDER_ACTIVE;
		}
		else
		{
			connectFlags |= RS_CB_FLAG_ORDER_PASSIVE;
		}
	}
	else
	{
		std::cerr << "p3BitDht::initiateConnection() ERROR, NOT either START or END";
		std::cerr << std::endl;
		/* ERROR */
		return;
	}

#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::initiateConnection() Connecting to ";
	bdStdPrintId(std::cerr, &peerConnectId);
	std::cerr << std::endl;
#endif

//	uint32_t touConnectMode = 0;
	RsPeerId rsId;
	
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

		DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerConnectId.id), RsDhtPeerType::FRIEND);
		/* grab a socket */
		if (!dpd)
		{
			std::cerr << "p3BitDht::initiateConnection() ERROR Peer not found";
			std::cerr << std::endl;
			return;
		}

		if (dpd->mPeerConnectState != RSDHT_PEERCONN_DISCONNECTED)
		{
			std::cerr << "p3BitDht::initiateConnection() ERROR Peer is not Disconnected";
			std::cerr << std::endl;
			return;
		}

		rsId = dpd->mRsId;

		/* start the connection */
		/* These Socket Modes must match the TOU Stack - or it breaks. */
		switch(mode)
		{
			default:
			case BITDHT_CONNECT_MODE_DIRECT:
//				touConnectMode = RSDHT_TOU_MODE_DIRECT;
				connectFlags |= RS_CB_FLAG_MODE_UDP_DIRECT;
				delay = delayOrBandwidth;
				break;

			case BITDHT_CONNECT_MODE_PROXY:
			{
				bool useProxyPort = dpd->mConnectLogic.getProxyPortChoice();

#ifdef DEBUG_PEERNET
				std::cerr << "p3BitDht::initiateConnection() Peer Mode Proxy... deciding which port to use.";
				std::cerr << " UseProxyPort? " << useProxyPort;
				std::cerr << std::endl;
#endif
				
				delay = delayOrBandwidth;
				if (useProxyPort)
				{
//					touConnectMode = RSDHT_TOU_MODE_PROXY;
					connectFlags |= RS_CB_FLAG_MODE_UDP_PROXY;
				}
				else
				{
//					touConnectMode = RSDHT_TOU_MODE_DIRECT;
					connectFlags |= RS_CB_FLAG_MODE_UDP_DIRECT;
				}

			}
				break;
	
			case BITDHT_CONNECT_MODE_RELAY:
//				touConnectMode = RSDHT_TOU_MODE_RELAY;
				connectFlags |= RS_CB_FLAG_MODE_UDP_RELAY;
				bandwidth = delayOrBandwidth;
				break;
		}


		dpd->mPeerConnectProxyId = *proxyId;
		dpd->mPeerConnectPeerId = peerConnectId;
	

		/* store results in Status */
		dpd->mPeerConnectMsg = "UDP started";
		dpd->mPeerConnectState = RSDHT_PEERCONN_UDP_STARTED;
		dpd->mPeerConnectUdpTS = time(NULL);
		dpd->mPeerConnectMode = mode;
		dpd->mPeerConnectPoint = loc;
	}
	
	/* finally we call out to start the connection (Outside of Mutex) */

	if ((mode ==  BITDHT_CONNECT_MODE_DIRECT) || (mode ==  BITDHT_CONNECT_MODE_PROXY))	
	{
		ConnectCalloutDirectOrProxy(rsId, peerConnectId.addr, connectFlags, delay);
	}
	else if (mode == BITDHT_CONNECT_MODE_RELAY)
	{
		if (loc == BD_PROXY_CONNECTION_START_POINT)
		{
			ConnectCalloutRelay(rsId, srcId->addr, proxyId->addr, destId->addr, connectFlags, bandwidth);
		}
		else // END_POINT
		{
			/* reverse order connection call */
			ConnectCalloutRelay(rsId, destId->addr, proxyId->addr, srcId->addr, connectFlags, bandwidth);
		}
	}

	
}


int p3BitDht::installRelayConnection(const bdId *srcId, const bdId *destId, uint32_t &bandwidth)
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	/* work out if either srcId or DestId is a friend */
	int relayClass = UDP_RELAY_CLASS_GENERAL;

#ifdef DEBUG_PEERNET
	RsPeerId strId1;
	bdStdPrintNodeId(strId1, &(srcId->id), false);

	RsPeerId strId2;
	bdStdPrintNodeId(strId2, &(destId->id), false);
#endif

        /* grab a socket */
	DhtPeerDetails *dpd_src = findInternalDhtPeer_locked(&(srcId->id), RsDhtPeerType::ANY);
	DhtPeerDetails *dpd_dest = findInternalDhtPeer_locked(&(destId->id), RsDhtPeerType::ANY);

	if ((dpd_src) && (dpd_src->mPeerType == RsDhtPeerType::FRIEND))
	{
		relayClass = UDP_RELAY_CLASS_FRIENDS;
	}
	else if ((dpd_dest) && (dpd_dest->mPeerType == RsDhtPeerType::FRIEND))
	{
		relayClass = UDP_RELAY_CLASS_FRIENDS;
	}
	else if ((dpd_src) && (dpd_src->mPeerType == RsDhtPeerType::FOF))
	{
		relayClass = UDP_RELAY_CLASS_FOF;
	}
	else if ((dpd_dest) && (dpd_dest->mPeerType == RsDhtPeerType::FOF))
	{
		relayClass = UDP_RELAY_CLASS_FOF;
	}
	else 
	{
		relayClass = UDP_RELAY_CLASS_GENERAL;
	}
	
	/* will install the Relay Here... so that we reserve the Relay Space for later. */
	UdpRelayAddrSet relayAddrs(&(srcId->addr), &(destId->addr));
	if (mRelay->addUdpRelay(&relayAddrs, relayClass, bandwidth))
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::installRelayConnection() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;
#endif

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::installRelayConnection() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;
#endif

		return 0;
		//return CONNECT_MODE_OVERLOADED;
	}
	return 0;
}


int p3BitDht::removeRelayConnection(const bdId *srcId, const bdId *destId)
{
	//RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	UdpRelayAddrSet relayAddrs(&(srcId->addr), &(destId->addr));
	if (mRelay->removeUdpRelay(&relayAddrs))
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::removeRelayConnection() Successfully removed Relay";
		std::cerr << std::endl;
#endif

		return 1;
	}
	else
	{
		std::cerr << "p3BitDht::removeRelayConnection() ERROR Failed to remove Relay";
		std::cerr << std::endl;

		return 0;
	}
}

/***************************************************** UDP Connections *****************************/


void p3BitDht::monitorConnections()
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	rstime_t now = time(NULL);

	std::map<bdNodeId, DhtPeerDetails>::iterator it;

	for(it = mPeers.begin(); it != mPeers.end(); ++it)
	{
		/* ignore ones which aren't friends */
		if (it->second.mPeerType != RsDhtPeerType::FRIEND)
		{
			continue;
		}
		
		if (it->second.mPeerConnectState == RSDHT_PEERCONN_UDP_STARTED)
		{
#ifdef DEBUG_PEERNET
			std::cerr << "p3BitDht::monitorConnections() Connection in progress to: ";
			
			bdStdPrintNodeId(std::cerr, &(it->second.mDhtId.id));
			std::cerr << std::endl;
#endif

			if (now - it->second.mPeerConnectUdpTS > PEERNET_CONNECT_TIMEOUT)
			{
				/* This CAN happen ;( */
#ifdef DEBUG_PEERNET
				std::cerr << "p3BitDht::monitorConnections() WARNING InProgress Connection Failed: ";
				bdStdPrintNodeId(std::cerr, &(it->second.mDhtId.id));
				std::cerr << std::endl;
#endif

				UdpConnectionFailed_locked(&(it->second));
			}
		}
	}
}




void p3BitDht::Feedback_Connected(const RsPeerId& pid)
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);

	if (!dpd)
	{
		/* ERROR */
		std::cerr << "p3BitDht::Feedback_Connected() ERROR peer not found: " << pid;
		std::cerr << std::endl;
		
		return;
	}
	
	/* sanity checking */
	if (dpd->mPeerConnectState != RSDHT_PEERCONN_UDP_STARTED)
	{
		/* ERROR */
		std::cerr << "p3BitDht::Feedback_Connected() ERROR not in UDP_STARTED mode for: ";
		bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
		std::cerr << std::endl;
		
		return;
	}
	
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::monitorConnections() InProgress Connection Now Active: ";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;
#endif
			
			/* switch state! */
	dpd->mPeerConnectState = RSDHT_PEERCONN_CONNECTED;
	dpd->mPeerConnectTS = time(NULL);
			
	dpd->mConnectLogic.updateCb(CSB_UPDATE_CONNECTED);
			
	rs_sprintf(dpd->mPeerConnectMsg, "Connected in %ld secs", dpd->mPeerConnectTS - dpd->mPeerConnectUdpTS);
			
	// Remove the Connection Request.
	if (dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::monitorConnections() Request Active, Stopping Request";
		std::cerr << std::endl;
#endif
				
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_KILLREQ;
		ca.mMode = dpd->mPeerConnectMode;
		//ca.mProxyId = *proxyId;
		//ca.mSrcId = *srcId;
		ca.mDestId = dpd->mPeerConnectPeerId;
		//ca.mPoint = point;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
				
		mActions.push_back(ca);
				
		// What the Action should do...	
		//struct sockaddr_in tmpaddr;
		//bdsockaddr_clear(&tmpaddr);
		//int start = 0;
		//mUdpBitDht->ConnectionRequest(&tmpaddr, &(it->second.mPeerConnectPeerId.id), it->second.mPeerConnectMode, start);
	}
	// only an error if we initiated the connection.
	else if (dpd->mPeerConnectPoint == BD_PROXY_CONNECTION_START_POINT)
	{
		std::cerr << "p3BitDht::monitorConnections() ERROR Request not active, can't stop";
		std::cerr << std::endl;										
	}

	ReleaseProxyExclusiveMode_locked(dpd, true);
}

void p3BitDht::Feedback_ConnectionFailed(const RsPeerId& pid)
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
	
	if (!dpd)
	{
		/* ERROR */
		std::cerr << "p3BitDht::Feedback_Connected() ERROR peer not found: " << pid;
		std::cerr << std::endl;
		
		return;
	}
	
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::Feedback_ConnectionFailed() UDP Connection Failed: ";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;
#endif

	return UdpConnectionFailed_locked(dpd);
}

void p3BitDht::Feedback_ConnectionClosed(const RsPeerId& pid)
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
	
	if (!dpd)
	{
		/* ERROR */
		std::cerr << "p3BitDht::Feedback_Connected() ERROR peer not found: " << pid;
		std::cerr << std::endl;
		
		return;
	}
	
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::Feedback_ConnectionClosed() Active Connection Closed: ";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;
#endif

	return UdpConnectionFailed_locked(dpd);
}


void p3BitDht::UdpConnectionFailed_locked(DhtPeerDetails *dpd)
{
	if (dpd->mPeerConnectState == RSDHT_PEERCONN_UDP_STARTED)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::UdpConnectionFailed_locked() UDP Connection Failed: ";
		bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
		std::cerr << std::endl;
#endif

		/* shut it down */

		/* ONLY need to update ConnectLogic - if it was our Attempt Running */
		if (dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING)
		{
			dpd->mConnectLogic.updateCb(CSB_UPDATE_FAILED_ATTEMPT);
		}
		dpd->mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;
		dpd->mPeerConnectMsg = "UDP Failed";

	}
	else
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::UdpConnectionFailed_locked() Active Connection Closed: ";
		bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
		std::cerr << std::endl;
#endif

		dpd->mConnectLogic.updateCb(CSB_UPDATE_DISCONNECTED);

		dpd->mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;
		dpd->mPeerConnectClosedTS = time(NULL);
		rs_sprintf(dpd->mPeerConnectMsg, "Closed, Alive for: %ld secs", dpd->mPeerConnectClosedTS - dpd->mPeerConnectTS);
	}



	if (dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING)
	{
#ifdef DEBUG_PEERNET
		std::cerr << "p3BitDht::UdpConnectionFailed_locked() ";
		std::cerr << "Request Active (Paused)... Killing for next Attempt";
		std::cerr << std::endl;					
#endif
		
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_KILLREQ;
		ca.mMode = dpd->mPeerConnectMode;
		//ca.mProxyId = *proxyId;
		//ca.mSrcId = *srcId;
		ca.mDestId = dpd->mPeerConnectPeerId;
		//ca.mPoint = point;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
		mActions.push_back(ca);
	}
	// only an error if we initiated the connection.
	else if (dpd->mPeerConnectPoint == BD_PROXY_CONNECTION_START_POINT)
	{
		std::cerr << "p3BitDht::UdpConnectionFailed_locked() ";
		std::cerr << "ERROR Request not active, can't stop";
		std::cerr << std::endl;										
	}

	ReleaseProxyExclusiveMode_locked(dpd, true);
}



void p3BitDht::ReleaseProxyExclusiveMode_locked(DhtPeerDetails *dpd, bool addrChgLikely)
{
#ifdef DEBUG_BITDHT_COMMON
	std::cerr << "p3BitDht::ReleaseProxyExclusiveMode_locked()";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;
#endif
	
	/* translate id into string for exclusive mode */
	std::string pid;
	bdStdPrintNodeId(pid, &(dpd->mDhtId.id), false);
	
	
	if (dpd->mExclusiveProxyLock)
	{
#ifdef RS_USE_DHT_STUNNER
		if (mProxyStunner->releaseExclusiveMode(pid, addrChgLikely))
		{
			dpd->mExclusiveProxyLock = false;

#ifdef DEBUG_PEERNET
			std::cerr << "p3BitDht::ReleaseProxyExclusiveMode_locked() Lock released by Connection to peer: ";
			bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
			std::cerr << std::endl;
#endif
		}
		else 
#else // RS_USE_DHT_STUNNER
		(void)addrChgLikely;
#endif // RS_USE_DHT_STUNNER
		{
			dpd->mExclusiveProxyLock = false;

			std::cerr << "p3BitDht::ReleaseProxyExclusiveMode_locked() ERROR ProxyStunner is not Locked";
			bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
			std::cerr << std::endl;			
		}

	}
	else
	{
#ifdef DEBUG_BITDHT_COMMON
		std::cerr << "p3BitDht::ReleaseProxyExclusiveMode_locked() Don't Have a Lock";
		std::cerr << std::endl;
#endif
	}

}


void p3BitDht::ConnectionFeedback(const RsPeerId& pid, int mode)
{
#ifdef DEBUG_PEERNET
	std::cerr << "p3BitDht::ConnectionFeedback() peer: " << pid;
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;
#endif

	switch(mode)
	{
		case NETMGR_DHT_FEEDBACK_CONNECTED:
#ifdef DEBUG_PEERNET
			std::cerr << "p3BitDht::ConnectionFeedback() HAVE CONNECTED (tcp?/udp) to peer: " << pid;
			std::cerr << std::endl;
#endif

			Feedback_Connected(pid);
			break;

		case NETMGR_DHT_FEEDBACK_CONN_FAILED:
#ifdef DEBUG_PEERNET
			std::cerr << "p3BitDht::ConnectionFeedback() UDP CONNECTION FAILED to peer: " << pid;
			std::cerr << std::endl;
#endif
			Feedback_ConnectionFailed(pid);
			break;

		case NETMGR_DHT_FEEDBACK_CONN_CLOSED:
#ifdef DEBUG_PEERNET
			std::cerr << "p3BitDht::ConnectionFeedback() CONNECTION (tcp?/udp) CLOSED to peer: " << pid;
			std::cerr << std::endl;
#endif
			Feedback_ConnectionClosed(pid);
			break;
	}
}


/***** Check for a RelayHandler... and call its functions preferentially */

int p3BitDht::RelayHandler_LogFailedProxyAttempt(const bdId *srcId, const bdId *destId, uint32_t mode, uint32_t errcode)
{

	{
        	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

		if ((mRelayHandler) && (mRelayHandler->mLogFailedConnection))
		{
			return mRelayHandler->mLogFailedConnection(srcId, destId, mode, errcode);
		}
	}

	/* NO standard handler */
	return 0;
}


int p3BitDht::RelayHandler_InstallRelayConnection(const bdId *srcId, const bdId *destId, 
							uint32_t mode, uint32_t &bandwidth)
{

	{
        	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

		if ((mRelayHandler) && (mRelayHandler->mInstallRelay))
		{
			return mRelayHandler->mInstallRelay(srcId, destId, mode, bandwidth);
		}
	}

	/* standard handler */
	return installRelayConnection(srcId, destId, bandwidth);
}





