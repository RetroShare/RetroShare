/*******************************************************************************
 * bitdht/bdiface.h                                                            *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
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
#ifndef BIT_DHT_INTERFACE_H
#define BIT_DHT_INTERFACE_H

#include <iosfwd>
#include <map>
#include <string>
#include <list>
#include <inttypes.h>

#include "util/bdnet.h"

/*
 * Functions and Classes required for Interfacing with the BitDht.
 * This should be the sole header file required to talk to Dht.
 * ... though setting it up will require including udpbitdht.h as well.
 * 
 */

#define BITDHT_KEY_LEN 20
#define BITDHT_KEY_INTLEN 5
#define BITDHT_KEY_BITLEN 160




#define BITDHT_MAX_PKTSIZE 1024

#define BITDHT_TTL 64

#define BITDHT_SEARCH_ONE_SHOT 		1
#define BITDHT_SEARCH_REPEATING		2

class bdNodeId
{
        public:
        unsigned char data[BITDHT_KEY_LEN];
};

class bdMetric: public bdNodeId {};

class bdId
{
        public:

        bdId();
        bdId(bdNodeId in_id, struct sockaddr_in in_addr);

        struct sockaddr_in addr;
        bdNodeId id;
};

#define BITDHT_LIKELY_SAME_NO			0x00000000
#define BITDHT_LIKELY_SAME_YES			0x00000001
#define BITDHT_LIKELY_SAME_PORT_CHANGED		0x00000002
#define BITDHT_LIKELY_SAME_LOC_CHANGED		0x00000004
#define BITDHT_LIKELY_SAME_IDENTICAL		0x00000008


class bdDhtFunctions
{
	public:

//	bdDhtFunctions();
	/* setup variables */
virtual uint16_t bdNumBuckets() = 0;
virtual uint16_t bdNodesPerBucket() = 0; /* used for bdspace */
virtual uint16_t bdNumQueryNodes() = 0; /* used for queries */
virtual uint16_t bdBucketBitSize() = 0;

virtual int bdDistance(const bdNodeId *n1, const bdNodeId *n2, bdMetric *metric) = 0;
virtual int bdBucketDistance(const bdNodeId *n1, const bdNodeId *n2) = 0;
virtual int bdBucketDistance(const bdMetric *metric) = 0;

virtual bool bdSimilarId(const bdId *id1, const bdId *id2) = 0;
virtual bool bdUpdateSimilarId(bdId *dest, const bdId *src) = 0;

virtual void bdRandomMidId(const bdNodeId *target, const bdNodeId *other, bdNodeId *mid) = 0;

virtual void bdPrintId(std::ostream &out, const bdId *a) = 0;
virtual void bdPrintNodeId(std::ostream &out, const bdNodeId *a) = 0;

};



// DHT MODES
#define BITDHT_MODE_TRAFFIC_MASK	0x00000f00
#define BITDHT_MODE_RELAYSERVER_MASK  	0x0000f000

// These are not ORd - only one can apply.
#define BITDHT_MODE_TRAFFIC_HIGH	0x00000100
#define BITDHT_MODE_TRAFFIC_MED		0x00000200
#define BITDHT_MODE_TRAFFIC_LOW		0x00000300
#define BITDHT_MODE_TRAFFIC_TRICKLE	0x00000400
#define BITDHT_MODE_TRAFFIC_DEFAULT	BITDHT_MODE_TRAFFIC_LOW

// These are not ORd - only one can apply.
#define BITDHT_MODE_RELAYSERVERS_IGNORED	0x00001000
#define BITDHT_MODE_RELAYSERVERS_FLAGGED	0x00002000
#define BITDHT_MODE_RELAYSERVERS_ONLY		0x00003000
#define BITDHT_MODE_RELAYSERVERS_SERVER		0x00004000


/* NODE OPTIONS */
#define BITDHT_OPTIONS_MAINTAIN_UNSTABLE_PORT		0x00000001
#define BITDHT_OPTIONS_ENABLE_RELAYS			0x00000002


/* peer flags
 * order is important!
 * higher bits = more priority.
 * BITDHT_PEER_STATUS_RECVPING
 * BITDHT_PEER_STATUS_RECVPONG
 * BITDHT_PEER_STATUS_RECVNODES
 * BITDHT_PEER_STATUS_RECVHASHES
 * BITDHT_PEER_STATUS_DHT_ENGINE  (dbXXxx)
 * BITDHT_PEER_STATUS_DHT_APPL    (XXRSxx)
 * BITDHT_PEER_STATUS_DHT_VERSION (XXxx50)
 * 
 */

