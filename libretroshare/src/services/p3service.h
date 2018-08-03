/*******************************************************************************
 * libretroshare/src/services: p3service.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 Robert Fernie <retroshare@lunamutt.com>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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
//	  :pqiService((RsServiceInfo::RsServiceInfoUIn16ToFullServiceId(type)),


class p3FastService: public pqiService
{
	protected:

	p3FastService() 
	:pqiService(), 
	srvMtx("p3FastService"), rsSerialiser(NULL)
	{
		rsSerialiser = new RsSerialiser();
		return; 
	}

	public:

virtual ~p3FastService() { delete rsSerialiser; return; }

/*************** INTERFACE ******************************/
int             sendItem(RsItem *);
virtual int	tick() { return 0; }
/*************** INTERFACE ******************************/

	public:
	// overloaded pqiService interface.
virtual bool	recv(RsRawItem *);

	// called by recv().
virtual bool	recvItem(RsItem *item) = 0;


	protected:
void 	addSerialType(RsSerialType *);

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

