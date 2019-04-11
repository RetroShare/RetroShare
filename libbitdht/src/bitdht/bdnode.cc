/*******************************************************************************
 * bitdht/bdnode.cc                                                            *
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

#include "bitdht/bdnode.h"

#include "bitdht/bencode.h"
#include "bitdht/bdmsgs.h"

#include "bitdht/bdquerymgr.h"
#include "bitdht/bdfilter.h"

#include "util/bdnet.h"
#include "util/bdrandom.h"
#include "util/bdstring.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>


#define BITDHT_QUERY_START_PEERS    10
#define BITDHT_QUERY_NEIGHBOUR_PEERS    8

#define BITDHT_MAX_REMOTE_QUERY_AGE	3 //  3 seconds, keep it fresh.
#define MAX_REMOTE_PROCESS_PER_CYCLE	5

/****
 * #define USE_HISTORY	1
 *
 * #define DEBUG_NODE_MULTIPEER 1
 * #define DEBUG_NODE_PARSE 1

 * #define DEBUG_NODE_MSGS 1
 * #define DEBUG_NODE_ACTIONS 1

 * #define DEBUG_NODE_MSGIN 1
 * #define DEBUG_NODE_MSGOUT 1
 *
 * #define DISABLE_BAD_PEER_FILTER		1
 *
 ***/

//#define DISABLE_BAD_PEER_FILTER		1

//#define USE_HISTORY	1

#define HISTORY_PERIOD  60

bdNode::bdNode(bdNodeId *ownId, std::string dhtVersion, const std::string& bootfile, const std::string& filterfile, bdDhtFunctions *fns, bdNodeManager *manager)
    :mNodeSpace(ownId, fns),
          mFilterPeers(filterfile,ownId, BITDHT_FILTER_REASON_OWNID, fns, manager),
          mQueryMgr(NULL),
          mConnMgr(NULL),
          mOwnId(*ownId), mDhtVersion(dhtVersion), mStore(bootfile, fns), mFns(fns),
          mFriendList(ownId), mHistory(HISTORY_PERIOD)
{
	init(); /* (uses this pointers) stuff it - do it here! */
}

void bdNode::init()
{
	mQueryMgr = new bdQueryManager(&mNodeSpace, mFns, this);
	mConnMgr = new bdConnectManager(&mOwnId, &mNodeSpace, mQueryMgr, mFns, this);

	//setNodeOptions(BITDHT_OPTIONS_MAINTAIN_UNSTABLE_PORT);
	setNodeOptions(0);

	mNodeDhtMode = 0;
	setNodeDhtMode(BITDHT_MODE_TRAFFIC_DEFAULT);

}
bool bdNode::getFilteredPeers(std::list<bdFilteredPeer>& peers)
{
    mFilterPeers.getFilteredPeers(peers) ;
    return true ;
}
//
//void bdNode::loadFilteredPeers(const std::list<bdFilteredPeer>& peers)
//{
//    mFilterPeers.loadFilteredPeers(peers) ;
//}
/* Unfortunately I've ended up with 2 calls down through the heirarchy...
 * not ideal - must clean this up one day.
 */

#define ATTACH_NUMBER 5
void bdNode::setNodeOptions(uint32_t optFlags)
{
	mNodeOptionFlags = optFlags;
	if (optFlags & BITDHT_OPTIONS_MAINTAIN_UNSTABLE_PORT)
	{
		mNodeSpace.setAttachedFlag(BITDHT_PEER_STATUS_DHT_ENGINE | BITDHT_PEER_STATUS_DHT_ENGINE_VERSION, ATTACH_NUMBER);
	}
	else
	{
		mNodeSpace.setAttachedFlag(BITDHT_PEER_STATUS_DHT_ENGINE | BITDHT_PEER_STATUS_DHT_ENGINE_VERSION, 0);
	}
}

#define BDNODE_HIGH_MSG_RATE	50
#define BDNODE_MED_MSG_RATE	10
#define BDNODE_LOW_MSG_RATE	5
#define BDNODE_TRICKLE_MSG_RATE	3

/* So we are setting this up so you can independently update each parameter....
 * if the mask is empty - it'll use the previous parameter.
 *
 */


uint32_t bdNode::setNodeDhtMode(uint32_t dhtFlags)
{
	std::cerr << "bdNode::setNodeDhtMode(" << dhtFlags << "), origFlags: " << mNodeDhtMode;
	std::cerr << std::endl;

	uint32_t origFlags = mNodeDhtMode;


	uint32_t traffic = dhtFlags &  BITDHT_MODE_TRAFFIC_MASK;
	if (traffic)
	{
		switch(traffic)
		{
			default:
			case BITDHT_MODE_TRAFFIC_LOW:
				mMaxAllowedMsgs = BDNODE_LOW_MSG_RATE;
				break;
			case BITDHT_MODE_TRAFFIC_MED:
				mMaxAllowedMsgs = BDNODE_MED_MSG_RATE;
				break;
			case BITDHT_MODE_TRAFFIC_HIGH:
				mMaxAllowedMsgs = BDNODE_HIGH_MSG_RATE;
				break;
			case BITDHT_MODE_TRAFFIC_TRICKLE:
				mMaxAllowedMsgs = BDNODE_TRICKLE_MSG_RATE;
				break;
		}
	}
	else
	{
		dhtFlags |= (origFlags & BITDHT_MODE_TRAFFIC_MASK);
	}

	uint32_t relay = dhtFlags & BITDHT_MODE_RELAYSERVER_MASK;
	if ((relay) && (relay != (origFlags & BITDHT_MODE_RELAYSERVER_MASK)))
	{
		/* changed */
		switch(relay)
		{
			default:
			case BITDHT_MODE_RELAYSERVERS_IGNORED:
				mRelayMode = BITDHT_RELAYS_OFF;
				dropRelayServers();
				break;
			case BITDHT_MODE_RELAYSERVERS_FLAGGED:
				mRelayMode = BITDHT_RELAYS_ON;
				pingRelayServers();
				break;
			case BITDHT_MODE_RELAYSERVERS_ONLY:
				mRelayMode = BITDHT_RELAYS_ONLY;
				pingRelayServers();
				break;
			case BITDHT_MODE_RELAYSERVERS_SERVER:
				mRelayMode = BITDHT_RELAYS_SERVER;
				pingRelayServers();
				break;
		}
		mConnMgr->setRelayMode(mRelayMode);
	}
	else
	{
		dhtFlags |= (origFlags & BITDHT_MODE_RELAYSERVER_MASK);
	}

	mNodeDhtMode = dhtFlags;

	std::cerr << "bdNode::setNodeDhtMode() newFlags: " << mNodeDhtMode;
	std::cerr << std::endl;

	return dhtFlags;
}


void bdNode::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
}

/***** Startup / Shutdown ******/
void bdNode::restartNode()
{
	mAccount.resetStats();

	mStore.reloadFromStore();

	/* setup */
	bdPeer peer;
	while(mStore.getPeer(&peer))
	{
		addPotentialPeer(&(peer.mPeerId), NULL);
	}
}


