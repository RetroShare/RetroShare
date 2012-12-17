
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
	bool enableSshPwd = false;
	bool enableTerminal = false;
	bool enableSshRsa = false;
	bool genPwdHash = false;
	std::string sshUser = "user";
	std::string sshPwdHash = "";
	std::string sshRsaFile = "";
	std::string sshPortStr = "7022";

	uint16_t extPort = 0;
	bool     extPortSet = false;

	while((c = getopt(argc, argv,"ChTL:P:K:GS::E:")) != -1)
	{
		switch(c)
		{
			case 'C':
				enableRpc = true;
				strictCheck = false;
				break;
			case 'S':
				enableSsh = true;
				if (optarg)
				{
					sshPortStr = optarg; // optional.
				}
				strictCheck = false;
				break;
			case 'E':
				extPort = atoi(optarg);
				extPortSet = true;
				strictCheck = false;
				break;
			case 'H':
				enableSshHtml = true;
				strictCheck = false;
				break;
			case 'T':
				enableTerminal = true;
				strictCheck = false;
				break;
			case 'L':
				sshUser = optarg;
				strictCheck = false;
				break;
			case 'P':
				enableSshPwd = true;
				sshPwdHash = optarg;
				strictCheck = false;
				break;
#if 0 // NOT FINISHED YET.
			case 'K':
				enableSshRsa = true;
				sshRsaFile = optarg;
				strictCheck = false;
				break;
#endif
			case 'G':
				genPwdHash = true;
				break;
			case 'h':
				/* nogui help */
				std::cerr << argv[0] << std::endl;
				std::cerr << "Specific Help Options: " << std::endl;
				std::cerr << "\t-G                  Generate a Password Hash for SSH Server" << std::endl;
				std::cerr << "\t-T                  Enable Terminal Interface" << std::endl;
				std::cerr << "\t-S [port]           Enable SSH Server, optionally specify port" << std::endl;
				std::cerr << "\t-E <port>           Specify Alternative External Port (provided to Clients)" << std::endl;
				std::cerr << "\t-L <user>           Specify SSH login user (default:user)" << std::endl;
				std::cerr << "\t-P <pwdhash>        Enable SSH login via Password" << std::endl;
				std::cerr << "\t-C                  Enable RPC Protocol (requires -S too)" << std::endl;
				//std::cerr << "\t-K [rsapubkeyfile]  Enable SSH login via RSA key" << std::endl;
				//std::cerr << "\t                    NB: Two Factor Auth, specify both -P & -K" << std::endl;
				std::cerr << std::endl;
				std::cerr << "\t To setup rs-nogui as a SSH Server is a three step process: " << std::endl;
				std::cerr << "\t 1) \"ssh-keygen -t rsa -f rs_ssh_host_rsa_key\" " << std::endl;
				std::cerr << "\t 2) \"./retroshare-nogui -G\" " << std::endl;
				std::cerr << "\t 3) \"./retroshare-nogui -S [port] -L <user> -P <passwordhash>\" " << std::endl;
				std::cerr << std::endl;
				std::cerr << "Further Options ";
				/* libretroshare will call exit(1) after printing its options */
				break;	

			default:
				/* let others through - for libretroshare */
				break;
		}
	}
	// reset optind for Retroshare commandline arguments.
	optind = 1;

	if (genPwdHash)
	{
		std::string saltBin;
		std::string pwdHashRadix64;
		std::string sshPwdForHash = "";

		std::cout << "Type in your Password:" << std::flush;
		char pwd[1024];
		if (!fgets(pwd, 1024, stdin))
		{
			std::cerr << "Error Reading Password";
			std::cerr << std::endl;
			exit(1);
		}

		// strip newline.	
		for(int i = 0; (i < 1024) && (pwd[i] != '\n') && (pwd[i] != '\0'); i++)
		{
			sshPwdForHash += pwd[i];
		}	

		std::cerr << "Chosen Password : " << sshPwdForHash;
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


		std::cerr << "Usage: ./retroshare-nogui -S [port] -L <username> -P " << pwdHashRadix64;
		std::cerr << std::endl;
		exit(1);
	}
	

	/* enforce conditions */
	if (((enableSshRsa) || (enableSshPwd)) && (!enableSsh))
	{
		std::cerr << "ERROR: SSH Server (-S) must be enabled to specify SSH Pwd (-P) or SSH RSA (-K)";
		std::cerr << std::endl;
		exit(1);
	}

	if (enableSsh && (!enableSshRsa) && (!enableSshPwd))
	{
		std::cerr << "ERROR: One of (or both) SSH Pwd (-P) and SSH RSA (-K) must be specified with SSH Server (-S)";
		std::cerr << std::endl;
		exit(1);
	}

	if (enableRpc && (!enableSsh))
	{
		std::cerr << "ERROR: RPC Mode (-C) requires SSH Server (-S) enabled";
		std::cerr << std::endl;
		exit(1);
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

	if (enableSshPwd)
	{
		/* try parse it */
		/* TODO */

	}
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

	NotifyTxt *notify = new NotifyTxt();
	RsIface *iface = createRsIface(*notify);
	RsControl *rsServer = createRsControl(*iface, *notify);
	rsicontrol = rsServer ;

	notify->setRsIface(iface);

	std::string preferredId, gpgId, gpgName, gpgEmail, sslName;
	RsInit::getPreferedAccountId(preferredId);

	if (RsInit::getAccountDetails(preferredId, gpgId, gpgName, gpgEmail, sslName))
	{
		RsInit::SelectGPGAccount(gpgId);
	}

	/* Key + Certificate are loaded into libretroshare */

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
		ssh = RsSshd::InitRsSshd(sshPortStr, "rs_ssh_host_rsa_key");
		// TODO Parse Option
		if (enableSshRsa)
		{
        		//ssh->adduser("anrsuser", "test");
		}

		if (enableSshPwd)
		{
        		ssh->adduserpwdhash(sshUser, sshPwdHash);
		}
			
		if (!extPortSet)
		{
			extPort = atoi(sshPortStr.c_str());
		}

		// NASTY GLOBAL VARIABLE HACK - NEED TO THINK OF A BETTER SYSTEM.
		RpcProtoSystem::mExtPort = extPort;
	}


#endif

	/* Start-up libretroshare server threads */
	rsServer -> StartupRetroShare();

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
