/*
 * "$Id: p3PortService.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "services/p3portservice.h"
#include "serialiser/rsserviceids.h"

p3PortService::p3PortService(p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_PORT), mConnMgr(cm), mEnabled(false)
{
	/* For Version 1, we'll just use the very simple RsRawItem packets 
	 * which are handled in the RsServiceSerialiser 
	 */
	addSerialType(new RsServiceSerialiser());
}

int	p3PortService::tick()
{
#ifdef PORT_DEBUG
	std::cerr << "p3PortService::tick()";
	std::cerr << std::endl;
#endif

	/* discard data if not enabled */
	if (!mEnabled)
	{
		RsItem *i;
		while(NULL != (i = recvItem()))
		{
			delete i;
		}
	}

	/* Rough list of what might happen (once the sockets are opened!) */


	/* check for data on sockets */
	//int sockfd;
	RsRawItem *item;

	//while(0 < (len = recv(sockfd, ....)))  ... etc.
	{
	
		/* example of how to package data up 
		 * We use RsRawItem as it'll handle a single binary chunk.
		 * This is enough for version 1.... but versions 2 + 3 will 
		 * need their own serialiser.
		 *
		 * */

		/* input data TODO! */
		void *data=NULL;
		uint32_t len=0;

		uint32_t packetId = (((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + 
				(((uint32_t) RS_SERVICE_TYPE_PORT) << 8);

		/* create Packet for RS Transport */	
		RsRawItem *item = new RsRawItem(packetId, len);
	        void *item_data = item->getRawData();
		memcpy(item_data, data, len);

		/* set the correct peer destination. */
		item->PeerId(mPeerId);
	
		/* send data */
		sendItem(item);

		/* no delete -> packet cleaned up by RS internals */
	}
	

	/* get any incoming data */
	while(NULL != (item = (RsRawItem*) recvItem()))
	{
		/* unpackage data */
		std::string src = item->PeerId();
//	        void *item_data = item->getRawData();
//		uint32_t item_len = item->getRawLength();

		/* push out to the socket .... */
		// send(sockfd, item_len, item_data....)

		/* cleanup packet */
		delete item;
	}

	return 0;
}

bool    p3PortService::enablePortForwarding(uint32_t port, std::string peerId)
{
	mPort = port;
	mPeerId = peerId;
	mEnabled = true;

	return true;
}