void bdNode::shutdownNode()
{
	/* clear the queries */
	mQueryMgr->shutdownQueries();
	mConnMgr->shutdownConnections();

	mRemoteQueries.clear();

	/* clear the space */
	mNodeSpace.clear();
	mHashSpace.clear();

	/* clear other stuff */
	mPotentialPeers.clear();
	mStore.clear();

	/* clean up any outgoing messages */
	while(mOutgoingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mOutgoingMsgs.front();
		mOutgoingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}


/* Crappy initial store... use bdspace as answer */
void bdNode::updateStore()
{
    mStore.writeStore();
}

bool bdNode::addressBanned(const sockaddr_in& raddr)
{
    return !mFilterPeers.addrOkay(const_cast<sockaddr_in*>(&raddr)) ;
}

void bdNode::printState()
{
	std::cerr << "bdNode::printState() for Peer: ";
	mFns->bdPrintNodeId(std::cerr, &mOwnId);
	std::cerr << std::endl;

	mNodeSpace.printDHT();

	mQueryMgr->printQueries();
	mConnMgr->printConnections();

	std::cerr << "Outstanding Potential Peers: " << mPotentialPeers.size();
	std::cerr << std::endl;
	
#ifdef USE_HISTORY
	mHistory.cleanupOldMsgs();
	mHistory.printMsgs();
	mHistory.analysePeers();
	mHistory.peerTypeAnalysis();

	// Incoming Query Analysis.
	std::cerr << "Outstanding Query Requests: " << mRemoteQueries.size();
	std::cerr << std::endl;

	mQueryHistory.printMsgs();
#endif

	mAccount.printStats(std::cerr);
}

void bdNode::iterationOff()
{
	/* clean up any incoming messages */
	while(mIncomingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}

void bdNode::iteration()
{
#ifdef DEBUG_NODE_MULTIPEER 
	std::cerr << "bdNode::iteration() of Peer: ";
	mFns->bdPrintNodeId(std::cerr, &mOwnId);
	std::cerr << std::endl;
#endif

	/* process incoming msgs */
	while(mIncomingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		recvPkt(msg->data, msg->mSize, msg->addr);

		/* cleanup message */
		delete msg;
	}


	/* assume that this is called once per second... limit the messages 
         * in theory, a query can generate up to 10 peers (which will all require a ping!).
	 * we want to handle all the pings we can... so we don't hold up the process.
	 * but we also want enough queries to keep things moving.
	 * so allow up to 90% of messages to be pings.
	 *
	 * ignore responses to other peers... as the number is very small generally
	 *
	 * The Rate IS NOW DEFINED IN NodeDhtMode.
	 */
	
	int allowedPings = 0.9 * mMaxAllowedMsgs;
	int sentMsgs = 0;
	int sentPings = 0;

#define BDNODE_MAX_POTENTIAL_PEERS_MULTIPLIER 	5

	/* Disable Queries if our Ping Queue is too long */
	if (mPotentialPeers.size() > mMaxAllowedMsgs * BDNODE_MAX_POTENTIAL_PEERS_MULTIPLIER)
	{
#ifdef DEBUG_NODE_MULTIPEER 
		std::cerr << "bdNode::iteration() Disabling Queries until PotentialPeer Queue reduced";
		std::cerr << std::endl;
#endif
		allowedPings = mMaxAllowedMsgs;
	}
	

	while((mPotentialPeers.size() > 0) && (sentMsgs < allowedPings))
	{
		/* check history ... is we have pinged them already...
		 * then simulate / pretend we have received a pong,
		 * and don't bother sending another ping.
		 */

		bdId pid = mPotentialPeers.front();	
		mPotentialPeers.pop_front();

		
		/* don't send too many queries ... check history first */
#if 0
#ifdef USE_HISTORY
		if (mHistory.validPeer(&pid))
		{
			/* just add as peer */

#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::iteration() Pinging Known Potential Peer : ";
			mFns->bdPrintId(std::cerr, &pid);
			std::cerr << std::endl;
#endif

		}
#endif
#endif

		/**** TEMP ****/

		{
			send_ping(&pid);
		
			sentMsgs++;
			sentPings++;

#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::iteration() Pinging Potential Peer : ";
			mFns->bdPrintId(std::cerr, &pid);
			std::cerr << std::endl;
#endif

		}

	}

	/* allow each query to send up to one query... until maxMsgs has been reached */
	int sentQueries = mQueryMgr->iterateQueries(mMaxAllowedMsgs-sentMsgs);
	sentMsgs += sentQueries;

	
#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdNode::iteration() maxMsgs: " << maxMsgs << " sentPings: " << sentPings;
	std::cerr << " / " << allowedPings;
	std::cerr << " sentQueries: " << sentQueries;
	std::cerr << std::endl;
#endif

	/* process remote query too */
	processRemoteQuery();

	std::list<bdId> peerIds;
	std::list<bdId>::iterator oit;
	mNodeSpace.scanOutOfDatePeers(peerIds);

	for(oit = peerIds.begin(); oit != peerIds.end(); oit++)
	{
		send_ping(&(*oit));
		mAccount.incCounter(BDACCOUNT_MSG_OUTOFDATEPING, true);

#ifdef DEBUG_NODE_MSGS 
		std::cerr << "bdNode::iteration() Pinging Out-Of-Date Peer: ";
		mFns->bdPrintId(std::cerr, &(*oit));
		std::cerr << std::endl;
#endif
	}

	// Handle Connection loops.
	mConnMgr->tickConnections();

	mAccount.doStats();
}



/***************************************************************************************
 ***************************************************************************************
 ***************************************************************************************/

void bdNode::send_ping(bdId *id)
{
	bdToken transId;
	genNewTransId(&transId);

	msgout_ping(id, &transId);
}


void bdNode::send_query(bdId *id, bdNodeId *targetNodeId, bool localnet)
{
	/* push out query */
	bdToken transId;
	genNewTransId(&transId);

	msgout_find_node(id, &transId, targetNodeId, localnet);

#ifdef DEBUG_NODE_MSGS 
	std::cerr << "bdNode::send_query() Find Node Req for : ";
	mFns->bdPrintId(std::cerr, &id);
	std::cerr << " searching for : ";
	mFns->bdPrintNodeId(std::cerr, &targetNodeId);
	std::cerr << std::endl;
#endif
}


void bdNode::send_connect_msg(bdId *id, int msgtype, bdId *srcAddr, bdId *destAddr, int mode, int param, int status)
{
	/* push out query */
	bdToken transId;
	genNewTransId(&transId);

	msgout_connect_genmsg(id, &transId, msgtype, srcAddr, destAddr, mode, param, status);

#ifdef DEBUG_NODE_MSGS 
	std::cerr << "bdNode::send_connect_msg() to: ";
	mFns->bdPrintId(std::cerr, &id);
	std::cerr << std::endl;
#endif
}


void bdNode::checkPotentialPeer(bdId *id, bdId *src)
{
	/* Check BadPeer Filters for Potential Peers too */

	/* first check the filters */
        if (!mFilterPeers.addrOkay(&(id->addr)))
	{
#ifdef DEBUG_NODE_MSGS 
		std::cerr << "bdNode::checkPotentialPeer(";
		mFns->bdPrintId(std::cerr, id);
		std::cerr << ") BAD ADDRESS!!!! SHOULD DISCARD POTENTIAL PEER";
		std::cerr << std::endl;
#endif

		return;
	}

	/* is it masquarading? */
	bdFriendEntry entry;
	if (mFriendList.findPeerEntry(&(id->id), entry))
	{
		struct sockaddr_in knownAddr;
		if (entry.addrKnown(&knownAddr))
		{
			if (knownAddr.sin_addr.s_addr != id->addr.sin_addr.s_addr)
			{
#ifndef DISABLE_BAD_PEER_FILTER	
				std::cerr << "bdNode::checkPotentialPeer(";
				mFns->bdPrintId(std::cerr, id);
				std::cerr << ") MASQUERADING AS KNOWN PEER - FLAGGING AS BAD";
				std::cerr << std::endl;

				// Stores in queue for later callback and desemination around the network.
	        		mBadPeerQueue.queuePeer(id, 0);

                    mFilterPeers.addPeerToFilter(id->addr, 0);

				std::list<struct sockaddr_in> filteredIPs;
                mFilterPeers.filteredIPs(filteredIPs);
				mStore.filterIpList(filteredIPs);

				return;
#endif
			}
		}
	}

	bool isWorthyPeer = mQueryMgr->checkPotentialPeer(id, src);

	if (isWorthyPeer)
	{
		addPotentialPeer(id, src);

	}

	if (src) // src can be NULL!
	{
		mConnMgr->addPotentialConnectionProxy(src, id); // CAUTION: Order switched!
	}

}


void bdNode::addPotentialPeer(bdId *id, bdId * /*src*/)
{
	mPotentialPeers.push_back(*id);
}

        // virtual so manager can do callback.
        // peer flags defined in bdiface.h
void bdNode::addPeer(const bdId *id, uint32_t peerflags)
{

#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdNode::addPeer(");
	mFns->bdPrintId(std::cerr, id);
	fprintf(stderr, ")\n");
#endif

	/* first check the filters */
        if (mFilterPeers.checkPeer(id, peerflags))
	{
		std::cerr << "bdNode::addPeer(";
		mFns->bdPrintId(std::cerr, id);
		std::cerr << ", " << std::hex << peerflags << std::dec;
		std::cerr << ") FAILED the BAD PEER FILTER!!!! DISCARDING MSG";
		std::cerr << std::endl;

		std::list<struct sockaddr_in> filteredIPs;
        mFilterPeers.filteredIPs(filteredIPs);
		mStore.filterIpList(filteredIPs);

	        mBadPeerQueue.queuePeer(id, peerflags);

		return;
	}

	/* next we check if it is a friend, whitelist etc, and adjust flags */
	bdFriendEntry entry;

	if (mFriendList.findPeerEntry(&(id->id), entry))
	{
		/* found! */
		peerflags |= entry.getPeerFlags(); // translates internal into general ones.

		struct sockaddr_in knownAddr;
		if (entry.addrKnown(&knownAddr))
		{
			if (knownAddr.sin_addr.s_addr != id->addr.sin_addr.s_addr)
			{
#ifndef DISABLE_BAD_PEER_FILTER	
				std::cerr << "bdNode::addPeer(";
				mFns->bdPrintId(std::cerr, id);
				std::cerr << ", " << std::hex << peerflags << std::dec;
				std::cerr << ") MASQUERADING AS KNOWN PEER - FLAGGING AS BAD";
				std::cerr << std::endl;
				

				// Stores in queue for later callback and desemination around the network.
	        		mBadPeerQueue.queuePeer(id, peerflags);

                    mFilterPeers.addPeerToFilter(id->addr, peerflags);

				std::list<struct sockaddr_in> filteredIPs;
                mFilterPeers.filteredIPs(filteredIPs);
				mStore.filterIpList(filteredIPs);

				// DO WE EXPLICITLY NEED TO DO THIS, OR WILL THEY JUST BE DROPPED?
				//mNodeSpace.remove_badpeer(id);
				//mQueryMgr->remove_badpeer(id);
	
				// FLAG in NodeSpace (Should be dropped very quickly anyway)
				mNodeSpace.flagpeer(id, 0, BITDHT_PEER_EXFLAG_BADPEER);

				return;		
#endif
			}
		}
	}

	mQueryMgr->addPeer(id, peerflags);
	mNodeSpace.add_peer(id, peerflags);

	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = peerflags;
	peer.mLastRecvTime = time(NULL);
	mStore.addStore(&peer);

	// Finally we pass to connections for them to use.
	mConnMgr->updatePotentialConnectionProxy(id, peerflags);


//#define DISPLAY_BITDHTNODES	1
#ifdef DISPLAY_BITDHTNODES
	/* TEMP to extract IDS for BloomFilter */
	if (peerflags & BITDHT_PEER_STATUS_DHT_ENGINE)
	{
		std::cerr << "bdNode::addPeer() FOUND BITDHT PEER";
		std::cerr << std::endl;
		mFns->bdPrintNodeId(std::cerr, &(id->id));
		std::cerr << std::endl;
	}
#endif
}

/************************************ Process Remote Query *************************/

/* increased the allowed processing rate from 1/sec => 5/sec */
void bdNode::processRemoteQuery()
{
	int nProcessed = 0;
	time_t oldTS = time(NULL) - BITDHT_MAX_REMOTE_QUERY_AGE;
	while(nProcessed < MAX_REMOTE_PROCESS_PER_CYCLE)
	{
		/* extra exit clause */
		if (mRemoteQueries.size() < 1) 
		{
#ifdef USE_HISTORY
			if (nProcessed)
			{
				mQueryHistory.cleanupOldMsgs();
			}
#endif
			return;
		}

		bdRemoteQuery &query = mRemoteQueries.front();

		// filtering.	
		bool badPeer = false;
#ifdef USE_HISTORY
		// store result in badPeer to activate the filtering.
		mQueryHistory.addIncomingQuery(query.mQueryTS, &(query.mId), &(query.mQuery));
#endif
	
		/* discard older ones (stops queue getting overloaded) */
		if ((query.mQueryTS > oldTS) && (!badPeer))
		{
			/* recent enough to process! */
			nProcessed++;

			switch(query.mQueryType)
			{
				case BD_QUERY_NEIGHBOURS:
				case BD_QUERY_LOCALNET:
				{
					/* search bdSpace for neighbours */
					
						std::list<bdId> nearList;
        				std::multimap<bdMetric, bdId> nearest;
        				std::multimap<bdMetric, bdId>::iterator it;

					if (query.mQueryType == BD_QUERY_LOCALNET)
					{
						std::list<bdId> excludeList;
						mNodeSpace.find_nearest_nodes_with_flags(&(query.mQuery), 
							BITDHT_QUERY_NEIGHBOUR_PEERS, 
							excludeList, nearest, BITDHT_PEER_STATUS_DHT_APPL);
					}
					else
					{
						if (mRelayMode == BITDHT_RELAYS_SERVER)
						{
							std::list<bdId> excludeList;
							mNodeSpace.find_nearest_nodes_with_flags(&(query.mQuery), 
								BITDHT_QUERY_NEIGHBOUR_PEERS, 
								excludeList, nearest, BITDHT_PEER_STATUS_DHT_RELAY_SERVER);
						}
						else
						{
							mNodeSpace.find_nearest_nodes(&(query.mQuery), BITDHT_QUERY_NEIGHBOUR_PEERS, nearest);
						}
					}

        				for(it = nearest.begin(); it != nearest.end(); it++)
        				{
                				nearList.push_back(it->second);
        				}
					msgout_reply_find_node(&(query.mId), &(query.mTransId), nearList);
#ifdef DEBUG_NODE_MSGS 
					std::cerr << "bdNode::processRemoteQuery() Reply to Find Node: ";
					mFns->bdPrintId(std::cerr, &(query.mId));
					std::cerr << " searching for : ";
					mFns->bdPrintNodeId(std::cerr, &(query.mQuery));
					std::cerr << ", found " << nearest.size() << " nodes ";
					std::cerr << std::endl;
#endif

					break;
				}
				case BD_QUERY_HASH:
				{
#ifdef DEBUG_NODE_MSGS 
					std::cerr << "bdNode::processRemoteQuery() Reply to Query Node: ";
					mFns->bdPrintId(std::cerr, &(query.mId));
					std::cerr << " TODO";
					std::cerr << std::endl;
#endif

					/* TODO */
					/* for now drop */
					/* unprocess! */
					nProcessed--;
					break;
				}
				default:
				{
					/* drop */
					/* unprocess! */
					nProcessed--;
					break;
				}
			}


				
		}
		else
		{
			if (badPeer)
			{
				std::cerr << "bdNode::processRemoteQuery() Query from BadPeer: Discarding: ";
			}
#ifdef DEBUG_NODE_MSGS 
			else
			{
				std::cerr << "bdNode::processRemoteQuery() Query Too Old: Discarding: ";
			}
#endif
#ifdef DEBUG_NODE_MSGS
			mFns->bdPrintId(std::cerr, &(query.mId));
			std::cerr << std::endl;
#endif
		}

		mRemoteQueries.pop_front();

	}

#ifdef USE_HISTORY
	mQueryHistory.cleanupOldMsgs();
#endif
}





/************************************ Message Buffering ****************************/

        /* interaction with outside world */
int     bdNode::outgoingMsg(struct sockaddr_in *addr, char *msg, int *len)
{
	if (mOutgoingMsgs.size() > 0)
	{
		bdNodeNetMsg *bdmsg = mOutgoingMsgs.front();
		//bdmsg->print(std::cerr);
		mOutgoingMsgs.pop_front();
		//bdmsg->print(std::cerr);

		/* truncate if necessary */
		if (bdmsg->mSize < *len)
		{
			//std::cerr << "bdNode::outgoingMsg space(" << *len << ") msgsize(" << bdmsg->mSize << ")";
			//std::cerr << std::endl;
			*len = bdmsg->mSize;
		}
		else
		{
			//std::cerr << "bdNode::outgoingMsg space(" << *len << ") small - trunc from: "
 			//<< bdmsg->mSize;
			//std::cerr << std::endl;
		}


		memcpy(msg, bdmsg->data, *len);
		*addr = bdmsg->addr;

		//bdmsg->print(std::cerr);

		delete bdmsg;
		return 1;
	}
	return 0;
}

void    bdNode::incomingMsg(struct sockaddr_in *addr, char *msg, int len)
{
	/* check against the filter */
    if (mFilterPeers.addrOkay(addr))
	{
		bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, addr);
		mIncomingMsgs.push_back(bdmsg);
	}
#ifdef DEBUG_NODE_MSGOUT
        else
        {
                std::cerr << "bdNode::incomingMsg() Incoming Packet Filtered";
                std::cerr << std::endl;
        }
#endif
}

/************************************ Message Handling *****************************/

/* Outgoing Messages */

void bdNode::msgout_ping(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_ping() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	// THIS IS CRASHING HISTORY.
	// LIKELY ID is not always valid!
	// Either PotentialPeers or Out-Of-Date Peers.
	//registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PING);

	bdId dupId(*id);
	registerOutgoingMsg(&dupId, transId, BITDHT_MSG_TYPE_PING, NULL);
	
	/* generate message, send to udp */
	bdToken vid;
	uint32_t vlen = BITDHT_TOKEN_MAX_LEN;
	if (mDhtVersion.size() < vlen)
	{
		vlen = mDhtVersion.size();
	}
	memcpy(vid.data, mDhtVersion.c_str(), vlen);	
	vid.len = vlen;

        /* create string */
        char msg[10240];
        int avail = 10240;

        int blen = bitdht_create_ping_msg(transId, &(mOwnId), &vid, msg, avail-1);
        sendPkt(msg, blen, id->addr);

	mAccount.incCounter(BDACCOUNT_MSG_PING, true);

}


void bdNode::msgout_pong(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_pong() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " Version: " << version;
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PONG, NULL);

	/* generate message, send to udp */
	bdToken vid;
	uint32_t vlen = BITDHT_TOKEN_MAX_LEN;
	if (mDhtVersion.size() < vlen)
	{
		vlen = mDhtVersion.size();
	}
	memcpy(vid.data, mDhtVersion.c_str(), vlen);	
	vid.len = vlen;

        char msg[10240];
        int avail = 10240;

        int blen = bitdht_response_ping_msg(transId, &(mOwnId), &vid, msg, avail-1);

        sendPkt(msg, blen, id->addr);
	
	mAccount.incCounter(BDACCOUNT_MSG_PONG, true);

}


