
#include "dht/p3bitdht.h"

#include <stdio.h>
#include <iostream>
#include <sstream>


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

#define PEERNET_CONNECT_TIMEOUT 45

/***
 *
 * #define DEBUG_BITDHT_COMMON	1     // These are the things that are called regularly (annoying for debugging specifics)
 *
 **/

#if 0
int p3BitDht::add_peer(std::string id)
{
	bdNodeId tmpId;

        if (!bdStdLoadNodeId(&tmpId, id))
        {
                std::cerr << "p3BitDht::add_peer() Failed to load own Id";
                std::cerr << std::endl;
		return 0;
        }
	
	std::ostringstream str;
	bdStdPrintNodeId(str, &tmpId);
	std::string filteredId = str.str();

	std::cerr << "p3BitDht::add_peer()";
	std::cerr << std::endl;

        std::cerr << "Input Id: " << id;
        std::cerr << std::endl;
        std::cerr << "Filtered Id: " << id;
        std::cerr << std::endl;
        std::cerr << "Final NodeId: ";
        bdStdPrintNodeId(std::cerr, &tmpId);
        std::cerr << std::endl;

	bool addedPeer = false;

	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	  if (mPeers.end() == mPeers.find(filteredId))
	  {
		std::cerr << "Adding New Peer: " << filteredId << std::endl;
		mPeers[filteredId] = PeerStatus();
		std::map<std::string, PeerStatus>::iterator it = mPeers.find(filteredId);

		//mUdpBitDht->addFindNode(&tmpId, BITDHT_QFLAGS_DO_IDLE);
		mUdpBitDht->addFindNode(&tmpId, BITDHT_QFLAGS_DO_IDLE | BITDHT_QFLAGS_UPDATES);

		it->second.mId = filteredId;
		bdsockaddr_clear(&(it->second.mDhtAddr));
		it->second.mDhtStatusMsg = "Just Added";	
		it->second.mDhtState = RSDHT_PEERDHT_SEARCHING;
		it->second.mDhtUpdateTS = time(NULL);

		// Initialise Everything.
		it->second.mConnectLogic.mPeerId = filteredId;

		it->second.mPeerReqStatusMsg = "Just Added";
		it->second.mPeerReqState = RSDHT_PEERREQ_STOPPED;
		  it->second.mPeerReqMode = 0;
		  //it->second.mPeerReqProxyId;
		it->second.mPeerReqTS = time(NULL);

		it->second.mPeerCbMsg = "No CB Yet";
		it->second.mPeerCbMode = 0;
		it->second.mPeerCbPoint = 0;
		  //it->second.mPeerCbProxyId = 0;
		  //it->second.mPeerCbDestId = 0;
		  it->second.mPeerCbTS = 0;

		it->second.mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;
		it->second.mPeerConnectMsg = "Disconnected";
		  it->second.mPeerConnectFd = 0;
		  it->second.mPeerConnectMode = 0;
		  //it->second.mPeerConnectProxyId;
		  it->second.mPeerConnectPoint = 0;
		  
		  it->second.mPeerConnectUdpTS = 0;
		  it->second.mPeerConnectTS = 0;
		  it->second.mPeerConnectClosedTS = 0;

		bdsockaddr_clear(&(it->second.mPeerConnectAddr));

		addedPeer = true;
	  }

		/* remove from FailedPeers */
	  std::map<std::string, PeerStatus>::iterator it = mFailedPeers.find(filteredId);
	  if (it != mFailedPeers.end())
	  {
		mFailedPeers.erase(it);
	  }
	}

	if (addedPeer)
	{
		// outside of mutex.
		storePeers(mPeersFile);
		return 1;
	}

	std::cerr << "Peer Already Exists, ignoring: " << filteredId << std::endl;

	return 0;
}

int p3BitDht::remove_peer(std::string id)
{
	bdNodeId tmpId;

        if (!bdStdLoadNodeId(&tmpId, id))
        {
                std::cerr << "p3BitDht::remove_peer() Failed to load own Id";
                std::cerr << std::endl;
		return 0;
        }

	std::ostringstream str;
	bdStdPrintNodeId(str, &tmpId);
	std::string filteredId = str.str();

	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(filteredId);

	if (mPeers.end() != it)
	{
		mUdpBitDht->removeFindNode(&tmpId);
		mPeers.erase(it);
		return 1;
	}
	return 0;
}

