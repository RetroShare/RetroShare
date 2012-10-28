/*
 * libretroshare/src/dht: bdhandler.h
 *
 * BitDht example interface 
 *
 * Copyright 2009-2010 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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


#include <udp/udpstack.h>
#include <udp/udpbitdht.h>
#include <bitdht/bdstddht.h>

#include <string.h>

#include "bdhandler.h"


/**** 
 * This example bitdht app is designed to perform a single shot DHT search.
 * Ww want to minimise the dht work, and number of UDP packets sent.
 *
 * This means we need to add:
 * - don't search for App network.        (libbitdht option)
 * - don't bother filling up Space.       (libbitdht option)
 * - Programmatically add bootstrap peers. (libbitdht option)
 *
 */



/* This is a conversion callback class 
 */

class BdCallback: public BitDhtCallback
{
	public:

	BdCallback(BitDhtHandler *parent)
	:mParent(parent) { return; }

virtual int dhtNodeCallback(const bdId *id, uint32_t peerflags)
{
	return mParent->NodeCallback(id, peerflags);
}

virtual int dhtPeerCallback(const bdId *id, uint32_t status)
{
	return mParent->PeerCallback(id, status);
}

virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	return mParent->ValueCallback(id, key, status);
}

virtual int dhtConnectCallback(const bdId*, const bdId*, const bdId*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
	return 1;
}

virtual int dhtInfoCallback(const bdId*, uint32_t, uint32_t, std::string)
{
	return 1;
}

	private:

	BitDhtHandler *mParent;
};


BitDhtHandler::BitDhtHandler(bdNodeId *ownId, uint16_t port, std::string appId, std::string bootstrapfile)
{
	std::cerr << "BitDhtHandler::BitDhtHandler()" << std::endl;
	std::cerr << "Using Id: ";
	bdStdPrintNodeId(std::cerr, ownId);
	std::cerr << std::endl;

	std::cerr << "Using Bootstrap File: " << bootstrapfile;
	std::cerr << std::endl;
	std::cerr << "Converting OwnId to bdNodeId....";
	std::cerr << std::endl;

	/* standard dht behaviour */
	bdDhtFunctions *stdfns = new bdStdDht();

	std::cerr << "BitDhtHandler() startup ... creating UdpBitDht";
	std::cerr << std::endl;

	/* create dht */
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = 0;
        local.sin_port = htons(port);

	mStack = new UdpStack(local);

	mUdpBitDht = new UdpBitDht(mStack, ownId, appId, bootstrapfile, stdfns);
	mStack->addReceiver(mUdpBitDht);

	/* setup callback to here */
	BdCallback *bdcb = new BdCallback(this);
	mUdpBitDht->addCallback(bdcb);

	std::cerr << "BitDhtHandler() starting threads and dht";
	std::cerr << std::endl;

	mUdpBitDht->start(); /* starts up the bitdht thread */

        /* setup best mode for quick search */
        uint32_t dhtFlags = BITDHT_MODE_TRAFFIC_MED | BITDHT_MODE_RELAYSERVERS_IGNORED;
        mUdpBitDht->setDhtMode(dhtFlags);
        mUdpBitDht->setAttachMode(false);

	/* switch on the dht too */
	mUdpBitDht->startDht();
}

	/* pqiNetAssist - external interface functions */
void    BitDhtHandler::enable(bool on)
{
        std::cerr << "p3BitDht::enable(" << on << ")";
        std::cerr << std::endl;
        if (on)
        {
                mUdpBitDht->startDht();
        }
        else
        {
                mUdpBitDht->stopDht();
        }
}
	
void    BitDhtHandler::shutdown() /* blocking call */
{
        mUdpBitDht->stopDht();
}


void	BitDhtHandler::restart()
{
        mUdpBitDht->stopDht();
        mUdpBitDht->startDht();
}

bool    BitDhtHandler::getEnabled()
{
        return (mUdpBitDht->stateDht() != 0);
}

bool    BitDhtHandler::getActive()
{
        return (mUdpBitDht->stateDht() >= BITDHT_MGR_STATE_ACTIVE);
}




	/* pqiNetAssistConnect - external interface functions */
	/* add / remove peers */
