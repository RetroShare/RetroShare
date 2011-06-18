/*
 * bitdht/bdconnection.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2011 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include <algorithm>
#include "bitdht/bdiface.h"

#include "bitdht/bdnode.h"
#include "bitdht/bdconnection.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bdstddht.h"
#include "util/bdnet.h"

#define DEBUG_NODE_CONNECTION	1 


/************************************************************************************************************
******************************************** Message Interface **********************************************
************************************************************************************************************/

/* Outgoing Messages */
std::string getConnectMsgType(int msgtype)
{
	switch(msgtype)
	{
		case BITDHT_MSG_TYPE_CONNECT_REQUEST:
			return "ConnectRequest";
			break;
		case BITDHT_MSG_TYPE_CONNECT_REPLY:
			return "ConnectReply";
			break;
		case BITDHT_MSG_TYPE_CONNECT_START:
			return "ConnectStart";
			break;
		case BITDHT_MSG_TYPE_CONNECT_ACK:
			return "ConnectAck";
			break;
		default:
			return "ConnectUnknown";
			break;
	}
}

void bdNode::msgout_connect_genmsg(bdId *id, bdToken *transId, int msgtype, bdId *srcAddr, bdId *destAddr, int mode, int status)
{
	std::cerr << "bdNode::msgout_connect_genmsg() Type: " << getConnectMsgType(msgtype);
	std::cerr << " TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " SrcAddr: ";
	mFns->bdPrintId(std::cerr, srcAddr);
	std::cerr << " DestAddr: ";
	mFns->bdPrintId(std::cerr, destAddr);
	std::cerr << " Mode: " << mode;
	std::cerr << " Status: " << status;
	std::cerr << std::endl;
#ifdef DEBUG_NODE_MSGOUT
#endif

	registerOutgoingMsg(id, transId, msgtype);
	
        /* create string */
        char msg[10240];
        int avail = 10240;

        int blen = bitdht_connect_genmsg(transId, &(mOwnId), msgtype, srcAddr, destAddr, mode, status, msg, avail-1);
        sendPkt(msg, blen, id->addr);


}

void bdNode::msgin_connect_genmsg(bdId *id, bdToken *transId, int msgtype, 
					bdId *srcAddr, bdId *destAddr, int mode, int status)
{
	std::list<bdId>::iterator it;

	std::cerr << "bdNode::msgin_connect_genmsg() Type: " << getConnectMsgType(msgtype);
	std::cerr << " TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " SrcAddr: ";
	mFns->bdPrintId(std::cerr, srcAddr);
	std::cerr << " DestAddr: ";
	mFns->bdPrintId(std::cerr, destAddr);
	std::cerr << " Mode: " << mode;
	std::cerr << " Status: " << status;
	std::cerr << std::endl;
#ifdef DEBUG_NODE_MSGS
#else
	(void) transId;
#endif

	/* switch to actual work functions */
	uint32_t peerflags = 0;
	switch(msgtype)
	{
		case BITDHT_MSG_TYPE_CONNECT_REQUEST:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mCounterRecvConnectRequest++;

			recvedConnectionRequest(id, srcAddr, destAddr, mode);

			break;
		case BITDHT_MSG_TYPE_CONNECT_REPLY:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mCounterRecvConnectReply++;

			recvedConnectionReply(id, srcAddr, destAddr, mode, status);

			break;
		case BITDHT_MSG_TYPE_CONNECT_START:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mCounterRecvConnectStart++;

			recvedConnectionStart(id, srcAddr, destAddr, mode, status);

			break;
		case BITDHT_MSG_TYPE_CONNECT_ACK:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mCounterRecvConnectAck++;

			recvedConnectionAck(id, srcAddr, destAddr, mode);

			break;
		default:
			break;
	}

	/* received message - so peer must be good */
	addPeer(id, peerflags);

}


/************************************************************************************************************
****************************************** Connection Initiation ********************************************
************************************************************************************************************/


/* This is called to initialise a connection.
 * the callback could be with regard to:
 * a Direct EndPoint.
 * a Proxy Proxy, or an Proxy EndPoint.
 * a Relay Proxy, or an Relay EndPoint.
 *
 * We have two alternatives:
 *  1) Direct Endpoint.
 *  2) Using a Proxy.
 */

int bdNode::requestConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{
	/* check if connection obj already exists */
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::requestConnection() Mode: " << mode;
	std::cerr << " Target: ";
	mFns->bdPrintNodeId(std::cerr, target);
	std::cerr << " Local NetAddress: " << inet_ntoa(laddr->sin_addr);
        std::cerr << ":" << ntohs(laddr->sin_port);
	std::cerr << std::endl;
#endif

	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		return requestConnection_direct(laddr, target);
	}
	else
	{
		return requestConnection_proxy(laddr, target, mode);
	}
}

int bdNode::checkExistingConnectionAttempt(bdNodeId *target)
{
	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	it = mConnectionRequests.find(*target);
	if (it != mConnectionRequests.end())
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::checkExistingConnectAttempt() Found Existing Connection!";
		std::cerr << std::endl;
#endif
		return 1;
	}
	return 0;
}

int bdNode::requestConnection_direct(struct sockaddr_in *laddr, bdNodeId *target)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::requestConnection_direct()";
	std::cerr << std::endl;
#endif
	/* create a bdConnect, and put into the queue */
	int mode = BITDHT_CONNECT_MODE_DIRECT;
	bdConnectionRequest connreq;
	
	if (checkExistingConnectionAttempt(target))
	{
		return 0;
	}

	connreq.setupDirectConnection(laddr, target);

	/* grab any peers from any existing query */
	std::list<bdQuery *>::iterator qit;
	for(qit = mLocalQueries.begin(); qit != mLocalQueries.end(); qit++)
	{
		if (!((*qit)->mId == (*target)))
		{
			continue;
		}

#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::requestConnection_direct() Found Matching Query";
		std::cerr << std::endl;
#endif
		/* matching query */
		/* find any potential proxies (must be same DHT type XXX TODO) */
		(*qit)->result(connreq.mPotentialProxies);		

		/* will only be one matching query.. so end loop */
		break;
	}

	/* now look in the bdSpace as well */

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::requestConnection_direct() Init Connection State";
	std::cerr << std::endl;
	std::cerr << connreq;
	std::cerr << std::endl;
