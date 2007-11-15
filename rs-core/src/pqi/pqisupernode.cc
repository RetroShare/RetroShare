/*
 * "$Id: pqisupernode.cc,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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




#include "pqi/pqisupernode.h"

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

const int pqisupernodezone = 65904;

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
int pqisupernode::tickTunnelServer()
{
        PQItem *pqi = NULL;
	{
		std::ostringstream out;
		out << "pqisupernode::tickTunnelServer()";
		pqioutput(PQL_DEBUG_ALL, pqisupernodezone, out.str());
	}

	PQTunnelServer::tick();

	while(NULL != (pqi =  SelectOtherPQItem(isTunnelInitItem)))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone, 
			"pqisupernode::tickTunnelServer() Incoming TunnelInitItem");
		incoming(pqi);
	}
	while(NULL != (pqi = SelectOtherPQItem(isTunnelItem)))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone, 
			"pqisupernode::tickTunnelServer() Incoming TunnelItem");
		incoming(pqi);
	}

	while(NULL != (pqi = outgoing()))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone, 
			"pqisupernode::tickTunnelServer() OutGoing PQItem");

		SendOtherPQItem(pqi);
	}
	return 1;
}


	// inits
pqisupernode::pqisupernode(SecurityPolicy *glob, sslroot *sr)
	:pqihandler(glob), sslr(sr), p3d(NULL)
#ifdef PQI_USE_PROXY
	,p3p(NULL)
#endif
{
	// add a p3proxy & p3disc.
	p3d = new p3disc(sr);
#ifdef PQI_USE_PROXY
	p3p = new p3udpproxy(p3d);
#endif

	if (!(sr -> active()))
	{
		pqioutput(PQL_ALERT, pqisupernodezone, "sslroot not active... exiting!");
		exit(1);
	}

	// make listen
	Person *us = sr -> getOwnCert();
	if (us != NULL)
	{
		pqisslnode = new pqisslsupernode(us -> localaddr, this);
#ifdef PQI_USE_PROXY
		pqiudpl = new pqiudplistener((p3udpproxy *) p3p, 
						us -> localaddr);
#endif
	}
	else
	{
		pqioutput(PQL_ALERT, pqisupernodezone, "No Us! what are we!");
		exit(1);
	}

	addService(p3d);
	registerTunnelType(PQI_TUNNEL_DISC_ITEM_TYPE, createDiscItems);

#ifdef PQI_USE_PROXY
	addService(p3p);
	registerTunnelType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxy);
	registerTunnelInitType(PQI_TUNNEL_PROXY_TYPE, createPQTunnelProxyInit);
#endif
	return;
}


int	pqisupernode::run()
{
	while(1)
	{
		sleep(1);
		tick();
	}
	return 1;
}

int	pqisupernode::tick()
{
	pqisslnode -> tick();
#ifdef PQI_USE_PROXY
	pqiudpl->tick();
#endif
	tickTunnelServer();

	return pqihandler::tick();
}


int	pqisupernode::status()
{
	pqisslnode -> status();
#ifdef PQI_USE_PROXY
	pqiudpl->status();
#endif
	return pqihandler::status();
}

        // control the connections.

int     pqisupernode::cert_accept(cert *a)
{
	/* this function does nothing */
	pqioutput(PQL_ALERT, pqisupernodezone, 
		"pqisupernode::cert_accept() Null Fn.");

	return -1;
}

int     pqisupernode::cert_deny(cert *a)
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

int     pqisupernode::cert_auto(cert *a, bool b)
{
	pqioutput(PQL_ALERT, pqisupernodezone, 
		"pqisupernode::cert_auto() Null Fn.");
	return 1;
}


int     pqisupernode::restart_listener()
{
        pqisslnode -> resetlisten();
	cert *own = sslr -> getOwnCert();
	pqisslnode -> setListenAddr(own -> localaddr);
	pqisslnode -> setuplisten();

#ifdef PQI_USE_PROXY
        pqiudpl -> resetlisten();
        pqiudpl -> setListenAddr(own -> localaddr);
        pqiudpl -> setuplisten();
#endif
	return 1;
}


static const std::string pqih_ftr("PQIH_FTR");

int     pqisupernode::save_config()
{
	char line[512];
	sprintf(line, "%f %f %f %f", getMaxRate(true), getMaxRate(false),
	getMaxIndivRate(true), getMaxIndivRate(false));
	sslr -> setSetting(pqih_ftr, std::string(line));
	return 1;
}
	


int     pqisupernode::load_config()
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
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone,
			"pqisupernode::load_config() Loading Default Rates!");

		setMaxRate(true, 20.0);
		setMaxRate(false, 20.0);
		setMaxIndivRate(true, 5.0);
		setMaxIndivRate(false, 5.0);
	}

	return 1;
}
	

        /* Overloaded PQItem Check */
