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

#include "bdhandler.h"

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

virtual int dhtPeerCallback(const bdNodeId *id, uint32_t status)
{
	return mParent->PeerCallback(id, status);
}

virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	return mParent->ValueCallback(id, key, status);
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

	/* add in peer */
	mUdpBitDht->addFindNode(peerId, BITDHT_QFLAGS_DO_IDLE);

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

	return true ;
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

int BitDhtHandler::PeerCallback(const bdNodeId *id, uint32_t status)
{
	std::cerr << "BitDhtHandler::PeerCallback() NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;

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

			std::cerr << "BitDhtHandler::PeerCallback() QUERY PEER UNREACHABLE ... flag? / do nothin ";
			std::cerr << std::endl;


		break;

		case BITDHT_MGR_QUERY_PEER_ONLINE:
			/* do something */

			std::cerr << "BitDhtHandler::PeerCallback() QUERY PEER ONLINE ... try udp connection";
			std::cerr << std::endl;

			connect = true;
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

