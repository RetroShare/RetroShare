#ifndef BITDHT_CONNECTION_H
#define BITDHT_CONNECTION_H

/*
 * bitdht/bdconnection.h
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


#include "bitdht/bdiface.h"


/************************************************************************************************************
************************************** ProxyTuple + Connection State ****************************************
************************************************************************************************************/

#define BITDHT_CONNREQUEST_INIT			1
#define BITDHT_CONNREQUEST_INPROGRESS		2
#define BITDHT_CONNREQUEST_DONE			3

#define BITDHT_CONNREQUEST_MAX_AGE		60


#define BITDHT_CONNECTION_WAITING_AUTH		1
#define BITDHT_CONNECTION_WAITING_REPLY		2
#define BITDHT_CONNECTION_WAITING_START		3
#define BITDHT_CONNECTION_WAITING_ACK		4
#define BITDHT_CONNECTION_COMPLETED		5


#define BD_CONNECTION_START_RETRY_PERIOD	10
#define BD_CONNECTION_START_MAX_RETRY		3
#define BD_CONNECTION_MAX_TIMEOUT		30



class bdProxyTuple 
{
	public:
	bdProxyTuple() { return; }
	bdProxyTuple(bdNodeId *s, bdNodeId *p, bdNodeId *d)
	:srcId(*s), proxyId(*p), destId(*d) { return; }

	bdNodeId srcId;
	bdNodeId proxyId;
	bdNodeId destId;
};

std::ostream &operator<<(std::ostream &out, const bdProxyTuple &t);
int operator<(const bdProxyTuple &a, const bdProxyTuple &b);
int operator==(const bdProxyTuple &a, const bdProxyTuple &b);


class bdConnection
{
	public:
	bdConnection();

	/** Functions to tweak the connection status */

	// User initialised Connection.
	int ConnectionSetup(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode);
	int ConnectionSetupDirect(bdId *destId, bdId *srcConnAddr);

	// Initialise a new Connection. (receiving a Connection Request)
	int ConnectionRequestDirect(bdId *id, bdId *srcConnAddr, bdId *destConnAddr);
	int ConnectionRequestProxy(bdId *id, bdId *srcConnAddr, bdNodeId *ownId, bdId *destConnAddr, int mode);
	int ConnectionRequestEnd(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode);

	// Setup Finishing Stage, (receiving a Connection Reply).
	int upgradeProxyConnectionToFinish(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status);

	int AuthoriseDirectConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc);
	int AuthoriseProxyConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc);
	int AuthoriseEndConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc);

	int CompleteConnection(bdId *id, bdId *srcConnAddr, bdId *destConnAddr);

	int checkForDefaultConnectAddress();

	/* Connection State, and TimeStamp of Update */
	int mState;
	time_t mLastEvent;

	/* Addresses of Start/Proxy/End Nodes */
	bdId mSrcId;
	bdId mDestId;
	bdId mProxyId;

	/* Where we are in the connection, 
	 * and what connection mode.
	 */
	int mPoint;
	int mMode;

	/* must have ip:ports of connection ends (if proxied) */
	bdId mSrcConnAddr;
	bdId mDestConnAddr;

	int mBandwidth;

	/* START/ACK Finishing ****/
	time_t mLastStart;   /* timer for retries */
	int mRetryCount;     /* retry counter */

	bool mSrcAck;
	bool mDestAck;

	// Completion TS.
	time_t mCompletedTS;


};

class bdConnectionRequest
{
	public:
	int setupDirectConnection(struct sockaddr_in *laddr, bdNodeId *target);
	int setupProxyConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode);

	int addPotentialProxy(bdId *srcId);

	bdNodeId mTarget;
	struct sockaddr_in mLocalAddr;
	int mMode;

	int mState;
	time_t mStateTS;

	std::list<bdId> mPotentialProxies;

	bdId mCurrentAttempt;
	std::list<bdId> mPeersTried;
};

std::ostream &operator<<(std::ostream &out, const bdConnectionRequest &req);
std::ostream &operator<<(std::ostream &out, const bdConnection &conn);

#endif

