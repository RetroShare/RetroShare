/*
 * libretroshare/src/pqi pqiperson.h
 *
 * 3P/PQI network interface for RetroShare.
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



#ifndef MRK_PQI_PERSON_HEADER
#define MRK_PQI_PERSON_HEADER


#include "pqi/pqi.h"

#include <list>

class pqiperson;
class RsPeerCryptoParams ;

static const int CONNECT_RECEIVED     = 1; 
static const int CONNECT_SUCCESS      = 2;
static const int CONNECT_UNREACHABLE  = 3;
static const int CONNECT_FIREWALLED   = 4;
static const int CONNECT_FAILED       = 5;

static const int HEARTBEAT_REPEAT_TIME = 5;

#include "pqi/pqiqosstreamer.h"

class pqiconnect: public pqiQoSstreamer, public NetInterface
{
public:
	pqiconnect(RsSerialiser *rss, NetBinInterface *ni_in)
			:pqiQoSstreamer(rss, ni_in->PeerId(), ni_in, 0),  // pqistreamer will cleanup NetInterface.
			 NetInterface(NULL, ni_in->PeerId()), // No need for callback
	 		 ni(ni_in) 
	{ 
		if (!ni_in)
		{
			std::cerr << "pqiconnect::pqiconnect() NetInterface == NULL, FATAL!";
			std::cerr << std::endl;
			exit(1);
		}
		return; 
	} 

virtual ~pqiconnect() { return; }
virtual bool getCryptoParams(RsPeerCryptoParams& params) ;

	// presents a virtual NetInterface -> passes to ni.
virtual int	connect(struct sockaddr_in raddr) { return ni->connect(raddr); }
virtual int	listen() 		{ return ni -> listen(); }
virtual int	stoplistening() 	{ return ni -> stoplistening(); }
virtual int 	reset() 		{ return ni -> reset(); }
virtual int 	disconnect() 		{ return ni -> reset(); }
virtual bool 	connect_parameter(uint32_t type, uint32_t value) { return ni -> connect_parameter(type, value);}
virtual bool 	connect_additional_address(uint32_t type, struct sockaddr_in *addr) { return ni -> connect_additional_address(type, addr);}



virtual int     getConnectAddress(struct sockaddr_in &raddr){ return ni->getConnectAddress(raddr); }

	// get the contact from the net side!
virtual std::string PeerId()
{
	if (ni)
	{
		return ni->PeerId();
	}
	else
	{
		return PQInterface::PeerId();
	}
}

	// to check if our interface.
virtual bool	thisNetInterface(NetInterface *ni_in) { return (ni_in == ni); }
protected:
	NetBinInterface *ni;
protected:
};


class pqipersongrp;

class pqiperson: public PQInterface
{
public:
	pqiperson(std::string id, pqipersongrp *ppg);
virtual ~pqiperson(); // must clean up children.

	// control of the connection.
int 	reset();
int 	listen();
int 	stoplistening();
	
int		connect(uint32_t type, struct sockaddr_in raddr, 
				struct sockaddr_in &proxyaddr, struct sockaddr_in &srcaddr,
				uint32_t delay, uint32_t period, uint32_t timeout, uint32_t flags, uint32_t bandwidth);
	
int     receiveHeartbeat();
	// add in connection method.
int	addChildInterface(uint32_t type, pqiconnect *pqi);

virtual bool getCryptoParams(RsPeerCryptoParams&) ;

	// The PQInterface interface.
virtual int     SendItem(RsItem *,uint32_t& serialized_size);
virtual int     SendItem(RsItem *item)
{
	std::cerr << "Warning pqiperson::sendItem(RsItem*) should not be called. Plz call SendItem(RsItem *,uint32_t& serialized_size) instead." << std::endl;
	uint32_t serialized_size ;
	return SendItem(item,serialized_size) ;
}
virtual RsItem *GetItem();
	
virtual int 	status();
virtual int	tick();

// overloaded callback function for the child - notify of a change.
int 	notifyEvent(NetInterface *ni, int event);

// PQInterface for rate control overloaded....
virtual int     getQueueSize(bool in);
virtual void    getRates(RsBwRates &rates);
virtual float   getRate(bool in);
virtual void    setMaxRate(bool in, float val);
virtual void    setRateCap(float val_in, float val_out);

pqiconnect *getKid(uint32_t type);

	private:

	std::map<uint32_t, pqiconnect *> kids;
	bool active;
	pqiconnect *activepqi;
	bool inConnectAttempt;
	int waittimes;
        time_t lastHeartbeatReceived;//use to track connection failure

private: /* Helper functions */


	pqipersongrp *pqipg; /* parent for callback */
};



#endif

