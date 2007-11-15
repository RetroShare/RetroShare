/*
 * "$Id: pqipersongrp.cc,v 1.14 2007-02-19 20:08:30 rmf24 Exp $"
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




#include "pqi/pqipersongrp.h"

/*
#include "pqi/pqiproxy.h"
#include "pqi/pqitunnelproxy.h"
 */


#include "pqi/p3disc.h"
#include "pqi/p3channel.h"

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"

/*
 * #include "pqi/pqiudpproxy.h"
 * #include "pqi/pqissludp.h"
 */

#ifdef PQI_USE_PROXY
  #include "pqi/pqiudpproxy.h"
  #include "pqi/pqissludp.h"
#endif

//#include "pqi/pqitunneltst.h"

#include "pqi/pqidebug.h"
#include <sstream>

const int pqipersongrpzone = 354;

// get the Tunnel and TunnelInit packets from the queue.
bool isTunnelItem(PQItem *item)
{
	if (item -> type == PQI_ITEM_TYPE_TUNNELITEM)
		return true;
	return false;
}

bool isTunnelInitItem(PQItem *item)
{
	if (item -> type == PQI_ITEM_TYPE_TUNNELINITITEM)
		return true;
	return false;
}

// handle the tunnel services.
int pqipersongrp::tickTunnelServer()
{
        PQItem *pqi = NULL;
	int i = 0;
	{
		std::ostringstream out;
		out << "pqipersongrp::tickTunnelServer()";
		pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, out.str());
	}

	PQTunnelServer::tick();

	while(NULL != (pqi =  SelectOtherPQItem(isTunnelInitItem)))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() Incoming TunnelInitItem");
		incoming(pqi);
	}
	while(NULL != (pqi = SelectOtherPQItem(isTunnelItem)))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() Incoming TunnelItem");
		incoming(pqi);
	}

	while(NULL != (pqi = outgoing()))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() OutGoing PQItem");

		SendOtherPQItem(pqi);
	}
	if (0 < i)
	{
		return 1;
	}
	return 0;
}


	// inits
pqipersongrp::pqipersongrp(SecurityPolicy *glob, sslroot *sr, unsigned long flags)
	:pqihandler(glob), sslr(sr), p3d(NULL), 
#ifdef PQI_USE_PROXY
	p3p(NULL),
#endif
	initFlags(flags)
{
	// add a p3proxy & p3disc.
	p3d = new p3disc(sr);
#ifdef PQI_USE_PROXY
	p3p = new p3udpproxy(p3d);
#endif
#ifdef PQI_USE_CHANNELS
	p3c = new p3channel(sr);
#endif

	if (!(sr -> active()))
	{
		pqioutput(PQL_ALERT, pqipersongrpzone, "sslroot not active... exiting!");
		exit(1);
	}

	// make listen
	Person *us = sr -> getOwnCert();
	if (us != NULL)
	{
		if (flags & PQIPERSON_NO_SSLLISTENER)
		{
			pqil = NULL;
		}
		else
		{
			pqil = new pqissllistener(us -> localaddr);
		}
#ifdef PQI_USE_PROXY
		pqiudpl = new pqiudplistener((p3udpproxy *) p3p, 
						us -> localaddr);
#endif
	}
	else
	{
		pqioutput(PQL_ALERT, pqipersongrpzone, "No Us! what are we!");
		exit(1);
	}


	// now we run through any certificates
	// already made... and reactivate them.
	std::list<cert *>::iterator it;
	std::list<cert *> &clist = sr -> getCertList();
	for(it = clist.begin(); it != clist.end(); it++)
	{
		cert *c = (*it);

		c -> InUse(false);
		c -> Listening(false);
		c -> Connected(false);

		// make new
		if (c -> Accepted())
		{
			cert_accept(c);
		}
	}


	// finally lets


	// add in a tunneltest.
	// register the packet creations.
	//addService(new PQTStst());
	//registerTunnelType(PQI_TUNNEL_TST_TYPE, createPQTStst);

	addService(p3d);
	registerTunnelType(PQI_TUNNEL_DISC_ITEM_TYPE, createDiscItems);

#ifdef PQI_USE_PROXY
	addService(p3p);
	registerTunnelType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxy);
	registerTunnelInitType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxyInit);
#endif

#ifdef PQI_USE_CHANNELS
	addService(p3c);
	registerTunnelType(PQI_TUNNEL_CHANNEL_ITEM_TYPE, createChannelItems);
#endif

	return;
}

int	pqipersongrp::tick()
{
	/* could limit the ticking of listener / tunnels to 1/sec...
	 * but not to important.
	 */

	if (pqil)
	{
		pqil -> tick();
	}
#ifdef PQI_USE_PROXY
	pqiudpl->tick();
#endif
	tickTunnelServer();

	return pqihandler::tick();
}


int	pqipersongrp::status()
{
	if (pqil)
	{
		pqil -> status();
	}
#ifdef PQI_USE_PROXY
	//pqiudpl->status();
#endif
	return pqihandler::status();
}

        // control the connections.