#endif

	/* push connect onto queue, for later completion */

	mConnectionRequests[*target] = connreq;

	/* connection continued via iterator */
	return 1;
}

 
int bdNode::requestConnection_proxy(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::requestConnection_proxy()";
	std::cerr << std::endl;
#endif

	/* create a bdConnect, and put into the queue */

	bdConnectionRequest connreq;
	connreq.setupProxyConnection(laddr, target, mode);

	/* grab any peers from any existing query */
	std::list<bdQuery *>::iterator qit;
	for(qit = mLocalQueries.begin(); qit != mLocalQueries.end(); qit++)
	{
		if (!((*qit)->mId == (*target)))
		{
			continue;
		}

#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::requestConnection_proxy() Found Matching Query";
		std::cerr << std::endl;
#endif
		/* matching query */
		/* find any potential proxies (must be same DHT type XXX TODO) */
		(*qit)->proxies(connreq.mPotentialProxies);		

		/* will only be one matching query.. so end loop */
		break;
	}

	/* find closest acceptable peers, 
	 * and trigger a search for target...
	 * this will hopefully find more suitable proxies.
	 */

	std::list<bdId> excluding;
	std::multimap<bdMetric, bdId> nearest;

#define CONNECT_NUM_PROXY_ATTEMPTS	10
	int number = CONNECT_NUM_PROXY_ATTEMPTS;

	mNodeSpace.find_nearest_nodes_with_flags(target, number, excluding, nearest, 
			BITDHT_PEER_STATUS_DHT_FOF       |
			BITDHT_PEER_STATUS_DHT_FRIEND);

	number = CONNECT_NUM_PROXY_ATTEMPTS - number;

	mNodeSpace.find_nearest_nodes_with_flags(target, number, excluding, nearest, 
							BITDHT_PEER_STATUS_DHT_ENGINE_VERSION );

	std::multimap<bdMetric, bdId>::iterator it;
	for(it = nearest.begin(); it != nearest.end(); it++)
	{
		bdNodeId midId;
		mFns->bdRandomMidId(target, &(it->second.id), &midId);
		/* trigger search */
		send_query(&(it->second), &midId);
	}

	/* push connect onto queue, for later completion */
	mConnectionRequests[*target] = connreq;
	
	return 1;
}


/** This function needs the Potential Proxies to be tested against DHT_APPL flags **/
void bdNode::addPotentialConnectionProxy(bdId *srcId, bdId *target)
{
	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	for(it = mConnectionRequests.begin(); it != mConnectionRequests.end(); it++)
	{
		if (target->id == it->first)
		{
			it->second.addPotentialProxy(srcId);
		}
	}
}


int bdNode::tickConnections()
{
	iterateConnectionRequests();
	iterateConnections();
	
	return 1;
}


void bdNode::iterateConnectionRequests()
{
	time_t now = time(NULL);

	std::list<bdNodeId> eraseList;
	std::list<bdNodeId>::iterator eit;

	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	for(it = mConnectionRequests.begin(); it != mConnectionRequests.end(); it++)
	{
		/* check status of connection */
		if (it->second.mState == BITDHT_CONNREQUEST_INIT)
		{
			/* kick off the connection if possible */
			startConnectionAttempt(&(it->second));
		}

		// Cleanup
		if (now - it->second.mStateTS > BITDHT_CONNREQUEST_MAX_AGE)
		{
			std::cerr << "bdNode::iterateConnectionAttempt() Cleaning Old ConnReq: ";
			std::cerr << std::endl;
			std::cerr << it->second;
			std::cerr << std::endl;

			/* cleanup */
			eraseList.push_back(it->first);
		}
	}
	
	for(eit = eraseList.begin(); eit != eraseList.end(); eit++)
	{
		it = mConnectionRequests.find(*eit);
		if (it != mConnectionRequests.end())
		{
			mConnectionRequests.erase(it);
		}
	}
}

int bdNode::startConnectionAttempt(bdConnectionRequest *req)
{
	std::cerr << "bdNode::startConnectionAttempt() ConnReq: ";
	std::cerr << std::endl;
	std::cerr << *req;
	std::cerr << std::endl;

	if (req->mPotentialProxies.size() < 1)
	{
		std::cerr << "bdNode::startConnectionAttempt() No Potential Proxies... delaying attempt";
		std::cerr << std::endl;
		return 0;
	}

	bdId proxyId;
	bdId srcConnAddr;
	bdId destConnAddr;

	int mode = req->mMode;

	destConnAddr.id = req->mTarget;
	bdsockaddr_clear(&(destConnAddr.addr));

	srcConnAddr.id = mOwnId;
	srcConnAddr.addr = req->mLocalAddr;

	proxyId = req->mPotentialProxies.front();
	req->mPotentialProxies.pop_front();

	req->mCurrentAttempt = proxyId;
	req->mPeersTried.push_back(proxyId);	

	req->mState = BITDHT_CONNREQUEST_INPROGRESS;
	req->mStateTS = time(NULL);

	return startConnectionAttempt(&proxyId, &srcConnAddr, &destConnAddr, mode);
}


/************************************************************************************************************
****************************************** Outgoing Triggers ************************************************
************************************************************************************************************/

/*** Called by iterator.
 * initiates the connection startup
 *
 * srcConnAddr must contain Own ID + Connection Port (DHT or TOU depending on Mode). 
 *
 * For a DIRECT Connection: proxyId == destination Id, and mode == DIRECT.
 * 
 * For RELAY | PROXY Connection: 
 *
 * In all cases, destConnAddr doesn't need to contain a valid address.
 */

