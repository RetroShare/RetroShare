/*******************************************************************************
 * libretroshare/src/rsserver: p3face-config.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie.                                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "rsserver/p3face.h"

#include <iostream>
#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "retroshare/rsinit.h"
#include "plugins/pluginmanager.h"
#include "util/rsdebug.h"

#ifdef RS_JSONAPI
#	include "jsonapi/jsonapi.h"
#endif // ifdef RS_JSONAPI

#include <sys/time.h>
#include "util/rstime.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"


// TO SHUTDOWN THREADS.
#ifdef RS_ENABLE_GXS

#include "services/autoproxy/rsautoproxymonitor.h"

#include "services/p3idservice.h"
#include "services/p3gxscircles.h"
#include "services/p3wiki.h"
#include "services/p3posted.h"
#include "services/p3photoservice.h"
#include "services/p3gxsforums.h"
#include "services/p3gxschannels.h"
#include "services/p3wire.h"

#endif

/****************************************/
/* RsIface Config */
/* Config */

void RsServer::ConfigFinalSave()
{
	//TODO: force saving of transfers
	//ftserver->saveFileTransferStatus();

#ifdef RS_AUTOLOGIN
	if(!RsInit::getAutoLogin()) RsInit::RsClearAutoLogin();
#endif // RS_AUTOLOGIN

	//AuthSSL::getAuthSSL()->FinalSaveCertificates();
	mConfigMgr->completeConfiguration();
}

void RsServer::startServiceThread(RsTickingThread *t, const std::string &threadName)
{
    t->start(threadName) ;
    mRegisteredServiceThreads.push_back(t) ;
}

void RsServer::rsGlobalShutDown()
{
	coreReady = false;
	// TODO: cache should also clean up old files

	ConfigFinalSave(); // save configuration before exit

	mPluginsManager->stopPlugins(pqih);

	mNetMgr->shutdown(); /* Handles UPnP */

#ifdef RS_JSONAPI
	rsJsonApi->fullstop();
#endif

	rsAutoProxyMonitor::instance()->stopAllRSShutdown();

    fullstop() ;

    // kill all registered service threads

    for(std::list<RsTickingThread*>::iterator it= mRegisteredServiceThreads.begin();it!=mRegisteredServiceThreads.end();++it)
	{
        (*it)->fullstop() ;
	}
// #ifdef RS_ENABLE_GXS
// 		// We should automate this.
// 		//
//         if(mGxsCircles) mGxsCircles->join();
//         if(mGxsForums) mGxsForums->join();
//         if(mGxsChannels) mGxsChannels->join();
//         if(mGxsIdService) mGxsIdService->join();
//         if(mPosted) mPosted->join();
//         if(mWiki) mWiki->join();
//         if(mGxsNetService) mGxsNetService->join();
//         if(mPhoto) mPhoto->join();
//         if(mWire) mWire->join();
// #endif

	AuthPGP::exit();

	mShutdownCallback(0);
}