void bdNode::msgout_find_node(bdId *id, bdToken *transId, bdNodeId *query, bool localnet)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_find_node() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Query: ";
	mFns->bdPrintNodeId(std::cerr, query);
	std::cerr << std::endl;
#endif

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_FIND_NODE, query);
	


        char msg[10240];
        int avail = 10240;

        int blen = bitdht_find_node_msg(transId, &(mOwnId), query, localnet, msg, avail-1);

        sendPkt(msg, blen, id->addr);

	mAccount.incCounter(BDACCOUNT_MSG_QUERYNODE, true);

}

void bdNode::msgout_reply_find_node(bdId *id, bdToken *transId, std::list<bdId> &peers)
{
        char msg[10240];
        int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NODE, NULL);
	
	mAccount.incCounter(BDACCOUNT_MSG_REPLYFINDNODE, true);

        int blen = bitdht_resp_node_msg(transId, &(mOwnId), peers, msg, avail-1);

        sendPkt(msg, blen, id->addr);

#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_reply_find_node() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Peers:";
	std::list<bdId>::iterator it;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		std::cerr << " ";
		mFns->bdPrintId(std::cerr, &(*it));
	}
	std::cerr << std::endl;
#endif
}

/*****************
 * SECOND HALF
 *
 *****/

void bdNode::msgout_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_get_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " InfoHash: ";
	mFns->bdPrintNodeId(std::cerr, info_hash);
	std::cerr << std::endl;
