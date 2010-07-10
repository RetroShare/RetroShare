/*
 * bitdht/bdmanager.cc
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


/*******
 * Node Manager.
 ******/

/******************************************
 * 1) Maintains a list of ids to search for.
 * 2) Sets up initial search for own node.
 * 3) Checks on status of queries.
 * 4) Callback on successful searches.
 *
 * This is pretty specific to RS requirements.
 ****/

#include "bdiface.h"
#include "bdstddht.h"
#include "bdmanager.h"
#include "bdmsgs.h"
#include "bencode.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/***
 * #define DEBUG_MGR 1
 * #define DEBUG_MGR_PKT 1
 ***/

//#define DEBUG_MGR 1

bdNodeManager::bdNodeManager(bdNodeId *id, std::string dhtVersion, std::string bootfile, bdDhtFunctions *fns)
	:bdNode(id, dhtVersion, bootfile, fns)
{
	mMode = BITDHT_MGR_STATE_STARTUP;
	mFns = fns;

	/* setup a query for self */
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::bdNodeManager() ID: ";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif
}



void bdNodeManager::addFindNode(bdNodeId *id, uint32_t qflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addFindNode() ";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif
	/* check if exists already */
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	it = mActivePeers.find(*id);
	if (it != mActivePeers.end())
	{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::addFindNode() Found existing....";
		std::cerr << std::endl;
#endif
		return;
	}

	/* add to map */
	bdQueryPeer peer;
	peer.mId.id = (*id);
	peer.mStatus = BITDHT_QUERY_QUERYING;
	peer.mQFlags = qflags;
	mActivePeers[*id] = peer;
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addFindNode() Added QueryPeer....";
	std::cerr << std::endl;
#endif
	addQuery(id, qflags | BITDHT_QFLAGS_DISGUISE); 
	return;
}


void bdNodeManager::removeFindNode(bdNodeId *id)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::removeFindNode() ";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	it = mActivePeers.find(*id);
	if (it == mActivePeers.end())
	{
		return;
	}

	/* cleanup any actions */
	clearQuery(&(it->first));
	//clearPing(&(it->first));

	/* remove from map */
	mActivePeers.erase(it);
	return;
}

void bdNodeManager::iteration()
{

	time_t now = time(NULL);
	time_t modeAge = now - mModeTS;
	switch(mMode)
	{
		case BITDHT_MGR_STATE_STARTUP:
			/* 10 seconds startup .... then switch to ACTIVE */
			if (modeAge > MAX_STARTUP_TIME)
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): STARTUP ";
				std::cerr << std::endl;
#endif
				bdNodeId id;
				getOwnId(&id);
				addQuery(&id, BITDHT_QFLAGS_DO_IDLE | BITDHT_QFLAGS_DISGUISE); 
				
				//mMode = BITDHT_MGR_STATE_ACTIVE;
				mMode = BITDHT_MGR_STATE_REFRESH;
				mModeTS = now;
			}
			break;

		case BITDHT_MGR_STATE_ACTIVE:
			if (modeAge > MAX_REFRESH_TIME)
			{
				mMode = BITDHT_MGR_STATE_REFRESH;
				mModeTS = now;
			}

			break;

		case BITDHT_MGR_STATE_REFRESH:
			{
				/* select random ids, and perform searchs to refresh space */
				mMode = BITDHT_MGR_STATE_ACTIVE;
				mModeTS = now;

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): Updating Stores";
				std::cerr << std::endl;
#endif

				updateStore();

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): REFRESH ";
				std::cerr << std::endl;
#endif


				status();
			}
			break;

		case BITDHT_MGR_STATE_QUIET:
			{


			}
			break;
	}


	/* tick parent */
	bdNode::iteration();
}


int bdNodeManager::status()
{
	/* do status of bdNode */
	printState();

	checkStatus();
	

	return 1;
}


int bdNodeManager::checkStatus()
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::checkStatus()";
	std::cerr << std::endl;