int bdNode::startConnectionAttempt(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::startConnectionAttempt()";
	std::cerr << std::endl;
#endif

	/* Check for existing Connection */
	bdConnection *conn = findExistingConnectionBySender(proxyId, srcConnAddr, destConnAddr);
	if (conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::startConnectAttempt() ERROR EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* INSTALL a NEW CONNECTION */
	// not offically playing by the rules, but it should work.
	conn = newConnectionBySender(proxyId, srcConnAddr, destConnAddr);

	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		/* proxy is the real peer address, destConnAddr has an invalid address */
        	conn->ConnectionSetupDirect(proxyId, srcConnAddr);
	}
	else
	{
		conn->ConnectionSetup(proxyId, srcConnAddr, destConnAddr, mode);
	}

	/* push off message */
	bdToken transId;
	genNewTransId(&transId);

	int msgtype =  BITDHT_MSG_TYPE_CONNECT_REQUEST;
	int status = BITDHT_CONNECT_ANSWER_OKAY;

	msgout_connect_genmsg(&(conn->mProxyId), &transId, msgtype, &(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);

	return 1;
}

/* This will be called in response to a callback.
 * the callback could be with regard to:
 * a Direct EndPoint.
 * a Proxy Proxy, or an Proxy EndPoint.
 * a Relay Proxy, or an Relay EndPoint.
 *
 * If we are going to store the minimal amount in the bdNode about connections, 
 * then the parameters must contain all the information:
 *  
 * case 1:
 *
 */
 
void bdNode::AuthConnectionOk(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::AuthConnectionOk()";
	std::cerr << std::endl;
#endif

	/* Check for existing Connection */
	bdConnection *conn = findExistingConnection(&(srcId->id), &(proxyId->id), &(destId->id));
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionOk() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return;
	}

	/* we need to continue the connection */
	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{	
		if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
		{	
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdNode::AuthConnectionOk() Direct Connection, in WAITING_AUTH state... Authorising Direct Connect";
			std::cerr << std::endl;
#endif
			/* This pushes it into the START/ACK cycle, 
			 * which handles messages elsewhere
			 */
			conn->AuthoriseDirectConnection(srcId, proxyId, destId, mode, loc);
		}
		else
		{
			/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdNode::AuthConnectionOk() ERROR Direct Connection, !WAITING_AUTH state... Ignoring";
			std::cerr << std::endl;
#endif

		}
		return;
	}

	if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
		{
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdNode::AuthConnectionOk() Proxy End Connection, in WAITING_AUTH state... Authorising";
			std::cerr << std::endl;
#endif
			/*** XXX MUST RECEIVE THE ADDRESS FROM DEST for connection */
			conn->AuthoriseEndConnection(srcId, proxyId, destId, mode, loc);
			
			/* we respond to the proxy which will finalise connection */
			bdToken transId;
			genNewTransId(&transId);

			int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
			int status = BITDHT_CONNECT_ANSWER_OKAY;
			msgout_connect_genmsg(&(conn->mProxyId), &transId, msgtype, &(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);
			
			return;
		}
		else
		{

			/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdNode::AuthConnectionOk() ERROR Proxy End Connection, !WAITING_AUTH state... Ignoring";
			std::cerr << std::endl;
#endif
		}
	}

	if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
	{
		/* otherwise we are the proxy (for either), pass on the request */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionOk() Proxy Mid Connection, in WAITING_AUTH state... Authorising";
		std::cerr << std::endl;
#endif
	
		/* SEARCH for IP:Port of destination is done before AUTH  */
	
		conn->AuthoriseProxyConnection(srcId, proxyId, destId, mode, loc);
	
		bdToken transId;
		genNewTransId(&transId);

		int msgtype = BITDHT_MSG_TYPE_CONNECT_REQUEST;
		int status = BITDHT_CONNECT_ANSWER_OKAY;
		msgout_connect_genmsg(&(conn->mDestId), &transId, msgtype, &(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);
	}
	else
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionOk() ERROR Proxy Mid Connection, !WAITING_AUTH state... Ignoring";
		std::cerr << std::endl;
#endif
	}

	return;	
}
	
 
void bdNode::AuthConnectionNo(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc)
{
	
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::AuthConnectionNo()";
	std::cerr << std::endl;
#endif
	
	/* Check for existing Connection */
	bdConnection *conn = findExistingConnection(&(srcId->id), &(proxyId->id), &(destId->id));
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionNo() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return;
	}
	
	/* we need to continue the connection */
	
	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		/* we respond to the proxy which will finalise connection */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionNo() Direct End Connection Cleaning up";
		std::cerr << std::endl;
#endif
		bdToken transId;
		genNewTransId(&transId);

		int status = BITDHT_CONNECT_ANSWER_NOK;
		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		msgout_connect_genmsg(&(conn->mSrcId), &transId, msgtype, 
							  &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);
		
		cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));
		return;
	}

	if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		/* we respond to the proxy which will finalise connection */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::AuthConnectionNo() Proxy End Connection Cleaning up";
		std::cerr << std::endl;
#endif
		bdToken transId;
		genNewTransId(&transId);

		int status = BITDHT_CONNECT_ANSWER_NOK;
		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		msgout_connect_genmsg(&(conn->mProxyId), &transId, msgtype, 
							  &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);
		
		cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));

		return;
	}

	/* otherwise we are the proxy (for either), reply FAIL */
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::AuthConnectionNo() Proxy Mid Connection Cleaning up";
	std::cerr << std::endl;
#endif
	bdToken transId;
	genNewTransId(&transId);

	int status = BITDHT_CONNECT_ANSWER_NOK;
	int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
	msgout_connect_genmsg(&(conn->mSrcId), &transId, msgtype, 
						  &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);

	cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));

	return;	
}


	


