/*
 * "$Id: pqiperson.h,v 1.7 2007-02-18 21:46:49 rmf24 Exp $"
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

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include <list>

class pqiperson;
class sslroot;

static const int CONNECT_RECEIVED     = 1; 
static const int CONNECT_SUCCESS      = 2;
static const int CONNECT_UNREACHABLE  = 3;
static const int CONNECT_FIREWALLED   = 4;
static const int CONNECT_FAILED       = 5;

#include "pqi/pqistreamer.h"

class pqiconnect: public pqistreamer, public NetInterface
{
public:
	pqiconnect(NetBinInterface *ni_in)
	:pqistreamer(ni_in, 0),  // pqistreamer will cleanup NetInterface.
	 NetInterface(NULL, NULL),     // No need for callback.
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

	// presents a virtual NetInterface -> passes to ni.
virtual int 	connectattempt() 	{ return ni -> connectattempt(); }
virtual int	listen() 		{ return ni -> listen(); }
virtual int	stoplistening() 	{ return ni -> stoplistening(); }
virtual int 	reset() 		{ return ni -> reset(); }
virtual int 	disconnect() 		{ return ni -> reset(); }

	// get the contact from the net side!
virtual Person *getContact() 
{
	if (ni)
	{
		return ni->getContact();
	}
	else
	{
		return PQInterface::getContact();
	}
}

	// to check if our interface.
virtual bool	thisNetInterface(NetInterface *ni_in) { return (ni_in == ni); }
//protected:
	NetBinInterface *ni;
protected:
};



class pqiperson: public PQInterface
{
public:
	pqiperson(cert *c);
virtual ~pqiperson(); // must clean up children.

	// control of the connection.
int 	reset();
int	autoconnect(bool);

	// add in connection method.
int	addChildInterface(pqiconnect *pqi);

	// The PQInterface interface.
virtual int     SendItem(PQItem *);
virtual PQItem *GetItem();
	
virtual int 	status();
virtual int	tick();
virtual cert *	getContact();

// overloaded callback function for the child - notify of a change.
int 	notifyEvent(NetInterface *ni, int event);

// PQInterface for rate control overloaded....
virtual float   getRate(bool in);
virtual void    setMaxRate(bool in, float val);

	private:

	// outgoing PQItems can be queued (nothing else).
	std::list<PQItem *> togo;
	std::list<pqiconnect *> kids;
	cert *sslcert;
	bool active;
	pqiconnect *activepqi;
	bool inConnectAttempt;
	int waittimes;

	sslroot *sroot;

private: /* Helper functions */

int	connectWait();
int	connectSuccess();
int 	listen();
int 	stoplistening();
int	connectattempt(pqiconnect *last);

};



#endif

