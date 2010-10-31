/*
 * bitdht/bdnode.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
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

#include "bitdht/bdnode.h"

#include "bitdht/bencode.h"
#include "bitdht/bdmsgs.h"

#include "util/bdnet.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>
#include <sstream>


#define BITDHT_QUERY_START_PEERS    10
#define BITDHT_QUERY_NEIGHBOUR_PEERS    8
#define BITDHT_MAX_REMOTE_QUERY_AGE	10

/****
 * #define USE_HISTORY	1
 *
 * #define DEBUG_NODE_MULTIPEER 1
 * #define DEBUG_NODE_PARSE 1

 * #define DEBUG_NODE_MSGS 1
 * #define DEBUG_NODE_ACTIONS 1

 * #define DEBUG_NODE_MSGIN 1
 * #define DEBUG_NODE_MSGOUT 1
 ***/

//#define DEBUG_NODE_MSGS 1


bdNode::bdNode(bdNodeId *ownId, std::string dhtVersion, std::string bootfile, bdDhtFunctions *fns)
	:mOwnId(*ownId), mNodeSpace(ownId, fns), mStore(bootfile, fns), mDhtVersion(dhtVersion), mFns(fns)
{
	resetStats();
}

void bdNode::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
}

/***** Startup / Shutdown ******/
void bdNode::restartNode()
{
	resetStats();

	mStore.reloadFromStore();

	/* setup */
	bdPeer peer;
	while(mStore.getPeer(&peer))
	{
		addPotentialPeer(&(peer.mPeerId));
	}
}


