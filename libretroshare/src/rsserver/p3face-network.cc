
/*
 * "$Id: p3face-people.cc,v 1.8 2007-04-15 18:45:23 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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



#include "dht/dhthandler.h"
#include "upnp/upnphandler.h"

#include "rsserver/p3face.h"
#include "rsserver/pqistrings.h"

#include <iostream>
#include <sstream>

#include "util/rsdebug.h"

#include <sys/time.h>
#include <time.h>



const int p3facenetworkzone = 4219;

/*****
int RsServer::NetworkDHTActive(bool active);
int RsServer::NetworkUPnPActive(bool active);
int RsServer::NetworkDHTStatus();
int RsServer::NetworkUPnPStatus();
********/

/* internal */

/*****
int	RsServer::CheckNetworking();
int	RsServer::InitNetworking();

int RsServer::InitDHT();
int RsServer::CheckDHT();

int RsServer::InitUPnP();
int RsServer::CheckUPnP();
********/

int RsServer::NetworkDHTActive(bool active)
{
	lockRsCore(); /* LOCK */

	server->setDHTEnabled(active);

	unlockRsCore(); /* UNLOCK */

	return 1;
}

int RsServer::NetworkUPnPActive(bool active)
{
	lockRsCore(); /* LOCK */

	server->setUPnPEnabled(active);

	unlockRsCore(); /* UNLOCK */

	return 1;
}

int RsServer::NetworkDHTStatus()
{
	lockRsCore(); /* LOCK */

	server->getDHTEnabled();

	unlockRsCore(); /* UNLOCK */

	return 1;
}


int RsServer::NetworkUPnPStatus()
{
	lockRsCore(); /* LOCK */

	server->getUPnPEnabled();

	unlockRsCore(); /* UNLOCK */

	return 1;
}



int	RsServer::InitNetworking(std::string dhtfile)
{
	InitDHT(dhtfile);
	InitUPnP();
	return 1;
}


int	RsServer::CheckNetworking()
{
	CheckDHT();
	CheckUPnP();
	return 1;
}


dhthandler *dhtp = NULL;
upnphandler *upnpp = NULL;

pqiAddrStore *getDHTServer()
{
	return dhtp;
}

int	RsServer::InitDHT(std::string file)
{
	/* only startup if it is supposed to be started! */
	if (server -> getDHTEnabled()) 
	{
	  dhtp = new dhthandler(file);
	}
	else
	{
	  dhtp = new dhthandler("");
	}
	  
	/* 
	 *
	 */
	dhtp -> start();
	cert *c = sslr -> getOwnCert();

	SetExternalPorts();

	/* give it our port, and hash */
	dhtp -> setOwnHash(c->Signature());
	return 1;
}

int	RsServer::SetExternalPorts()
{
	cert *c = sslr -> getOwnCert();

	/* decide on the most sensible port */

	unsigned short port = ntohs(c -> serveraddr.sin_port);
	if (port < 100)
	{
		port = ntohs(c -> localaddr.sin_port);
	}

	/* set for both DHT and UPnP -> so they are the same! */
	if (upnpp)
		upnpp -> setExternalPort(port);
	if (dhtp)
		dhtp -> setOwnPort(port);

	return 1;
}




int	RsServer::CheckDHT()
{
	lockRsCore(); /* LOCK */

	int i;
	int ret = 1;

	if (server -> getDHTEnabled()) 
	{
		/* startup if necessary */

	}
	else
	{
		/* shutdown if necessary */
	}
		
	/* for each friend */

	/* add, and then check */

	std::list<cert *>::iterator it;
	std::list<cert *> &certs = sslr -> getCertList();
	
	std::string emptystr("");
	//int online = 0;

	for(it = certs.begin(), i = 0; it != certs.end(); it++, i++)
	{
		cert *c = (*it);

		/* skip own cert */
		if (c == sslr -> getOwnCert())
		{
			continue;
		}

		if (c -> hasDHT())
		{
			/* ignore */
		}
		else
		{
			std::string id = c -> Signature();
			dhtp -> addFriend(id);

			struct sockaddr_in addr;
			unsigned int flags;
			if (dhtp -> addrFriend(id, addr, flags))
			{
				c -> setDHT(addr, flags);

				/* connect attempt! */
				c -> nc_timestamp = 0;
				c -> WillConnect(true);
			}
		}
	}

	unlockRsCore(); /* UNLOCK */
	return ret;
}