void bdNode::iterateConnections()
{
	std::map<bdProxyTuple, bdConnection>::iterator it;
	std::list<bdProxyTuple> eraseList;
	time_t now = time(NULL);
	
	for(it = mConnections.begin(); it != mConnections.end(); it++)
	{
		if (now - it->second.mLastEvent > BD_CONNECTION_MAX_TIMEOUT)
		{
			/* cleanup event */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdNode::iterateConnections() Connection Timed Out: " << (it->first);
			std::cerr << std::endl;
#endif
			eraseList.push_back(it->first);
			continue;
		}

		if ((it->second.mState == BITDHT_CONNECTION_WAITING_ACK) &&
			(now - it->second.mLastStart > BD_CONNECTION_START_RETRY_PERIOD))
		{
			if (it->second.mRetryCount > BD_CONNECTION_START_MAX_RETRY)
			{
#ifdef DEBUG_NODE_CONNECTION
				std::cerr << "bdNode::iterateConnections() Start/ACK cycle, Too many iterations: " << it->first;
				std::cerr << std::endl;
#endif
				/* connection failed! cleanup */
				callbackConnect(&(it->second.mSrcId),&(it->second.mProxyId),&(it->second.mDestId),
							it->second.mMode, it->second.mPoint, BITDHT_CONNECT_CB_FAILED);

				/* add to erase list */
				eraseList.push_back(it->first);
			}
			else
			{
#ifdef DEBUG_NODE_CONNECTION
				std::cerr << "bdNode::iterateConnections() Start/ACK cycle, Retransmitting START: " << it->first;
				std::cerr << std::endl;
#endif
				it->second.mLastStart = now;
				it->second.mRetryCount++;
				if (!it->second.mSrcAck)
				{
					bdToken transId;
					genNewTransId(&transId);

					int msgtype = BITDHT_MSG_TYPE_CONNECT_START;
					msgout_connect_genmsg(&(it->second.mSrcId), &transId, msgtype, 
										  &(it->second.mSrcConnAddr), &(it->second.mDestConnAddr), 
										  it->second.mMode, it->second.mBandwidth);
				}
				if (!it->second.mDestAck)
				{
					bdToken transId;
					genNewTransId(&transId);

					int msgtype = BITDHT_MSG_TYPE_CONNECT_START;
					msgout_connect_genmsg(&(it->second.mDestId), &transId, msgtype, 
										  &(it->second.mSrcConnAddr), &(it->second.mDestConnAddr), 
										  it->second.mMode, it->second.mBandwidth);
				}
			}
		}
	}

	/* clean up */
	while(eraseList.size() > 0)
	{
		bdProxyTuple tuple = eraseList.front();
		eraseList.pop_front();

		std::map<bdProxyTuple, bdConnection>::iterator eit = mConnections.find(tuple);
		mConnections.erase(eit);
	}
}




/************************************************************************************************************
****************************************** Callback Functions    ********************************************
************************************************************************************************************/


void bdNode::callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int point, int cbtype)
{
	/* This is overloaded at a higher level */
}


/************************************************************************************************************
************************************** ProxyTuple + Connection State ****************************************
************************************************************************************************************/

int operator<(const bdProxyTuple &a, const bdProxyTuple &b)
{
	if (a.srcId < b.srcId)
	{
		return 1;
	}
	
	if (a.srcId == b.srcId)
	{
 		if (a.proxyId < b.proxyId)
		{
			return 1;
		}
 		else if (a.proxyId == b.proxyId)
		{
			if (a.destId < b.destId)
			{
				return 1;
			}
		}
	}
	return 0;
}

int operator==(const bdProxyTuple &a, const bdProxyTuple &b)
{
	if ((a.srcId == b.srcId) && (a.proxyId == b.proxyId) && (a.destId == b.destId))
	{
		return 1;
	}
	return 0;
}

std::ostream &operator<<(std::ostream &out, const bdProxyTuple &t)
{
	out << "[---";
	bdStdPrintNodeId(out, &(t.srcId));
	out << "---";
	bdStdPrintNodeId(out, &(t.proxyId));
	out << "---";
	bdStdPrintNodeId(out, &(t.destId));
	out << "---]";

	return out;
}

bdConnection::bdConnection()
{
	///* Connection State, and TimeStamp of Update */
	//int mState;
	//time_t mLastEvent;
	//
	///* Addresses of Start/Proxy/End Nodes */
	//bdId mSrcId;
	//bdId mDestId;
	//bdId mProxyId;
	//
	///* Where we are in the connection,
	//* and what connection mode.
	//*/
	//int mPoint;
	//int mMode;
	//
	///* must have ip:ports of connection ends (if proxied) */
	//bdId mSrcConnAddr;
	//bdId mDestConnAddr;
	//
	//int mBandwidth;
	//
	///* START/ACK Finishing ****/
	//time_t mLastStart;   /* timer for retries */
	//int mRetryCount;     /* retry counter */
	//
	//bool mSrcAck;
	//bool mDestAck;
	//
	//// Completion TS.
	//time_t mCompletedTS;
}

	/* heavy check, used to check for alternative connections, coming from other direction
	 * Caller must switch src/dest to use it properly (otherwise it'll find your connection!)
	 */
bdConnection *bdNode::findSimilarConnection(bdNodeId *srcId, bdNodeId *destId)
{
	std::map<bdProxyTuple, bdConnection>::iterator it;
	for(it = mConnections.begin(); it != mConnections.end(); it++)
	{
		if ((it->first.srcId == *srcId) && (it->first.destId == *destId))
		{
			/* found similar connection */
			return &(it->second);
		}
	}
	return NULL;
}

bdConnection *bdNode::findExistingConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);

	std::cerr << "bdNode::findExistingConnection() Looking For: " << tuple << std::endl;

	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdNode::findExistingConnection() Failed to Find: " << tuple << std::endl;
		return NULL;
	}

	std::cerr << "bdNode::findExistingConnection() Found: " << tuple << std::endl;
	return &(it->second);
}

bdConnection *bdNode::newConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);
	bdConnection conn;

	std::cerr << "bdNode::newConnection() Installing: " << tuple << std::endl;

	mConnections[tuple] = conn;
	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdNode::newConnection() ERROR Installing: " << tuple << std::endl;
		return NULL;
	}
	return &(it->second);
}

