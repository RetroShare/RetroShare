/*
 * "$Id: pqipersongrp.h,v 1.13 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_PERSON_HANDLER_HEADER
#define MRK_PQI_PERSON_HANDLER_HEADER

#include "pqi/pqihandler.h"
#include "pqi/pqiperson.h"
#include "pqi/pqiservice.h"


// So this is a specific implementation 
//
// it is designed to have one pqilistensocket + a series of pqisockets
//
// as an added bonus, we are going to 
// make this a pqitunnelserver, to which services can be attached.

#ifdef PQI_USE_PROXY
	class p3proxy;
	class pqiudplistener;
#endif


const unsigned long PQIPERSON_NO_SSLLISTENER = 	0x0001;

const unsigned long PQIPERSON_ALL_BW_LIMITED =  0x0010;

#ifdef PQI_USE_DISC
	class p3disc;
#endif

#ifdef PQI_USE_CHANNELS
	class p3channel;
#endif

class pqissllistener;

class pqipersongrp: public pqihandler, public p3ServiceServer
{
	public:
	pqipersongrp(SecurityPolicy *, sslroot *sr, unsigned long flags);

	// control the connections.
int	cert_accept(cert *a);
int	cert_deny(cert *a);
int	cert_auto(cert *a, bool b);

int	restart_listener();

int	save_config();
int	load_config();

	// tick interfaces.
virtual int tick();
virtual int status();

	// + SearchInterface which should automatically handle stuff

	// acess to services.
#ifdef PQI_USE_DISC
	p3disc *getP3Disc()   { return p3d; } 
#endif

#ifdef PQI_USE_PROXY
	p3proxy *getP3Proxy() { return p3p; }
#endif

#ifdef PQI_USE_CHANNELS
	p3channel *getP3Channel() { return p3c; }
#endif

	protected:
	/* Overloaded RsItem Check
	 * checks item->cid vs Person
	 */
virtual int checkOutgoingRsItem(RsItem *item, int global) { return 1; }

	private:

	// The tunnelserver operation.
	int tickServiceRecv();
	int tickServiceSend();

		pqissllistener *pqil;

		sslroot *sslr;

#ifdef PQI_USE_DISC
		p3disc    *p3d;
#endif

#ifdef PQI_USE_PROXY
		p3proxy   *p3p;
		pqiudplistener *pqiudpl;
#endif
#ifdef PQI_USE_CHANNELS
		p3channel *p3c;
#endif
	unsigned long initFlags;
};

#endif // MRK_PQI_PERSON_HANDLER_HEADER
