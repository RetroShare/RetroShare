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


//	p3FastService(uint16_t type) 
//	:pqiService((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + (((uint32_t) type) << 8)), 


class p3FastService: public pqiService
{
protected:
	p3FastService() : pqiService(), srvMtx("p3FastService")
	{
		rsSerialiser = new RsSerialiser();
	}

public:
	virtual ~p3FastService() { delete rsSerialiser; }

	/*************** INTERFACE ******************************/
	int sendItem(RsItem *);
	virtual int	tick() { return 0; }
	/*************** INTERFACE ******************************/


	// overloaded pqiService interface.
	virtual bool recv(RsRawItem *);

	// called by recv().
	virtual bool recvItem(RsItem *item) = 0;


protected:
	void addSerialType(RsSerialType *);

	RsMutex srvMtx; /* below locked by Mutex */

	RsSerialiser *rsSerialiser;
};


class p3Service: public p3FastService
{
	protected:

	p3Service() 
	:p3FastService()
	{
		return; 
	}

	public:

/*************** INTERFACE ******************************/
        /* called from Thread/tick/GUI */
//int             sendItem(RsItem *);
RsItem *        recvItem();
bool		receivedItems();

//virtual int	tick() { return 0; }
/*************** INTERFACE ******************************/

	public:
	// overloaded p3FastService interface.
virtual bool	recvItem(RsItem *item);

	private:

	/* below locked by srvMtx Mutex */
	std::list<RsItem *> recv_queue;
};


class nullService: public pqiService
{
	protected:

	nullService() 
	:pqiService()
	{
		return; 
	}

//virtual int	tick() 

	public:
	// overloaded NULL pqiService interface.
virtual bool	recv(RsRawItem *item)
	{
		/* drop any items */
		delete item;
		return true;
	}

virtual bool	send(RsRawItem *item)
	{
		delete item;
		return true;
	}

};


class p3ThreadedService: public p3Service, public RsTickingThread
{
	protected:

	p3ThreadedService() 
	:p3Service() { return; }

	public:

virtual ~p3ThreadedService() { return; }

	private:

};


#endif // P3_GENERIC_SERVICE_HEADER