int bdNode::cleanConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);
	bdConnection conn;

	std::cerr << "bdNode::cleanConnection() Removing: " << tuple << std::endl;

	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdNode::cleanConnection() ERROR Removing: " << tuple << std::endl;
		return 0;
	}
	mConnections.erase(it);

	return 1;
}


int bdNode::determinePosition(bdNodeId *sender, bdNodeId *src, bdNodeId *dest)
{
	int pos =  BD_PROXY_CONNECTION_UNKNOWN_POINT;
	if (mOwnId == *src)
	{
		pos = BD_PROXY_CONNECTION_START_POINT;
	}
	else if (mOwnId == *dest)
	{
		pos = BD_PROXY_CONNECTION_END_POINT;
	}
	else
	{
		pos = BD_PROXY_CONNECTION_MID_POINT;
	}
	return pos;	
}

int bdNode::determineProxyId(bdNodeId *sender, bdNodeId *src, bdNodeId *dest, bdNodeId *proxyId)
{
	int pos = determinePosition(sender, src, dest);
	switch(pos)
	{
		case BD_PROXY_CONNECTION_START_POINT:
		case BD_PROXY_CONNECTION_END_POINT:
			*proxyId = *sender;
			return 1;
			break;
		default:
		case BD_PROXY_CONNECTION_MID_POINT:
			*proxyId = mOwnId;
			return 1;
			break;
	}
	return 0;
}



bdConnection *bdNode::findExistingConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return findExistingConnection(srcId, &proxyId, destId);
}

bdConnection *bdNode::newConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return newConnection(srcId, &proxyId, destId);
}


int bdNode::cleanConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return cleanConnection(srcId, &proxyId, destId);
}


/************************************************************************************************************
****************************************** Received Connect Msgs ********************************************
************************************************************************************************************/


/* This function is triggered by a CONNECT_REQUEST message.
 * it will occur on both the Proxy/Dest in the case of a Proxy (PROXY | RELAY) and on the Dest (DIRECT) nodes.
 *
 * In all cases, we store the request and ask for authentication.
 *
 */



int bdNode::recvedConnectionRequest(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdNode::recvedConnectionRequest()";
	std::cerr << std::endl;
#endif
	/* Check for existing Connection */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::recvedConnectionRequest() ERROR EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* Switch the order of peers around to test for "opposite connections" */
	if (NULL != findSimilarConnection(&(destConnAddr->id), &(srcConnAddr->id)))
	{
		std::cerr << "bdNode::recvedConnectionRequest() Found Similar Connection. Replying NO";
		std::cerr << std::endl;

		/* reply existing connection */
		bdToken transId;
		genNewTransId(&transId);

		int status = BITDHT_CONNECT_ANSWER_NOK;
		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		msgout_connect_genmsg(id, &transId, msgtype, srcConnAddr, destConnAddr, mode, status);
		return 0;
	}

	/* INSTALL a NEW CONNECTION */
	conn = bdNode::newConnectionBySender(id, srcConnAddr, destConnAddr);

	int point = 0;
	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::recvedConnectionRequest() Installing DIRECT CONNECTION";
		std::cerr << std::endl;
#endif

		/* we are actually the end node, store stuff, get auth and on with it! */
		point = BD_PROXY_CONNECTION_END_POINT;

		conn->ConnectionRequestDirect(id, srcConnAddr, destConnAddr);

		callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH);
	}
	else
	{
		/* check if we are proxy, or end point */
		bool areProxy = (srcConnAddr->id == id->id);
		if (areProxy)
		{
			std::cerr << "bdNode::recvedConnectionRequest() We are MID Point for Proxy / Relay Connection.";
			std::cerr << std::endl;

			point = BD_PROXY_CONNECTION_MID_POINT;

			/* SEARCH for IP:Port of destination before AUTH  */
			int numNodes = 10;
			std::list<bdId> matchingIds;

			std::cerr << "bdNode::recvedConnectionRequest() WARNING searching for \"VERSION\" flag... TO FIX LATER";
			std::cerr << std::endl;

			uint32_t with_flag = BITDHT_PEER_STATUS_DHT_ENGINE_VERSION;
			//BITDHT_PEER_STATUS_DHT_APPL | BITDHT_PEER_STATUS_DHT_APPL_VERSION);

			bool proxyOk = false;
			bdId destId;

			if (mNodeSpace.find_node(&(destConnAddr->id), numNodes, matchingIds, with_flag))
			{
				std::cerr << "bdNode::recvedConnectionRequest() Found Suitable Destination Addr";
				std::cerr << std::endl;

				if (matchingIds.size() > 1)
				{
					/* WARNING multiple matches */
					std::cerr << "bdNode::recvedConnectionRequest() WARNING Found Multiple Matching Destination Addr";
					std::cerr << std::endl;
				}

				proxyOk = true;
				destId = matchingIds.front();
			}

			if (proxyOk)
			{
				std::cerr << "bdNode::recvedConnectionRequest() Proxy Addr Ok: ";
				bdStdPrintId(std::cerr, destConnAddr);
				std::cerr << "asking for AUTH to continue";
				std::cerr << std::endl;

				conn->ConnectionRequestProxy(id, srcConnAddr, &mOwnId, &destId, mode);

				callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH);
			}
			else
			{
				/* clean up connection... its not going to work */
				std::cerr << "bdNode::recvedConnectionRequest() ERROR No Proxy Addr, Shutting Connect Attempt";
				std::cerr << std::endl;


				/* send FAIL message to SRC */
				bdToken transId;
				genNewTransId(&transId);

				int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
				int status = BITDHT_CONNECT_ANSWER_NOK; /* NO DEST ADDRESS */
				msgout_connect_genmsg(id, &transId, msgtype, srcConnAddr, destConnAddr, mode, status);

				/* remove connection */
				bdNode::cleanConnectionBySender(id, srcConnAddr, destConnAddr);

			}
		}
		else
		{
			std::cerr << "bdNode::recvedConnectionRequest() END Proxy/Relay Connection, asking for AUTH to continue";
			std::cerr << std::endl;

			point = BD_PROXY_CONNECTION_END_POINT;

			conn->ConnectionRequestEnd(id, srcConnAddr, destConnAddr, mode);

			callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH);
		}
	}
	return 1;
}