#endif

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


		if ((mProxyStunner) && (mProxyStunner->needStunPeers()))
		{
#ifdef DEBUG_BITDHT_COMMON
			std::cerr << "p3BitDht::NodeCallback() Passing BitDHT Peer to DhtStunner: ";
			bdStdPrintId(std::cerr, id);
			std::cerr << std::endl;
#endif
			mProxyStunner->addStunPeer(id->addr, "");
		}
		/* else */ // removed else until we have lots of peers.

		if ((mDhtStunner) && (mDhtStunner->needStunPeers()))
		{
#ifdef DEBUG_BITDHT_COMMON
			std::cerr << "p3BitDht::NodeCallback() Passing BitDHT Peer to DhtStunner: ";
			bdStdPrintId(std::cerr, id);
			std::cerr << std::endl;
#endif
			mDhtStunner->addStunPeer(id->addr, "");
		}
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
	std::ostringstream str;
	bdStdPrintNodeId(str, &(id->id));
	std::string strId = str.str();

	//std::cerr << "p3BitDht::dhtPeerCallback()";
	//std::cerr << std::endl;

	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(id->id), RSDHT_PEERTYPE_FRIEND);

	if (!dpd)
	{
		/* ERROR */
		std::cerr << "p3BitDht::PeerCallback() ERROR Unknown Peer: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << " status: " << status;
		std::cerr << std::endl;

		return 0;
	}

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
			UnreachablePeerCallback_locked(id, status, dpd);

			break;

		case BITDHT_MGR_QUERY_PEER_ONLINE:
			dpd->mDhtState = RSDHT_PEERDHT_ONLINE;
			OnlinePeerCallback_locked(id, status, dpd);

			break;

	}

	time_t now = time(NULL);
	dpd->mDhtUpdateTS = now;

	return 1;
}



int p3BitDht::OnlinePeerCallback_locked(const bdId *id, uint32_t status, DhtPeerDetails *dpd)
{

	if (dpd->mPeerConnectState != RSDHT_PEERCONN_DISCONNECTED)
	{

		std::cerr << "p3BitDht::OnlinePeerCallback_locked() WARNING Ignoring Callback: connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		return 1;
	}

	bool connectOk = true;
	bool doTCPCallback = false;

	/* work out network state */
	uint32_t connectFlags = dpd->mConnectLogic.connectCb(CSB_CONNECT_DIRECT,
					mNetMgr->getNetworkMode(), mNetMgr->getNatTypeMode());
	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

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
		std::cerr << "dhtPeerCallback. Peer Online, triggering Direct Connection for: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

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
		std::cerr << "dhtPeerCallback. Peer Online, triggering TCP Connection for: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_TCPATTEMPT;
		ca.mMode = BITDHT_CONNECT_MODE_DIRECT;
		ca.mDestId = *id;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;

		//ConnectCalloutTCPAttempt(dpd->mRsId, id->addr);
	}

	return 1;
}



int p3BitDht::UnreachablePeerCallback_locked(const bdId *id, uint32_t status, DhtPeerDetails *dpd)
{

	if (dpd->mPeerConnectState != RSDHT_PEERCONN_DISCONNECTED)
	{

		std::cerr << "p3BitDht::UnreachablePeerCallback_locked() WARNING Ignoring Callback: connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		return 1;
	}

	std::cerr << "dhtPeerCallback. Peer Unreachable, triggering Proxy | Relay Connection for: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;

	bool proxyOk = false;
	bool connectOk = true;

	/* work out network state */
	uint32_t connectFlags = dpd->mConnectLogic.connectCb(CSB_CONNECT_UNREACHABLE,
					mNetMgr->getNetworkMode(), mNetMgr->getNatTypeMode());
	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

	switch(connectFlags & CSB_ACTION_MASK_MODE)
	{
		default:
		case CSB_ACTION_WAIT:
		{
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
		time_t now = time(NULL);
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_CONNECT;
		ca.mDestId = *id;

		dpd->mConnectLogic.storeProxyPortChoice(connectFlags, useProxyPort);

		if (proxyOk)
		{
			ca.mMode = BITDHT_CONNECT_MODE_PROXY;
			std::cerr << "dhtPeerCallback. Trying Proxy Connection.";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "dhtPeerCallback. Trying Relay Connection.";
			std::cerr << std::endl;
			ca.mMode = BITDHT_CONNECT_MODE_RELAY;
		}

		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		mActions.push_back(ca);
	}
	else
	{
		std::cerr << "dhtPeerCallback. Cancelled Connection Attempt for";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
	}

	return 1;
}



int p3BitDht::ValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "p3BitDht::ValueCallback() Does nothing!";
	std::cerr << std::endl;

	return 1;
}

