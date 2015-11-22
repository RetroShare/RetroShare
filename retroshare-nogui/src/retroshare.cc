
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

#ifdef RS_SSH_SERVER
#include "ssh/rssshd.h"

#include "menu/menus.h"
#include "menu/stdiocomms.h"

#include "rpc/rpcsetup.h"

// NASTY GLOBAL VARIABLE HACK - NEED TO THINK OF A BETTER SYSTEM.
#include "rpc/proto/rpcprotosystem.h"

void generatePasswordHash() ;
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
#ifdef __APPLE__    
    args >> help('h',"help","Display this Help");
#else
    args >> help();
#endif
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

#ifdef RS_SSH_SERVER
	/* parse commandline for additional nogui options */

	int c;
	// libretroshare's getopt str - so don't use any of these: "hesamui:p:c:w:l:d:U:r:R:"
	// need options for 
	// enable SSH.   (-S)
	// set user/password for SSH. -L "user:pwdhash"
	// accept RSA Key Auth. -K "RsaPubKeyFile"
	// Terminal mode. -T 
	bool enableRpc = false;
	bool enableSsh = false;
	bool enableSshHtml = false;
	bool enableTerminal = false;
	bool enableSshRsa = false;
	bool genPwdHash = false;
	std::string sshUser = "user";
	std::string sshPwdHash = "";
	std::string sshRsaFile = "";

	uint16_t extPort = 0;
	uint16_t sshPort = 7022;
	bool     extPortSet = false;
	bool displayRPCInfo = false ;

	argstream as(argc,argv) ;

	as >> option('X',"enable-ssh"     ,enableSsh        ,"Enable SSH"        )
	   >> option('T',"enable-terminal",enableTerminal   ,"Enable terminal interface."  )
		>> option('C',"enable-rpc"     ,enableRpc        ,"Enable RPC protocol. To be used with e.g. -X (SSH).")
	   >> option('G',"gen-password"   ,genPwdHash       ,"Generate password hash (to supply to option -P)")
#if 0
	   >> option('H',"enable-ssh-html",enableSshHtml    ,"Enable SSH html."  )
#endif
	   >> parameter('S',"ssh-port"       ,sshPort       ,"port"  ,"SSH port to contact the interface.",false)
	   >> parameter('E',"ext-port"       ,extPort       ,"port"  ,"Specify Alternative External Port (provided to Clients)",false)
	   >> parameter('L',"ssh-user"       ,sshUser       ,"name"  ,"Ssh login user",false)
	   >> parameter('P',"ssh-p-hash"     ,sshPwdHash    ,"hash"  ,"Ssh login password hash (Generated by retroshare-nogui -G)",false)
	   >> parameter('K',"ssh-key-file"   ,sshRsaFile    ,"RSA key file", "RSA key file for SSH login (not yet implemented).",false  )// NOT FINISHED YET.

#ifdef __APPLE__
		>> help('h',"help","Display this Help");
#else
		>> help() ;
#endif
	// Normally argstream would handle this by itself, if we called
	// 	as.defaultErrorHandling() ;
	//
	// but we have other parameter handling functions after, so we don't want to quit if help is requested.
	//
	if (as.helpRequested())
	{
		std::cerr << "\nSpecific Help Options:" << std::endl;
		std::cerr << as.usage() << std::endl;
		std::cerr << "\t To setup rs-nogui as a SSH Server is a three step process: " << std::endl;
		std::cerr << "\t 1) Generate a RSA keypair in the current directory: \"ssh-keygen -t rsa -f rs_ssh_host_rsa_key\" " << std::endl;
		std::cerr << "\t 2) Generate a password hash for the RPC login:      \"./retroshare-nogui -G\" " << std::endl;
		std::cerr << "\t 3) Launch the RS with remote control enabled:       \"./retroshare-nogui -X/-T [-C] -S [port] -L <user> -P <passwordhash>\" " << std::endl;

		std::cerr << "\nAdditional options: \n" << std::endl;
	}
	if (!as.isOk())
	{
		 std::cerr << as.errorLog();
		 return 1; 
	}

	if (genPwdHash)
	{
		generatePasswordHash() ;
		return 0 ;
	}

	/* enforce conditions */
	if ((!sshRsaFile.empty() || !sshPwdHash.empty()) && (!enableSsh))
	{
		std::cerr << "ERROR: SSH Server (-X) must be enabled to specify SSH Pwd (-P) or SSH RSA (-K)";
		std::cerr << std::endl;
		return 1 ;
	}

	if (enableSsh && (!enableSshRsa) && sshPwdHash.empty())
	{
		std::cerr << "ERROR: One of (or both) SSH Pwd (-P) and SSH RSA (-K) must be specified with SSH Server (-X)";
		std::cerr << std::endl;
		return 1 ;
	}

	if (enableRpc && (!enableSsh))
	{
		std::cerr << "ERROR: RPC Mode (-C) requires SSH Server (-X) enabled";
		std::cerr << std::endl;
		return 1 ;
	}


	/* parse -S, -L & -K parameters */
	if (enableSshRsa)
	{
		/* check the file exists */
		/* TODO */

	}

	if (enableSsh)
	{
		/* try parse it */
		/* TODO */

	}

