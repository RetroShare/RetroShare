
/*
 * "$Id: p3face-config.cc,v 1.4 2007-05-05 16:10:05 rmf24 Exp $"
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


#include "rsserver/p3face.h"

#include <iostream>
#include <sstream>

#include "pqi/pqidebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>


/****************************************/
/* RsIface Config */
/* Config */

int RsServer::ConfigAddSharedDir( std::string dir )
{
	/* call the server... */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	server -> addSearchDirectory(dir);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}

int RsServer::ConfigRemoveSharedDir( std::string dir )
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	server -> removeSearchDirectory(dir);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;

}

int RsServer::ConfigSetIncomingDir( std::string dir )
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	server -> setSaveDir(dir);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;



}

#if 0

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

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}



int     RsServer::ConfigSetLanConfig( bool firewalled, bool forwarded )
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* a little rough and ready, should be moved to the server! */
	cert *c = sslr -> getOwnCert();
		
	c -> Firewalled(firewalled);
	c -> Forwarded(forwarded);
		
	sslr -> checkNetAddress();
	sslr -> CertsChanged();

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}


int     RsServer::ConfigSetExtName( std::string addr )
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* a little rough and ready, should be moved to the server! */
	//cert *c = sslr -> getOwnCert();

	/* set the Name here */
		
	sslr -> checkNetAddress();
	sslr -> CertsChanged();

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}

#endif /* 0 */

int     RsServer::ConfigSetDataRates( int total, int indiv ) /* in kbrates */
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

        pqih -> setMaxRate(true, total);
	pqih -> setMaxRate(false, total);
	pqih -> setMaxIndivRate(true, indiv);
	pqih -> setMaxIndivRate(false, indiv);

	pqih -> save_config();

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}


int     RsServer::ConfigSetBootPrompt( bool on )
{

	return 1;
}



int RsServer::UpdateAllConfig()
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

 	RsConfig &config = iface.mConfig;

	config.ownId = mAuthMgr->OwnId();
	config.ownName = mAuthMgr->getName(config.ownId);
	peerConnectState pstate;
	mConnMgr->getOwnNetStatus(pstate);

	/* shared dirs */
	std::list<std::string> &dirs = server -> getSearchDirectories();
 	config.sharedDirList = dirs;
 	config.incomingDir = server->getSaveDir();


	/* ports */
	config.localAddr = inet_ntoa(pstate.localaddr.sin_addr);
	config.localPort = ntohs(pstate.localaddr.sin_port);

	config.firewalled = true;
	config.forwardPort  = true;
	
	config.extAddr = inet_ntoa(pstate.serveraddr.sin_addr);
	config.extPort = ntohs(pstate.serveraddr.sin_port);

	/* data rates */
	config.maxDataRate = (int) pqih -> getMaxRate(true);     /* kb */
	config.maxIndivDataRate  = (int) pqih -> getMaxIndivRate(true);/* kb */
	config.promptAtBoot = true; /* popup the password prompt */      

	/* update DHT/UPnP config */

	config.uPnPState  = mConnMgr->getUPnPState();
	config.uPnPActive = mConnMgr->getUPnPEnabled();
	config.DHTPeers   = 20;
	config.DHTActive  = mConnMgr->getDHTEnabled();;

	/* Notify of Changes */
	iface.setChanged(RsIface::Config);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	return 1;


}




void    RsServer::ConfigFinalSave()
{
	/* force saving of transfers */
	server->saveFileTransferStatus();

	mAuthMgr->FinalSaveCertificates();
	mConfigMgr->completeConfiguration();
}

void RsServer::rsGlobalShutDown()
{
	ConfigFinalSave(); // save configuration before exit
}