void bdNode::shutdownNode()
{
	/* clear the queries */
	mLocalQueries.clear();
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

void bdNode::printState()
{
	std::cerr << "bdNode::printState() for Peer: ";
	mFns->bdPrintNodeId(std::cerr, &mOwnId);
	std::cerr << std::endl;

	mNodeSpace.printDHT();

	printQueries();

#ifdef USE_HISTORY
	mHistory.printMsgs();
#endif
	
	printStats(std::cerr);
}

void bdNode::printQueries()
{
	std::cerr << "bdNode::printQueries() for Peer: ";
	mFns->bdPrintNodeId(std::cerr, &mOwnId);
	std::cerr << std::endl;

	int i = 0;
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++, i++)
	{
		fprintf(stderr, "Query #%d:\n", i);
		(*it)->printQuery();
		fprintf(stderr, "\n");
	}
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
	/* iterate through queries */

	bdId id;
	bdNodeId targetNodeId;
	std::list<bdQuery>::iterator it;
	std::list<bdId>::iterator bit;

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
	 */
	
#define BDNODE_MESSAGE_RATE_HIGH 	1
#define BDNODE_MESSAGE_RATE_MED 	2
#define BDNODE_MESSAGE_RATE_LOW 	3
#define BDNODE_MESSAGE_RATE_TRICKLE 	4
	
#define BDNODE_HIGH_MSG_RATE	100
#define BDNODE_MED_MSG_RATE	50
#define BDNODE_LOW_MSG_RATE	20
#define BDNODE_TRICKLE_MSG_RATE	5

	int maxMsgs = BDNODE_MED_MSG_RATE;
	int mAllowedMsgRate = BDNODE_MESSAGE_RATE_MED;
	
	switch(mAllowedMsgRate)
	{
		case BDNODE_MESSAGE_RATE_HIGH:
			maxMsgs = BDNODE_HIGH_MSG_RATE;
			break;

		case BDNODE_MESSAGE_RATE_MED:
			maxMsgs = BDNODE_MED_MSG_RATE;
			break;

		case BDNODE_MESSAGE_RATE_LOW:
			maxMsgs = BDNODE_LOW_MSG_RATE;
			break;

		case BDNODE_MESSAGE_RATE_TRICKLE:
			maxMsgs = BDNODE_TRICKLE_MSG_RATE;
			break;

		default:
			break;

	}



	int allowedPings = 0.9 * maxMsgs;
	int sentMsgs = 0;
	int sentPings = 0;

#if 0
	int ilim = mLocalQueries.size() * 15;
	if (ilim < 20)
	{
		ilim = 20;
	}
	if (ilim > 500)
	{
		ilim = 500;
	}
#endif

	while((mPotentialPeers.size() > 0) && (sentMsgs < allowedPings))
	{
		/* check history ... is we have pinged them already...
		 * then simulate / pretend we have received a pong,
		 * and don't bother sending another ping.
		 */

		bdId pid = mPotentialPeers.front();	
		mPotentialPeers.pop_front();

		
		/* don't send too many queries ... check history first */
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

		/**** TEMP ****/

		{


			bdToken transId;
			genNewTransId(&transId);
			//registerOutgoingMsg(&pid, &transId, BITDHT_MSG_TYPE_PING);
			msgout_ping(&pid, &transId);
		
			sentMsgs++;
			sentPings++;

#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::iteration() Pinging Potential Peer : ";
			mFns->bdPrintId(std::cerr, &pid);
			std::cerr << std::endl;
#endif

			mCounterPings++;
		}

	}
	
	/* allow each query to send up to one query... until maxMsgs has been reached */
	int numQueries = mLocalQueries.size();
	int sentQueries = 0;
	int i = 0;
	while((i < numQueries) && (sentMsgs < maxMsgs))
	{
		bdQuery *query = mLocalQueries.front();
		mLocalQueries.pop_front();
		mLocalQueries.push_back(query);

		/* go through the possible queries */
		if (query->nextQuery(id, targetNodeId))
		{
			/* push out query */
			bdToken transId;
			genNewTransId(&transId);
			//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_FIND_NODE);

			msgout_find_node(&id, &transId, &targetNodeId);

#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::iteration() Find Node Req for : ";
			mFns->bdPrintId(std::cerr, &id);
			std::cerr << " searching for : ";
			mFns->bdPrintNodeId(std::cerr, &targetNodeId);
			std::cerr << std::endl;
#endif
			mCounterQueryNode++;
			sentMsgs++;
			sentQueries++;
		}
		i++;
	}
	
#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdNode::iteration() maxMsgs: " << maxMsgs << " sentPings: " << sentPings;
	std::cerr << " / " << allowedPings;
	std::cerr << " sentQueries: " << sentQueries;
	std::cerr << " / " << numQueries;
	std::cerr << std::endl;
#endif

	/* process remote query too */
	processRemoteQuery();

	while(mNodeSpace.out_of_date_peer(id))
	{
		/* push out ping */
		bdToken transId;
		genNewTransId(&transId);
		//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_PING);
		msgout_ping(&id, &transId);

#ifdef DEBUG_NODE_MSGS 
		std::cerr << "bdNode::iteration() Pinging Out-Of-Date Peer: ";
		mFns->bdPrintId(std::cerr, &id);
		std::cerr << std::endl;
#endif

		mCounterOutOfDatePing++;

		//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_FIND_NODE);
		//msgout_find_node(&id, &transId, &(id.id));
	}

	doStats();

	//printStats(std::cerr);

	//printQueries();
}

#define LPF_FACTOR  (0.90)