int p3BitDht::ConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
						uint32_t mode, uint32_t point, uint32_t cbtype, uint32_t errcode)
{
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

	std::cerr << "mode: " << mode;
	std::cerr << " point: " << point;
	std::cerr << " cbtype: " << cbtype;
	std::cerr << std::endl;


	/* we handle MID and START/END points differently... this is biggest difference.
	 * so handle first.
	 */

	bdId peerId;
	time_t now = time(NULL);

	switch(point)
	{
		default:
		case BD_PROXY_CONNECTION_UNKNOWN_POINT:
		{
			std::cerr << "p3BitDht::dhtConnectCallback() UNKNOWN point, ignoring Callback";
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
					std::cerr << "dhtConnectionCallback() Proxy Connection Requested Between:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;

					int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
					if (checkProxyAllowed(srcId, destId, mode))
					{
						connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
						std::cerr << "dhtConnectionCallback() Connection Allowed";
						std::cerr << std::endl;
					}
					else
					{
						connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
						std::cerr << "dhtConnectionCallback() Connection Denied";
						std::cerr << std::endl;
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
		
					mActions.push_back(ca);
				}
				break;
				case BITDHT_CONNECT_CB_PENDING:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Pending:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
				}
				break;
				case BITDHT_CONNECT_CB_PROXY:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Starting:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
				}
				break;
				case BITDHT_CONNECT_CB_FAILED:
				{
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

	switch(cbtype)
	{
		case BITDHT_CONNECT_CB_AUTH:
		{
			std::cerr << "dhtConnectionCallback() Connection Requested By: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;

			int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
			if (checkConnectionAllowed(&(peerId), mode))
			{
				connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
				std::cerr << "dhtConnectionCallback() Connection Allowed";
				std::cerr << std::endl;
			}
			else
			{
				connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
				std::cerr << "dhtConnectionCallback() Connection Denied";
				std::cerr << std::endl;
			}
	
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_AUTHORISE;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
	
			/* Check Proxy ExtAddress Status (but only if connection is Allowed) */	
			if ((connectionAllowed == BITDHT_CONNECT_ANSWER_OKAY) &&
			 		 (mode == BITDHT_CONNECT_MODE_PROXY))
			{
				std::cerr << "dhtConnectionCallback() Checking Address for Proxy";
				std::cerr << std::endl;

				struct sockaddr_in extaddr;
				uint8_t extStable = 0;
				sockaddr_clear(&extaddr);

				bool connectOk = false;
				bool proxyPort = false; 
				std::cerr << "dhtConnectionCallback():  Proxy... deciding which port to use.";
				std::cerr << std::endl;
	
				DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RSDHT_PEERTYPE_FRIEND);
				if (dpd)
				{
					proxyPort = dpd->mConnectLogic.shouldUseProxyPort(
						mNetMgr->getNetworkMode(), mNetMgr->getNatTypeMode());

					dpd->mConnectLogic.storeProxyPortChoice(0, proxyPort);

					std::cerr << "dhtConnectionCallback: Setting ProxyPort: ";
					std::cerr << " UseProxyPort? " << proxyPort;
					std::cerr << std::endl;

					connectOk = true;
				}
				else
				{
					std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
					std::cerr << std::endl;
				}

				UdpStunner *stunner = mProxyStunner;
				if (!proxyPort)
				{
					stunner = mDhtStunner;
				}
				
				if ((connectOk) && (stunner) && (stunner->externalAddr(extaddr, extStable)))
				{
					if (extStable)
					{
						std::cerr << "dhtConnectionCallback() Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(peerId));
						std::cerr << " is OkGo as we have Stable Own External Proxy Address";
						std::cerr << std::endl;

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
						
					}
					else
					{
						connectionAllowed = BITDHT_CONNECT_ERROR_UNREACHABLE;
						std::cerr << "dhtConnectionCallback() Proxy Connection";
						std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
						std::cerr << std::endl;
					}
				}
				else
				{
					connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
					std::cerr << "dhtConnectionCallback() ERROR Proxy Connection ";
					std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
					std::cerr << std::endl;
				}
			}

			ca.mMode = mode;
			ca.mPoint = point;
			ca.mAnswer = connectionAllowed;

			mActions.push_back(ca);
		}
		break;

		case BITDHT_CONNECT_CB_START:
		{
			std::cerr << "dhtConnectionCallback() Connection Starting with: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;

			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_START;
			ca.mMode = mode;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
			ca.mPoint = point;
			ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
			mActions.push_back(ca);

		}
		break;

		case BITDHT_CONNECT_CB_FAILED:
		{
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
			
			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RSDHT_PEERTYPE_FRIEND);
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
			std::cerr << "dhtConnectionCallback() Local Connection Request Feedback:";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
			
			std::cerr << "dhtConnectionCallback() Proxy:";			
			bdStdPrintId(std::cerr, proxyId);
			std::cerr << std::endl;

			if (point != BD_PROXY_CONNECTION_START_POINT)
			{
				std::cerr << "dhtConnectionCallback() ERROR not START_POINT";
				std::cerr << std::endl;
				return 0;
			}

			RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

			DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId.id), RSDHT_PEERTYPE_FRIEND);
			if (dpd)
			{
				if (errcode)
				{
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

						/* standard failed attempts */
						case BITDHT_CONNECT_ERROR_GENERIC:
						case BITDHT_CONNECT_ERROR_PROTOCOL:
						case BITDHT_CONNECT_ERROR_TIMEOUT:
						case BITDHT_CONNECT_ERROR_TEMPUNAVAIL:
						case BITDHT_CONNECT_ERROR_NOADDRESS:
						case BITDHT_CONNECT_ERROR_OVERLOADED:
						case BITDHT_CONNECT_ERROR_DUPLICATE:
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

	time_t now = time(NULL);

#ifdef PEERNET_DEBUG
	std::cerr << "p3BitDht::tick() TIME: " << ctime(&now) << std::endl;
	std::cerr.flush();
#endif

	return 1;
}

#define MINUTE_IN_SECS	60

int p3BitDht::minuteTick()
{
	time_t now = time(NULL);
	int deltaT = 0;
	
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/
		deltaT = now-mMinuteTS;
	}
	
	if (deltaT > MINUTE_IN_SECS)
	{
		mRelay->checkRelays();

		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/
		mMinuteTS = now;
	}
	return 1;
}

