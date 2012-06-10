
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
#include "retroshare/rsinit.h"
#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"


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


int     RsServer::ConfigSetBootPrompt( bool /*on*/ )
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
	peerState pstate;
        mPeerMgr->getOwnNetStatus(pstate);

	/* ports */
	config.localAddr = rs_inet_ntoa(pstate.localaddr.sin_addr);
	config.localPort = ntohs(pstate.localaddr.sin_port);

	config.firewalled = true;
	config.forwardPort  = true;
	
	config.extAddr = rs_inet_ntoa(pstate.serveraddr.sin_addr);
	config.extPort = ntohs(pstate.serveraddr.sin_port);

	/* data rates */
	config.maxDownloadDataRate = (int) pqih -> getMaxRate(true);     /* kb */
	config.maxUploadDataRate = (int) pqih -> getMaxRate(false);     /* kb */

	config.promptAtBoot = true; /* popup the password prompt */      

	/* update network configuration */

	pqiNetStatus status;	
	mNetMgr->getNetStatus(status);

	config.netLocalOk = status.mLocalAddrOk;  
	config.netUpnpOk  = status.mUpnpOk;
	config.netStunOk  = false;
	config.netExtraAddressOk = status.mExtAddrOk;

	config.netDhtOk   = status.mDhtOk;
	config.netDhtNetSize = status.mDhtNetworkSize;
	config.netDhtRsNetSize = status.mDhtRsNetworkSize;

	/* update DHT/UPnP config */

	config.uPnPState  = mNetMgr->getUPnPState();
	config.uPnPActive = mNetMgr->getUPnPEnabled();
	config.DHTPeers   = 20;
	config.DHTActive  = mNetMgr->getDHTEnabled();

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

	ConfigFinalSave(); // save configuration before exit
	mNetMgr->shutdown(); /* Handles UPnP */

	join();
	ftserver->StopThreads();

	// stop the p3distrib threads
#ifndef MINIMAL_LIBRS
	mForums->join();
	mChannels->join();

#ifdef RS_USE_BLOGS
	mBlogs->join();
#endif
#endif // MINIMAL_LIBRS

	AuthGPG::exit();
}
