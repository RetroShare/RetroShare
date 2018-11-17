/*******************************************************************************
 * retroshare-nogui/src/retroshare.cc                                          *
 *                                                                             *
 * retroshare-nogui: headless version of retroshare                            *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare.project@gmail.com>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h"
#include "notifytxt.h"
#include "util/argstream.h"
#include "util/rstime.h"

#include <unistd.h>
#include <iostream>

#ifdef WINDOWS_SYS
#include <winsock2.h>
#endif
                                
#ifdef RS_INTRO_SERVER
#include "introserver.h"
#endif

#ifdef ENABLE_WEBUI
#include <stdarg.h>
#include <csignal>
#include "api/ApiServerMHD.h"
#include "api/RsControlModule.h"
#include "TerminalApiClient.h"
#endif

/* Basic instructions for running libretroshare as background thread.
 * ******************************************************************* *
 * This allows your program to communicate with authenticated peers. 
 *
 * libretroshare's interfaces are defined in libretroshare/src/rsiface.
 * This should be the only headers that you need to include.
 *
 * The startup routine's are defined in rsiface.h
 */

int main(int argc, char **argv)
{
#ifdef ENABLE_WEBUI

    std::string docroot = resource_api::getDefaultDocroot();
    uint16_t httpPort = 0;
	std::string listenAddress;
    bool allowAllIps = false;

    argstream args(argc, argv);
    args >> parameter("webinterface", httpPort, "port", "Enable webinterface on the specified port", false);
    args >> parameter("docroot",      docroot,  "path", "Serve static files from this path.", false);
    // unfinished
    //args >> parameter("http-listen", listenAddress, "ipv6 address", "Listen only on the specified address.", false);
    args >> option("http-allow-all", allowAllIps, "allow connections from all IP adresses (default= localhost only)"); 
    args >> help('h',"help","Display this Help");

    if (args.helpRequested())
    {
        std::cerr << args.usage() << std::endl;
        // print libretroshare command line args and exit
        RsInit::InitRsConfig();
        RsInit::InitRetroShare(argc, argv, true);
        return 0;
    }

    resource_api::ApiServer api;
    resource_api::RsControlModule ctrl_mod(argc, argv, api.getStateTokenServer(), &api, true);
    api.addResourceHandler("control", dynamic_cast<resource_api::ResourceRouter*>(&ctrl_mod), &resource_api::RsControlModule::handleRequest);

    resource_api::ApiServerMHD* httpd = 0;
    if(httpPort != 0)
    {
        httpd = new resource_api::ApiServerMHD(&api);
        if(!httpd->configure(docroot, httpPort, listenAddress, allowAllIps))
        {
            std::cerr << "Failed to configure the http server. Check your parameters." << std::endl;
            return 1;
        }
        httpd->start();
    }

	RsControl::earlyInitNotificationSystem();
	rsControl->setShutdownCallback([](int){std::raise(SIGTERM);});

	resource_api::TerminalApiClient tac(&api);
	tac.start();
	bool already = false ;

	while(!ctrl_mod.processShouldExit())
    {
        rstime::rs_usleep(1000*1000);

		if(!tac.isRunning() && !already)
		{
			std::cerr << "Terminal API client terminated." << std::endl;
			already = true ;
		}
    }

    if(httpd)
    {
        httpd->stop();
        delete httpd;
    }

    return 0;
#endif

	bool strictCheck = true;
	RsInit::InitRsConfig();
	int initResult = RsInit::InitRetroShare(argc, argv, strictCheck);

	if (initResult < 0) {
		/* Error occured */
		switch (initResult) {
		case RS_INIT_AUTH_FAILED:
			std::cerr << "RsInit::InitRetroShare AuthGPG::InitAuth failed" << std::endl;
			break;
		default:
			/* Unexpected return code */
			std::cerr << "RsInit::InitRetroShare unexpected return code " << initResult << std::endl;
			break;
		}
		return 1;
	}

	 /* load password should be called at this point: LoadPassword()
	  * otherwise loaded from commandline.
	  */


	/* Now setup the libretroshare interface objs 
	 * You will need to create you own NotifyXXX class
	 * if you want to receive notifications of events */

	// This is needed to allocate rsNotify, so that it can be used to ask for PGP passphrase
	RsControl::earlyInitNotificationSystem();

	// an atomic might be safer but is probably unneded for this simple usage
	bool keepRunning = true;
	rsControl->setShutdownCallback([&](int){keepRunning = false;});

	NotifyTxt *notify = new NotifyTxt();
	rsNotify->registerNotifyClient(notify);

	/* PreferredId => Key + Certificate are loaded into libretroshare */

	std::string error_string ;
	int retVal = RsInit::LockAndLoadCertificates(false,error_string);
	switch(retVal)
	{
		case 0:	break;
		case 1:	std::cerr << "Error: another instance of retroshare is already using this profile" << std::endl;
				return 1;
		case 2: std::cerr << "An unexpected error occurred while locking the profile" << std::endl;
				return 1;
		case 3: std::cerr << "An error occurred while login with the profile" << std::endl;
				return 1;
		default: std::cerr << "Main: Unexpected switch value " << retVal << std::endl;
				return 1;
	}

	/* Start-up libretroshare server threads */
	RsControl::instance()->StartupRetroShare();

#ifdef RS_INTRO_SERVER
	RsIntroServer rsIS;
#endif

	while(keepRunning)
	{
#ifdef RS_INTRO_SERVER
		rsIS.tick();
#endif
		rstime::rs_usleep(10*1000);
	}

	return 0;
}