#endif

        char msg[10240];
        int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_GET_HASH, info_hash);

	
        int blen = bitdht_get_peers_msg(transId, &(mOwnId), info_hash, msg, avail-1);

        sendPkt(msg, blen, id->addr);

	mAccount.incCounter(BDACCOUNT_MSG_QUERYHASH, true);

}

void bdNode::msgout_reply_hash(bdId *id, bdToken *transId, bdToken *token, std::list<std::string> &values)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_reply_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);

	std::cerr << " Peers: ";
	std::list<std::string>::iterator it;
	for(it = values.begin(); it != values.end(); it++)
	{
		std::cerr << " ";
		bdPrintCompactPeerId(std::cerr, *it);
	}
	std::cerr << std::endl;
#endif

        char msg[10240];
        int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_HASH, NULL);

        int blen = bitdht_peers_reply_hash_msg(transId, &(mOwnId), token, values, msg, avail-1);

        sendPkt(msg, blen, id->addr);

	mAccount.incCounter(BDACCOUNT_MSG_REPLYQUERYHASH, true);

}

void bdNode::msgout_reply_nearest(bdId *id, bdToken *transId, bdToken *token, std::list<bdId> &nodes)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_reply_nearest() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);
	std::cerr << " Nodes:";

	std::list<bdId>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		std::cerr << " ";
		mFns->bdPrintId(std::cerr, &(*it));
	}
	std::cerr << std::endl;
#endif

        char msg[10240];
        int avail = 10240;
	
	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NEAR, NULL);
	

	
        int blen = bitdht_peers_reply_closest_msg(transId, &(mOwnId), token, nodes, msg, avail-1);

        sendPkt(msg, blen, id->addr);
	mAccount.incCounter(BDACCOUNT_MSG_REPLYQUERYHASH, true);


}

void bdNode::msgout_post_hash(bdId *id, bdToken *transId, bdNodeId *info_hash, uint32_t port, bdToken *token)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_post_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Info_Hash: ";
	mFns->bdPrintNodeId(std::cerr, info_hash);
	std::cerr << " Port: " << port;
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);
	std::cerr << std::endl;
#endif

        char msg[10240];
        int avail = 10240;
	
	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_POST_HASH, info_hash);
	
	
        int blen = bitdht_announce_peers_msg(transId,&(mOwnId),info_hash,port,token,msg,avail-1);

        sendPkt(msg, blen, id->addr);
	mAccount.incCounter(BDACCOUNT_MSG_POSTHASH, true);


}

void bdNode::msgout_reply_post(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdNode::msgout_reply_post() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	/* generate message, send to udp */
	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_POST, NULL);

	int blen = bitdht_reply_announce_msg(transId, &(mOwnId), msg, avail-1);

	sendPkt(msg, blen, id->addr);
	mAccount.incCounter(BDACCOUNT_MSG_REPLYPOSTHASH, true);

}