void bdNode::doStats()
{
	mLpfOutOfDatePing *= (LPF_FACTOR) ;
	mLpfOutOfDatePing += (1.0 - LPF_FACTOR) * mCounterOutOfDatePing;	
	mLpfPings *= (LPF_FACTOR);  	
	mLpfPings += (1.0 - LPF_FACTOR) * mCounterPings;	
	mLpfPongs *= (LPF_FACTOR);  	
	mLpfPongs += (1.0 - LPF_FACTOR) * mCounterPongs;	
	mLpfQueryNode *= (LPF_FACTOR);  	
	mLpfQueryNode += (1.0 - LPF_FACTOR) * mCounterQueryNode;	
	mLpfQueryHash *= (LPF_FACTOR);  	
	mLpfQueryHash += (1.0 - LPF_FACTOR) * mCounterQueryHash;	
	mLpfReplyFindNode *= (LPF_FACTOR);  	
	mLpfReplyFindNode += (1.0 - LPF_FACTOR) * mCounterReplyFindNode;	
	mLpfReplyQueryHash *= (LPF_FACTOR);  	
	mLpfReplyQueryHash += (1.0 - LPF_FACTOR) * mCounterReplyQueryHash;	

	mLpfRecvPing *= (LPF_FACTOR);  	
	mLpfRecvPing += (1.0 - LPF_FACTOR) * mCounterRecvPing;	
	mLpfRecvPong *= (LPF_FACTOR);  	
	mLpfRecvPong += (1.0 - LPF_FACTOR) * mCounterRecvPong;	
	mLpfRecvQueryNode *= (LPF_FACTOR);  	
	mLpfRecvQueryNode += (1.0 - LPF_FACTOR) * mCounterRecvQueryNode;	
	mLpfRecvQueryHash *= (LPF_FACTOR);  	
	mLpfRecvQueryHash += (1.0 - LPF_FACTOR) * mCounterRecvQueryHash;	
	mLpfRecvReplyFindNode *= (LPF_FACTOR);  	
	mLpfRecvReplyFindNode += (1.0 - LPF_FACTOR) * mCounterRecvReplyFindNode;	
	mLpfRecvReplyQueryHash *= (LPF_FACTOR);  	
	mLpfRecvReplyQueryHash += (1.0 - LPF_FACTOR) * mCounterRecvReplyQueryHash;	


	resetCounters();
}

void bdNode::printStats(std::ostream &out)
{
	out << "bdNode::printStats()" << std::endl;
	out << "  Send                                                 Recv: ";
	out << std::endl;
	out << "  mLpfOutOfDatePing      : " << std::setw(10) << mLpfOutOfDatePing;
	out << std::endl;
	out << "  mLpfPings              : " << std::setw(10) <<  mLpfPings;
	out << "  mLpfRecvPongs          : " << std::setw(10) << mLpfRecvPong;
	out << std::endl;
	out << "  mLpfPongs              : " << std::setw(10) << mLpfPongs;
	out << "  mLpfRecvPings          : " << std::setw(10) << mLpfRecvPing;
	out << std::endl;
	out << "  mLpfQueryNode          : " << std::setw(10) << mLpfQueryNode;
	out << "  mLpfRecvReplyFindNode  : " << std::setw(10) << mLpfRecvReplyFindNode;
	out << std::endl;
	out << "  mLpfQueryHash          : " << std::setw(10) << mLpfQueryHash;
	out << "  mLpfRecvReplyQueryHash : " << std::setw(10) << mLpfRecvReplyQueryHash;
	out << std::endl;
	out << "  mLpfReplyFindNode      : " << std::setw(10) << mLpfReplyFindNode; 
	out << "  mLpfRecvQueryNode      : " << std::setw(10) << mLpfRecvQueryNode;
	out << std::endl;
	out << "  mLpfReplyQueryHash/sec : " << std::setw(10) << mLpfReplyQueryHash;
	out << "  mLpfRecvQueryHash/sec  : " << std::setw(10) << mLpfRecvQueryHash;
	out << std::endl;
	out << std::endl;
}

void bdNode::resetCounters()
{
	mCounterOutOfDatePing = 0;
	mCounterPings = 0;
	mCounterPongs = 0;
	mCounterQueryNode = 0;
	mCounterQueryHash = 0;
	mCounterReplyFindNode = 0;
	mCounterReplyQueryHash = 0;

	mCounterRecvPing = 0;
	mCounterRecvPong = 0;
	mCounterRecvQueryNode = 0;
	mCounterRecvQueryHash = 0;
	mCounterRecvReplyFindNode = 0;
	mCounterRecvReplyQueryHash = 0;
}

void bdNode::resetStats()
{
	mLpfOutOfDatePing = 0;
	mLpfPings = 0;
	mLpfPongs = 0;
	mLpfQueryNode = 0;
	mLpfQueryHash = 0;
	mLpfReplyFindNode = 0;
	mLpfReplyQueryHash = 0;

	mLpfRecvPing = 0;
	mLpfRecvPong = 0;
	mLpfRecvQueryNode = 0;
	mLpfRecvQueryHash = 0;
	mLpfRecvReplyFindNode = 0;
	mLpfRecvReplyQueryHash = 0;

	resetCounters();
}