bool 	BitDhtHandler::FindNode(bdNodeId *peerId)
{
	std::cerr << "BitDhtHandler::FindNode(";
	bdStdPrintNodeId(std::cerr, peerId);
	std::cerr << ")" << std::endl;


	BssResult res;
        res.id.id = *peerId;
        res.mode = BSS_SINGLE_SHOT;
        res.status = 0;

	{
        	bdStackMutex stack(resultsMtx); /********** MUTEX LOCKED *************/
        	mSearchNodes[*peerId] = res;
	}

	/* add in peer */
	mUdpBitDht->addFindNode(peerId, BITDHT_QFLAGS_DISGUISE);

	return true ;
}

bool 	BitDhtHandler::DropNode(bdNodeId *peerId)
{
	std::cerr << "BitDhtHandler::DropNode(";
	bdStdPrintNodeId(std::cerr, peerId);
	std::cerr << ")" << std::endl;
	std::cerr << std::endl;

	/* remove in peer */
	mUdpBitDht->removeFindNode(peerId);

        bdStackMutex stack(resultsMtx); /********** MUTEX LOCKED *************/

	/* find the node from our list */
	std::map<bdNodeId, BssResult>::iterator it;
	it = mSearchNodes.find(*peerId);
	if (it != mSearchNodes.end())
	{
		std::cerr << "BitDhtHandler::DropNode() Found NodeId, removing";
		std::cerr << std::endl;

		mSearchNodes.erase(it);
	}
	return true ;
}


bool    BitDhtHandler::SearchResult(bdId *id, uint32_t &status)
{
        bdStackMutex stack(resultsMtx); /********** MUTEX LOCKED *************/

	/* find the node from our list */
	std::map<bdNodeId, BssResult>::iterator it;
	it = mSearchNodes.find(id->id);
	if (it != mSearchNodes.end())
	{
		if (it->second.status != 0)
		{
			std::cerr << "BitDhtHandler::SearchResults() Found Results";
			std::cerr << std::endl;
			status = it->second.status;
			*id = it->second.id;
			return true;
		}

		std::cerr << "BitDhtHandler::SearchResults() No Results Yet";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "BitDhtHandler::SearchResults() ERROR: No Search Entry";
	std::cerr << std::endl;
	return false;
}


/********************** Callback Functions **************************/

int BitDhtHandler::NodeCallback(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_BITDHT
	std::cerr << "BitDhtHandler::NodeCallback()";
	bdStdPrintNodeId(std::cerr, &(id->id));
	std::cerr << " flags: " << peerflags;
	std::cerr << std::endl;
#endif

	return 0;
}

int BitDhtHandler::PeerCallback(const bdId *id, uint32_t status)
{
	std::cerr << "BitDhtHandler::PeerCallback() NodeId: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;

        bdStackMutex stack(resultsMtx); /********** MUTEX LOCKED *************/

	/* find the node from our list */
	std::map<bdNodeId, BssResult>::iterator it;
	it = mSearchNodes.find(id->id);
	if (it == mSearchNodes.end())
	{
		std::cerr << "BitDhtHandler::PeerCallback() Unknown NodeId !!! ";
		std::cerr << std::endl;

		return 1;
	}
	it->second.status = status;

	bool connect = false;
	switch(status)
	{
  		case BITDHT_MGR_QUERY_FAILURE:
			/* do nothing */
			std::cerr << "BitDhtHandler::PeerCallback() QUERY FAILURE ... do nothin ";
			std::cerr << std::endl;


		break;

		case BITDHT_MGR_QUERY_PEER_OFFLINE:
			/* do nothing */

			std::cerr << "BitDhtHandler::PeerCallback() QUERY PEER OFFLINE ... do nothin ";
			std::cerr << std::endl;

			break;

		case BITDHT_MGR_QUERY_PEER_UNREACHABLE:
			/* do nothing */

			std::cerr << "BitDhtHandler::PeerCallback() QUERY PEER UNREACHABLE ... saving address ";
			std::cerr << std::endl;
			it->second.id = *id;

		break;

		case BITDHT_MGR_QUERY_PEER_ONLINE:
			/* do something */

			std::cerr << "BitDhtHandler::PeerCallback() QUERY PEER ONLINE ... saving address";
			std::cerr << std::endl;

			it->second.id = *id;
		break;
	}
	return 1;
}



int BitDhtHandler::ValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "BitDhtHandler::ValueCallback() NOOP for NOW";
	std::cerr << std::endl;

	std::cerr << "BitDhtHandler::ValueCallback()";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << " key: " << key;
	std::cerr << " status: " << status;
	std::cerr << std::endl;

	/* ignore for now */
	return 0;
}