void    bdNode::sendPkt(char *msg, int len, struct sockaddr_in addr)
{
	//fprintf(stderr, "bdNode::sendPkt(%d) to %s:%d\n", 
	//		len, inet_ntoa(addr.sin_addr), htons(addr.sin_port));

	/* filter outgoing packets */
        if (mFilterPeers.addrOkay(&addr))
	{
		bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, &addr);
		//bdmsg->print(std::cerr);
		mOutgoingMsgs.push_back(bdmsg);
		//bdmsg->print(std::cerr);
	}
	else
	{
		std::cerr << "bdNode::sendPkt() Outgoing Packet Filtered";
		std::cerr << std::endl;
	}

	return;
}


/********************* Incoming Messages *************************/
/*
 * These functions are holding up udp queue -> so quickly
 * parse message, and get on with it!
 */

void    bdNode::recvPkt(char *msg, int len, struct sockaddr_in addr)
{
#ifdef DEBUG_NODE_PARSE
	std::cerr << "bdNode::recvPkt() msg[" << len << "] = ";
	for(int i = 0; i < len; i++)
	{
		if ((msg[i] > 31) && (msg[i] < 127))
		{
			std::cerr << msg[i];
		}
		else
		{
			std::cerr << "[" << (int) msg[i] << "]";
		}
	}
	std::cerr << std::endl;
#endif

	/* convert to a be_node */
	be_node *node = be_decoden(msg, len);
	if (!node)
	{
		/* invalid decode */
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() Failure to decode. Dropping Msg";
		std::cerr << std::endl;
		std::cerr << "message length: " << len;
		std::cerr << std::endl;
		std::cerr << "msg[] = ";
		for(int i = 0; i < len; i++)
		{
			if ((msg[i] > 31) && (msg[i] < 127))
			{
				std::cerr << msg[i];
			}
			else
			{
				std::cerr << "[" << (int) msg[i] << "]";
			}
		}
		std::cerr << std::endl;
#endif
		return;
	}

	/* find message type */
	uint32_t beType = beMsgType(node);
	bool     beQuery = (BE_Y_Q == beMsgGetY(node));

	if (!beType)
	{
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() Invalid Message Type. Dropping Msg";
		std::cerr << std::endl;
#endif
		/* invalid message */
		be_free(node);
		return;
	}

	/************************* handle token (all) **************************/
	be_node *be_transId = beMsgGetDictNode(node, "t");
        bdToken transId;
	if (be_transId)
	{
		beMsgGetToken(be_transId, transId);
	}
	else
	{
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() TransId Failure. Dropping Msg";
		std::cerr << std::endl;
#endif
		be_free(node);
		return;
	}

	/************************* handle data  (all) **************************/

	/* extract common features */
	char dictkey[2] = "r";
	if (beQuery)
	{
		dictkey[0] = 'a';
	}

	be_node *be_data = beMsgGetDictNode(node, dictkey);
	if (!be_data)
	{
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() Missing Data Body. Dropping Msg";
		std::cerr << std::endl;
#endif
		be_free(node);
		return;
	}

	/************************** handle id (all) ***************************/
        be_node *be_id = beMsgGetDictNode(be_data, "id");
	bdNodeId id;
	if(!be_id || !beMsgGetNodeId(be_id, id))
	{
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() Missing Peer Id. Dropping Msg";
		std::cerr << std::endl;
#endif
		be_free(node);
		return;
	}

	/************************ handle version (optional:pong) **************/
        be_node *be_version = NULL;
        bdToken versionId;
	if ((beType == BITDHT_MSG_TYPE_PONG) || (beType == BITDHT_MSG_TYPE_PING))
	{
		be_version = beMsgGetDictNode(node, "v");
		if (!be_version)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() NOTE: PONG missing Optional Version.";
			std::cerr << std::endl;
#endif
		}
	}

	if (be_version)
	{
		beMsgGetToken(be_version, versionId);
	}

	/************************ handle options (optional:bitdht extension) **************/
	be_node  *be_options = beMsgGetDictNode(node, "o");
	bool localnet = false;
	if (be_options)
	{
#ifdef DEBUG_NODE_PARSE
		std::cerr << "bdNode::recvPkt() Found Options Node, localnet";
		std::cerr << std::endl;
#endif
		localnet = true;
	}

	/*********** handle target (query) or info_hash (get_hash) ************/
	bdNodeId target_info_hash;
	be_node  *be_target = NULL;
	if (beType == BITDHT_MSG_TYPE_FIND_NODE)
	{
		be_target = beMsgGetDictNode(be_data, "target");
		if (!be_target)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

	}
	else if ((beType == BITDHT_MSG_TYPE_GET_HASH) ||
			(beType == BITDHT_MSG_TYPE_POST_HASH))
	{
		be_target = beMsgGetDictNode(be_data, "info_hash");
		if (!be_target)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_target)
	{
		beMsgGetNodeId(be_target, target_info_hash);
	}

	/*********** handle nodes (reply_query or reply_near) *****************/
	std::list<bdId> nodes;
	be_node  *be_nodes = NULL;
	if ((beType == BITDHT_MSG_TYPE_REPLY_NODE) ||
		(beType == BITDHT_MSG_TYPE_REPLY_NEAR))
	{
		be_nodes = beMsgGetDictNode(be_data, "nodes");
		if (!be_nodes)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() Missing Nodes. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_nodes)
	{
		beMsgGetListBdIds(be_nodes, nodes);
	}

	/******************* handle values (reply_hash) ***********************/
	std::list<std::string> values;
	be_node  *be_values = NULL;
	if (beType == BITDHT_MSG_TYPE_REPLY_HASH)
	{
		be_values = beMsgGetDictNode(be_data, "values");
		if (!be_values)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() Missing Values. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_values)
	{
		beMsgGetListStrings(be_values, values);
	}

	/************ handle token (reply_hash, reply_near, post hash) ********/
        bdToken token;
	be_node  *be_token = NULL;
	if ((beType == BITDHT_MSG_TYPE_REPLY_HASH) ||
		(beType == BITDHT_MSG_TYPE_REPLY_NEAR) ||
		(beType == BITDHT_MSG_TYPE_POST_HASH))
	{
		be_token = beMsgGetDictNode(be_data, "token");
		if (!be_token)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() Missing Token. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_token)
	{
		beMsgGetToken(be_transId, transId);
	}

	/****************** handle port (post hash) ***************************/
        uint32_t port;
	be_node  *be_port = NULL;
	if (beType == BITDHT_MSG_TYPE_POST_HASH)
	{
		be_port = beMsgGetDictNode(be_data, "port");
		if (!be_port)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() POST_HASH Missing Port. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_port)
	{
		beMsgGetUInt32(be_port, &port);
	}

	/****************** handle Connect (lots) ***************************/
	bdId connSrcAddr;
	bdId connDestAddr;
	uint32_t connMode;
	uint32_t connParam = 0;
	uint32_t connStatus;
	uint32_t connType;

	be_node  *be_ConnSrcAddr = NULL;
	be_node  *be_ConnDestAddr = NULL;
	be_node  *be_ConnMode = NULL;
	be_node  *be_ConnParam = NULL;
	be_node  *be_ConnStatus = NULL;
	be_node  *be_ConnType = NULL;
	if (beType == BITDHT_MSG_TYPE_CONNECT)
	{
		/* SrcAddr */
		be_ConnSrcAddr = beMsgGetDictNode(be_data, "src");
		if (!be_ConnSrcAddr)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing SrcAddr. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

		/* DestAddr */
		be_ConnDestAddr = beMsgGetDictNode(be_data, "dest");
		if (!be_ConnDestAddr)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing DestAddr. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

		/* Mode */
		be_ConnMode = beMsgGetDictNode(be_data, "mode");
		if (!be_ConnMode)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing Mode. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

		/* Param */
		be_ConnParam = beMsgGetDictNode(be_data, "param");
		if (!be_ConnParam)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing Param. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

		/* Status */
		be_ConnStatus = beMsgGetDictNode(be_data, "status");
		if (!be_ConnStatus)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing Status. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}

		/* Type */
		be_ConnType = beMsgGetDictNode(be_data, "type");
		if (!be_ConnType)
		{
#ifdef DEBUG_NODE_PARSE
			std::cerr << "bdNode::recvPkt() CONNECT Missing Type. Dropping Msg";
			std::cerr << std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_ConnSrcAddr)
	{
		beMsgGetBdId(be_ConnSrcAddr, connSrcAddr);
	}

	if (be_ConnDestAddr)
	{
		beMsgGetBdId(be_ConnDestAddr, connDestAddr);
	}

	if (be_ConnMode)
	{
		beMsgGetUInt32(be_ConnMode, &connMode);
	}

	if (be_ConnParam)
	{
		beMsgGetUInt32(be_ConnParam, &connParam);
	}

	if (be_ConnStatus)
	{
		beMsgGetUInt32(be_ConnStatus, &connStatus);
	}

	if (be_ConnType)
	{
		beMsgGetUInt32(be_ConnType, &connType);
	}



	/****************** Bits Parsed Ok. Process Msg ***********************/
	/* Construct Source Id */
	bdId srcId(id, addr);

	if (be_target)
	{	
		registerIncomingMsg(&srcId, &transId, beType, &target_info_hash);
	}
	else
	{
		registerIncomingMsg(&srcId, &transId, beType, NULL);
	}

	switch(beType)
	{
		case BITDHT_MSG_TYPE_PING:  /* a: id, transId */
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Responding to Ping : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			if (be_version)
			{
				msgin_ping(&srcId, &transId, &versionId);
			}
			else
			{
				msgin_ping(&srcId, &transId, NULL);
			}
			break;
		}
		case BITDHT_MSG_TYPE_PONG:  /* r: id, transId */          
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Received Pong from : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			if (be_version)
			{
				msgin_pong(&srcId, &transId, &versionId);
			}
			else
			{
				msgin_pong(&srcId, &transId, NULL);
			}

			break;
		}
		case BITDHT_MSG_TYPE_FIND_NODE: /* a: id, transId, target */     
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Req Find Node from : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << " Looking for: ";
			mFns->bdPrintNodeId(std::cerr, &target_info_hash);
			std::cerr << std::endl;