int p3BitDht::doActions()
{
#ifdef PEERNET_DEBUG
	std::cerr << "p3BitDht::doActions()" << std::endl;
#endif

	time_t now = time(NULL);

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
				std::cerr << "PeerAction. Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				bool connectionRequested = false;

				if ((action.mMode == BITDHT_CONNECT_MODE_DIRECT) ||
						(action.mMode == BITDHT_CONNECT_MODE_RELAY))
				{
					struct sockaddr_in laddr; // We zero this address. The DHT layer should be able to handle this!
					sockaddr_clear(&laddr);
					uint32_t start = 1;
					mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
					connectionRequested = true;
				}
				else if (action.mMode == BITDHT_CONNECT_MODE_PROXY)
				{
					struct sockaddr_in extaddr;
					uint8_t extStable = 0;
					sockaddr_clear(&extaddr);
					bool proxyPort = true;
					bool connectOk = false;

					std::cerr << "PeerAction:  Proxy... deciding which port to use.";
					std::cerr << std::endl;
					{
						RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	
						DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RSDHT_PEERTYPE_FRIEND);
						if (dpd)
						{
							connectOk = true;
							proxyPort = dpd->mConnectLogic.getProxyPortChoice();


						}
						else
						{
							std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
							std::cerr << std::endl;
						}
					}
					UdpStunner *stunner = mProxyStunner;
					if (!proxyPort)
					{
						stunner = mDhtStunner;
					}
					
					if ((connectOk) && (stunner) && (stunner->externalAddr(extaddr, extStable)))
					{
						if (extStable)
						{
							std::cerr << "PeerAction: Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is OkGo as we have Stable Own External Proxy Address";
							std::cerr << std::endl;

							int start = 1;
							mUdpBitDht->ConnectionRequest(&extaddr, &(action.mDestId.id), action.mMode, start);
							connectionRequested = true;
						}
						else
						{
							std::cerr << "PeerAction: ERROR Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
							std::cerr << std::endl;

							RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
							DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RSDHT_PEERTYPE_FRIEND);
							if (dpd)
							{
								dpd->mConnectLogic.updateCb(CSB_UPDATE_MODE_UNAVAILABLE);
							}
						}
					}
					else
					{
						std::cerr << "PeerAction: ERROR Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(action.mDestId));
						std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
						std::cerr << std::endl;

						RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	
	
						DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RSDHT_PEERTYPE_FRIEND);
						if (dpd)
						{
							dpd->mConnectLogic.updateCb(CSB_UPDATE_FAILED_ATTEMPT);
						}
					}
				}

				if (connectionRequested)
				{
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RSDHT_PEERTYPE_FRIEND);
					if (dpd)
					{
						dpd->mPeerReqStatusMsg = "Connect Request";
						dpd->mPeerReqState = RSDHT_PEERREQ_RUNNING;
						dpd->mPeerReqMode = action.mMode;
						dpd->mPeerReqTS = now;
					}
					else
					{
						std::cerr << "PeerAction: Connect ERROR Cannot find PeerStatus";
						std::cerr << std::endl;
					}
				}

			}
			break;

			case PEERNET_ACTION_TYPE_AUTHORISE:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Authorise Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				mUdpBitDht->ConnectionAuth(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, action.mAnswer);

				// Only feedback to the gui if we are at END.
				if (action.mPoint == BD_PROXY_CONNECTION_END_POINT)
				{
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mSrcId.id), RSDHT_PEERTYPE_ANY);
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
						std::cerr << "PeerAction Authorise Connection ";			
						std::cerr << "Denied Unknown Peer";
						std::cerr << std::endl;
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
				std::cerr << "PeerAction. Start Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				initiateConnection(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, action.mAnswer);
			}
			break;

			case PEERNET_ACTION_TYPE_RESTARTREQ:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Restart Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 1;
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
			}
			break;

			case PEERNET_ACTION_TYPE_KILLREQ:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Kill Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 0;
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
			}
			break;

			case PEERNET_ACTION_TYPE_TCPATTEMPT:
			{
				/* connect attempt */
				std::cerr << "PeerAction. TCP Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 0;
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);

				std::string peerRsId;
				bool foundPeerId = false;
				{
					RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

					DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(action.mDestId.id), RSDHT_PEERTYPE_ANY);
					if (dpd)
					{
						peerRsId = dpd->mRsId;
						foundPeerId = true;
						std::cerr << "PeerAction. TCP Connection Attempt. DoingCallback for RsID: ";
						std::cerr << peerRsId;
						std::cerr << std::endl;
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

int p3BitDht::checkProxyAllowed(const bdId *srcId, const bdId *destId, int mode)
{
	std::cerr << "p3BitDht::checkProxyAllowed()";
	std::cerr << std::endl;

	// Dont think that a mutex is required here! But might be so just lock to ensure that it is possible.
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	}

	if (mode == BITDHT_CONNECT_MODE_PROXY) 
	{
		std::cerr << "p3BitDht::checkProxyAllowed() Allowing all PROXY connections, OKAY";
		std::cerr << std::endl;

		return 1;
		//return CONNECTION_OKAY;
	}

	if (mode != BITDHT_CONNECT_MODE_RELAY)
	{
		std::cerr << "p3BitDht::checkProxyAllowed() unknown Connect Mode DENIED";
		std::cerr << std::endl;
		return 0;
	}

	/* will install the Relay Here... so that we reserve the Relay Space for later. */
	if (installRelayConnection(srcId, destId))
	{
		std::cerr << "p3BitDht::checkProxyAllowed() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
		std::cerr << "p3BitDht::checkProxyAllowed() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;

		return 0;
		//return CONNECT_MODE_OVERLOADED;
	}

	return 0;
	//return CONNECT_MODE_NOTAVAILABLE;
	//return CONNECT_MODE_OVERLOADED;
}


