/*******************************************************************************
 * libretroshare/src/pqi: pqiservice.h                                         *
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
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_BASIC, pqiservicezone, 
		"p3ServiceServer::p3ServiceServer()");
#endif

	return;
}

int	p3ServiceServer::addService(pqiService *ts, bool defaultOn)
{
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

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
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

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
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

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
	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	std::cerr << "p3ServiceServer::incoming()";
	std::cerr << std::endl;

	{
		std::string out;
		rs_sprintf(out, "p3ServiceServer::incoming() PacketId: %x\nLooking for Service: %x\nItem:\n", item -> PacketId(), (item -> PacketId() & 0xffffff00));
		item -> print_string(out);
		std::cerr << out;
		std::cerr << std::endl;
	}
#endif

	// Packet Filtering.
	// This doesn't need to be in Mutex.
	if (!mServiceControl->checkFilter(item->PacketId() & 0xffffff00, item->PeerId()))
	{
#ifdef  SERVICE_DEBUG
        std::cerr << "p3ServiceServer::recvItem() Fails Filtering " << std::endl;
#endif
		delete item;
		return false;
	}


	std::map<uint32_t, pqiService *>::iterator it;
	it = services.find(item -> PacketId() & 0xffffff00);
	if (it == services.end())
	{
#ifdef  SERVICE_DEBUG
		std::cerr << "p3ServiceServer::incoming() Service: No Service - deleting";
		std::cerr << std::endl;
#endif
		delete item;
		return false;
	}

	{
#ifdef  SERVICE_DEBUG
		std::cerr << "p3ServiceServer::incoming() Sending to : " << (void *) it -> second;
		std::cerr << std::endl;
#endif

		return (it->second) -> recv(item);
	}

	delete item;
	return false;
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

	RsStackMutex stack(srvMtx); /********* LOCKED *********/

#ifdef  SERVICE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqiservicezone, 
		"p3ServiceServer::tick()");
#endif

	std::map<uint32_t, pqiService *>::iterator it;

	// from the beginning to where we started.
	for(it = services.begin();it != services.end(); ++it)
	{

#ifdef  SERVICE_DEBUG
		std::string out;
		rs_sprintf(out, "p3ServiceServer::service id: %u -> Service: %p", it -> first, it -> second);
		pqioutput(PQL_DEBUG_ALL, pqiservicezone, out);
#endif

		// now we should actually tick the service.
		(it -> second) -> tick();
	}
	return 1;
}