/* This function is triggered by a CONNECT_REPLY message.
 * it will occur on either the Proxy or Source. And indicates YES / NO to the connection, 
 * as well as supplying address info to the proxy.
 *
 */

int bdNode::recvedConnectionReply(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status)
{
	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::recvedConnectionReply() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	switch(conn->mPoint)
	{
		case BD_PROXY_CONNECTION_START_POINT:
		case BD_PROXY_CONNECTION_END_POINT:		/* NEVER EXPECT THIS */
		case BD_PROXY_CONNECTION_UNKNOWN_POINT:		/* NEVER EXPECT THIS */
		default:					/* NEVER EXPECT THIS */
		{


			/* Only situation we expect this, is if the connection is not allowed.
			 * DEST has sent back an ERROR Message
			 */

			if ((status == BITDHT_CONNECT_ANSWER_NOK) && (conn->mPoint == BD_PROXY_CONNECTION_START_POINT))
			{
				/* connection is killed */
				std::cerr << "bdNode::recvedConnectionReply() Connection Rejected, Killing It: "; 
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;

			}
			else
			{
				/* ERROR in protocol */
				std::cerr << "bdNode::recvedConnectionReply() ERROR Unexpected Message, Killing It: ";
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;
			}

			/* do Callback for Failed Connection */
			callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_FAILED);

			/* Kill Connection always */
			cleanConnectionBySender(id, srcConnAddr, destConnAddr);

			return 0;
		}
			break;

		case BD_PROXY_CONNECTION_MID_POINT:
		{
			 /*    We are proxy. and OK / NOK for connection proceed.
			  */

			if ((status == BITDHT_CONNECT_ANSWER_OKAY) && (conn->mState == BITDHT_CONNECTION_WAITING_REPLY))
			{
				/* OK, continue connection! */
				std::cerr << "bdNode::recvedConnectionReply() @MIDPOINT. Reply + State OK, continuing connection";
				std::cerr << std::endl;

				/* Upgrade Connection to Finishing Mode */
				conn->upgradeProxyConnectionToFinish(id, srcConnAddr, destConnAddr, mode, status);

				/* do Callback for Pending Connection */
				callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
								conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_PENDING);

				return 1;
			}
			else
			{

				std::cerr << "bdNode::recvedConnectionReply() @MIDPOINT Some ERROR, Killing It: ";
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;

				/* do Callback for Failed Connection */
				callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
								conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_FAILED);

				/* send on message to SRC */
				bdToken transId;
				genNewTransId(&transId);

				int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
				msgout_connect_genmsg(&(conn->mSrcId), &transId, msgtype, &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);

				/* connection is killed */
				cleanConnectionBySender(id, srcConnAddr, destConnAddr);
			}
			return 0;
		}
			break;
	}
	return 0;
}


/* This function is triggered by a CONNECT_START message.
 * it will occur on both the Src/Dest in the case of a Proxy (PROXY | RELAY) and on the Src (DIRECT) nodes.
 *
 * parameters are checked against pending connections.
 *  Acks are set, and connections completed if possible (including callback!).
 */

int bdNode::recvedConnectionStart(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int bandwidth)
{
	std::cerr << "bdNode::recvedConnectionStart()";
	std::cerr << std::endl;

	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::recvedConnectionStart() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}


	if (conn->mPoint == BD_PROXY_CONNECTION_MID_POINT)
	{
		std::cerr << "bdNode::recvedConnectionStart() ERROR We Are Connection MID Point";
		std::cerr << std::endl;
		/* ERROR */
	}

	/* check state */
	if ((conn->mState != BITDHT_CONNECTION_WAITING_START) && (conn->mState != BITDHT_CONNECTION_COMPLETED))
	{
		/* ERROR */
		std::cerr << "bdNode::recvedConnectionStart() ERROR State != WAITING_START && != COMPLETED";
		std::cerr << std::endl;

		return 0;
	}

	/* ALL Okay, Send ACK */
	std::cerr << "bdNode::recvedConnectionStart() Passed basic tests, Okay to send ACK";
	std::cerr << std::endl;

	bdToken transId;
	genNewTransId(&transId);

	int msgtype = BITDHT_MSG_TYPE_CONNECT_ACK;
	int status = BITDHT_CONNECT_ANSWER_OKAY;
	msgout_connect_genmsg(id, &transId, msgtype, &(conn->mSrcId), &(conn->mDestId), mode, status);

	/* do complete Callback */

	/* flag as completed */
	if (conn->mState != BITDHT_CONNECTION_COMPLETED)
	{
		std::cerr << "bdNode::recvedConnectionStart() Switching State to COMPLETED, doing callback";
		std::cerr << std::endl;

		conn->CompleteConnection(id, srcConnAddr, destConnAddr);

		callbackConnect(&(conn->mSrcConnAddr),&(conn->mProxyId),&(conn->mDestConnAddr),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_START);

	}
	else
	{
		std::cerr << "bdNode::recvedConnectionStart() Just sent duplicate ACK";
		std::cerr << std::endl;
	}
	/* don't delete, if ACK is lost, we want to be able to re-respond */

	return 1;
}


/* This function is triggered by a CONNECT_ACK message.
 * it will occur on both the Proxy (PROXY | RELAY) and on the Dest (DIRECT) nodes.
 *
 * parameters are checked against pending connections.
 *  Acks are set, and connections completed if possible (including callback!).
 */

