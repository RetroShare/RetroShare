/*******************************************************************************
 * libretroshare/src/pqi: pqiservice.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie.                                       *
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
#ifndef PQI_SERVICE_HEADER
#define PQI_SERVICE_HEADER

#include "pqi/pqi.h"
#include "pqi/pqi_base.h"
#include "util/rsthreads.h"

#include "retroshare/rsservicecontrol.h"
#include "pqi/p3servicecontrol.h"

// PQI Service, is a generic lower layer on which services can run on.
// 
// these packets get passed through the 
// server, to a service that is registered.
//
// example services:
// 	proxytunnel. -> p3proxy.
// 	sockettunnel
// 		-> either broadcast (attach to 
// 				open socket instead
// 				of broadcast address)
// 		-> or requested/signon.
//
// 	games,
// 	voice
// 	video
//
//
// DataType is defined in the serialiser directory.

class RsRawItem;
class p3ServiceServerIface;


class pqiService
{
protected:

	pqiService() // our type of packets.
	    :mServiceServer(NULL) { return; }

	virtual ~pqiService() { return; }

public:
	void 	setServiceServer(p3ServiceServerIface *server);
	//
	virtual bool	recv(RsRawItem *) = 0;
	virtual bool	send(RsRawItem *item);

	virtual RsServiceInfo getServiceInfo() = 0;

	virtual int	tick() { return 0; }

	virtual void getItemNames(std::map<uint8_t,std::string>& /*names*/) const {}	// This does nothing by default. Service should derive it in order to give info for the UI

private:
	p3ServiceServerIface *mServiceServer; // const, no need for mutex.
};

#include <map>

/* We are pushing the packets back through p3ServiceServer, 
 * so that we can filter services at this level later...
 * if we decide not to do this, pqiService can call through
 * to the base level pqiPublisher instead.
 */

// use interface to allow DI
class p3ServiceServerIface
{
public:

	virtual ~p3ServiceServerIface() {}


	virtual bool	recvItem(RsRawItem *) = 0;
	virtual bool	sendItem(RsRawItem *) = 0;

	virtual bool    getServiceItemNames(uint32_t service_type,std::map<uint8_t,std::string>& names) =0;
};

class p3ServiceServer : public p3ServiceServerIface
{
public:
	p3ServiceServer(pqiPublisher *pub, p3ServiceControl *ctrl);

	int	addService(pqiService *, bool defaultOn);
	int	removeService(pqiService *);

	bool	recvItem(RsRawItem *);
	bool	sendItem(RsRawItem *);

	bool getServiceItemNames(uint32_t service_type, std::map<uint8_t,std::string>& names) ;

	int	tick();
public:

private:

	pqiPublisher *mPublisher;	// constant no need for mutex.
	p3ServiceControl *mServiceControl;

	RsMutex srvMtx;
	std::map<uint32_t, pqiService *> services;

};


#endif // PQI_SERVICE_HEADER