int p3BitDht::checkConnectionAllowed(const bdId *peerId, int mode)
{
	std::cerr << "p3BitDht::checkConnectionAllowed() to: ";
	bdStdPrintId(std::cerr,peerId);
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;


	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	time_t now = time(NULL);

	/* check if they are in our friend list */
	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerId->id), RSDHT_PEERTYPE_FRIEND);
	if (!dpd)
	{
		std::cerr << "p3BitDht::checkConnectionAllowed() Peer Not Friend, DENIED";
		std::cerr << std::endl;

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

		it->second.mPeerType = RSDHT_PEERTYPE_OTHER;
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

 
void p3BitDht::ConnectCalloutTCPAttempt(const std::string &peerId, struct sockaddr_in raddr)
{
	struct sockaddr_in proxyaddr;
	struct sockaddr_in srcaddr;

	sockaddr_clear(&proxyaddr);
	sockaddr_clear(&srcaddr);

	uint32_t source = RS_CB_DHT;
	uint32_t connectFlags = RS_CB_FLAG_ORDER_UNSPEC | RS_CB_FLAG_MODE_TCP;
	uint32_t delay = 0;
	uint32_t bandwidth = 0;

	mConnCb->peerConnectRequest(peerId, raddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}

 
void p3BitDht::ConnectCalloutDirectOrProxy(const std::string &peerId, struct sockaddr_in raddr, uint32_t connectFlags, uint32_t delay)
{
        struct sockaddr_in proxyaddr;
	struct sockaddr_in srcaddr;

	sockaddr_clear(&proxyaddr);
	sockaddr_clear(&srcaddr);

	uint32_t source = RS_CB_DHT;
	uint32_t bandwidth = 0;

	mConnCb->peerConnectRequest(peerId, raddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}

void p3BitDht::ConnectCalloutRelay(const std::string &peerId, 
		struct sockaddr_in srcaddr, struct sockaddr_in proxyaddr, struct sockaddr_in destaddr,
			uint32_t connectFlags, uint32_t bandwidth)
{

	sockaddr_clear(&proxyaddr);
	sockaddr_clear(&srcaddr);

	uint32_t source = RS_CB_DHT;
	uint32_t delay = 0;

	mConnCb->peerConnectRequest(peerId, destaddr, proxyaddr, srcaddr, source, connectFlags, delay, bandwidth);
}


	

//virtual void    peerConnectRequest(std::string id, struct sockaddr_in raddr,
//                        struct sockaddr_in proxyaddr,  struct sockaddr_in srcaddr,
//                        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth) = 0;
 

void p3BitDht::initiateConnection(const bdId *srcId, const bdId *proxyId, const bdId *destId, uint32_t mode, uint32_t loc, uint32_t answer)
{
	std::cerr << "p3BitDht::initiateConnection()";
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;

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

	std::cerr << "p3BitDht::initiateConnection() Connecting to ";
	bdStdPrintId(std::cerr, &peerConnectId);
	std::cerr << std::endl;

	uint32_t touConnectMode = 0;
	std::string rsId;
	
	{
		RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

		DhtPeerDetails *dpd = findInternalDhtPeer_locked(&(peerConnectId.id), RSDHT_PEERTYPE_FRIEND);
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
				touConnectMode = RSDHT_TOU_MODE_DIRECT;
				connectFlags |= RS_CB_FLAG_MODE_UDP_DIRECT;
				delay = answer;
				break;

			case BITDHT_CONNECT_MODE_PROXY:
			{
				time_t now = time(NULL);
				bool useProxyPort = dpd->mConnectLogic.getProxyPortChoice();

				std::cerr << "p3BitDht::initiateConnection() Peer Mode Proxy... deciding which port to use.";
				std::cerr << " UseProxyPort? " << useProxyPort;
				std::cerr << std::endl;
				
				delay = answer;
				if (useProxyPort)
				{
					touConnectMode = RSDHT_TOU_MODE_PROXY;
					connectFlags |= RS_CB_FLAG_MODE_UDP_PROXY;
				}
				else
				{
					touConnectMode = RSDHT_TOU_MODE_DIRECT;
					connectFlags |= RS_CB_FLAG_MODE_UDP_DIRECT;
				}

			}
				break;
	
			case BITDHT_CONNECT_MODE_RELAY:
				touConnectMode = RSDHT_TOU_MODE_RELAY;
				connectFlags |= RS_CB_FLAG_MODE_UDP_DIRECT;
				bandwidth = answer;
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


int p3BitDht::installRelayConnection(const bdId *srcId, const bdId *destId)
{
	RsStackMutex stack(dhtMtx); /********** LOCKED MUTEX ***************/	

	/* work out if either srcId or DestId is a friend */
	int relayClass = UDP_RELAY_CLASS_GENERAL;

	std::ostringstream str;
	bdStdPrintNodeId(str, &(srcId->id));
	std::string strId1 = str.str();

	str.clear();
	bdStdPrintNodeId(str, &(destId->id));
	std::string strId2 = str.str();

        /* grab a socket */
	DhtPeerDetails *dpd_src = findInternalDhtPeer_locked(&(srcId->id), RSDHT_PEERTYPE_ANY);
	DhtPeerDetails *dpd_dest = findInternalDhtPeer_locked(&(destId->id), RSDHT_PEERTYPE_ANY);

	if ((dpd_src) && (dpd_src->mPeerType == RSDHT_PEERTYPE_FRIEND))
	{
		relayClass = UDP_RELAY_CLASS_FRIENDS;
	}
	else if ((dpd_dest) && (dpd_dest->mPeerType == RSDHT_PEERTYPE_FRIEND))
	{
		relayClass = UDP_RELAY_CLASS_FRIENDS;
	}
	else if ((dpd_src) && (dpd_src->mPeerType == RSDHT_PEERTYPE_FOF))
	{
		relayClass = UDP_RELAY_CLASS_FOF;
	}
	else if ((dpd_dest) && (dpd_dest->mPeerType == RSDHT_PEERTYPE_FOF))
	{
		relayClass = UDP_RELAY_CLASS_FOF;
	}
	else 
	{
		relayClass = UDP_RELAY_CLASS_GENERAL;
	}
	
	/* will install the Relay Here... so that we reserve the Relay Space for later. */
	UdpRelayAddrSet relayAddrs(&(srcId->addr), &(destId->addr));
	if (mRelay->addUdpRelay(&relayAddrs, relayClass))
	{
		std::cerr << "p3BitDht::installRelayConnection() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
		std::cerr << "p3BitDht::installRelayConnection() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;

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
		std::cerr << "p3BitDht::removeRelayConnection() Successfully removed Relay";
		std::cerr << std::endl;

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
	time_t now = time(NULL);

	std::map<bdNodeId, DhtPeerDetails>::iterator it;

	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		/* ignore ones which aren't friends */
		if (it->second.mPeerType != RSDHT_PEERTYPE_FRIEND)
		{
			continue;
		}
		
		if (it->second.mPeerConnectState == RSDHT_PEERCONN_UDP_STARTED)
		{
			std::cerr << "p3BitDht::monitorConnections() Connection in progress to: ";
			
			bdStdPrintNodeId(std::cerr, &(it->second.mDhtId.id));
			std::cerr << std::endl;

			if (now - it->second.mPeerConnectUdpTS > PEERNET_CONNECT_TIMEOUT)
			{
				std::cerr << "p3BitDht::monitorConnections() ERROR InProgress Connection Failed: ";
				bdStdPrintNodeId(std::cerr, &(it->second.mDhtId.id));
				std::cerr << std::endl;

				UdpConnectionFailed_locked(&(it->second));
			}
		}
	}
}




void p3BitDht::Feedback_Connected(std::string pid)
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
	
	std::cerr << "p3BitDht::monitorConnections() InProgress Connection Now Active: ";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;
			
			/* switch state! */
	dpd->mPeerConnectState = RSDHT_PEERCONN_CONNECTED;
	dpd->mPeerConnectTS = time(NULL);
			
	dpd->mConnectLogic.updateCb(CSB_UPDATE_CONNECTED);
			
	std::ostringstream msg;
	msg << "Connected in " << dpd->mPeerConnectTS - dpd->mPeerConnectUdpTS;
	msg << " secs";
	dpd->mPeerConnectMsg = msg.str();
			
	// Remove the Connection Request.
	if (dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING)
	{
		std::cerr << "p3BitDht::monitorConnections() Request Active, Stopping Request";
		std::cerr << std::endl;
				
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
}

void p3BitDht::Feedback_ConnectionFailed(std::string pid)
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
	
	return UdpConnectionFailed_locked(dpd);
}
																	 

void p3BitDht::UdpConnectionFailed_locked(DhtPeerDetails *dpd)
{

	/* shut id down */
	dpd->mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;
	dpd->mPeerConnectMsg = "UDP Failed";
	
	if (dpd->mPeerReqState == RSDHT_PEERREQ_RUNNING)
	{
		std::cerr << "p3BitDht::monitorConnections() Request Active (Paused)... restarting";
		std::cerr << std::endl;					
		
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_RESTARTREQ;
		ca.mMode = dpd->mPeerConnectMode;
		//ca.mProxyId = *proxyId;
		//ca.mSrcId = *srcId;
		ca.mDestId = dpd->mPeerConnectPeerId;
		//ca.mPoint = point;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
		mActions.push_back(ca);
		
		// What the Action should do!	
		// tell it to keep going.
		//struct sockaddr_in tmpaddr;
		//bdsockaddr_clear(&tmpaddr);
		//int start = 1;
		//mUdpBitDht->ConnectionRequest(&tmpaddr, &(it->second.mPeerConnectPeerId.id), it->second.mPeerConnectMode, start);
	}
	// only an error if we initiated the connection.
	else if (dpd->mPeerConnectPoint == BD_PROXY_CONNECTION_START_POINT)
	{
		std::cerr << "p3BitDht::monitorConnections() ERROR Request not active, can't stop";
		std::cerr << std::endl;										
	}
}


void p3BitDht::Feedback_ConnectionClosed(std::string pid)
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
	
	std::cerr << "p3BitDht::monitorConnections() Active Connection Closed: ";
	bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
	std::cerr << std::endl;

	dpd->mConnectLogic.updateCb(CSB_UPDATE_DISCONNECTED);

	dpd->mPeerConnectState = RSDHT_PEERCONN_DISCONNECTED;
	dpd->mPeerConnectClosedTS = time(NULL);
	std::ostringstream msg;
	msg << "Closed, Alive for: " << dpd->mPeerConnectClosedTS - dpd->mPeerConnectTS;
	msg << " secs";
	dpd->mPeerConnectMsg = msg.str();
}




void p3BitDht::ConnectionFeedback(std::string pid, int mode)
{
	std::cerr << "p3BitDht::ConnectionFeedback() peer: " << pid;
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;

	switch(mode)
	{
		case NETMGR_DHT_FEEDBACK_CONNECTED:
			std::cerr << "p3BitDht::ConnectionFeedback() HAVE CONNECTED (tcp?/udp) to peer: " << pid;
			std::cerr << std::endl;

			Feedback_Connected(pid);
			break;

		case NETMGR_DHT_FEEDBACK_CONN_FAILED:
			std::cerr << "p3BitDht::ConnectionFeedback() UDP CONNECTION FAILED to peer: " << pid;
			std::cerr << std::endl;
			Feedback_ConnectionFailed(pid);
			break;

		case NETMGR_DHT_FEEDBACK_CONN_CLOSED:
			std::cerr << "p3BitDht::ConnectionFeedback() CONNECTION (tcp?/udp) CLOSED to peer: " << pid;
			std::cerr << std::endl;
			Feedback_ConnectionClosed(pid);
			break;
	}
}



