
/*
 * "$Id: retroshare.cc,v 1.4 2007-04-21 19:08:51 rmf24 Exp $"
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

#include <retroshare/rsiface.h>   /* definition of iface */
#include <retroshare/rsinit.h>   /* definition of iface */

#include "notifytxt.h"

#include <unistd.h>
#include <util/argstream.h>
#include <iostream>
#ifdef WINDOWS_SYS
#include <winsock2.h>
#endif
                                
#ifdef RS_INTRO_SERVER
#include "introserver.h"
#endif

#ifdef ENABLE_WEBUI
#include <stdarg.h>
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

    resource_api::TerminalApiClient tac(&api);
    while(ctrl_mod.processShouldExit() == false)
    {
        usleep(20*1000);
    }

    if(httpd)
    {
        httpd->stop();
        delete httpd;
    }

    return 0;
#endif

	/* Retroshare startup is configured using an RsInit object.
	 * This is an opaque class, which the user cannot directly tweak
	 * If you want to peek at whats happening underneath look in
	 * libretroshare/src/rsserver/p3face-startup.cc
	 *
	 * You create it with InitRsConfig(), and delete with CleanupRsConfig()
	 * InitRetroshare(argv, argc, config) parses the command line options, 
	 * and initialises the config paths.
	 *
	 * *** There are several functions that I should add to modify 
	 * **** the config the moment these can only be set via the commandline 
	 *   - RsConfigDirectory(...) is probably the most useful.
	 *   - RsConfigNetAddr(...) for setting port, etc.
	 *   - RsConfigOutput(...) for logging and debugging.
	 *
	 * Next you need to worry about loading your certificate, or making
	 * a new one:
	 *
	 *   RsGenerateCertificate(...) To create a new key, certificate 
	 *   LoadPassword(...) set password for existing certificate.
	 **/

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
	//
	RsControl::earlyInitNotificationSystem() ;

	NotifyTxt *notify = new NotifyTxt() ;
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
	RsControl::instance() -> StartupRetroShare();

#ifdef RS_INTRO_SERVER
	RsIntroServer rsIS;
#endif
	
	/* pass control to the GUI */
	while(1)
	{
		//std::cerr << "GUI Tick()" << std::endl;

#ifdef RS_INTRO_SERVER
		rsIS.tick();
#endif

		int rt = 0;
		// If we have a MenuTerminal ...
		// only want to sleep if there is no input. (rt == 0).
		if (rt == 0)
		{
#ifndef WINDOWS_SYS
			sleep(1);
#else
			Sleep(1000);
#endif
		}

		usleep(1000);

	}
	return 1;
}	