#define 	BITDHT_PEER_STATUS_MASK_RECVD		0x000000ff
#define 	BITDHT_PEER_STATUS_MASK_DHT		0x0000ff00
#define 	BITDHT_PEER_STATUS_MASK_KNOWN		0x00ff0000

#define 	BITDHT_PEER_STATUS_RECV_PING		0x00000001
#define 	BITDHT_PEER_STATUS_RECV_PONG		0x00000002
#define 	BITDHT_PEER_STATUS_RECV_NODES		0x00000004
#define 	BITDHT_PEER_STATUS_RECV_HASHES		0x00000008
#define 	BITDHT_PEER_STATUS_RECV_CONNECT_MSG	0x00000010

#define 	BITDHT_PEER_STATUS_DHT_ENGINE		0x00000100
#define 	BITDHT_PEER_STATUS_DHT_ENGINE_VERSION	0x00000200
#define 	BITDHT_PEER_STATUS_DHT_APPL		0x00000400
#define 	BITDHT_PEER_STATUS_DHT_APPL_VERSION	0x00000800

#define 	BITDHT_PEER_STATUS_DHT_WHITELIST	0x00010000
#define 	BITDHT_PEER_STATUS_DHT_FOF		0x00020000
#define 	BITDHT_PEER_STATUS_DHT_FRIEND		0x00040000
#define 	BITDHT_PEER_STATUS_DHT_RELAY_SERVER	0x00080000	// (Flag must be enabled)
#define 	BITDHT_PEER_STATUS_DHT_SELF		0x00100000	


// EXTRA FLAGS are our internal thoughts about the peer.
#define 	BITDHT_PEER_EXFLAG_MASK_BASIC		0x000000ff
#define 	BITDHT_PEER_EXFLAG_UNSTABLE		0x00000001	// Port changes.
#define 	BITDHT_PEER_EXFLAG_ATTACHED		0x00000002 	// We will ping in heavily. (if unstable)
#define 	BITDHT_PEER_EXFLAG_BADPEER		0x00000004 	// For testing, we flag rather than discard.





#define 	BITDHT_CONNECT_MODE_DIRECT		0x00000001
#define 	BITDHT_CONNECT_MODE_PROXY		0x00000002
#define 	BITDHT_CONNECT_MODE_RELAY		0x00000004

#define 	BITDHT_CONNECT_OPTION_AUTOPROXY		0x00000001

// STATUS CODES. == 0 is okay, != 0 is error.
#define 	BITDHT_CONNECT_ANSWER_OKAY		0x00000000
#define 	BITDHT_CONNECT_ERROR_NONE		(BITDHT_CONNECT_ANSWER_OKAY)

#define 	BITDHT_CONNECT_ERROR_MASK_TYPE		0x0000ffff
#define 	BITDHT_CONNECT_ERROR_MASK_SOURCE	0x00ff0000
#define 	BITDHT_CONNECT_ERROR_MASK_CRMOVE	0xff000000

#define 	BITDHT_CONNECT_ERROR_SOURCE_START	0x00010000
#define 	BITDHT_CONNECT_ERROR_SOURCE_MID		0x00020000
#define 	BITDHT_CONNECT_ERROR_SOURCE_END 	0x00040000
#define 	BITDHT_CONNECT_ERROR_SOURCE_OTHER 	0x00080000

#define 	BITDHT_CONNECT_ERROR_CRMOVE_FATAL 	0x01000000
#define 	BITDHT_CONNECT_ERROR_CRMOVE_NOMOREIDS 	0x02000000
#define 	BITDHT_CONNECT_ERROR_CRMOVE_NEXTID 	0x04000000
#define 	BITDHT_CONNECT_ERROR_CRMOVE_PAUSED 	0x08000000

// ERROR CODES.
#define 	BITDHT_CONNECT_ERROR_GENERIC		0x00000001
#define 	BITDHT_CONNECT_ERROR_PROTOCOL		0x00000002
#define 	BITDHT_CONNECT_ERROR_TIMEOUT		0x00000003
#define 	BITDHT_CONNECT_ERROR_TEMPUNAVAIL	0x00000004   // Haven't got ext address yet.
#define 	BITDHT_CONNECT_ERROR_NOADDRESS		0x00000005   // Can't find the peer in tables.
#define 	BITDHT_CONNECT_ERROR_UNREACHABLE	0x00000006   // Symmetric NAT

