/*
 * "$Id: pqipersongrp.cc,v 1.14 2007-02-19 20:08:30 rmf24 Exp $"
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




#include "pqi/pqipersongrp.h"

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"

#ifdef PQI_USE_PROXY
  #include "pqi/pqiudpproxy.h"
  #include "pqi/pqissludp.h"
#endif

//#include "pqi/pqitunneltst.h"

#include "pqi/pqidebug.h"
#include <sstream>

const int pqipersongrpzone = 354;


// handle the tunnel services.
int pqipersongrp::tickServiceRecv()
{
        RsRawItem *pqi = NULL;
	int i = 0;
	{
		std::ostringstream out;
		out << "pqipersongrp::tickTunnelServer()";
		pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, out.str());
	}

	//p3ServiceServer::tick();

	while(NULL != (pqi = GetRsRawItem()))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() Incoming TunnelItem");
		incoming(pqi);
	}

	if (0 < i)
	{
		return 1;
	}
	return 0;
}

// handle the tunnel services.
int pqipersongrp::tickServiceSend()
{
        RsRawItem *pqi = NULL;
	int i = 0;
	{
		std::ostringstream out;
		out << "pqipersongrp::tickServiceSend()";
		pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, out.str());
	}

	p3ServiceServer::tick();

	while(NULL != (pqi = outgoing()))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() OutGoing RsItem");

		SendRsRawItem(pqi);
	}
	if (0 < i)
	{
		return 1;
	}
	return 0;
}


	// inits
pqipersongrp::pqipersongrp(SecurityPolicy *glob, sslroot *sr, unsigned long flags)
	:pqihandler(glob), sslr(sr), 
#ifdef PQI_USE_PROXY
	p3p(NULL),
#endif
	initFlags(flags)
{
	// add a p3proxy & p3disc.
#ifdef PQI_USE_PROXY
	p3p = new p3udpproxy(p3d);
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


#ifdef PQI_USE_PROXY
	addService(p3p);
	registerTunnelType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxy);
	registerTunnelInitType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxyInit);
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
	int i = 0;

	if (tickServiceSend()) i = 1;

	if (pqihandler::tick()) i = 1; /* does actual Send/Recv */

	if (tickServiceRecv()) i = 1;

	return 1;
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

	{
		std::ostringstream out;
		out << "pqipersongrp::cert_accept() PeerId: " << a->PeerId();
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, out.str());
	}

	pqiperson *pqip = new pqiperson(a, a->PeerId());
	pqissl *pqis   = new pqissl(a, pqil, pqip);

	/* construct the serialiser ....
	 * Needs:
	 * * FileItem
	 * * FileData
	 * * ServiceGeneric
	 */

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());
	rss->addSerialType(new RsCacheItemSerialiser());
	rss->addSerialType(new RsServiceSerialiser());

	pqiconnect *pqisc = new pqiconnect(rss, pqis);

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
	sm -> peerid = a->PeerId();
	sm -> pqi = pqip;
	sm -> sp = secpolicy_create();

	// reset it to start it working.
	pqis -> reset();


	return AddSearchModule(sm);
}

int     pqipersongrp::cert_deny(cert *a)
{
	std::map<std::string, SearchModule *>::iterator it;
	SearchModule *mod;
	bool found = false;

	// if used find search module....
	if (a -> InUse())
	{
		// find module.
		for(it = mods.begin(); (!found) && (it != mods.end());it++)
		{
			mod = it -> second;
			pqiperson *p = (pqiperson *) mod -> pqi;
			if (a->PeerId() == p->PeerId())
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
	std::map<std::string, SearchModule *>::iterator it;
	if (b)
	{
		cert_accept(a);
		// find module.
		for(it = mods.begin(); it != mods.end();it++)
		{
			SearchModule *mod = it -> second;
			pqiperson *p = (pqiperson *) mod -> pqi;
			if (a->PeerId() == p->PeerId())
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
	