#endif
			msgin_find_node(&srcId, &transId, &target_info_hash, localnet);
			break;
		}
		case BITDHT_MSG_TYPE_REPLY_NODE: /* r: id, transId, nodes  */    
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Received Reply Node from : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_reply_find_node(&srcId, &transId, nodes);
			break;
		}
		case BITDHT_MSG_TYPE_GET_HASH:    /* a: id, transId, info_hash */     
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Received SearchHash : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << " for Hash: ";
			mFns->bdPrintNodeId(std::cerr, &target_info_hash);
			std::cerr << std::endl;
#endif
			msgin_get_hash(&srcId, &transId, &target_info_hash);
			break;
		}
		case BITDHT_MSG_TYPE_REPLY_HASH:  /* r: id, transId, token, values */
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Received Reply Hash : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_reply_hash(&srcId, &transId, &token, values);
			break;
		}
		case BITDHT_MSG_TYPE_REPLY_NEAR:  /* r: id, transId, token, nodes */    
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Received Reply Near : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_reply_nearest(&srcId, &transId, &token, nodes);
			break;
		}
		case BITDHT_MSG_TYPE_POST_HASH:   /* a: id, transId, info_hash, port, token */    
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Post Hash from : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << " to post: ";
			mFns->bdPrintNodeId(std::cerr, &target_info_hash);
			std::cerr << " with port: " << port;
			std::cerr << std::endl;
#endif
			msgin_post_hash(&srcId, &transId, &target_info_hash, port, &token);
			break;
		}
		case BITDHT_MSG_TYPE_REPLY_POST:  /* r: id, transId */     
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Reply Post from: ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_reply_post(&srcId, &transId);
			break;
		}
		case BITDHT_MSG_TYPE_CONNECT:  /* a: id, src, dest, mode, status, type */     
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() ConnectMsg from: ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_connect_genmsg(&srcId, &transId, connType,
					&connSrcAddr, &connDestAddr, 
					connMode, connParam, connStatus);
			break;
		}
		default:
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() ERROR";
			std::cerr << std::endl;
#endif
			/* ERROR */
			break;
		}
	}

	be_free(node);
	return;
}

/* Input: id, token.
 * Response: pong(id, token)
 */

void bdNode::msgin_ping(bdId *id, bdToken *transId, bdToken *versionId)
{
#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_ping() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	mAccount.incCounter(BDACCOUNT_MSG_PING, false);
	
	/* peer is alive */
	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_PING; /* no id typically, so cant get version */
	peerflags |= parseVersion(versionId);
	addPeer(id, peerflags);

	/* reply */
	msgout_pong(id, transId);

}

/* Input: id, token, (+optional version)
 * Response: store peer.
 */

void bdNode::msgin_pong(bdId *id, bdToken *transId, bdToken *versionId)
{
#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_pong() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " Version: TODO!"; // << version;
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#else
	(void) transId;
#endif

	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_PONG; 
	peerflags |= parseVersion(versionId);

	mAccount.incCounter(BDACCOUNT_MSG_PONG, false);
	addPeer(id, peerflags);
}


uint32_t bdNode::parseVersion(bdToken *versionId)
{
	/* recv pong, and peer is alive. add to DHT */
	//uint32_t vId = 0; // TODO XXX convertBdVersionToVID(versionId);

	/* calculate version match with peer */
	bool sameDhtEngine = false;
	bool sameDhtVersion = false;
	bool sameAppl = false;
	bool sameApplVersion = false;

	if (versionId)
	{
		
#ifdef DEBUG_NODE_MSGIN
		std::cerr << "bdNode::parseVersion() Peer Version: ";
		for(int i = 0; i < versionId->len; i++)
		{
			std::cerr << versionId->data[i];
		}
		std::cerr << std::endl;
#endif

#ifdef USE_HISTORY
		std::string version;
		for(int i = 0; i < versionId->len; i++)
		{
			if (isalnum(versionId->data[i]))
			{
				version += versionId->data[i];
			}
			else
			{
				version += 'X';
			}
		}
		mHistory.setPeerType(id, version);
#endif
	
		/* check two bytes */
		if ((versionId->len >= 2) && (mDhtVersion.size() >= 2) &&
			(versionId->data[0] == mDhtVersion[0]) && (versionId->data[1] == mDhtVersion[1]))
		{
			sameDhtEngine = true;
		}
	
		/* check two bytes. 
		 * Due to Old Versions not having this field, we need to check that they are numbers. 
		 * We have a Major version, and minor version....	
		 * This flag is set if Major is same, and minor is greater or equal to our version.
		 */
		if ((versionId->len >= 4) && (mDhtVersion.size() >= 4))
		{
			if ((isdigit(versionId->data[2]) && isdigit(versionId->data[3])) && 
				(versionId->data[2] == mDhtVersion[2]) && (versionId->data[3] >= mDhtVersion[3]))
			{
				sameDhtVersion = true;
			}
		}

		if ((sameDhtVersion) && (!sameDhtEngine))
		{
			sameDhtVersion = false;
#ifdef DEBUG_NODE_MSGIN
			std::cerr << "bdNode::parseVersion() STRANGE Peer Version: ";
			for(uint32_t i = 0; i < versionId->len; i++)
			{
				std::cerr << versionId->data[i];
			}
			std::cerr << std::endl;
#endif
		}
	
		/* check two bytes */
		if ((versionId->len >= 6) && (mDhtVersion.size() >= 6) &&
			(versionId->data[4] == mDhtVersion[4]) && (versionId->data[5] == mDhtVersion[5]))
		{
			sameAppl = true;
		}
	
	
		/* check two bytes */
		if ((versionId->len >= 8) && (mDhtVersion.size() >= 8))
		{
			if ((isdigit(versionId->data[6]) && isdigit(versionId->data[7])) && 
				(versionId->data[6] == mDhtVersion[6]) && (versionId->data[7] >= mDhtVersion[7]))
			{
				sameApplVersion = true;
			}
		}
	}
	else
	{
		
#ifdef DEBUG_NODE_MSGIN
		std::cerr << "bdNode::parseVersion() No Version";
		std::cerr << std::endl;
#endif
	}
	
	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_PONG; /* should have id too */
	if (sameDhtEngine)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_ENGINE; 
	}
	if (sameDhtVersion)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_ENGINE_VERSION; 
	}
	if (sameAppl)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_APPL; 
	}
	if (sameApplVersion)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_APPL_VERSION; 
	}
	return peerflags;
}



