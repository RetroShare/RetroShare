/*
 * libbitdht/src/example/bdhandler.h
 *
 * BitDht interface example
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


#ifndef BITDHT_HANDLER_H
#define BITDHT_HANDLER_H

#include <string>
#include <map>

#include "udp/udpstack.h"
#include "udp/udpbitdht.h"
#include "bitdht/bdiface.h"

/*** This class can be overloaded to use the XXXXCallback() Functions *****/
class BitDhtIntCallback;

class BitDhtHandler
{

	public:

	BitDhtHandler(bdNodeId *ownId, uint16_t port, std::string appId, std::string bootstrapfile);

void    enable(bool on);  
void    shutdown(); /* blocking call */
void	restart();

bool    getEnabled();
bool    getActive();

bool	FindNode(bdNodeId *peerId);
bool	DropNode(bdNodeId *peerId);

virtual int 	NodeCallback(const bdId *id, uint32_t peerflags);			
virtual int 	PeerCallback(const bdNodeId *id, uint32_t status);
virtual int 	ValueCallback(const bdNodeId *id, std::string key, uint32_t status);

	private:

	/* real DHT classes */
	UdpStack *mStack;
	UdpBitDht *mUdpBitDht; 

};


#endif /* BITDHT_HANDLER_H */

