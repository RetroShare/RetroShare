/*******************************************************************************
 * bitdht/bdconnection.h                                                       *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef BITDHT_CONNECTION_H
#define BITDHT_CONNECTION_H

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

#define BITDHT_CONNREQUEST_TIMEOUT_CONNECT	300  // MAKE THIS LARGE - SHOULD NEVER HAPPEN.
#define BITDHT_CONNREQUEST_TIMEOUT_INPROGRESS	30
#define BITDHT_CONNREQUEST_MAX_AGE		60


#define BITDHT_CONNECTION_WAITING_AUTH		1
#define BITDHT_CONNECTION_WAITING_REPLY		2
#define BITDHT_CONNECTION_WAITING_START		3
#define BITDHT_CONNECTION_WAITING_ACK		4
#define BITDHT_CONNECTION_COMPLETED		5


#define BD_CONNECTION_START_RETRY_PERIOD	3  // Should only take a couple of seconds to get reply.
#define BD_CONNECTION_START_MAX_RETRY		3
#define BD_CONNECTION_MAX_TIMEOUT		20 /* should be quick */



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
	int ConnectionSetup(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delay);
	int ConnectionSetupDirect(bdId *destId, bdId *srcConnAddr);

	// Initialise a new Connection. (receiving a Connection Request)
	int ConnectionRequestDirect(bdId *id, bdId *srcConnAddr, bdId *destConnAddr);
	int ConnectionRequestProxy(bdId *id, bdId *srcConnAddr, bdNodeId *ownId, bdId *destConnAddr, int mode, int delay);
	int ConnectionRequestEnd(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode);

	// Setup Finishing Stage, (receiving a Connection Reply).
	int upgradeProxyConnectionToFinish(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delay, int status);

	int AuthoriseDirectConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc);
	int AuthoriseProxyConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc, int bandwidth);
	int AuthoriseEndConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc, int delay);

	int CompleteConnection(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int bandwidth, int delay);

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
	int mMaxDelay;
	time_t mConnectionStartTS;

	/* START/ACK Finishing ****/
	time_t mLastStart;   /* timer for retries */
	int mRetryCount;     /* retry counter */

	bool mSrcAck;
	bool mDestAck;

	// Completion TS.
	time_t mCompletedTS;


};

#define BD_PI_SRC_UNKNOWN					0
#define BD_PI_SRC_QUERYRESULT				1
#define BD_PI_SRC_QUERYPROXY				2
#define BD_PI_SRC_NODESPACE_FRIEND			3
#define BD_PI_SRC_NODESPACE_SERVER			4
#define BD_PI_SRC_NODESPACE_ENGINEVERSION	5
#define BD_PI_SRC_ADDGOODPROXY				6


class bdProxyId
{
public:
	bdProxyId(const bdId &in_id, uint32_t in_srctype, uint32_t in_errcode)
	:id(in_id), srcType(in_srctype), errcode(in_errcode) { return; }

	bdProxyId() :srcType(BD_PI_SRC_UNKNOWN), errcode(0) { return; }

	std::string proxySrcType() const;

	bdId id;
	uint32_t srcType;
	uint32_t errcode;
};


class bdConnectionRequest
{
public:
	bdConnectionRequest() : mMode(0), mState(0), mStateTS(0), mPauseTS(0), mErrCode(0), mDelay(0), mRequestTS(0), mRecycled(0), mCurrentSrcType(0)
	{
		bdsockaddr_clear(&mLocalAddr);
	}

public:
	int setupDirectConnection(struct sockaddr_in *laddr, bdNodeId *target);
	int setupProxyConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay);

	int addGoodProxy(const bdId *srcId);
	int checkGoodProxyPeer(const bdId *Id);

	bdNodeId mTarget;
	struct sockaddr_in mLocalAddr;
	int mMode;

	int mState;
	time_t mStateTS;

	time_t mPauseTS;
	uint32_t mErrCode;
	
	int mDelay;
	time_t mRequestTS;   // reference Time for mDelay.

	std::list<bdProxyId> mGoodProxies;
	std::list<bdId> mPotentialProxies;
	//std::list<bdId> mGoodProxies;
	int mRecycled;

	bdId mCurrentAttempt;
	uint32_t mCurrentSrcType;

	std::list<bdProxyId> mPeersTried;
	//std::list<bdId> mPeersTried;
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

	int requestConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start);
	int requestConnection_direct(struct sockaddr_in *laddr, bdNodeId *target);	
	int requestConnection_proxy(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay);

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
				int mode, int point, int param, int cbtype, int errcode);

	/* Connections: Outgoing */

	int  startConnectionAttempt(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delay);
	void AuthConnectionOk(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc, int bandwidth, int delay);
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
				int mode, int point, int param, int cbtype, int errcode);

	/* Connections: */
	int recvedConnectionRequest(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delay);
	int recvedConnectionReply(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delay, int status);
	int recvedConnectionStart(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int delayOrBandwidth);
	int recvedConnectionAck(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode);

	/* setup Relay Mode */
	void setRelayMode(uint32_t mode);

	private:

	std::map<bdProxyTuple, bdConnection> mConnections;
	std::map<bdNodeId, bdConnectionRequest> mConnectionRequests;

        uint32_t mConfigAllowedModes;
        bool mConfigAutoProxy;

	uint32_t mRelayMode;

	/****************************** Connection Code (in bdconnection.cc) ****************************/

	private:

	bdNodeId mOwnId;
	bdSpace *mNodeSpace;
	bdQueryManager *mQueryMgr;
	bdDhtFunctions *mFns;
	bdNodePublisher *mPub;
};


#endif // BITDHT_CONNECTION_H