int bdNode::recvedConnectionAck(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdNode::recvedConnectionAck() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	if (conn->mPoint == BD_PROXY_CONNECTION_START_POINT)
	{
		/* ERROR */
		std::cerr << "bdNode::recvedConnectionAck() ERROR ACK received at START POINT";
		std::cerr << std::endl;

		return 0;
	}

	/* check state */
	if (conn->mState != BITDHT_CONNECTION_WAITING_ACK)
	{
		/* ERROR */
		std::cerr << "bdNode::recvedConnectionAck() conn->mState != WAITING_ACK, actual State: " << conn->mState;
		std::cerr << std::endl;

		return 0;
	}

	if (id->id == srcConnAddr->id)
	{
		std::cerr << "bdNode::recvedConnectionAck() from Src, marking So";
		std::cerr << std::endl;

		/* recved Ack from source */
		conn->mSrcAck = true;
	}
	else if (id->id == destConnAddr->id)
	{
		std::cerr << "bdNode::recvedConnectionAck() from Dest, marking So";
		std::cerr << std::endl;
		/* recved Ack from dest */
		conn->mDestAck = true;
	}

	if (conn->mSrcAck && conn->mDestAck)
	{
		std::cerr << "bdNode::recvedConnectionAck() ACKs from Both Src & Dest, Connection Complete: callback & cleanup";
		std::cerr << std::endl;

		/* connection complete! cleanup */
		if (conn->mMode == BITDHT_CONNECT_MODE_DIRECT)
		{
			int mode = conn->mMode | BITDHT_CONNECT_ANSWER_OKAY;
			/* callback to connect to Src address! */
			// Slightly different callback, use ConnAddr for start message!
			callbackConnect(&(conn->mSrcConnAddr),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_START);

		}
		else
		{
			int mode = conn->mMode | BITDHT_CONNECT_ANSWER_OKAY;
			callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_PROXY);

		}

		/* Finished Connection! */
		cleanConnectionBySender(id, srcConnAddr, destConnAddr);
	}
	return 1;
}



/************************************************************************************************************
********************************* Connection / ConnectionRequest Functions **********************************
************************************************************************************************************/

// Initialise a new Connection (request by User)

// Any Connection initialised at Source (START_POINT), prior to Auth.
int bdConnection::ConnectionSetup(bdId *proxyId, bdId *srcConnAddr, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_START; /* or REPLY, no AUTH required */
	mLastEvent = time(NULL);
	mSrcId = *srcConnAddr;    /* self, IP unknown */
	mDestId = *destId;  /* dest, IP unknown */
	mProxyId =  *proxyId;  /* full proxy/dest address */

	mPoint = BD_PROXY_CONNECTION_START_POINT;
	mMode = mode;

	mSrcConnAddr = *srcConnAddr; /* self, full ID/IP */
	mDestConnAddr = *destId; /* IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}

int bdConnection::ConnectionSetupDirect(bdId *destId, bdId *srcConnAddr)
{
	mState = BITDHT_CONNECTION_WAITING_START; /* or REPLY, no AUTH required */
	mLastEvent = time(NULL);
	mSrcId = *srcConnAddr;    /* self, IP unknown */
	mDestId = *destId;  /* full proxy/dest address */
	mProxyId =  *destId;  /* full proxy/dest address */

	mPoint = BD_PROXY_CONNECTION_START_POINT;
	mMode = BITDHT_CONNECT_MODE_DIRECT;

	mSrcConnAddr = *srcConnAddr; /* self, full ID/IP */
	mDestConnAddr = *destId; /* IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}



// Initialise a new Connection. (receiving a Connection Request)
// Direct Connection initialised at Destination (END_POINT), prior to Auth.
int bdConnection::ConnectionRequestDirect(bdId *id, bdId *srcConnAddr, bdId *destId)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *id;       /* peer ID/IP known */
	mDestId = *destId;  /* self, IP unknown */
	mProxyId = *id;  /* src ID/IP known */

	mPoint = BD_PROXY_CONNECTION_END_POINT;
	mMode = BITDHT_CONNECT_MODE_DIRECT;

	mSrcConnAddr = *srcConnAddr; /* connect address ID/IP known */
	mDestConnAddr = *destId; /* self IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


// Proxy Connection initialised at Proxy (MID_POINT), prior to Auth.
int bdConnection::ConnectionRequestProxy(bdId *id, bdId *srcConnAddr, bdNodeId *ownId, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *id;		/* ID/IP Known */
	mDestId = *destId;  /* destination, ID/IP known  */
	mProxyId.id =  *ownId;  /* own id, must be set for callback, IP Unknown */

	mPoint = BD_PROXY_CONNECTION_MID_POINT;
	mMode = mode;

	mSrcConnAddr = *srcConnAddr;
	mDestConnAddr = *destId; /* other peer, IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


// Proxy Connection initialised at Destination (END_POINT), prior to Auth.
int bdConnection::ConnectionRequestEnd(bdId *id, bdId *srcId, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *srcId;   /* src IP unknown */
	mDestId = *destId;  /* self, IP unknown */
	mProxyId = *id;  /* src of message, full ID/IP of proxy */

	mPoint = BD_PROXY_CONNECTION_END_POINT;
	mMode = mode;

	mSrcConnAddr = *srcId;   /* ID, not IP */
	mDestConnAddr = *destId; /* ID, not IP */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mSrcConnAddr.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}

// Received AUTH, step up to next stage.
// Search for dest ID/IP is done before AUTH. so actually nothing to do here, except set the state
int bdConnection::AuthoriseProxyConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_REPLY;
	mLastEvent = time(NULL);

	//mSrcId, (peer) (ID/IP known)
	//mDestId (other peer) (ID/IP known)
	//mProxyId (self) (IP unknown)

	// mPoint, mMode should be okay.

	// mSrcConnAddr (ID/IP known)
	// mDestConnAddr is still pending.

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


/* we are end of a Proxy Connection */
int bdConnection::AuthoriseEndConnection(bdId *srcId, bdId *proxyId, bdId *destConnAddr, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_START;
	mLastEvent = time(NULL);

	//mSrcId, (peer) should be okay. (IP unknown)
	//mDestId (self) doesn't matter. (IP unknown)
	//mProxyId (peer) should be okay. (ID/IP known)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay. (IP unknown)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mSrcConnAddr.addr)); 


	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = true; // Automatic ACK, as it is from us.
	mCompletedTS = 0;
	
	return 1;
}




