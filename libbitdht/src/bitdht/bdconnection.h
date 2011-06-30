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

class bdQueryManager;
class bdNodePublisher;

/************************************************************************************************************
************************************** ProxyTuple + Connection State ****************************************
************************************************************************************************************/

#define BITDHT_CONNREQUEST_READY		1
#define BITDHT_CONNREQUEST_PAUSED		2
#define BITDHT_CONNREQUEST_INPROGRESS		3
#define BITDHT_CONNREQUEST_EXTCONNECT		4
#define BITDHT_CONNREQUEST_DONE			5

#define BITDHT_CONNREQUEST_TIMEOUT_CONNECT	30
#define BITDHT_CONNREQUEST_TIMEOUT_INPROGRESS	30
#define BITDHT_CONNREQUEST_MAX_AGE		60


#define BITDHT_CONNECTION_WAITING_AUTH		1
#define BITDHT_CONNECTION_WAITING_REPLY		2
#define BITDHT_CONNECTION_WAITING_START		3
#define BITDHT_CONNECTION_WAITING_ACK		4
#define BITDHT_CONNECTION_COMPLETED		5


#define BD_CONNECTION_START_RETRY_PERIOD	5  // Should only take a couple of seconds to get reply.
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

	int addGoodProxy(const bdId *srcId);
	int checkGoodProxyPeer(const bdId *Id);

	bdNodeId mTarget;
	struct sockaddr_in mLocalAddr;
	int mMode;

	int mState;
	time_t mStateTS;

	time_t mPauseTS;
	uint32_t mErrCode;
	

	std::list<bdId> mGoodProxies;
	std::list<bdId> mPotentialProxies;
	int mRecycled;

	bdId mCurrentAttempt;
	std::list<bdId> mPeersTried;
};

std::ostream &operator<<(std::ostream &out, const bdConnectionRequest &req);
std::ostream &operator<<(std::ostream &out, const bdConnection &conn);


/*********
 * The Connection Management Class.
 * this encapsulates all of the functionality..
 * except for a couple of message in/outs + callback.
 */

class bdConnectManager
{
	public:

	bdConnectManager(bdNodeId *ownid, bdSpace *space, bdQueryManager *qmgr, bdDhtFunctions *fns, bdNodePublisher *pub);	


	/* connection functions */
	void requestConnection(bdNodeId *id, uint32_t modes);
	void allowConnection(bdNodeId *id, uint32_t modes);


	/* high level */

        void shutdownConnections();
        void printConnections();

	/* Connections: Configuration */
	void defaultConnectionOptions();
	virtual void setConnectionOptions(uint32_t allowedModes, uint32_t flags);

	/* Connections: Initiation */

	int requestConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t start);
	int requestConnection_direct(struct sockaddr_in *laddr, bdNodeId *target);	
	int requestConnection_proxy(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode);

	int killConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode);

	int checkExistingConnectionAttempt(bdNodeId *target);
	void addPotentialConnectionProxy(const bdId *srcId, const bdId *target);
	void updatePotentialConnectionProxy(const bdId *id, uint32_t mode);

	int checkPeerForFlag(const bdId *id, uint32_t with_flag);

	int tickConnections();
	void iterateConnectionRequests();
	int startConnectionAttempt(bdConnectionRequest *req);

	// internal Callback -> normally continues to callbackConnect().
	void callbackConnectRequest(bdId *srcId, bdId *proxyId, bdId *destId, 
				int mode, int point, int cbtype, int errcode);

	/* Connections: Outgoing */

	int  startConnectionAttempt(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode);
	void AuthConnectionOk(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc);
	void AuthConnectionNo(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc, int errcode);
	void iterateConnections();


	/* Connections: Utility State */

	bdConnection *findExistingConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId);
	bdConnection *newConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId);
	int cleanConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId);

	int determinePosition(bdNodeId *sender, bdNodeId *src, bdNodeId *dest);
	int determineProxyId(bdNodeId *sender, bdNodeId *src, bdNodeId *dest, bdNodeId *proxyId);

	bdConnection *findSimilarConnection(bdNodeId *srcId, bdNodeId *destId);
	bdConnection *findExistingConnectionBySender(bdId *sender, bdId *src, bdId *dest);
	bdConnection *newConnectionBySender(bdId *sender, bdId *src, bdId *dest);
	int cleanConnectionBySender(bdId *sender, bdId *src, bdId *dest);

	// Overloaded Generalised Connection Callback.
	virtual void callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId, 
				int mode, int point, int cbtype, int errcode);

	/* Connections: */
	int recvedConnectionRequest(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode);
	int recvedConnectionReply(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status);
	int recvedConnectionStart(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int bandwidth);
	int recvedConnectionAck(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode);

	private:

	std::map<bdProxyTuple, bdConnection> mConnections;
	std::map<bdNodeId, bdConnectionRequest> mConnectionRequests;

        uint32_t mConfigAllowedModes;
        bool mConfigAutoProxy;

	/****************************** Connection Code (in bdconnection.cc) ****************************/

	private:

	bdNodeId mOwnId;
	bdSpace *mNodeSpace;
	bdQueryManager *mQueryMgr;
	bdDhtFunctions *mFns;
	bdNodePublisher *mPub;
};


#endif // BITDHT_CONNECTION_H