#define 	BITDHT_CONNECT_ERROR_UNSUPPORTED	0x00000007
#define 	BITDHT_CONNECT_ERROR_OVERLOADED		0x00000008
#define 	BITDHT_CONNECT_ERROR_AUTH_DENIED	0x00000009
#define 	BITDHT_CONNECT_ERROR_DUPLICATE		0x0000000a

// These are slightly special ones used for CB_REQUEST
#define 	BITDHT_CONNECT_ERROR_TOOMANYRETRY	0x0000000b
#define 	BITDHT_CONNECT_ERROR_OUTOFPROXY		0x0000000c
#define 	BITDHT_CONNECT_ERROR_USER		0x0000000d


/*************/
// FRIEND_ENTRY_FLAGS... used by updateKnownPeers().

#define BD_FRIEND_ENTRY_ONLINE          0x0001
#define BD_FRIEND_ENTRY_ADDR_OK         0x0002

#define BD_FRIEND_ENTRY_WHITELIST       BITDHT_PEER_STATUS_DHT_WHITELIST
#define BD_FRIEND_ENTRY_FOF             BITDHT_PEER_STATUS_DHT_FOF
#define BD_FRIEND_ENTRY_FRIEND          BITDHT_PEER_STATUS_DHT_FRIEND
#define BD_FRIEND_ENTRY_RELAY_SERVER    BITDHT_PEER_STATUS_DHT_RELAY_SERVER

#define BD_FRIEND_ENTRY_SELF            BITDHT_PEER_STATUS_DHT_SELF

#define BD_FRIEND_ENTRY_MASK_KNOWN      BITDHT_PEER_STATUS_MASK_KNOWN




/* Definitions of bdSpace Peer and Bucket are publically available, 
 * so we can expose the bucket entries for the gui.
 */

class bdPeer
{
	public:
	bdPeer():mPeerFlags(0), mLastSendTime(0), mLastRecvTime(0), mFoundTime(0), mExtraFlags(0) { return; }

	bdId   mPeerId;
	uint32_t mPeerFlags;
	time_t mLastSendTime;
	time_t mLastRecvTime;
	time_t mFoundTime;     /* time stamp that peer was found */

	uint32_t mExtraFlags;
};
	
class bdBucket
{
	public:
	
	bdBucket();
	/* list so we can queue properly */
	std::list<bdPeer> entries;
};

class bdQueryStatus
{
        public:
        uint32_t mStatus;
        uint32_t mQFlags;
        std::list<bdId> mResults;
};

class bdQuerySummary
{
        public:

        bdNodeId mId;
        bdMetric mLimit;
        uint32_t mState;
        time_t mQueryTS;
        uint32_t mQueryFlags;
        int32_t mSearchTime;

        int32_t mQueryIdlePeerRetryPeriod; // seconds between retries.

        // closest peers
        std::multimap<bdMetric, bdPeer>  mClosest;
        std::multimap<bdMetric, bdPeer>  mPotentialPeers;
        std::list<bdPeer>  mProxiesUnknown;
        std::list<bdPeer>  mProxiesFlagged;
};




/* Status options */
#define BITDHT_QUERY_READY		1
#define BITDHT_QUERY_QUERYING           2
#define BITDHT_QUERY_FAILURE            3
#define BITDHT_QUERY_FOUND_CLOSEST      4
#define BITDHT_QUERY_PEER_UNREACHABLE   5
#define BITDHT_QUERY_SUCCESS            6

/* Query Flags */
#define BITDHT_QFLAGS_NONE		0x0000
#define BITDHT_QFLAGS_DISGUISE		0x0001  // Don't search directly for target.
#define BITDHT_QFLAGS_DO_IDLE		0x0002
#define BITDHT_QFLAGS_INTERNAL		0x0004  // runs through startup. (limited callback)
#define BITDHT_QFLAGS_UPDATES		0x0008  // Do regular updates.