void bdNode::checkPotentialPeer(bdId *id)
{
	bool isWorthyPeer = false;
	/* also push to queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		if ((*it)->addPotentialPeer(id, 0))
		{
			isWorthyPeer = true;
		}
	}

	if (isWorthyPeer)
	{
		addPotentialPeer(id);
	}
}


void bdNode::addPotentialPeer(bdId *id)
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

	/* iterate through queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		(*it)->addPeer(id, peerflags);
	}

	mNodeSpace.add_peer(id, peerflags);

	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = peerflags;
	peer.mLastRecvTime = time(NULL);
	mStore.addStore(&peer);
}


#if 0
        // virtual so manager can do callback.
        // peer flags defined in bdiface.h
void bdNode::PeerResponse(const bdId *id, const bdNodeId *target, uint32_t peerflags)
{

#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdNode::PeerResponse(";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << ", target: ";
	mFns->bdPrintNodeId(std::cerr, target);
	fprintf(stderr, ")\n");
#endif

	/* iterate through queries */
	std::list<bdQuery>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		it->PeerResponse(id, target, peerflags);
	}

	mNodeSpace.add_peer(id, peerflags);

	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = peerflags;
	peer.mLastRecvTime = time(NULL);
	mStore.addStore(&peer);
}

#endif

/************************************ Query Details        *************************/
void bdNode::addQuery(const bdNodeId *id, uint32_t qflags)
{

	std::list<bdId> startList;
        std::multimap<bdMetric, bdId> nearest;
        std::multimap<bdMetric, bdId>::iterator it;

        mNodeSpace.find_nearest_nodes(id, BITDHT_QUERY_START_PEERS, startList, nearest);

	fprintf(stderr, "bdNode::addQuery(");
	mFns->bdPrintNodeId(std::cerr, id);
	fprintf(stderr, ")\n");

        for(it = nearest.begin(); it != nearest.end(); it++)
        {
                startList.push_back(it->second);
        }

        bdQuery *query = new bdQuery(id, startList, qflags, mFns);
	mLocalQueries.push_back(query);
}


void bdNode::clearQuery(const bdNodeId *rmId)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end();)
	{
		if ((*it)->mId == *rmId)
		{
			bdQuery *query = (*it);
			it = mLocalQueries.erase(it);
			delete query;
		}
		else
		{
			it++;
		}
	}
}

void bdNode::QueryStatus(std::map<bdNodeId, bdQueryStatus> &statusMap)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		bdQueryStatus status;
		status.mStatus = (*it)->mState;
		status.mQFlags = (*it)->mQueryFlags;
		(*it)->result(status.mResults);
		statusMap[(*it)->mId] = status;
	}
}


/************************************ Process Remote Query *************************/
void bdNode::processRemoteQuery()
{
	bool processed = false;
	time_t oldTS = time(NULL) - BITDHT_MAX_REMOTE_QUERY_AGE;
	while(!processed)
	{
		/* extra exit clause */
		if (mRemoteQueries.size() < 1) return;

		bdRemoteQuery &query = mRemoteQueries.front();
		
		/* discard older ones (stops queue getting overloaded) */
		if (query.mQueryTS > oldTS)
		{
			/* recent enough to process! */
			processed = true;

			switch(query.mQueryType)
			{
				case BD_QUERY_NEIGHBOURS:
				{
					/* search bdSpace for neighbours */
					std::list<bdId> excludeList;
					std::list<bdId> nearList;
        				std::multimap<bdMetric, bdId> nearest;
        				std::multimap<bdMetric, bdId>::iterator it;

        				mNodeSpace.find_nearest_nodes(&(query.mQuery), BITDHT_QUERY_NEIGHBOUR_PEERS, excludeList, nearest);

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
					
					mCounterReplyFindNode++;

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
					mCounterReplyQueryHash++;


					/* TODO */
					break;
				}
				default:
				{
					/* drop */
					/* unprocess! */
					processed = false;
					break;
				}
			}


				
		}
		else
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::processRemoteQuery() Query Too Old: Discarding: ";
			mFns->bdPrintId(std::cerr, &(query.mId));
			std::cerr << std::endl;
#endif
		}


		mRemoteQueries.pop_front();
	}
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
	bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, addr);
	mIncomingMsgs.push_back(bdmsg);
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

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PING);
	
	
        /* create string */
        char msg[10240];
        int avail = 10240;

        int blen = bitdht_create_ping_msg(transId, &(mOwnId), msg, avail-1);
        sendPkt(msg, blen, id->addr);


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

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PONG);

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

}


