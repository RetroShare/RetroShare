/*******************************************************************************
 * libretroshare/src/pqi: pqiservice.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 by Robert Fernie.                                       *
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
#include "pqi/pqiservice.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"

#ifdef  SERVICE_DEBUG
const int pqiservicezone = 60478;
#endif

/****
 * #define SERVICE_DEBUG 1
 ****/

void pqiService::setServiceServer(p3ServiceServerIface *server)
{
	mServiceServer = server;
}

bool pqiService::send(RsRawItem *item)
{
	return mServiceServer->sendItem(item);
}


p3ServiceServer::p3ServiceServer(pqiPublisher *pub, p3ServiceControl *ctrl) : mPublisher(pub), mServiceControl(ctrl), srvMtx("p3ServiceServer") 
{
	RS_STACK_MUTEX(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::p3ServiceServer()");
#endif

	return;
}

int	p3ServiceServer::addService(pqiService *ts, bool defaultOn)
{
	RS_STACK_MUTEX(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::addService()");
#endif

	RsServiceInfo info = ts->getServiceInfo();
	std::map<uint32_t, pqiService *>::iterator it;
	it = services.find(info.mServiceType);
	if (it != services.end())
	{
		std::cerr << "p3ServiceServer::addService(): Service already added with id " << info.mServiceType << "!" << std::endl;
		// it exists already!
		return -1;
	}

	ts->setServiceServer(this);
	services[info.mServiceType] = ts;

	// This doesn't need to be in Mutex.
	mServiceControl->registerService(info,defaultOn);

	return 1;
}

bool p3ServiceServer::getServiceItemNames(uint32_t service_type,std::map<uint8_t,std::string>& names)
{
	RS_STACK_MUTEX(srvMtx); /********* LOCKED *********/

 	std::map<uint32_t, pqiService *>::iterator it=services.find(service_type) ;

	if(it != services.end())
    {
	   it->second->getItemNames(names) ;
       return true ;
    }
    else
        return false ;
}

int p3ServiceServer::removeService(pqiService *ts)
{
	RS_STACK_MUTEX(srvMtx); /********* LOCKED *********/

#ifdef SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, "p3ServiceServer::removeService()");
#endif

	RsServiceInfo info = ts->getServiceInfo();

	// This doesn't need to be in Mutex.
	mServiceControl->deregisterService(info.mServiceType);

	std::map<uint32_t, pqiService *>::iterator it = services.find(info.mServiceType);
	if (it == services.end())
	{
		std::cerr << "p3ServiceServer::removeService(): Service not found with id " << info.mServiceType << "!" << std::endl;
		return -1;
	}

	services.erase(it);

	return 1;
}

bool	p3ServiceServer::recvItem(RsRawItem *item)
{
	// Packet Filtering.
	// This doesn't need to be in Mutex.
	if (!mServiceControl->checkFilter(item->PacketId() & 0xffffff00, item->PeerId()))
	{
		delete item;
		return false;
	}

	pqiService *s = NULL;

	// access the service map under mutex lock
	{
		RS_STACK_MUTEX(srvMtx);
		auto it = services.find(item -> PacketId() & 0xffffff00);
		if (it == services.end())
		{
			delete item;
			return false;
		}
		s = it->second;
	}

	// then call recv off mutex
	bool result = s->recv(item);
	return result;
}

bool p3ServiceServer::sendItem(RsRawItem *item)
{
#ifdef  SERVICE_DEBUG
	std::cerr << "p3ServiceServer::sendItem()";
	std::cerr << std::endl;
	item -> print(std::cerr);
	std::cerr << std::endl;
#endif
	if (!item)
	{
		std::cerr << "p3ServiceServer::sendItem() Caught Null item";
		std::cerr << std::endl;
		return false;
	}

	// Packet Filtering.
	if (!mServiceControl->checkFilter(item->PacketId() & 0xffffff00, item->PeerId()))
	{
#ifdef  SERVICE_DEBUG
		std::cerr << "p3ServiceServer::sendItem() Fails Filtering for packet id=" << std::hex << item->PacketId() << std::dec << ", and peer " << item->PeerId() << std::endl;
#endif
		delete item;
		return false;
	}

	mPublisher->sendItem(item);

	return true;
}

int	p3ServiceServer::tick()
{
	mServiceControl->tick();

	// make a copy of the service map
	std::map<uint32_t,pqiService *> local_map;
	{	
		RS_STACK_MUTEX(srvMtx);
		local_map=services;
	}

	// tick all services off mutex
	for(auto it(local_map.begin());it!=local_map.end();++it)
	{
		(it->second)->tick();
	}

	return 1;

}