#else
	std::cerr << "\nRetroshare command line interface." << std::endl;
#endif


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

#ifdef RS_SSH_SERVER
	// Says it must be called before all the threads are launched! */
        // NB: this port number is not currently used.
	RsSshd *ssh = NULL;

	if (enableSsh)
	{
		std::ostringstream os ;
		os << sshPort ;
		ssh = RsSshd::InitRsSshd(os.str(), "rs_ssh_host_rsa_key");

		// TODO Parse Option
		if (enableSshRsa)
		{
        		//ssh->adduser("anrsuser", "test");
		}

		if (!sshPwdHash.empty())
		{
        		ssh->adduserpwdhash(sshUser, sshPwdHash);
		}
			
		if (!extPortSet)
		{
			extPort = sshPort;
		}

		// NASTY GLOBAL VARIABLE HACK - NEED TO THINK OF A BETTER SYSTEM.
		RpcProtoSystem::mExtPort = extPort;
	}
#endif

	/* Start-up libretroshare server threads */
	RsControl::instance() -> StartupRetroShare();

#ifdef RS_INTRO_SERVER
	RsIntroServer rsIS;
#endif
	
#ifdef RS_SSH_SERVER
	uint32_t baseDrawFlags = 0;
	if (enableSshHtml)
	{
		baseDrawFlags = MENU_DRAW_FLAGS_HTML;
	}

	if (enableSsh)
	{
		if (enableRpc)
		{
			/* Build RPC Server */
			RpcMediator *med = CreateRpcSystem(ssh, notify);
			ssh->setRpcSystem(med);
			ssh->setSleepPeriods(0.01, 0.1);
		}
		else
		{
			/* create menu system for SSH */
			Menu *baseMenu = CreateMenuStructure(notify);
			MenuInterface *menuInterface = new MenuInterface(ssh, baseMenu, baseDrawFlags | MENU_DRAW_FLAGS_ECHO);
			ssh->setRpcSystem(menuInterface);
			ssh->setSleepPeriods(0.05, 0.5);
		}
	
		ssh->start();
	}

	MenuInterface *terminalMenu = NULL;
	if (enableTerminal)
	{
		/* Terminal Version */
		RpcComms *stdioComms = new StdioComms(fileno(stdin), fileno(stdout)); 
		Menu *baseMenu = CreateMenuStructure(notify);
		terminalMenu = new MenuInterface(stdioComms, baseMenu, baseDrawFlags | MENU_DRAW_FLAGS_NOQUIT);
		//menuTerminal = new RsConsole(menuInterface, fileno(stdin), fileno(stdout));
	}


#endif

	/* pass control to the GUI */
	while(1)
	{
		//std::cerr << "GUI Tick()" << std::endl;

#ifdef RS_INTRO_SERVER
		rsIS.tick();
#endif

		int rt = 0;
#ifdef RS_SSH_SERVER
		if (terminalMenu)
		{
			rt = terminalMenu->tick();
		}
#endif

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

#ifdef RS_SSH_SERVER
void generatePasswordHash()
{
	std::string saltBin;
	std::string pwdHashRadix64;
	std::string sshPwdForHash = "";

	std::string passwd1,passwd2 ;
	bool cancel ;

	if(!NotifyTxt().askForPassword("Type your password (at least 8 chars) : ",false,passwd1,cancel)) exit(1) ;

	if(passwd1.length() < 8)
	{
		std::cerr << "Password must be at least 8 characters long." << std::endl;
		exit(1);
	}

	if(!NotifyTxt().askForPassword("Type your password (checking)         : ",false,passwd2,cancel)) exit(1) ;

	if(passwd1 != passwd2)
	{
		std::cerr << "Passwords differ. Please retry." << std::endl;
		exit(1);
	}

	sshPwdForHash = passwd1 ;

	//std::cerr << "Chosen Password : " << sshPwdForHash;
	std::cerr << std::endl;

	GenerateSalt(saltBin);
	if (!GeneratePasswordHash(saltBin, sshPwdForHash, pwdHashRadix64))
	{
		std::cerr << "Error Generating Password Hash, password probably too short";
		std::cerr << pwdHashRadix64;
		std::cerr << std::endl;
		exit(1);
	}

	std::cout << "Generated Password Hash for rs-nogui: ";
	std::cout << pwdHashRadix64;
	std::cout << std::endl;
	std::cout << std::endl;

	/* checking match */
	if (CheckPasswordHash(pwdHashRadix64, sshPwdForHash))
	{
		std::cerr << "Passed Check Okay!";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "ERROR: Failed CheckPassword!";
		std::cerr << std::endl;
		exit(1);
	}


	std::cerr << "Usage:";
	std::cerr << std::endl;
	std::cerr << " - for SSH access: ./retroshare-nogui    -X -S [port] -L <username> -P " << pwdHashRadix64;
	std::cerr << std::endl;
	std::cerr << " - for RPC access: ./retroshare-nogui -C -X -S [port] -L <username> -P " << pwdHashRadix64;
	std::cerr << std::endl;
}
#endif
	