int     pqipersongrp::cert_accept(cert *a)
{
	if (a -> InUse())
	{
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::cert_accept() Cert in Use!");

		return -1;
	}

	pqiperson *pqip = new pqiperson(a);
	pqissl *pqis   = new pqissl(a, pqil, pqip);
	pqiconnect *pqisc = new pqiconnect(pqis);

	pqip -> addChildInterface(pqisc);

#ifdef PQI_USE_PROXY
	pqiudpproxy *pqipxy 	= new pqiudpproxy(a, (p3udpproxy *) p3p, NULL);
	pqissludp *pqius 	= new pqissludp(a, pqip, pqipxy);
	pqiconnect *pqiusc 	= new pqiconnect(pqius);

	// add a ssl + proxy interface.
	// Add Proxy First.
	pqip -> addChildInterface(pqiusc);
#endif

	a -> InUse(true);
	a -> Accepted(true);


	// setup no behaviour. (no remote address)

	// attach to pqihandler
	SearchModule *sm = new SearchModule();
	sm -> smi = 2;
	sm -> pqi = pqip;
	sm -> sp = secpolicy_create();

	// reset it to start it working.
	pqis -> reset();


	return AddSearchModule(sm);
}

int     pqipersongrp::cert_deny(cert *a)
{
	std::map<int, SearchModule *>::iterator it;
	SearchModule *mod;
	bool found = false;

	// if used find search module....
	if (a -> InUse())
	{
		// find module.
		for(it = mods.begin(); (!found) && (it != mods.end());it++)
		{
			mod = it -> second;
			if (a == (cert *) ((pqiperson *) 
					(mod -> pqi)) -> getContact())
			{
				found = true;
			}
		}
		if (found)
		{
			RemoveSearchModule(mod);
			secpolicy_delete(mod -> sp);
			pqiperson *p = (pqiperson *) mod -> pqi;
			p -> reset();
			delete p;
			a -> InUse(false);
		}
	}
	a -> Accepted(false);
	return 1;
}

int     pqipersongrp::cert_auto(cert *a, bool b)
{
	std::map<int, SearchModule *>::iterator it;
	if (b)
	{
		cert_accept(a);
		// find module.
		for(it = mods.begin(); it != mods.end();it++)
		{
			SearchModule *mod = it -> second;
			pqiperson *p = (pqiperson *) mod -> pqi;
			if (a == (cert *) p -> getContact())
			{
				p -> autoconnect(b);
				return 1;
			}
		}
	}
	else
	{
		a -> Manual(true);
		cert_deny(a);
	}
	return 1;
}


int     pqipersongrp::restart_listener()
{
	// stop it, 
	// change the address.
	// restart.
	cert *own = sslr -> getOwnCert();
	if (pqil)
	{
		pqil -> resetlisten();
		pqil -> setListenAddr(own -> localaddr);
		pqil -> setuplisten();
	}
#ifdef PQI_USE_PROXY
	pqiudpl -> resetlisten();
	pqiudpl -> setListenAddr(own -> localaddr);
	pqiudpl -> setuplisten();
#endif
	return 1;
}


static const std::string pqih_ftr("PQIH_FTR");

int     pqipersongrp::save_config()
{
	char line[512];
	sprintf(line, "%f %f %f %f", getMaxRate(true), getMaxRate(false),
	getMaxIndivRate(true), getMaxIndivRate(false));
	sslr -> setSetting(pqih_ftr, std::string(line));
	return 1;
}
	


int     pqipersongrp::load_config()
{
	std::string line = sslr -> getSetting(pqih_ftr);
	float mri, mro, miri, miro;
	
	if (4 == sscanf(line.c_str(), "%f %f %f %f", &mri, &mro, &miri, &miro))
	{
		setMaxRate(true, mri);
		setMaxRate(false, mro);
		setMaxIndivRate(true, miri);
		setMaxIndivRate(false, miro);
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone,
			"pqipersongrp::load_config() Loading Default Rates!");

		setMaxRate(true, 50.0);
		setMaxRate(false,50.0);
		setMaxIndivRate(true, 50.0);
		setMaxIndivRate(false, 50.0);
	}

	return 1;
}
	

        /* Overloaded PQItem Check */
int pqipersongrp::checkOutgoingPQItem(PQItem *item, int global)
{
	/* check cid vs Person */
	if ((global) && (item->cid.route[0] == 0))
	{
		/* allowed through as for all! */
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone,
			"pqipersongrp::checkOutgoingPQItem() Allowing global");
		return 1;
	}
	if ((!global) && (item->cid.route[0] == 0))
	{
		/* not allowed for all! */
		std::ostringstream out;
		out << "Bad CID on non-global traffic" << std::endl;
		item -> print(out);
		pqioutput(PQL_ALERT, pqipersongrpzone,out.str());
		return 0;
	}

	if (item -> p == NULL)
	{
		pqioutput(PQL_ALERT, pqipersongrpzone,
			"pqipersongrp::checkOutgoingPQItem() ERROR: NULL Person");

		std::ostringstream out;
		item -> print(out);
		pqioutput(PQL_ALERT, pqipersongrpzone,out.str());

		return 0;
	}

	cert *c = (cert *) item -> p;
	if (0 != pqicid_cmp(&(c -> cid), &(item -> cid)))
	{
		std::ostringstream out;
		out << "pqipersongrp::checkOutgoingPQItem() c->cid != item->cid";
		out << std::endl;
		out << "c -> CID    [" << c->cid.route[0];
		for(int i = 0; i < 10; i++)
		{
			out << ":" << c->cid.route[i];
		}
	        out << "]" << std::endl;

		out << "item -> CID [" << item->cid.route[0];
		for(int i = 0; i < 10; i++)
		{
			out << ":" << item->cid.route[i];
		}
	        out << "]" << std::endl;

		item -> print(out);

		pqioutput(PQL_ALERT, pqipersongrpzone,out.str());
		pqicid_copy(&(c->cid), &(item->cid));
	}

	/* check the top one */

	return 1;
}

		


		
	