/* Connect Callback Flags */
#define BITDHT_CONNECT_CB_AUTH		1
#define BITDHT_CONNECT_CB_PENDING	2
#define BITDHT_CONNECT_CB_START		3
#define BITDHT_CONNECT_CB_PROXY		4
#define BITDHT_CONNECT_CB_FAILED	5
#define BITDHT_CONNECT_CB_REQUEST	6

#define BD_PROXY_CONNECTION_UNKNOWN_POINT       0
#define BD_PROXY_CONNECTION_START_POINT         1
#define BD_PROXY_CONNECTION_MID_POINT           2
#define BD_PROXY_CONNECTION_END_POINT           3

#define BITDHT_INFO_CB_TYPE_BADPEER	1

/* Relay Modes */
#define BITDHT_RELAYS_OFF		0
#define BITDHT_RELAYS_ON		1
#define BITDHT_RELAYS_ONLY		2
#define BITDHT_RELAYS_SERVER		3


class BitDhtCallback
{
	public:
//        ~BitDhtCallback();

	// dummy cos not needed for standard dht behaviour;
	virtual int dhtNodeCallback(const bdId *  /*id*/, uint32_t /*peerflags*/)  { return 0; }

	// must be implemented.
	virtual int dhtPeerCallback(const bdId *id, uint32_t status) = 0;
	virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status) = 0;

	// connection callback. Not required for basic behaviour, but forced for initial development.
	virtual int dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
	uint32_t mode, uint32_t point, uint32_t param, uint32_t cbtype, uint32_t errcode) = 0; /*  { return 0; }  */

	// Generic Info callback - initially will be used to provide bad peers.
	virtual int dhtInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info) = 0;

	// ask upper layer whether an IP is banned or not
	// must not be implemented
	// when set it will be used instead of the own ban list
	// return code is used to express availability/absence
	virtual int dhtIsBannedCallback(const sockaddr_in */*addr*/, bool */*isBanned*/) { return 0;}
};


class BitDhtInterface
{
	public:

	/* bad peer notification */
virtual void addBadPeer(const struct sockaddr_in &addr, uint32_t source, uint32_t reason, uint32_t age) = 0;

	/* Friend Tracking */
virtual void updateKnownPeer(const bdId *id, uint32_t type, uint32_t flags) = 0;

        /***** Request Lookup (DHT Peer & Keyword) *****/
virtual void addFindNode(bdNodeId *id, uint32_t mode) = 0;
virtual void removeFindNode(bdNodeId *id) = 0;
virtual void findDhtValue(bdNodeId *id, std::string key, uint32_t mode) = 0;

	/***** Connections Requests *****/
virtual bool ConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start) = 0;
virtual void ConnectionAuth(bdId *srcId, bdId *proxyId, bdId *destId, uint32_t mode, uint32_t loc, 
						uint32_t bandwidth, uint32_t delay, uint32_t answer) = 0;
virtual void ConnectionOptions(uint32_t allowedModes, uint32_t flags) = 0;

virtual bool setAttachMode(bool on) = 0;


        /***** Add / Remove Callback Clients *****/
virtual void addCallback(BitDhtCallback *cb) = 0;
virtual void removeCallback(BitDhtCallback *cb) = 0;

	/***** Get Results Details *****/
virtual int getDhtPeerAddress(const bdNodeId *id, struct sockaddr_in &from) = 0;
virtual int getDhtValue(const bdNodeId *id, std::string key, std::string &value) = 0;
virtual int getDhtBucket(const int idx, bdBucket &bucket) = 0;

virtual int getDhtQueries(std::map<bdNodeId, bdQueryStatus> &queries) = 0;
virtual int getDhtQueryStatus(const bdNodeId *id, bdQuerySummary &query) = 0;

        /* stats and Dht state */
virtual int startDht() = 0;
virtual int stopDht() = 0;
virtual int stateDht() = 0; /* STOPPED, STARTING, ACTIVE, FAILED */
virtual uint32_t statsNetworkSize() = 0;
virtual uint32_t statsBDVersionSize() = 0; /* same version as us! */

virtual uint32_t setDhtMode(uint32_t dhtFlags) = 0;
};


// general helper functions for decoding error messages.
std::string decodeConnectionError(uint32_t errcode);
std::string decodeConnectionErrorCRMove(uint32_t errcode);
std::string decodeConnectionErrorSource(uint32_t errcode);
std::string decodeConnectionErrorType(uint32_t errcode);


#endif