int pqisupernode::checkOutgoingPQItem(PQItem *item, int global)
{
	/* check cid vs Person */
	if ((global) && (item->cid.route[0] == 0))
	{
		/* allowed through as for all! */
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone,
			"pqisupernode::checkOutgoingPQItem() Allowing global");
		return 1;
	}

	if (item -> p == NULL)
	{
		pqioutput(PQL_ALERT, pqisupernodezone,
			"pqisupernode::checkOutgoingPQItem() ERROR: NULL Person");

		std::ostringstream out;
		item -> print(out);
		pqioutput(PQL_ALERT, pqisupernodezone,out.str());

		return 0;
	}

	cert *c = (cert *) item -> p;
	if (0 != pqicid_cmp(&(c -> cid), &(item -> cid)))
	{
		std::ostringstream out;
		out << "pqisupernode::checkOutgoingPQItem() c->cid != item->cid";
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

		pqioutput(PQL_ALERT, pqisupernodezone,out.str());
		pqicid_copy(&(c->cid), &(item->cid));
	}

	/* check the top one */

	return 1;
}

int pqisupernode::recvdConnection(int fd, SSL *in_connection, 
				cert *peer, struct sockaddr_in *raddr)
{
	/* so we need to accept the certificate */
	if (peer -> InUse())
	{
		pqioutput(PQL_DEBUG_BASIC, pqisupernodezone, 
			"pqisupernode::recvdConnection() Cert in Use!");

		return -1;
	}

	pqiperson *pqip = new pqiperson(peer);
	pqissl *pqis   = new pqissl(peer, pqisslnode, pqip);
	pqiconnect *pqisc = new pqiconnect(pqis);
	pqip -> addChildInterface(pqisc);

	/* first set the certificate options off! */
	/* listening off, connecting off */
	// setup no behaviour. (no remote address)
	//

	// add the certificate to sslroot...
	sslr -> addUntrustedCertificate(peer);

	peer -> InUse(true);
	peer -> Accepted(true);
	peer -> WillListen(false);
	peer -> WillConnect(false);
	peer -> Manual(true);

	// attach to pqihandler
	SearchModule *sm = new SearchModule();
	sm -> smi = 2;
	sm -> pqi = pqip;
	sm -> sp = secpolicy_create();

	// reset it to start it working.
	pqis -> reset();
	AddSearchModule(sm); // call to pqihandler....

	pqis -> accept(in_connection, fd, *raddr);

	/* finally tell the system to listen - if the connection fails */
	peer -> WillListen(true);

	/* done! */
	return 1;
}

		
/************************ PQI SSL SUPER NODE ****************************
 *
 * This is the special listener, that accepts all connections.
 *
 */

pqisslsupernode::pqisslsupernode(struct sockaddr_in addr, pqisupernode *grp)
	:pqissllistener(addr), psn(grp)
{
	return;
}

pqisslsupernode::~pqisslsupernode()
{
	return;
}


int pqisslsupernode::completeConnection(int fd, SSL *ssl, struct sockaddr_in &remote_addr)
{ 
	/* first attempt a pqissllistener connect (ie if we are listening for it!)
	 *
	 * if this fails, try adding it!
	 */

	if (0 < pqissllistener::completeConnection(fd,ssl, remote_addr))
	{
		return 1; /* done! */
	}

	/* else ask the pqisupernode to create it! */

	// Get the Peer Certificate....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	XPGP *peercert = SSL_get_peer_pgp_certificate(ssl);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	X509 *peercert = SSL_get_peer_certificate(ssl);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (peercert == NULL)
	{
  	        pqioutput(PQL_WARNING, pqisupernodezone, 
		 "pqisslsupernode::completeConnection() Peer Did Not Provide Cert!");
		return -1;
	}


	// save certificate... (and ip locations)
	/* the first registerCertificate can fail, 
	 * but this is in pqissllistener ....
	 * so this one should succeed.
	 */
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	cert *npc = sslccr -> registerCertificateXPGP(peercert, remote_addr, true);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	cert *npc = sslccr -> registerCertificate(peercert, remote_addr, true);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if ((npc == NULL) || (npc -> Connected()))
	{
		std::ostringstream out;
		out << "No Matching Certificate/Already Connected";
		out << " for Connection:" << inet_ntoa(remote_addr.sin_addr);
		out << std::endl;
		out << "Shutting it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqisupernodezone, out.str());
		return -1;
	}

	// hand off ssl conection.
	psn -> recvdConnection(fd, ssl, npc, &remote_addr);
	return 1;
}

