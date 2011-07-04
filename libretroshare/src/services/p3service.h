/*
 * libretroshare/src/services p3service.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#ifndef P3_GENERIC_SERVICE_HEADER
#define P3_GENERIC_SERVICE_HEADER

#include "pqi/pqi.h"
#include "pqi/pqiservice.h"
#include "util/rsthreads.h"

/* This provides easy to use extensions to the pqiservice class provided in src/pqi.
 * 
 * We will have a number of different strains.
 *
 * (1) p3Service -> pqiService
 *    
 *    Basic service with serialisation handled by a RsSerialiser.
 * 
 * (2) p3ThreadedService -> p3service.
 *
 *    Independent thread with mutex locks for i/o Queues.
 *     ideal for games etc.
 *
 * (3) p3CacheService -> p3service + CacheSource + CacheStore.
 *
 *    For both Cached and Messages.
 */

std::string generateRandomServiceId();

//TODO : encryption and upload / download rate implementation

class p3Service: public pqiService
{
	protected:

	p3Service(uint16_t type) 
	:pqiService((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + (((uint32_t) type) << 8)), 
	srvMtx("p3Service"), rsSerialiser(NULL)
	{
		rsSerialiser = new RsSerialiser();
		return; 
	}

	public:

virtual ~p3Service() { delete rsSerialiser; return; }

/*************** INTERFACE ******************************/
        /* called from Thread/tick/GUI */
int             sendItem(RsItem *);
RsItem *        recvItem();
bool		receivedItems();

virtual int	tick() { return 0; }
/*************** INTERFACE ******************************/


	public:
	// overloaded pqiService interface.
virtual int		receive(RsRawItem *);
virtual RsRawItem *	send();

	protected:
void 	addSerialType(RsSerialType *);

	private:

	RsMutex srvMtx;
	/* below locked by Mutex */

	RsSerialiser *rsSerialiser;
	std::list<RsItem *> recv_queue, send_queue;
};


class nullService: public pqiService
{
	protected:

	nullService(uint16_t type) 
	:pqiService((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + (((uint32_t) type) << 8))
	{
		return; 
	}

//virtual int	tick() 

	public:
	// overloaded NULL pqiService interface.
virtual int		receive(RsRawItem *item)
	{
		/* drop any items */
		delete item;
		return 1;
	}

virtual RsRawItem *	send()
	{
		return NULL;
	}

};


class p3ThreadedService: public p3Service, public RsThread
{
	protected:

	p3ThreadedService(uint16_t type) 
	:p3Service(type) { return; }

	public:

virtual ~p3ThreadedService() { return; }

	private:

};


#endif // P3_GENERIC_SERVICE_HEADER