void bdNode::msgout_find_node(bdId *id, bdToken *transId, bdNodeId *query)
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

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_FIND_NODE);
	


        char msg[10240];
        int avail = 10240;

        int blen = bitdht_find_node_msg(transId, &(mOwnId), query, msg, avail-1);

        sendPkt(msg, blen, id->addr);


}

void bdNode::msgout_reply_find_node(bdId *id, bdToken *transId, std::list<bdId> &peers)
{
        char msg[10240];
        int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NODE);
	

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

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_GET_HASH);

	
        int blen = bitdht_get_peers_msg(transId, &(mOwnId), info_hash, msg, avail-1);

        sendPkt(msg, blen, id->addr);


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

		registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_HASH);

        int blen = bitdht_peers_reply_hash_msg(transId, &(mOwnId), token, values, msg, avail-1);

        sendPkt(msg, blen, id->addr);


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
	
	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NEAR);
	

	
        int blen = bitdht_peers_reply_closest_msg(transId, &(mOwnId), token, nodes, msg, avail-1);

        sendPkt(msg, blen, id->addr);

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
	
	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_POST_HASH);
	
	
        int blen = bitdht_announce_peers_msg(transId,&(mOwnId),info_hash,port,token,msg,avail-1);

        sendPkt(msg, blen, id->addr);

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

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_POST);

        int blen = bitdht_reply_announce_msg(transId, &(mOwnId), msg, avail-1);

        sendPkt(msg, blen, id->addr);

}


void    bdNode::sendPkt(char *msg, int len, struct sockaddr_in addr)
{
	//fprintf(stderr, "bdNode::sendPkt(%d) to %s:%d\n", 
	//		len, inet_ntoa(addr.sin_addr), htons(addr.sin_port));

	bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, &addr);
	//bdmsg->print(std::cerr);
	mOutgoingMsgs.push_back(bdmsg);
	//bdmsg->print(std::cerr);

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
	if (be_id)
	{
		beMsgGetNodeId(be_id, id);
	}
	else
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
	if (beType == BITDHT_MSG_TYPE_PONG)
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

	/****************** Bits Parsed Ok. Process Msg ***********************/
	/* Construct Source Id */
	bdId srcId(id, addr);
	
	checkIncomingMsg(&srcId, &transId, beType);
	switch(beType)
	{
		case BITDHT_MSG_TYPE_PING:  /* a: id, transId */
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdNode::recvPkt() Responding to Ping : ";
			mFns->bdPrintId(std::cerr, &srcId);
			std::cerr << std::endl;
#endif
			msgin_ping(&srcId, &transId);
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
			msgin_find_node(&srcId, &transId, &target_info_hash);
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

void bdNode::msgin_ping(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGIN
	std::cerr << "bdNode::msgin_ping() TransId: ";
	bdPrintTransId(std::cerr, transId);
	std::cerr << " To: ";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif
	mCounterRecvPing++;
	mCounterPongs++;

	/* peer is alive */
	uint32_t peerflags = 0; /* no id typically, so cant get version */
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

	mCounterRecvPong++;
	/* recv pong, and peer is alive. add to DHT */
	//uint32_t vId = 0; // TODO XXX convertBdVersionToVID(versionId);

	/* calculate version match with peer */
	bool sameDhtEngine = false;
	bool sameAppl = false;
	bool sameVersion = false;

	if (versionId)
	{
		
#ifdef DEBUG_NODE_MSGIN
		std::cerr << "bdNode::msgin_pong() Peer Version: ";
		for(int i = 0; i < versionId->len; i++)
		{
			std::cerr << versionId->data[i];
		}
		std::cerr << std::endl;
#endif
	
		/* check two bytes */
		if ((versionId->len >= 2) && (mDhtVersion.size() >= 2) &&
			(versionId->data[0] == mDhtVersion[0]) && (versionId->data[1] == mDhtVersion[1]))
		{
			sameDhtEngine = true;
		}
	
		/* check two bytes */
		if ((versionId->len >= 4) && (mDhtVersion.size() >= 4) &&
			(versionId->data[2] == mDhtVersion[2]) && (versionId->data[3] == mDhtVersion[3]))
		{
			sameAppl = true;
		}
	
		/* check two bytes */
		if ((versionId->len >= 6) && (mDhtVersion.size() >= 6) &&
			(versionId->data[4] == mDhtVersion[4]) && (versionId->data[5] == mDhtVersion[5]))
		{
			sameVersion = true;
		}
	}
	else
	{
		
#ifdef DEBUG_NODE_MSGIN
		std::cerr << "bdNode::msgin_pong() No Version";
		std::cerr << std::endl;
#endif
	}
	

	

	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_PONG; /* should have id too */
	if (sameDhtEngine)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_ENGINE; 
	}
	if (sameAppl)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_APPL; 
	}
	if (sameVersion)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_VERSION; 
	}

	addPeer(id, peerflags);
}