int	RsServer::InitUPnP()
{
	upnpp = new upnphandler();
	/* 
	 *
	 */

	/* set our internal address to it */
	cert *c = sslr -> getOwnCert();
	upnpp -> setInternalAddress(c -> localaddr);

	SetExternalPorts();

	upnpp -> start();


	return 1;
}

int	RsServer::CheckUPnP()
{
	lockRsCore(); /* LOCK */

	int ret = 1;

	/* set our internal address to it */
	cert *c = sslr -> getOwnCert();
	upnpp -> setInternalAddress(c -> localaddr);

	/* get the state */
	upnpentry ent;
	int state = upnpp -> getUPnPStatus(ent);

	if (server -> getUPnPEnabled()) 
	{
		std::cerr << "UPnP ENABLED: ";
		switch(state)
		{
			case RS_UPNP_S_ACTIVE:
				std::cerr << "UPnP Forwarding already up";
			break;
			case RS_UPNP_S_UDP_FAILED:
				std::cerr << "UPnP TCP Forwarding Ok / UDP Failed";
			break;
			case RS_UPNP_S_TCP_FAILED:
				std::cerr << "UPnP Forwarding Failed";
			break;
			case RS_UPNP_S_READY:
				std::cerr << "Setting up UPnP Forwarding";
				upnpp -> setupUPnPForwarding();
			break;
			case RS_UPNP_S_UNAVAILABLE:
			case RS_UPNP_S_UNINITIALISED:
				std::cerr << "Error UPNP not working";
			break;
		}
	}
	else
	{
		std::cerr << "UPnP DISABLED: ";
                /* shutdown a forward */
		switch(state)
		{
			case RS_UPNP_S_ACTIVE:
			case RS_UPNP_S_UDP_FAILED:
			case RS_UPNP_S_TCP_FAILED:
				std::cerr << "Shutting down UPnP Forwarding";
                		upnpp->shutdownUPnPForwarding();
			break;
			case RS_UPNP_S_READY:
				std::cerr << "UPnP Forwarding already down";
			break;
			case RS_UPNP_S_UNAVAILABLE:
			case RS_UPNP_S_UNINITIALISED:
				std::cerr << "Error UPNP not working";
			break;
		}

	}
	std::cerr << std::endl;

	unlockRsCore(); /* UNLOCK */
	return ret;
}

/* called from update Config (inside locks) */
int	RsServer::UpdateNetworkConfig(RsConfig &config)
{
	upnpentry ent;
	int state = upnpp -> getUPnPStatus(ent);

	config.DHTActive  = server -> getDHTEnabled();
	config.DHTPeers   = dhtp   -> dhtPeers();
	config.uPnPActive = server -> getUPnPEnabled();
	config.uPnPState  = state;

	return 1;
}

/************************/

int     RsServer::ConfigSetLocalAddr( std::string ipAddr, int port )
{
	/* check if this is all necessary */
	struct in_addr inaddr_local;
	if (0 == inet_aton(ipAddr.c_str(), &inaddr_local))
	{
		//bad address - reset.
		return 0;
	}
		
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* a little rough and ready, should be moved to the server! */
	cert *c = sslr -> getOwnCert();
		
	/* always change the address (checked by sslr->checkNetAddress()) */
	c -> localaddr.sin_addr = inaddr_local;
	c -> localaddr.sin_port = htons((short) port);
		
	sslr -> checkNetAddress();
	pqih -> restart_listener();
	sslr -> CertsChanged();

	/* update local port address on uPnP */
	upnpp -> setInternalAddress(c -> localaddr);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}


int     RsServer::ConfigSetExtAddr( std::string ipAddr, int port )
{
	/* check if this is all necessary */
	struct in_addr inaddr;
	if (0 == inet_aton(ipAddr.c_str(), &inaddr))
	{
		//bad address - reset.
		return 0;
	}
		
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* a little rough and ready, should be moved to the server! */
	cert *c = sslr -> getOwnCert();
		
	/* always change the address (checked by sslr->checkNetAddress()) */
	c -> serveraddr.sin_addr = inaddr;
	c -> serveraddr.sin_port = htons((short) port);
		
	sslr -> checkNetAddress();
	sslr -> CertsChanged();

	/* update the DHT/UPnP port (in_addr is auto found ) */
	SetExternalPorts();

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}


