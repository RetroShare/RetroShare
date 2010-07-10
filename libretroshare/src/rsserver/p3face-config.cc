
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
#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "rsiface/rsinit.h"
#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>


/****************************************/
/* RsIface Config */
/* Config */

int     RsServer::ConfigSetDataRates( int totalDownload, int totalUpload ) /* in kbrates */
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	pqih -> setMaxRate(true, totalDownload);
	pqih -> setMaxRate(false, totalUpload);

	pqih -> save_config();

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	/* does its own locking */
	UpdateAllConfig();
	return 1;
}


int     RsServer::ConfigGetDataRates( float &inKb, float &outKb ) /* in kbrates */
{
	/* fill the rsiface class */
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

        pqih -> getCurrentRates(inKb, outKb);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

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

        config.ownId = AuthSSL::getAuthSSL()->OwnId();
        config.ownName = AuthGPG::getAuthGPG()->getGPGOwnName();
	peerConnectState pstate;
        mConnMgr->getOwnNetStatus(pstate);

	/* ports */
	config.localAddr = rs_inet_ntoa(pstate.currentlocaladdr.sin_addr);
	config.localPort = ntohs(pstate.currentlocaladdr.sin_port);

	config.firewalled = true;
	config.forwardPort  = true;
	
	config.extAddr = rs_inet_ntoa(pstate.currentserveraddr.sin_addr);
	config.extPort = ntohs(pstate.currentserveraddr.sin_port);

	/* data rates */
	config.maxDownloadDataRate = (int) pqih -> getMaxRate(true);     /* kb */
	config.maxUploadDataRate = (int) pqih -> getMaxRate(false);     /* kb */

	config.promptAtBoot = true; /* popup the password prompt */      

	/* update network configuration */

	pqiNetStatus status;	
	mConnMgr->getNetStatus(status);
	config.netLocalOk = status.mLocalAddrOk;  
	config.netUpnpOk  = status.mUpnpOk;
	config.netDhtOk   = status.mDhtOk;
	config.netStunOk  = false;
	config.netExtraAddressOk = status.mExtAddrOk;

	/* update DHT/UPnP config */

	config.uPnPState  = mConnMgr->getUPnPState();
	config.uPnPActive = mConnMgr->getUPnPEnabled();
	config.DHTPeers   = 20;
	config.DHTActive  = mConnMgr->getDHTEnabled();

	/* Notify of Changes */
//	iface.setChanged(RsIface::Config);
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CONFIG, NOTIFY_TYPE_MOD);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	return 1;


}

void    RsServer::ConfigFinalSave()
{
	/* force saving of transfers TODO */
	//ftserver->saveFileTransferStatus();
	if(!RsInit::getAutoLogin())
		RsInit::RsClearAutoLogin();

        //AuthSSL::getAuthSSL()->FinalSaveCertificates();
	mConfigMgr->completeConfiguration();
}

void RsServer::rsGlobalShutDown()
{
	// TODO: cache should also clean up old files
	mChannels->cleanUpOldFiles();
	ConfigFinalSave(); // save configuration before exit
	mConnMgr->shutdown(); /* Handles UPnP */

	join();
	ftserver->StopThreads();
}