// Auth of the Direct Connection, means we move straight to WAITING_ACK mode.
int bdConnection::AuthoriseDirectConnection(bdId *srcId, bdId *proxyId, bdId *destConnAddr, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_ACK;
	mLastEvent = time(NULL);

	//mSrcId, (peer) should be okay. (ID/IP known)
	//mDestId (self) doesn't matter. (IP Unknown)
	//mProxyId (peer) should be okay. (ID/IP known)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay.  (ID/IP known)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mDestId.addr)); 

	// Should we check for these? This will not help for the Dest address, as we just blanked it above! */
	checkForDefaultConnectAddress();

	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = true; // Automatic ACK, as it is from us.
	mCompletedTS = 0;
	
	return 1;
}

// Proxy Connection => at Proxy, Ready to send out Start and get back ACKs!!
int bdConnection::upgradeProxyConnectionToFinish(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status)
{
	mState = BITDHT_CONNECTION_WAITING_ACK;
	mLastEvent = time(NULL);

	//mSrcId,mDestId should be okay. (ID/IP okay)
	//mProxyId, not set, doesn't matter. (IP Unknown)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay. (ID/IP known)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 

	checkForDefaultConnectAddress();

	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = false;
	mCompletedTS = 0;

	return 1;
}


// Final Sorting out of Addresses.
int bdConnection::CompleteConnection(bdId *id, bdId *srcConnAddr, bdId *destConnAddr)
{
	/* Store Final Addresses */
	time_t now = time(NULL);

	mState = BITDHT_CONNECTION_COMPLETED;
	mCompletedTS = now;
	mLastEvent = now;

	// Received Definitive Final Addresses from Proxy.
	// These have to be done by proxy, as its the only one who know both our addresses.

	mSrcConnAddr = *srcConnAddr;
	mDestConnAddr = *destConnAddr;

	checkForDefaultConnectAddress();

	return 1;
}



int bdConnection::checkForDefaultConnectAddress()
{
	// We can check if the DestConnAddr / SrcConnAddr are real.
	// If there is nothing there, we assume that that want to connect on the 
	// same IP:Port as the DHT Node.

	if (mSrcConnAddr.addr.sin_addr.s_addr == 0)
	{
		std::cerr << "bdNode::checkForDefaultConnectAddress() SrcConnAddr.addr is BLANK, installing Dht Node Address";
		std::cerr << std::endl;

		mSrcConnAddr.addr = mSrcId.addr;
	}

	if (mDestConnAddr.addr.sin_addr.s_addr == 0)
	{
		std::cerr << "bdNode::checkForDefaultConnectAddress() DestConnAddr.addr is BLANK, installing Dht Node Address";
		std::cerr << std::endl;

		mDestConnAddr.addr = mDestId.addr;
	}

	return 1;

}


int bdConnectionRequest::setupDirectConnection(struct sockaddr_in *laddr, bdNodeId *target)
{
	mState = BITDHT_CONNREQUEST_INIT;
	mStateTS = time(NULL);
	mTarget = *target;
	mLocalAddr = *laddr;
	mMode = BITDHT_CONNECT_MODE_DIRECT;

	return 1;
}

int bdConnectionRequest::setupProxyConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{
	mState = BITDHT_CONNREQUEST_INIT;
	mStateTS = time(NULL);
	mTarget = *target;
	mLocalAddr = *laddr;
	mMode = mode;

	return 1;
}

int bdConnectionRequest::addPotentialProxy(bdId *srcId)
{
	std::cerr << "bdConnectionRequest::addPotentialProxy() ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << std::endl;

	std::list<bdId>::iterator it = std::find(mPeersTried.begin(), mPeersTried.end(), *srcId);
	if (it == mPeersTried.end())
	{
		it = std::find(mPotentialProxies.begin(), mPotentialProxies.end(), *srcId);
		if (it == mPotentialProxies.end())
		{
			mPotentialProxies.push_back(*srcId);
			return 1;
		}
		else
		{
			std::cerr << "bdConnectionRequest::addPotentialProxy() Duplicate in mPotentialProxies List";
			std::cerr << std::endl;
		}
	}
	else
	{
		std::cerr << "bdConnectionRequest::addPotentialProxy() Already tried this peer";
		std::cerr << std::endl;
	}
	return 0;
}


std::ostream &operator<<(std::ostream &out, const bdConnectionRequest &req)
{
	out << "bdConnectionRequest: ";
	out << "State: " << req.mState;
	out << std::endl;
	out << "PotentialProxies:";
	out << std::endl;

        std::list<bdId>::const_iterator it;
	for(it = req.mPotentialProxies.begin(); it != req.mPotentialProxies.end(); it++)
	{
		out << "\t";
		bdStdPrintId(out, &(*it));
		out << std::endl;
	}
	return out;
}

std::ostream &operator<<(std::ostream &out, const bdConnection &conn)
{
	time_t now = time(NULL);
	out << "bdConnection: ";
	out << "State: " << conn.mState;
	out << " LastEvent: " << now -  conn.mLastEvent;
	out << " Point: " << conn.mPoint;
	out << " Mode: " << conn.mMode;
	out << std::endl;

	out << "\tsrcId: ";
	bdStdPrintId(out, &(conn.mSrcId));
	out << std::endl;
	out << "\tproxyId: ";
	bdStdPrintId(out, &(conn.mProxyId));
	out << std::endl;
	out << "\tdestId: ";
	bdStdPrintId(out, &(conn.mDestId));
	out << std::endl;

	out << "\tsrcConnAddr: ";
	bdStdPrintId(out, &(conn.mSrcConnAddr));
	out << std::endl;

	out << "\tdestConnAddr: ";
	bdStdPrintId(out, &(conn.mDestConnAddr));
	out << std::endl;

	out << "\tretryCount: " << conn.mRetryCount;
	out << " retryCount: " << conn.mLastStart;
	out << " srcAck: " << conn.mSrcAck;
	out << " destAck: " << conn.mDestAck;
	out << " completedTS: " << now - conn.mCompletedTS;
	out << std::endl;

	return out;

}


