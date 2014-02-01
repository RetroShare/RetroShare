/*
 * libretroshare/src/services p3service.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "pqi/pqi.h"
#include "util/rsstring.h"
#include "services/p3service.h"
#include <iomanip>

#ifdef WINDOWS_SYS
#include <time.h>
#endif

/*****
 * #define SERV_DEBUG 1
 ****/



RsItem *p3Service::recvItem()
{
	RsStackMutex stack(srvMtx);  /*****   LOCK MUTEX *****/

	if (recv_queue.size() == 0)
	{
		return NULL; /* nothing there! */
	}

	/* get something off front */
	RsItem *item = recv_queue.front();
	recv_queue.pop_front();

	return item;
}


bool    p3Service::receivedItems()
{
	RsStackMutex stack(srvMtx);  /*****   LOCK MUTEX *****/

	return (!recv_queue.empty());
}


bool p3Service::recvItem(RsItem *item)
{
	if (item)
	{
		RsStackMutex stack(srvMtx);  /*****   LOCK MUTEX *****/

		recv_queue.push_back(item);
	}
	return true;
}





void    p3FastService::addSerialType(RsSerialType *st)
{
	rsSerialiser->addSerialType(st);
}


	// overloaded pqiService interface.
bool p3FastService::recv(RsRawItem *raw)
{
	RsItem *item = NULL;
	{
		RsStackMutex stack(srvMtx);  /*****   LOCK MUTEX *****/
	
	#ifdef SERV_DEBUG 
		std::cerr << "p3Service::recv()";
		std::cerr << std::endl;
	#endif
	
		/* convert to RsServiceItem */
		uint32_t size = raw->getRawLength();
		item = rsSerialiser->deserialise(raw->getRawData(), &size);
		if ((!item) || (size != raw->getRawLength()))
		{
			/* error in conversion */
	#ifdef SERV_DEBUG 
			std::cerr << "p3Service::recv() Error" << std::endl;
			std::cerr << "p3Service::recv() Size: " << size << std::endl;
			std::cerr << "p3Service::recv() RawLength: " << raw->getRawLength() << std::endl;
	#endif
	
			if (item)
			{
	#ifdef SERV_DEBUG 
				std::cerr << "p3Service::recv() Bad Item:";
				std::cerr << std::endl;
				item->print(std::cerr, 0);
				std::cerr << std::endl;
	#endif
				delete item;
				item=NULL ;
			}
		}
	}
	
	/* if we have something - pass it on */
	if (item)
	{
#ifdef SERV_DEBUG 
		std::cerr << "p3Service::recv() item:";
		std::cerr << std::endl;
		item->print(std::cerr, 0);
		std::cerr << std::endl;
#endif

		/* ensure PeerId is transferred */
		item->PeerId(raw->PeerId());
		recvItem(item);
	}

	/* cleanup input */
	delete raw;
	return (item != NULL);
}



int p3FastService::sendItem(RsItem *si)
{
	RsStackMutex stack(srvMtx);  /*****   LOCK MUTEX *****/

#ifdef SERV_DEBUG 
	std::cerr << "p3Service::sendItem() Sending item:";
	std::cerr << std::endl;
	si->print(std::cerr, 0);
	std::cerr << std::endl;
#endif

	/* try to convert */
	uint32_t size = rsSerialiser->size(si);
	if (!size)
	{
		std::cerr << "p3Service::send() ERROR size == 0";
		std::cerr << std::endl;

		/* can't convert! */
		delete si;
		return 0;
	}

	RsRawItem *raw = new RsRawItem(si->PacketId(), size);
	if (!rsSerialiser->serialise(si, raw->getRawData(), &size))
	{
		std::cerr << "p3Service::send() ERROR serialise failed";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	if ((raw) && (size != raw->getRawLength()))
	{
		std::cerr << "p3Service::send() ERROR serialise size mismatch";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	/* ensure PeerId is transferred */
	if (raw)
	{
		raw->PeerId(si->PeerId());

		if(si->priority_level() == QOS_PRIORITY_UNKNOWN)
		{
			std::cerr << "************************************************************" << std::endl;
			std::cerr << "********** Warning: p3Service::send()              ********" << std::endl;
			std::cerr << "********** Warning: caught a RsItem with undefined  ********" << std::endl;
			std::cerr << "**********          priority level. That should not ********" << std::endl;
			std::cerr << "**********          happen. Please fix your items!  ********" << std::endl;
			std::cerr << "************************************************************" << std::endl;
		}
		raw->setPriorityLevel(si->priority_level()) ;
	}

	/* cleanup */
	delete si;

#ifdef SERV_DEBUG
	std::cerr << "p3Service::send() returning RawItem.";
	std::cerr << std::endl;
#endif
	return pqiService::send(raw);	
}