/* Input: id, token, queryId */

void bdNode::msgin_find_node(bdId *id, bdToken *transId, bdNodeId *query, bool localnet)
{
#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_find_node() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Query: ";
	mFns->bdPrintNodeId(std::cerr, query);
	std::cerr << std::endl;
#endif

	mAccount.incCounter(BDACCOUNT_MSG_QUERYNODE, false);


	/* store query... */
	uint32_t query_type = BD_QUERY_NEIGHBOURS;
	if (localnet)
	{
		query_type = BD_QUERY_LOCALNET;
	}
	queueQuery(id, query, transId, query_type);


	uint32_t peerflags = 0; /* no id, and no help! */
	addPeer(id, peerflags);
}

void bdNode::msgin_reply_find_node(bdId *id, bdToken *transId, std::list<bdId> &nodes)
{
	std::list<bdId>::iterator it;

#ifdef DEBUG_NODE_MSGS
	std::cerr << "bdNode::msgin_reply_find_node() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Peers:";
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		std::cerr << " ";
		mFns->bdPrintId(std::cerr, &(*it));
	}
	std::cerr << std::endl;
#else
	(void) transId;
#endif

	mAccount.incCounter(BDACCOUNT_MSG_REPLYFINDNODE, false);

	/* add neighbours to the potential list */
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		checkPotentialPeer(&(*it), id);
	}

	/* received reply - so peer must be good */
	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_NODES; /* no id ;( */
	addPeer(id, peerflags);
}

/********* THIS IS THE SECOND STAGE
 *
 */

void bdNode::msgin_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash)
{

	
#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_get_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " InfoHash: ";
	mFns->bdPrintNodeId(std::cerr, info_hash);
	std::cerr << std::endl;
#endif


	mAccount.incCounter(BDACCOUNT_MSG_QUERYHASH, false);

	/* generate message, send to udp */
	queueQuery(id, info_hash, transId, BD_QUERY_HASH);

}

void bdNode::msgin_reply_hash(bdId *id, bdToken *transId, bdToken *token, std::list<std::string> &values)
{
	mAccount.incCounter(BDACCOUNT_MSG_REPLYQUERYHASH, false);

#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_reply_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);

	std::cerr << " Peers: ";
	std::list<std::string>::iterator it;
	for(it = values.begin(); it != values.end(); it++)
	{
		std::cerr << " ";
		bdPrintCompactPeerId(std::cerr, *it);
	}
	std::cerr << std::endl;
#else
	(void) id;
	(void) transId;
	(void) token;
	(void) values;
#endif
}

void bdNode::msgin_reply_nearest(bdId *id, bdToken *transId, bdToken *token, std::list<bdId> &nodes)
{
	mAccount.incCounter(BDACCOUNT_MSG_REPLYQUERYHASH, false);

#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_reply_nearest() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);
	std::cerr << " Nodes:";

	std::list<bdId>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		std::cerr << " ";
		mFns->bdPrintId(std::cerr, &(*it));
	}
	std::cerr << std::endl;
#else
	(void) id;
	(void) transId;
	(void) token;
	(void) nodes;
#endif
}



void bdNode::msgin_post_hash(bdId *id,  bdToken *transId,  bdNodeId *info_hash,  uint32_t port, bdToken *token)
{

	mAccount.incCounter(BDACCOUNT_MSG_POSTHASH, false);

#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_post_hash() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " Info_Hash: ";
	mFns->bdPrintNodeId(std::cerr, info_hash);
	std::cerr << " Port: " << port;
	std::cerr << " Token: ";
	bdPrintToken(std::cerr, token);
	std::cerr << std::endl;
#else
	(void) id;
	(void) transId;
	(void) info_hash;
	(void) port;
	(void) token;
#endif

}


void bdNode::msgin_reply_post(bdId *id, bdToken *transId)
{
	/* generate message, send to udp */
	mAccount.incCounter(BDACCOUNT_MSG_REPLYPOSTHASH, false);

#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_reply_post() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#else
	(void) id;
	(void) transId;
#endif
}



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

void bdNode::msgout_connect_genmsg(bdId *id, bdToken *transId, int msgtype, bdId *srcAddr, bdId *destAddr, int mode, int param, int status)
{
#ifdef DEBUG_NODE_MSGOUT
	std::cerr << "bdConnectManager::msgout_connect_genmsg() Type: " << getConnectMsgType(msgtype);
	std::cerr << " TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " SrcAddr: ";
	mFns->bdPrintId(std::cerr, srcAddr);
	std::cerr << " DestAddr: ";
	mFns->bdPrintId(std::cerr, destAddr);
	std::cerr << " Mode: " << mode;
	std::cerr << " Param: " << param;
	std::cerr << " Status: " << status;
	std::cerr << std::endl;
#endif

	switch(msgtype)
	{
		default:
		case BITDHT_MSG_TYPE_CONNECT_REQUEST:
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTREQUEST, true);
			break;
		case BITDHT_MSG_TYPE_CONNECT_REPLY:
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTREPLY, true);
			break;
		case BITDHT_MSG_TYPE_CONNECT_START:
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTSTART, true);
			break;
		case BITDHT_MSG_TYPE_CONNECT_ACK:
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTACK, true);
			break;
	}
			
	registerOutgoingMsg(id, transId, msgtype, NULL);
	
        /* create string */
        char msg[10240];
        int avail = 10240;

        int blen = bitdht_connect_genmsg(transId, &(mOwnId), msgtype, srcAddr, destAddr, mode, param, status, msg, avail-1);
        sendPkt(msg, blen, id->addr);
}


void bdNode::msgin_connect_genmsg(bdId *id, bdToken *transId, int msgtype, 
					bdId *srcAddr, bdId *destAddr, int mode, int param, int status)
{
	std::list<bdId>::iterator it;

#ifdef DEBUG_NODE_MSGS
	std::cerr << "bdConnectManager::msgin_connect_genmsg() Type: " << getConnectMsgType(msgtype);
	std::cerr << " TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " From: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << " SrcAddr: ";
	mFns->bdPrintId(std::cerr, srcAddr);
	std::cerr << " DestAddr: ";
	mFns->bdPrintId(std::cerr, destAddr);
	std::cerr << " Mode: " << mode;
	std::cerr << " Param: " << param;
	std::cerr << " Status: " << status;
	std::cerr << std::endl;
#else
	(void) transId;
#endif

	/* switch to actual work functions */
	uint32_t peerflags = 0;
	switch(msgtype)
	{
		case BITDHT_MSG_TYPE_CONNECT_REQUEST:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTREQUEST, false);


			mConnMgr->recvedConnectionRequest(id, srcAddr, destAddr, mode, param);

			break;
		case BITDHT_MSG_TYPE_CONNECT_REPLY:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTREPLY, false);

			mConnMgr->recvedConnectionReply(id, srcAddr, destAddr, mode, param, status);

			break;
		case BITDHT_MSG_TYPE_CONNECT_START:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTSTART, false);

			mConnMgr->recvedConnectionStart(id, srcAddr, destAddr, mode, param);

			break;
		case BITDHT_MSG_TYPE_CONNECT_ACK:
			peerflags = BITDHT_PEER_STATUS_RECV_CONNECT_MSG; 
			mAccount.incCounter(BDACCOUNT_MSG_CONNECTACK, false);

			mConnMgr->recvedConnectionAck(id, srcAddr, destAddr, mode);

			break;
		default:
			break;
	}

	/* received message - so peer must be good */
	addPeer(id, peerflags);

}