/* Input: id, token, queryId */

void bdNode::msgin_find_node(bdId *id, bdToken *transId, bdNodeId *query)
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

	mCounterRecvQueryNode++;

	/* store query... */
	queueQuery(id, query, transId, BD_QUERY_NEIGHBOURS);


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
	mCounterRecvReplyFindNode++;


	/* add neighbours to the potential list */
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		checkPotentialPeer(&(*it));
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

	mCounterRecvQueryHash++;

	/* generate message, send to udp */
	queueQuery(id, info_hash, transId, BD_QUERY_HASH);

}

void bdNode::msgin_reply_hash(bdId *id, bdToken *transId, bdToken *token, std::list<std::string> &values)
{
	mCounterRecvReplyQueryHash++;

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
	//mCounterRecvReplyNearestHash++;

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
	//mCounterRecvPostHash++;

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
	//mCounterRecvReplyPostHash++;

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


/****************** Other Functions ******************/

void bdNode::genNewToken(bdToken *token)
{
#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdNode::genNewToken()");
	fprintf(stderr, ")\n");
#endif

	std::ostringstream out;
	out << std::setw(4) << std::setfill('0') << rand() << std::setw(4) << std::setfill('0') << rand();
	std::string num = out.str();
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

	std::ostringstream out;
	out << std::setw(2) << std::setfill('0') << transIdCounter++;
	std::string num = out.str();
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

void bdNode::registerOutgoingMsg(bdId *id, bdToken *transId, uint32_t msgType)
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
	mHistory.addMsg(id, transId, msgType, false);
#else
	(void) transId;
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



uint32_t bdNode::checkIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType)
{
	
#ifdef DEBUG_MSG_CHECKS
	std::cerr << "bdNode::checkIncomingMsg(";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << ", " << msgType << ")";
	std::cerr << std::endl;
#else
	(void) id;
	(void) msgType;
#endif
	
#ifdef USE_HISTORY
	mHistory.addMsg(id, transId, msgType, true);
#else
	(void) transId;
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
	memcpy(data, msg, len);
	//print(std::cerr);
}

void bdNodeNetMsg::print(std::ostream &out)
{
	out << "bdNodeNetMsg::print(" << mSize << ") to "
			<< inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
	out << std::endl;
}


bdNodeNetMsg::~bdNodeNetMsg()
{
	free(data);
}


