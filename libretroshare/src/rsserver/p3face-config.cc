
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
#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "retroshare/rsinit.h"
#include "plugins/pluginmanager.h"
#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"


// TO SHUTDOWN THREADS.
#ifdef RS_ENABLE_GXS

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

int     RsServer::ConfigSetBootPrompt( bool /*on*/ )
{

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

	mPluginsManager->stopPlugins();


#ifdef RS_ENABLE_GXS
        if(mGxsCircles) mGxsCircles->join();
        if(mGxsForums) mGxsForums->join();
        if(mGxsChannels) mGxsChannels->join();
        if(mGxsIdService) mGxsIdService->join();
        if(mPosted) mPosted->join();
        //if(mPhoto) mPhoto->join();
        if(mWiki) mWiki->join();
        if(mWire) mWire->join();
#endif


	AuthGPG::exit();
}