/****************** Other Functions ******************/

void bdNode::genNewToken(bdToken *token)
{
#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdNode::genNewToken()");
	fprintf(stderr, ")\n");
#endif

	// XXX is this a good way to do it?
	// Variable length, from 4 chars up to lots... 10?
	// leave for the moment, but fix.
	std::string num;
	bd_sprintf(num, "%04lx", bdRandom::random_u32());
	int len = num.size();
	if (len > BITDHT_TOKEN_MAX_LEN)
		len = BITDHT_TOKEN_MAX_LEN;

	for(int i = 0; i < len; i++)
	{
		token->data[i] = num[i];
	}
	token->len = len;

}

uint32_t transIdCounter = 0;
void bdNode::genNewTransId(bdToken *token)
{
	/* generate message, send to udp */
#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdNode::genNewTransId()");
	fprintf(stderr, ")\n");
#endif

	std::string num;
	bd_sprintf(num, "%02lx", transIdCounter++);
	int len = num.size();
	if (len > BITDHT_TOKEN_MAX_LEN)
		len = BITDHT_TOKEN_MAX_LEN;

	for(int i = 0; i < len; i++)
	{
		token->data[i] = num[i];
	}
	token->len = len;
}


/* Store Remote Query for processing */
int bdNode::queueQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type)
{
#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdnode::queueQuery()" << std::endl;
#endif

	mRemoteQueries.push_back(bdRemoteQuery(id, query, transId, query_type));	

	return 1;
}

/*************** Register Transaction Ids *************/

void bdNode::registerOutgoingMsg(bdId *id, bdToken *transId, uint32_t msgType, bdNodeId *aboutId)
{
	
#ifdef DEBUG_MSG_CHECKS
	std::cerr << "bdNode::registerOutgoingMsg(";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << ", " << msgType << ")";
	std::cerr << std::endl;
#else
	(void) id;
	(void) msgType;
#endif
	
#ifdef USE_HISTORY

	// splitting up - to see if we can isolate the crash causes.
	switch(msgType)
	{
		// disabled types (which appear to crash it!)
		case BITDHT_MSG_TYPE_PING:
			if (!id)
			{
				return;
			}
			if ((id->id.data[0] == 0)
			  && (id->id.data[1] == 0)
			  && (id->id.data[2] == 0)
			  && (id->id.data[3] == 0))
			{
				return;
			}
			break;
		case BITDHT_MSG_TYPE_UNKNOWN:
		case BITDHT_MSG_TYPE_PONG:
		case BITDHT_MSG_TYPE_FIND_NODE:
		case BITDHT_MSG_TYPE_REPLY_NODE:
		case BITDHT_MSG_TYPE_GET_HASH:
		case BITDHT_MSG_TYPE_REPLY_HASH:
		case BITDHT_MSG_TYPE_REPLY_NEAR:
		case BITDHT_MSG_TYPE_POST_HASH:
		case BITDHT_MSG_TYPE_REPLY_POST:
			break;
	}

	// This line appears to cause crashes on OSX.
	mHistory.addMsg(id, transId, msgType, false, aboutId);
#else
	(void) transId;
	(void) aboutId;
#endif
	
	

/****
#define BITDHT_MSG_TYPE_UNKNOWN         0
#define BITDHT_MSG_TYPE_PING            1
#define BITDHT_MSG_TYPE_PONG            2
#define BITDHT_MSG_TYPE_FIND_NODE       3
#define BITDHT_MSG_TYPE_REPLY_NODE      4
#define BITDHT_MSG_TYPE_GET_HASH        5
#define BITDHT_MSG_TYPE_REPLY_HASH      6
#define BITDHT_MSG_TYPE_REPLY_NEAR      7
#define BITDHT_MSG_TYPE_POST_HASH       8
#define BITDHT_MSG_TYPE_REPLY_POST      9
***/

}



uint32_t bdNode::registerIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType, bdNodeId *aboutId)
{
	
#ifdef DEBUG_MSG_CHECKS
	std::cerr << "bdNode::registerIncomingMsg(";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << ", " << msgType << ")";
	std::cerr << std::endl;
#else
	(void) id;
	(void) msgType;
#endif
	
#ifdef USE_HISTORY
	mHistory.addMsg(id, transId, msgType, true, aboutId);
#else
	(void) transId;
	(void) aboutId;
#endif
	
	return 0;
}

void bdNode::cleanupTransIdRegister()
{
	return;
}



/*************** Internal Msg Storage *****************/

bdNodeNetMsg::bdNodeNetMsg(char *msg, int len, struct sockaddr_in *in_addr)
	:data(NULL), mSize(len), addr(*in_addr)
{
	data = (char *) malloc(len);
    
    	if(data == NULL)
        {
            std::cerr << "(EE) " << __PRETTY_FUNCTION__ << ": ERROR. cannot allocate memory for " << len << " bytes." << std::endl;
            return ;
        }
                         
	memcpy(data, msg, len);
	//print(std::cerr);
}

void bdNodeNetMsg::print(std::ostream &out)
{
	out << "bdNodeNetMsg::print(" << mSize << ") to "
			<< bdnet_inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
	out << std::endl;
}


bdNodeNetMsg::~bdNodeNetMsg()
{
	free(data);
}



/**************** In/Out of Relay Mode ******************/

void bdNode::dropRelayServers()
{
	/* We leave them there... just drop the flags */
	uint32_t flags = BITDHT_PEER_STATUS_DHT_RELAY_SERVER;
	std::list<bdNodeId> peerList;
	std::list<bdNodeId>::iterator it;

	mFriendList.findPeersWithFlags(flags, peerList);
	for(it = peerList.begin(); it != peerList.end(); it++)
	{
		mFriendList.removePeer(&(*it));
	}

	mNodeSpace.clean_node_flags(flags);
}

void bdNode::pingRelayServers()
{
	/* if Relay's have been switched on, do search/ping to locate servers */
#ifdef DEBUG_NODE_MSGS
	std::cerr << "bdNode::pingRelayServers()";
	std::cerr << std::endl;
#endif

	bool doSearch = true;

	uint32_t flags = BITDHT_PEER_STATUS_DHT_RELAY_SERVER;
	std::list<bdNodeId> peerList;
	std::list<bdNodeId>::iterator it;

	mFriendList.findPeersWithFlags(flags, peerList);
	for(it = peerList.begin(); it != peerList.end(); it++)
	{
		if (doSearch)
		{
			uint32_t qflags = BITDHT_QFLAGS_INTERNAL | BITDHT_QFLAGS_DISGUISE;
			mQueryMgr->addQuery(&(*it), qflags);

#ifdef DEBUG_NODE_MSGS
			std::cerr << "bdNode::pingRelayServers() Adding Internal Search for Relay Server: ";
			mFns->bdPrintNodeId(std::cerr, &(*it));
			std::cerr << std::endl;
#endif

		}
		else
		{
			/* try ping - if we have an address??? */

		}
	}
}