#endif

	/* check queries */
        std::map<bdNodeId, bdQueryStatus>::iterator it;
        std::map<bdNodeId, bdQueryStatus> queryStatus;


        QueryStatus(queryStatus);

	for(it = queryStatus.begin(); it != queryStatus.end(); it++)
	{
		bool doPing = false;
		bool doRemove = false;
		bool doCallback = false;
		uint32_t callbackStatus = 0;

		switch(it->second.mStatus)
		{
			default:
			case BITDHT_QUERY_QUERYING:
				{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Query in Progress id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
				}
				break;

			case BITDHT_QUERY_FAILURE:
				{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Query Failed: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
					// BAD.
					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_FAILURE;
				}
				break;

			case BITDHT_QUERY_FOUND_CLOSEST:
				{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Found Closest: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
					
					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_OFFLINE;
				}
				break;

			case BITDHT_QUERY_PEER_UNREACHABLE:
				{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() the Peer Online but Unreachable: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
					
					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_UNREACHABLE;
				}
				break;

			case BITDHT_QUERY_SUCCESS:
				{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Found Query: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
					//foundId = 
					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_ONLINE;
				}
				break;
		}

		/* remove done queries */
		if (doRemove) 
		{
			if (it->second.mQFlags & BITDHT_QFLAGS_DO_IDLE)
			{
				doRemove = false;
			}
		}

		if (doRemove) 
		{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Removing query: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
			clearQuery(&(it->first));
		}

		/* FIND in activePeers */
		std::map<bdNodeId, bdQueryPeer>::iterator pit;
		pit = mActivePeers.find(it->first);

		if (pit == mActivePeers.end())
		{
			/* only internal! - disable Callback / Ping */
			doPing = false;
			doCallback = false;
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::checkStatus() Internal: no cb for id: ";
			mFns->bdPrintNodeId(std::cerr, &(it->first));
			std::cerr << std::endl;
#endif
		}
		else if (pit->second.mStatus == it->second.mStatus)
		{
			/* status is unchanged */
			doPing = false;
			doCallback = false;
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::checkStatus() Status unchanged for : ";
			mFns->bdPrintNodeId(std::cerr, &(it->first));
			std::cerr << " status: " << it->second.mStatus;
			std::cerr << std::endl;
#endif
		}
		else
		{
				
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::checkStatus() Updating External Status for : ";
			mFns->bdPrintNodeId(std::cerr, &(it->first));
			std::cerr << " to: " << it->second.mStatus;
			std::cerr << std::endl;
#endif
			/* update status */
			pit->second.mStatus = it->second.mStatus;
		}


		/* add successful queries to ping list */
		if (doPing)
		{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Starting Ping (TODO): id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
			/* add first matching peer */
			//addPeerPing(foundId);
		}

		/* callback on new successful queries */
		if (doCallback)
		{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Doing Callback: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
			doPeerCallback(&(it->first), callbackStatus);
		}
	}
	return 1;
}

#if 0
bdNodeManager::checkPingStatus()
{

	/* check queries */
        std::map<bdNodeId, bdPingStatus>::iterator it;
        std::map<bdNodeId, bdPingStatus> pingStatus;

        PingStatus(pingStatus);

	for(it = pingStatus.begin(); it != pingStatus.end(); it++)
	{
		switch(it->second.mStatus)
		{
			case BITDHT_QUERY_QUERYING:
				{

				}
				break;

			case BITDHT_QUERY_FAILURE:
				{
					// BAD.
					doRemove = true;
				}
				break;

			case BITDHT_QUERY_FOUND_CLOSEST:
				{
					
					doRemove = true;
				}
				break;

			case BITDHT_QUERY_SUCCESS:
				{
					foundId = 
					doRemove = true;
				}
				break;
		}

		/* remove done queries */
		if (doRemove)
		{
			clearQuery(it->first);
		}

		/* add successful queries to ping list */
		if (doPing)
		{
			/* add first matching peer */
			addPeerPing(foundId);
		}

		/* callback on new successful queries */
		if (doCallback)
		{

		}
	}
}
#endif


int bdNodeManager::SearchOutOfDate()
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::SearchOutOfDate()";
	std::cerr << std::endl;
#endif

	std::map<bdNodeId, bdQueryPeer>::iterator it;

	/* search for out-of-date peers */
	for(it = mActivePeers.begin(); it != mActivePeers.end(); it++)
	{
#if 0
		if (old)
		{
			addQuery(it->first);
		}
#endif
	}

	return 1;
}



        /***** Functions to Call down to bdNodeManager ****/
        /* Request DHT Peer Lookup */
        /* Request Keyword Lookup */

void bdNodeManager::findDhtValue(bdNodeId *id, std::string key, uint32_t mode)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::findDhtValue() TODO";
	std::cerr << std::endl;
#endif
}


        /***** Get Results Details *****/
int bdNodeManager::getDhtPeerAddress(bdNodeId *id, struct sockaddr_in &from)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::getDhtPeerAddress() Id: ";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << " ... ? TODO" << std::endl;
#endif

	return 1;
}

