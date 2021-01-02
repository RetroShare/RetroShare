/*******************************************************************************
 * bitdht/bdnode.h                                                             *
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
#ifndef BITDHT_NODE_H
#define BITDHT_NODE_H

#include "bitdht/bdpeer.h"
#include "bitdht/bdquery.h"
#include "bitdht/bdstore.h"
#include "bitdht/bdobj.h"
#include "bitdht/bdhash.h"
#include "bitdht/bdhistory.h"
#include "bitdht/bdfilter.h"

#include "bitdht/bdconnection.h"
#include "bitdht/bdaccount.h"

#include "bitdht/bdfriendlist.h"

class bdFilter;


#define BD_QUERY_NEIGHBOURS		1
#define BD_QUERY_LOCALNET		2
#define BD_QUERY_HASH			3

/**********************************
 * Running a node....
 *
 * run().
 * loops through and checks out of date peers.
 * handles searches.
 * prints out dht Table.
 *

The node handles the i/o traffic from peers.
It 



ping, return
peers, return
hash store, return
hash get, return



respond queue.

query queue.





input -> call into recvFunction()
output -> call back to Udp().




 *********/

class bdFilteredPeer ;
class bdNodeManager;

class bdNodeNetMsg
{

	public:
	bdNodeNetMsg(char *data, int size, struct sockaddr_in *addr);
	~bdNodeNetMsg();

	void print(std::ostream &out);

	char *data;
	int mSize;
	struct sockaddr_in addr;

};

class bdNodePublisher
{
	public:
	/* simplified outgoing msg functions (for the managers) */
	virtual void send_ping(bdId *id) = 0; /* message out */
	virtual void send_query(bdId *id, bdNodeId *targetNodeId, bool localnet) = 0; /* message out */
	virtual void send_connect_msg(bdId *id, int msgtype, 
				bdId *srcAddr, bdId *destAddr, int mode, int param, int status) = 0;

        // internal Callback -> normally continues to callbackConnect().
        virtual void callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId,
                                int mode, int point, int param, int cbtype, int errcode) = 0;

};


class bdNode: public bdNodePublisher
{
	public:

	bdNode(bdNodeId *id, std::string dhtVersion, const std::string& bootfile, const std::string& bootfilebak, const std::string& filterfile,
		bdDhtFunctions *fns, bdNodeManager* manager);

	void init(); /* sets up the self referential classes (mQueryMgr & mConnMgr) */

	void setNodeOptions(uint32_t optFlags);
	uint32_t setNodeDhtMode(uint32_t dhtFlags);

	/* startup / shutdown node */
	void restartNode();
	void shutdownNode();

	void getOwnId(bdNodeId *id);

	// virtual so manager can do callback.
	// peer flags defined in bdiface.h
	virtual void addPeer(const bdId *id, uint32_t peerflags);

	void printState();
	void checkPotentialPeer(bdId *id, bdId *src);
	void addPotentialPeer(bdId *id, bdId *src);

	void iterationOff();
	void iteration();
	void processRemoteQuery();
	void updateStore();

    bool addressBanned(const sockaddr_in &raddr) ;
    bool getFilteredPeers(std::list<bdFilteredPeer> &peers);
    //void loadFilteredPeers(const std::list<bdFilteredPeer> &peers);

	/* simplified outgoing msg functions (for the managers) */
	virtual void send_ping(bdId *id); /* message out */
	virtual void send_query(bdId *id, bdNodeId *targetNodeId, bool localnet); /* message out */
	virtual void send_connect_msg(bdId *id, int msgtype, 
				bdId *srcAddr, bdId *destAddr, int mode, int param, int status);

// This is implemented in bdManager.
//        virtual void callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId,
//                                int mode, int point, int param, int cbtype, int errcode);

	/* interaction with outside world (Accessed by controller to deliver us msgs) */
int 	outgoingMsg(struct sockaddr_in *addr, char *msg, int *len);
void 	incomingMsg(struct sockaddr_in *addr, char *msg, int len);

	// For Relay Mode switching.
void	dropRelayServers();
void	pingRelayServers();

// Below is internal Management of incoming / outgoing messages.

private:

	/* internal interaction with network */
void	sendPkt(char *msg, int len, struct sockaddr_in addr);
void	recvPkt(char *msg, int len, struct sockaddr_in addr);


	/* output functions (send msg) */
	void msgout_ping(bdId *id, bdToken *transId);
	void msgout_pong(bdId *id, bdToken *transId);
	void msgout_find_node(bdId *id, bdToken *transId, bdNodeId *query, bool localnet);
	void msgout_reply_find_node(bdId *id, bdToken *transId, 
						std::list<bdId> &peers);
	void msgout_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash);
	void msgout_reply_hash(bdId *id, bdToken *transId, 
				bdToken *token, std::list<std::string> &values);
	void msgout_reply_nearest(bdId *id, bdToken *transId, 
				bdToken *token, std::list<bdId> &peers);

	void msgout_post_hash(bdId *id, bdToken *transId, bdNodeId *info_hash, 
				uint32_t port, bdToken *token);
	void msgout_reply_post(bdId *id, bdToken *transId);


	/* input functions (once mesg is parsed) */
	uint32_t parseVersion(bdToken *versionId);
	void msgin_ping(bdId *id, bdToken *token, bdToken *versionId);
	void msgin_pong(bdId *id, bdToken *transId, bdToken *versionId);

	void msgin_find_node(bdId *id, bdToken *transId, bdNodeId *query, bool localnet);
	void msgin_reply_find_node(bdId *id, bdToken *transId, 
						std::list<bdId> &entries);

	void msgin_get_hash(bdId *id, bdToken *transId, bdNodeId *nodeid);
	void msgin_reply_hash(bdId *id, bdToken *transId, 
				bdToken *token, std::list<std::string> &values);
	void msgin_reply_nearest(bdId *id, bdToken *transId, 
				bdToken *token, std::list<bdId> &nodes);

	void msgin_post_hash(bdId *id,  bdToken *transId,  
				bdNodeId *info_hash,  uint32_t port, bdToken *token);
	void msgin_reply_post(bdId *id, bdToken *transId);

	void msgout_connect_genmsg(bdId *id, bdToken *transId, int msgtype, 
				bdId *srcAddr, bdId *destAddr, int mode, int param, int status);
	void msgin_connect_genmsg(bdId *id, bdToken *transId, int msgtype,
                                        bdId *srcAddr, bdId *destAddr, int mode, int param, int status);



	/* token handling */
	void genNewToken(bdToken *token);
	int queueQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type);

	/* transId handling */
	void genNewTransId(bdToken *token);
	void registerOutgoingMsg(bdId *id, bdToken *transId, uint32_t msgType, bdNodeId *aboutId);
	uint32_t registerIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType, bdNodeId *aboutId);

	void cleanupTransIdRegister();


	void doStats();

	/********** Variables **********/
	private:

	/**** Some Variables are Protected to allow inherited classes to use *****/
	protected:

	bdSpace mNodeSpace;
	bdFilter mFilterPeers;

	bdQueryManager *mQueryMgr;
	bdConnectManager *mConnMgr;

	bdNodeId mOwnId;
	bdId 	mLikelyOwnId; // Try to workout own id address.
	std::string mDhtVersion;

	bdAccount mAccount;
	bdStore mStore;

	bdDhtFunctions *mFns;
	bdHashSpace mHashSpace;

	bdFriendList mFriendList;
	bdPeerQueue  mBadPeerQueue;

	bdHistory mHistory; /* for understanding the DHT */

	bdQueryHistory mQueryHistory; /* for determining old peers */

	private:

	uint32_t mNodeOptionFlags;	
	uint32_t mNodeDhtMode;

	uint32_t mMaxAllowedMsgs;
	uint32_t mRelayMode;


	std::list<bdRemoteQuery> mRemoteQueries;

	std::list<bdId> mPotentialPeers;

	std::list<bdNodeNetMsg *> mOutgoingMsgs;
	std::list<bdNodeNetMsg *> mIncomingMsgs;

};



#endif // BITDHT_NODE_H