int bdNodeManager::getDhtValue(bdNodeId *id, std::string key, std::string &value)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::getDhtValue() Id: ";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << " key: " << key;
	std::cerr << " ... ? TODO" << std::endl;
#endif

	return 1;
}



        /***** Add / Remove Callback Clients *****/
void bdNodeManager::addCallback(BitDhtCallback *cb)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
        if (it == mCallbacks.end())
        {
                /* add it */
                mCallbacks.push_back(cb);
        }
}

void bdNodeManager::removeCallback(BitDhtCallback *cb)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::removeCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
        if (it == mCallbacks.end())
        {
                /* not found! */
                return;
        }
        it = mCallbacks.erase(it);
}


void bdNodeManager::addPeer(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addPeer() Overloaded (doing Callback)";
	std::cerr << std::endl;
#endif
	doNodeCallback(id, peerflags);

	// call parent.
	bdNode::addPeer(id, peerflags);

        return;
}



void bdNodeManager::doNodeCallback(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doPeerCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtNodeCallback(id, peerflags);
        }
        return;
}

void bdNodeManager::doPeerCallback(const bdNodeId *id, uint32_t status)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doPeerCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtPeerCallback(id, status);
        }
        return;
}

void bdNodeManager::doValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doValueCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtPeerCallback(id, status);
        }
        return;
}

        /******************* Internals *************************/
int     bdNodeManager::isBitDhtPacket(char *data, int size, struct sockaddr_in &from)

{
#ifdef DEBUG_MGR_PKT
	std::cerr << "bdNodeManager::isBitDhtPacket() *******************************";
        std::cerr << " from " << inet_ntoa(from.sin_addr);
        std::cerr << ":" << ntohs(from.sin_port);
	std::cerr << std::endl;
	{
		/* print the fucker... only way to catch bad ones */
		std::ostringstream out;
		for(int i = 0; i < size; i++)
		{
			if (isascii(data[i]))
			{
				out << data[i];
			}
			else
			{
				out << "[";
		                out << std::setw(2) << std::setfill('0') 
					<< std::hex << (uint32_t) data[i];
				out << "]";
			}
			if ((i % 16 == 0) && (i != 0))
			{
				out << std::endl;
			}
		}
		std::cerr << out.str();
	}
	std::cerr << "bdNodeManager::isBitDhtPacket() *******************************";
	std::cerr << std::endl;
#endif

	/* try to parse it! */
        /* convert to a be_node */
        be_node *node = be_decoden(data, size);
        if (!node)
        {
                /* invalid decode */
#ifdef DEBUG_MGR
                std::cerr << "bdNodeManager::isBitDhtPacket() be_decode failed. dropping";
		std::cerr << std::endl;
		std::cerr << "bdNodeManager::BadPacket ******************************";
        	std::cerr << " from " << inet_ntoa(from.sin_addr);
        	std::cerr << ":" << ntohs(from.sin_port);
		std::cerr << std::endl;
		{
			/* print the fucker... only way to catch bad ones */
			std::ostringstream out;
			for(int i = 0; i < size; i++)
			{
				if (isascii(data[i]))
				{
					out << data[i];
				}
				else
				{
					out << "[";
			                out << std::setw(2) << std::setfill('0') 
						<< std::hex << (uint32_t) data[i];
					out << "]";
				}
				if ((i % 16 == 0) && (i != 0))
				{
					out << std::endl;
				}
			}
			std::cerr << out.str();
		}
		std::cerr << "bdNodeManager::BadPacket ******************************";
		std::cerr << std::endl;
#endif
		return 0;
	}

        /* find message type */
        uint32_t beType = beMsgType(node);	
	int ans = (beType != BITDHT_MSG_TYPE_UNKNOWN);
	be_free(node);

#ifdef DEBUG_MGR_PKT
	if (ans)
	{
		std::cerr << "bdNodeManager::isBitDhtPacket() YES";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "bdNodeManager::isBitDhtPacket() NO: Unknown Type";
		std::cerr << std::endl;
	}
	
#endif
	return ans;
}


bdDebugCallback::~bdDebugCallback()
{
}

int bdDebugCallback::dhtPeerCallback(const bdNodeId *id, uint32_t status)
{
	std::cerr << "bdDebugCallback::dhtPeerCallback() Id: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << " status: " << std::hex << status << std::dec << std::endl;
	return 1;
}

int bdDebugCallback::dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "bdDebugCallback::dhtValueCallback() Id: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << " key: " << key;
	std::cerr << " status: " << std::hex << status << std::dec << std::endl;

	return 1;
}


