
/*
 * "$Id: p3face-startup.cc,v 1.9 2007-05-05 16:10:06 rmf24 Exp $"
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

#include <unistd.h>
//#include <getopt.h>

#include "dbase/cachestrapper.h"
#include "server/ftfiler.h"
#include "server/filedexserver.h"

#include "pqi/pqipersongrp.h"
#include "pqi/pqisslpersongrp.h"
#include "pqi/pqiloopback.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqidebug.h"

#include "util/rsdir.h"

#include "upnp/upnphandler.h"
#include "dht/opendhtmgr.h"

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3gamelauncher.h"
#include "services/p3ranking.h"

#include <list>
#include <string>
#include <sstream>

// for blocking signals
#include <signal.h>

#include "rsserver/p3face.h"
#include "rsserver/p3peers.h"
#include "rsserver/p3rank.h"
#include "rsserver/p3msgs.h"
#include "rsiface/rsgame.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	#include "pqi/authxpgp.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

const int p3facestartupzone = 47238;

// initial configuration bootstrapping...
static const std::string configInitFile = "default_cert.txt";
static const std::string configConfFile = "config.rs";
static const std::string configCertDir = "friends";
static const std::string configKeyDir = "keys";
static const std::string configCaFile = "cacerts.pem";
static const std::string configLogFileName = "retro.log";
static const std::string configHelpName = "retro.htm";


/* Helper Functions */
void load_check_basedir(RsInit *config);
int  create_configinit(RsInit *config);

RsControl *createRsControl(RsIface &iface, NotifyBase &notify)
{
	RsServer *srv = new RsServer(iface, notify);
	return srv;
}

void    CleanupRsConfig(RsInit *config)
{
	delete config;
}

static std::string getHomePath();



RsInit *InitRsConfig()
{
	RsInit *config = new RsInit();

	config -> load_trustedpeer = false;
	config -> firsttime_run = false;
	config -> port = 7812; // default port.
	config -> forceLocalAddr = false;
	config -> haveLogFile    = false;
	config -> outStderr      = false;

	strcpy(config->inet, "127.0.0.1");
	strcpy(config->logfname, "");

	config -> autoLogin      = false;
	config -> passwd         = "";
	config -> havePasswd     = false;
	config -> haveDebugLevel = false;
	config -> debugLevel	= PQL_WARNING;
	config -> udpListenerOnly = false;

#ifndef WINDOWS_SYS
	config -> dirSeperator = '/'; // For unix.
#else
	config -> dirSeperator = '\\'; // For windows.
#endif

	/* setup the homePath (default save location) */

	config -> homePath = getHomePath();

	/* Setup the Debugging */
	// setup debugging for desired zones.
	setOutputLevel(PQL_WARNING); // default to Warnings.

	// For Testing purposes.
	// We can adjust everything under Linux.
	//setZoneLevel(PQL_DEBUG_BASIC, 38422); // pqipacket.
	//setZoneLevel(PQL_DEBUG_BASIC, 96184); // pqinetwork;
	//setZoneLevel(PQL_DEBUG_BASIC, 82371); // pqiperson.
	//setZoneLevel(PQL_DEBUG_BASIC, 60478); // pqitunnel.
	//setZoneLevel(PQL_DEBUG_BASIC, 34283); // pqihandler.
	//setZoneLevel(PQL_DEBUG_BASIC, 44863); // discItems.
	//setZoneLevel(PQL_DEBUG_BASIC, 2482); // p3disc
	//setZoneLevel(PQL_DEBUG_BASIC, 1728); // pqi/p3proxy
	//setZoneLevel(PQL_DEBUG_BASIC, 1211); // sslroot.
	//setZoneLevel(PQL_DEBUG_BASIC, 37714); // pqissl.
	//setZoneLevel(PQL_DEBUG_BASIC, 8221); // pqistreamer.
	//setZoneLevel(PQL_DEBUG_BASIC,  9326); // pqiarchive
	//setZoneLevel(PQL_DEBUG_BASIC, 3334); // p3channel.
	//setZoneLevel(PQL_DEBUG_BASIC, 354); // pqipersongrp.
	//setZoneLevel(PQL_DEBUG_BASIC, 6846); // pqiudpproxy
	//setZoneLevel(PQL_DEBUG_BASIC, 3144); // pqissludp;
	//setZoneLevel(PQL_DEBUG_BASIC, 86539); // pqifiler.
	//setZoneLevel(PQL_DEBUG_BASIC, 91393); // Funky_Browser.
	//setZoneLevel(PQL_DEBUG_BASIC, 25915); // fltkserver
	//setZoneLevel(PQL_DEBUG_BASIC, 47659); // fldxsrvr
	//setZoneLevel(PQL_DEBUG_BASIC, 49787); // pqissllistener

	return config;
}

const char *RsConfigDirectory(RsInit *config)
{
	return (config->basedir).c_str();
}

//int InitRetroShare(int argc, char **argv, RsInit *config)
//{

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
int InitRetroShare(int argc, char **argv, RsInit *config)
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

   /* for static PThreads under windows... we need to init the library...
    */
   #ifdef PTW32_STATIC_LIB
      #include <pthread.h>
   #endif 

int InitRetroShare(int argcIgnored, char **argvIgnored, RsInit *config)
{

  /* THIS IS A HACK TO ALLOW WINDOWS TO ACCEPT COMMANDLINE ARGUMENTS */

  const int MAX_ARGS = 32;
  int i,j;

  int argc;
  char *argv[MAX_ARGS];
  char *wholeline = GetCommandLine();
  int cmdlen = strlen(wholeline);
  // duplicate line, so we can put in spaces..
  char dupline[cmdlen+1];
  strcpy(dupline, wholeline);

  /* break wholeline down .... 
   * NB. This is very simplistic, and will not 
   * handle multiple spaces, or quotations etc, only for debugging purposes
   */
  argv[0] = dupline;
  for(i = 1, j = 0; (j + 1 < cmdlen) && (i < MAX_ARGS);)
  {
	/* find next space. */
	for(;(j + 1 < cmdlen) && (dupline[j] != ' ');j++);
	if (j + 1 < cmdlen)
	{
		dupline[j] = '\0';
		argv[i++] = &(dupline[j+1]);
	}
  }
  argc = i;
  for( i=0; i<argc; i++)
  {
    printf("%d: %s\n", i, argv[i]);
  }

/* for static PThreads under windows... we need to init the library...
 */
  #ifdef PTW32_STATIC_LIB
	 pthread_win32_process_attach_np();
  #endif 

#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	int c;
	while((c = getopt(argc, argv,"ai:p:c:sw:l:d:u")) != -1)
	{
		switch (c)
		{
			case 'a':
				config->autoLogin = true;
				std::cerr << "AutoLogin Allowed";
				std::cerr << std::endl;
				break;
			case 'l':
				strncpy(config->logfname, optarg, 1024);
				std::cerr << "LogFile (" << config->logfname;
				std::cerr << ") Selected" << std::endl;
				config->haveLogFile = true;
				break;
			case 'w':
				config->passwd = optarg;
				std::cerr << "Password Specified(" << config->passwd;
				std::cerr << ") Selected" << std::endl;
				config->havePasswd = true;
				break;
			case 'i':
				strncpy(config->inet, optarg, 256);
				std::cerr << "New Inet Addr(" << config->inet;
				std::cerr << ") Selected" << std::endl;
				config->forceLocalAddr = true;
				break;
			case 'p':
				config->port = atoi(optarg);
				std::cerr << "New Listening Port(" << config->port;
				std::cerr << ") Selected" << std::endl;
				break;
			case 'c':
				config->basedir = optarg;
				std::cerr << "New Base Config Dir(";
				std::cerr << config->basedir;
				std::cerr << ") Selected" << std::endl;
				break;
			case 's':
				config->outStderr = true;
				config->haveLogFile = false;
				std::cerr << "Output to Stderr";
				std::cerr << std::endl;
				break;
			case 'd':
				config->haveDebugLevel = true;
				config->debugLevel = atoi(optarg);
				std::cerr << "Opt for new Debug Level";
				std::cerr << std::endl;
				break;
			case 'u':
				config->udpListenerOnly = true;
				std::cerr << "Opt for only udpListener";
				std::cerr << std::endl;
				break;
			default:
				std::cerr << "Unknown Option!";
				exit(1);
		}
	}


	// set the default Debug Level...
	if (config->haveDebugLevel)
	{
		if ((config->debugLevel > 0) && 
			(config->debugLevel <= PQL_DEBUG_ALL))
		{
			std::cerr << "Setting Debug Level to: ";
			std::cerr << config->debugLevel;
			std::cerr << std::endl;
  			setOutputLevel(config->debugLevel);
		}
		else
		{
			std::cerr << "Ignoring Invalid Debug Level: ";
			std::cerr << config->debugLevel;
			std::cerr << std::endl;
		}
	}

	// set the debug file.
	if (config->haveLogFile) 
	{
		setDebugFile(config->logfname);
	}

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else
	// Windows Networking Init.
	WORD wVerReq = MAKEWORD(2,2);
	WSADATA wsaData;

	if (0 != WSAStartup(wVerReq, &wsaData))
	{
		std::cerr << "Failed to Startup Windows Networking";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "Started Windows Networking";
		std::cerr << std::endl;
	}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// first check config directories, and set bootstrap values.
	load_check_basedir(config);

	// SWITCH off the SIGPIPE - kills process on Linux.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	struct sigaction sigact;
	sigact.sa_handler = SIG_IGN;
	sigact.sa_flags = 0;

	if (0 == sigaction(SIGPIPE, &sigact, NULL))
	{
		std::cerr << "RetroShare:: Successfully Installed";
		std::cerr << "the SIGPIPE Block" << std::endl;
	}
	else
	{
		std::cerr << "RetroShare:: Failed to Install";
		std::cerr << "the SIGPIPE Block" << std::endl;
	}
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	std::string userName;
	bool existingUser = false;
	if (LoadCheckXPGPandGetName(config->load_cert.c_str(), userName))
	{
		std::cerr << "Existing Name: " << userName << std::endl;
		existingUser = true;
	}
	else
	{
		std::cerr << "No Existing User" << std::endl;
	}

	/* do a null init to allow the SSL libray to startup! */
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	/* do a null init to allow the SSL libray to startup! */
	getAuthMgr() -> InitAuth(NULL, NULL, NULL); 
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	getAuthMgr() -> InitAuth(NULL, NULL, NULL, NULL); 
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	/* if existing user, and havePasswd .... we can skip the login prompt */
	if (existingUser)
	{
		if (config -> havePasswd)
		{
			return 1;
		}
		if (RsTryAutoLogin(config))
		{
			return 1;
		}
	}
	return 0;
}


/* 
 * The Real RetroShare Startup Function.
 */

int RsServer::StartupRetroShare(RsInit *config)
{
	/**************************************************************************/
	/* STARTUP procedure */
	/**************************************************************************/
	/**************************************************************************/
	/* (1) Load up own certificate (DONE ALREADY) - just CHECK */
	/**************************************************************************/

	mAuthMgr = getAuthMgr(); 

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (1 != mAuthMgr -> InitAuth(NULL, NULL, NULL))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (1 != mAuthMgr -> InitAuth(NULL, NULL, NULL, NULL))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Invalid Certificate configuration!" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	std::string ownId = mAuthMgr->OwnId();

	/**************************************************************************/
	/* Any Initial Configuration (Commandline Options)  */
	/**************************************************************************/

	/* set the debugging to crashMode */
	if ((!config->haveLogFile) && (!config->outStderr))
	{
		std::string crashfile = config->basedir + config->dirSeperator;
		crashfile += configLogFileName;
		setDebugCrashMode(crashfile.c_str());
	}

	unsigned long flags = 0;
	if (config->udpListenerOnly)
	{
		flags |= PQIPERSON_NO_LISTENER;
	}

	/**************************************************************************/

	// Load up Certificates, and Old Configuration (if present)
	
	std::string certConfigFile = config->basedir.c_str();
	std::string certNeighDir   = config->basedir.c_str();
	if (certConfigFile != "")
	{
		certConfigFile += "/";
		certNeighDir += "/";
	}
	certConfigFile += configConfFile;
	certNeighDir +=   configCertDir;

	/* if we've loaded an old format file! */
        bool oldFormat = false;
	std::map<std::string, std::string> oldConfigMap;

	mAuthMgr -> setConfigDirectories(certConfigFile, certNeighDir);
	((AuthXPGP *) mAuthMgr) -> loadCertificates(oldFormat, oldConfigMap);


	/**************************************************************************/
	/* setup classes / structures */
	/**************************************************************************/

	mConnMgr = new p3ConnectMgr(mAuthMgr);
	p3UpnpMgr *mUpnpMgr = new upnphandler();
	p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr);

	CacheStrapper *mCacheStrapper = new CacheStrapper(mAuthMgr, mConnMgr);
	ftfiler       *mCacheTransfer = new ftfiler(mCacheStrapper);

	SecurityPolicy *none = secpolicy_create();
	pqih = new pqisslpersongrp(none, flags);
	//pqih = new pqipersongrpDummy(none, flags);

	// filedex server.
	server = new filedexserver();
	server->setConfigDir(config->basedir.c_str());
	server->setSaveDir(config->homePath.c_str()); /* Default Save Dir - config will overwrite */
	server->setSearchInterface(pqih, mAuthMgr, mConnMgr);
	server->setFileCallback(ownId, mCacheStrapper, mCacheTransfer, &(getNotify()));

	mConfigMgr = new p3ConfigMgr(mAuthMgr, config->basedir, "rs-v0.4.cfg", "rs-v0.4.sgn");
	mGeneralConfig = new p3GeneralConfig();

	/* create Services */
	ad = new p3disc(mAuthMgr, mConnMgr);
	msgSrv = new p3MsgService(mConnMgr);
	chatSrv = new p3ChatService(mConnMgr);
	p3GameLauncher *gameLauncher = new p3GameLauncher();

	pqih -> addService(ad);
	pqih -> addService(msgSrv);
	pqih -> addService(chatSrv);
	pqih -> addService(gameLauncher);

	/* create Cache Services */
	std::string config_dir = config->basedir;
        std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";

	mRanking = new p3Ranking(RS_SERVICE_TYPE_RANK, 
			mCacheStrapper, mCacheTransfer, 
			localcachedir, remotecachedir, 3600 * 24 * 30);

        CachePair cp(mRanking, mRanking, CacheId(RS_SERVICE_TYPE_RANK, 0));
	mCacheStrapper -> addCachePair(cp);

	/**************************************************************************/

	mConnMgr->setDhtMgr(mDhtMgr);
	mConnMgr->setUpnpMgr(mUpnpMgr);

	/**************************************************************************/
	/* need to Monitor too! */

	mConnMgr->addMonitor(pqih);
	mConnMgr->addMonitor(mCacheStrapper);
	mConnMgr->addMonitor(ad);
	mConnMgr->addMonitor(msgSrv);

	/**************************************************************************/

	mConfigMgr->addConfiguration("server.cfg", server); 
	mConfigMgr->addConfiguration("peers.cfg", mConnMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);

	/**************************************************************************/


	/**************************************************************************/
	/* (2) Load configuration files */
	/**************************************************************************/

	mConfigMgr->loadConfiguration();

	/**************************************************************************/
	/* Hack Old Configuration into new System (first load only) */
	/**************************************************************************/

	if (oldFormat)
	{
		std::cerr << "Startup() Loaded Old Certificate Format" << std::endl;

		/* transfer all authenticated peers to friend list */
		std::list<std::string> authIds;
		mAuthMgr->getAuthenticatedList(authIds);

		std::list<std::string>::iterator it;
		for(it = authIds.begin(); it != authIds.end(); it++)
		{
			mConnMgr->addFriend(*it);
		}

		/* move other configuration options */
	}

	/**************************************************************************/
	/* trigger generalConfig loading for classes that require it */
	/**************************************************************************/

	pqih->setConfig(mGeneralConfig);

        pqih->load_config();

	/**************************************************************************/
	/* Force Any Configuration before Startup (After Load) */
	/**************************************************************************/

	if (config->forceLocalAddr)
	{
		struct sockaddr_in laddr;

		laddr.sin_family = AF_INET;
		laddr.sin_port = htons(config->port);

		// universal
		laddr.sin_addr.s_addr = inet_addr(config->inet);

		mConnMgr->setLocalAddress(ownId, laddr);
	}

#if 0
	/* must load the trusted_peer before setting up the pqipersongrp */
	if (config->firsttime_run)
	{
		/* at this point we want to load and start the trusted peer -> if selected */
		if (config->load_trustedpeer)
		{
			/* sslroot does further checks */
        		sslr -> loadInitialTrustedPeer(config->load_trustedpeer_file);
		}
	}
#endif

	mConnMgr -> checkNetAddress();

	/**************************************************************************/
	/* startup (stuff dependent on Ids/peers is after this point) */
	/**************************************************************************/

	pqih->init_listener();




	/**************************************************************************/
	/* load caches and secondary data */
	/**************************************************************************/


	/**************************************************************************/
	/* Force Any Last Configuration Options */
	/**************************************************************************/

	/**************************************************************************/
	/* Start up Threads */
	/**************************************************************************/

	server->StartupMonitor();
	mUpnpMgr->start();
	mDhtMgr->start();


#ifdef PQI_USE_CHANNELS
	server->setP3Channel(pqih->getP3Channel());
#endif

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback(ownId);

	mod -> peerid = ownId;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

	rsPeers = new p3Peers(mConnMgr, mAuthMgr);
	rsGameLauncher = gameLauncher;
	rsRanks = new p3Rank(mRanking);
	rsMsgs  = new p3Msgs(mAuthMgr, msgSrv, chatSrv);

	/* put a welcome message in! */
	if (config->firsttime_run)
	{
		msgSrv->loadWelcomeMsg();
	}

	// load up the help page
	std::string helppage = config->basedir + config->dirSeperator;
	helppage += configHelpName;

	/* for DHT/UPnP stuff */
	//InitNetworking(config->basedir + "/kadc.ini");

	/* Startup this thread! */
        createThread(*this);

	return 1;
}



int LoadCertificates(RsInit *config, bool autoLoginNT)
{
	if (config->load_cert == "")
	{
	  std::cerr << "RetroShare needs a certificate" << std::endl;
	  return 0;
	}

	if (config->load_key == "")
	{
	  std::cerr << "RetroShare needs a key" << std::endl;
	  return 0;
	}

	if ((!config->havePasswd) || (config->passwd == ""))
	{
	  std::cerr << "RetroShare needs a Password" << std::endl;
	  return 0;
	}

	std::string ca_loc = config->basedir + config->dirSeperator;
	ca_loc += configCaFile;

	p3AuthMgr *authMgr = getAuthMgr();

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (0 < authMgr -> InitAuth(config->load_cert.c_str(), 
				config->load_key.c_str(), 
				config->passwd.c_str()))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (0 < authMgr -> InitAuth(config->load_cert.c_str(), 
				config->load_key.c_str(), 
				ca_loc.c_str(), 
				config->passwd.c_str()))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	{
		if (autoLoginNT)
		{
			std::cerr << "RetroShare will AutoLogin next time";
			std::cerr << std::endl;

			RsStoreAutoLogin(config);
		}
		/* wipe password */
		config->passwd = "";
		create_configinit(config);
		return 1;
	}

	std::cerr << "RetroShare Failed To Start!" << std::endl;
	std::cerr << "Please Check File Names/Password" << std::endl;

	return 0;
}

/* To Enter RetroShare.... must call either:
 * LoadPassword, or
 * RsGenerateCertificate.
 *
 * Then call LoadCertificate .... if it returns true....
 * its all okay.
 */

/* Assistance for Login */
bool ValidateCertificate(RsInit *config, std::string &userName)
{
	std::string fname = config->load_cert;
	if (fname != "")
	{
		return LoadCheckXPGPandGetName(fname.c_str(), userName);
	}
	return false;
}

bool ValidateTrustedUser(RsInit *config, std::string fname, std::string &userName)
{
	bool valid = LoadCheckXPGPandGetName(fname.c_str(), userName);
	if (valid)
	{
		config -> load_trustedpeer = true;
		config -> load_trustedpeer_file = fname;
	}
	else
	{
		config -> load_trustedpeer = false;
	}
	return valid;
}

bool LoadPassword(RsInit *config, std::string passwd)
{
	config -> passwd = passwd;
	config -> havePasswd = true;
	return true;
}

/* A little nasty fn....
 * (1) returns true, if successful, and updates config.
 * (2) returns false if fails, with error msg to errString.
 */

bool RsGenerateCertificate(RsInit *config, 
			std::string name, 
			std::string org, 
			std::string loc, 
			std::string country, 
			std::string passwd, 
			std::string &errString)
{
	// In the XPGP world this is easy...
	// generate the private_key / certificate.
	// save to file.
	//
	// then load as if they had entered a passwd.

	// check password.
	if (passwd.length() < 4) 	
	{
		errString = "Password is Unsatisfactory (must be 4+ chars)";
		return false;
	}

	if (name.length() < 3) 	
	{
		errString = "Name is too short (must be 3+ chars)";
		return false;
	}

	int nbits = 2048;

	// Create the filename.
	std::string basename = config->basedir + config->dirSeperator;
	basename += configKeyDir + config->dirSeperator;
	basename += "user"; 

	std::string key_name = basename + "_pk.pem";
	std::string cert_name = basename + "_cert.pem";


/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (!generate_xpgp(cert_name.c_str(), key_name.c_str(), 
			passwd.c_str(),
			name.c_str(), 
			"", //ui -> gen_email -> value(), 
			org.c_str(),
			loc.c_str(),
			"", //ui -> gen_state -> value(), 
			country.c_str(),
			nbits))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	/* UNTIL THIS IS FILLED IN CANNOT GENERATE X509 REQ */
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	{
		errString = "Generation of XPGP Failed";
		return false;
	}

	/* set the load passwd to the gen version 
	 * and try to load it!
	 */

	/* if we get here .... then save details to the configuration class */
	config -> load_cert = cert_name;
	config -> load_key  = key_name;
	config -> passwd = passwd;
	config -> havePasswd = true;
	config -> firsttime_run = true;

	{
		std::ostringstream out;
		out << "RetroShare has Successfully generated";
		out << "a Certficate/Key" << std::endl;
		out << "\tCert Located: " << cert_name << std::endl;
		out << "\tLocated: " << key_name << std::endl;
		std::cerr << out.str();
	}
	return true;
}



void load_check_basedir(RsInit *config)
{
	// get the default configuration location.
	
	if (config->basedir == "")
	{
		// if unix. homedir + /.pqiPGPrc
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		char *h = getenv("HOME");
		std::cerr << "retroShare::basedir() -> $HOME = ";
		std::cerr << h << std::endl;
		if (h == NULL)
		{
			std::cerr << "load_check_basedir() Fatal Error --";
		  	std::cerr << std::endl;
			std::cerr << "\tcannot determine $HOME dir" <<std::endl;
			exit(1);
		}
		config->basedir = h;
		config->basedir += "/.pqiPGPrc";
#else
		char *h = getenv("APPDATA");
		std::cerr << "retroShare::basedir() -> $APPDATA = ";
		std::cerr << h << std::endl;
		char *h2 = getenv("HOMEDRIVE");
		std::cerr << "retroShare::basedir() -> $HOMEDRIVE = ";
		std::cerr << h2 << std::endl;
		char *h3 = getenv("HOMEPATH");
		std::cerr << "retroShare::basedir() -> $HOMEPATH = ";
		std::cerr << h3 << std::endl;
		if (h == NULL)
		{
			// generating default
			std::cerr << "load_check_basedir() getEnv Error --Win95/98?";
		  	std::cerr << std::endl;

			config->basedir="C:\\Retro";

		}
		else
		{
			config->basedir = h;
		}

		if (!RsDirUtil::checkCreateDirectory(config->basedir))
		{
			std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
			exit(1);
		}
		config->basedir += "\\RetroShare";
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}


	std::string subdir1 = config->basedir + config->dirSeperator;
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	std::string subdir3 = config->basedir + config->dirSeperator;
	subdir3 += "cache";

	std::string subdir4 = subdir3 + config->dirSeperator;
	std::string subdir5 = subdir3 + config->dirSeperator;
	subdir4 += "local";
	subdir5 += "remote";

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(config->basedir))
	{
		std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
		exit(1);
	}
	if (!RsDirUtil::checkCreateDirectory(subdir1))
	{
		std::cerr << "Cannot Create Config/Key Dir" << std::endl;
		exit(1);
	}
	if (!RsDirUtil::checkCreateDirectory(subdir2))
	{
		std::cerr << "Cannot Create Config/Cert Dir" << std::endl;
		exit(1);
	}
	if (!RsDirUtil::checkCreateDirectory(subdir3))
	{
		std::cerr << "Cannot Create Config/Cache Dir" << std::endl;
		exit(1);
	}
	if (!RsDirUtil::checkCreateDirectory(subdir4))
	{
		std::cerr << "Cannot Create Config/Cache/local Dir" << std::endl;
		exit(1);
	}
	if (!RsDirUtil::checkCreateDirectory(subdir5))
	{
		std::cerr << "Cannot Create Config/Cache/remote Dir" << std::endl;
		exit(1);
	}

	// have a config directories.
	
	// Check for config file.
	std::string initfile = config->basedir + config->dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "r");
	char path[1024];
	int i;

	if (ifd != NULL)
	{
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++);
			path[i] = '\0';
			config->load_cert = path;
		}
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++);
			path[i] = '\0';
			config->load_key = path;
		}
		fclose(ifd);
	}

	// we have now 
	// 1) checked or created the config dirs.
	// 2) loaded the config_init file - if possible.
	return;
}

int	create_configinit(RsInit *config)
{
	// Check for config file.
	std::string initfile = config->basedir + config->dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "w");

	if (ifd != NULL)
	{
		fprintf(ifd, "%s\n", config->load_cert.c_str());
		fprintf(ifd, "%s\n", config->load_key.c_str());
		fclose(ifd);

		std::cerr << "Creating Init File: " << initfile << std::endl;
		std::cerr << "\tLoad Cert: " << config->load_cert << std::endl;
		std::cerr << "\tLoad Key: " <<  config->load_key << std::endl;

		return 1;
	}
	std::cerr << "Failed To Create Init File: " << initfile << std::endl;
	return -1;
}

#if 0

int	check_create_directory(std::string dir)
{
	struct stat buf;
	int val = stat(dir.c_str(), &buf);
	if (val == -1)
	{
		// directory don't exist. create.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX
		if (-1 == mkdir(dir.c_str(), 0777))
#else // WIN
		if (-1 == mkdir(dir.c_str()))
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

		{
		  std::cerr << "check_create_directory() Fatal Error --";
		  std::cerr <<std::endl<< "\tcannot create:" <<dir<<std::endl;
		  exit(1);
		}

		std::cerr << "check_create_directory()";
		std::cerr <<std::endl<< "\tcreated:" <<dir<<std::endl;
	} 
	else if (!S_ISDIR(buf.st_mode))
	{
		// Some other type - error.
		std::cerr<<"check_create_directory() Fatal Error --";
		std::cerr<<std::endl<<"\t"<<dir<<" is nor Directory"<<std::endl;
		exit(1);
	}
	std::cerr << "check_create_directory()";
	std::cerr <<std::endl<< "\tDir Exists:" <<dir<<std::endl;
	return 1;
}

#endif


std::string getHomePath()
{
	std::string home;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */

	home = getenv("HOME");

#else /* Windows */

	std::ostringstream out;
	char *h2 = getenv("HOMEDRIVE");
	out << "getHomePath() -> $HOMEDRIVE = ";
	out << h2 << std::endl;
	char *h3 = getenv("HOMEPATH");
	out << "getHomePath() -> $HOMEPATH = ";
	out << h3 << std::endl;

	if (h2 == NULL)
	{
		// Might be Win95/98
		// generate default.
		home = "C:\\Retro";
	}
	else
	{
		home = h2;
		home += h3;
		home += "\\Desktop";
	}

	out << "fltkserver::getHomePath() -> " << home << std::endl;
	std::cerr << out;

	// convert to FLTK desired format.
	home = make_path_unix(home);
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return home;
}

std::string make_path_unix(std::string path)
{
	for(unsigned int i = 0; i < path.length(); i++)
	{
		if (path[i] == '\\')
			path[i] = '/';
	}
	return path;
}

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
/* WINDOWS STRUCTURES FOR DPAPI */

#ifndef WINDOWS_SYS /* UNIX */
#else
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/


#include <windows.h>
#include <Wincrypt.h>
#include <iomanip>

/*
class CRYPTPROTECT_PROMPTSTRUCT;
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CRYPTPROTECT_PROMPTSTRUCT {
  DWORD cbSize;
  DWORD dwPromptFlags;
  HWND hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT, 
 *PCRYPTPROTECT_PROMPTSTRUCT;

/* definitions for the two functions */
__declspec (dllimport)
extern BOOL WINAPI CryptProtectData(
  DATA_BLOB* pDataIn,
  LPCWSTR szDataDescr,
  DATA_BLOB* pOptionalEntropy,
  PVOID pvReserved,
  /* PVOID prompt, */
  /* CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, */
  CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, 
  DWORD dwFlags,
  DATA_BLOB* pDataOut
);

__declspec (dllimport)
extern BOOL WINAPI CryptUnprotectData(
  DATA_BLOB* pDataIn,
  LPWSTR* ppszDataDescr,
  DATA_BLOB* pOptionalEntropy,
  PVOID pvReserved,
  /* PVOID prompt, */
  /* CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, */
  CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, 
  DWORD dwFlags,
  DATA_BLOB* pDataOut
);

#ifdef __cplusplus
}
#endif

#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/



bool  RsStoreAutoLogin(RsInit *config)
{
	std::cerr << "RsStoreAutoLogin()" << std::endl;
	/* Windows only */
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	/* store password encrypted in a file */
	std::string entropy = config->load_cert;

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;
	BYTE *pbDataInput = (BYTE *) strdup(config->passwd.c_str());
	DWORD cbDataInput = strlen((char *)pbDataInput)+1;
	BYTE *pbDataEnt   =(BYTE *)  strdup(entropy.c_str());
	DWORD cbDataEnt   = strlen((char *)pbDataEnt)+1;
	DataIn.pbData = pbDataInput;    
	DataIn.cbData = cbDataInput; 
	DataEnt.pbData = pbDataEnt;    
	DataEnt.cbData = cbDataEnt; 
	LPWSTR pDescrOut = NULL;

        CRYPTPROTECT_PROMPTSTRUCT prom; 

        prom.cbSize = sizeof(prom);
        prom.dwPromptFlags = 0;

	/*********
     	std::cerr << "Password (" << cbDataInput << "):";
	std::cerr << pbDataInput << std::endl;
     	std::cerr << "Entropy (" << cbDataEnt << "):";
	std::cerr << pbDataEnt   << std::endl;
	*********/

	if(CryptProtectData(
     		&DataIn,
		NULL,
     		&DataEnt, /* entropy.c_str(), */
     		NULL,                               // Reserved.
		&prom,
     		0,
     		&DataOut))
	{

		/**********
     		std::cerr << "The encryption phase worked. (";
     		std::cerr << DataOut.cbData << ")" << std::endl;

		for(unsigned int i = 0; i < DataOut.cbData; i++)
		{
			std::cerr << std::setw(2) << (int) DataOut.pbData[i];
			std::cerr << " ";
		}
     		std::cerr << std::endl;
		**********/

		/* save the data to the file */
		std::string passwdfile = config->basedir;
		passwdfile += config->dirSeperator;
		passwdfile += "help.dta";

     		//std::cerr << "Save to: " << passwdfile;
     		//std::cerr << std::endl;

		FILE *fp = fopen(passwdfile.c_str(), "wb");
		if (fp != NULL)
		{
			fwrite(DataOut.pbData, 1, DataOut.cbData, fp);
			fclose(fp);
			
     			std::cerr << "AutoLogin Data saved: ";
     			std::cerr << std::endl;
		}
	}
	else
	{
     		std::cerr << "Encryption Failed";
     		std::cerr << std::endl;
	}

	free(pbDataInput);
	free(pbDataEnt);
	LocalFree(DataOut.pbData);

#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	return false;
}



bool  RsTryAutoLogin(RsInit *config)
{
	std::cerr << "RsTryAutoLogin()" << std::endl;
	/* Windows only */
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	/* Require a AutoLogin flag in the config to do this */
	if (!config->autoLogin)
	{
		return false;
	}

	/* try to load from file */
	std::string entropy = config->load_cert;
	/* get the data out */

	/* open the data to the file */
	std::string passwdfile = config->basedir;
	passwdfile += config->dirSeperator;
	passwdfile += "help.dta";

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;

	BYTE *pbDataEnt   =(BYTE *)  strdup(entropy.c_str());
	DWORD cbDataEnt   = strlen((char *)pbDataEnt)+1;
	DataEnt.pbData = pbDataEnt;    
	DataEnt.cbData = cbDataEnt;

	char *dataptr = NULL;
	int   datalen = 0;

	FILE *fp = fopen(passwdfile.c_str(), "rb");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		datalen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		dataptr = (char *) malloc(datalen);
		fread(dataptr, 1, datalen, fp);
		fclose(fp);

		/*****
     		std::cerr << "Data loaded from: " << passwdfile;
     		std::cerr << std::endl;

     		std::cerr << "Size :";
     		std::cerr << datalen << std::endl;

		for(unsigned int i = 0; i < datalen; i++)
		{
			std::cerr << std::setw(2) << (int) dataptr[i];
			std::cerr << " ";
		}
     		std::cerr << std::endl;
		*****/
	}
	else
	{
		return false;
	}

	BYTE *pbDataInput =(BYTE *) dataptr;
	DWORD cbDataInput = datalen;
	DataIn.pbData = pbDataInput;    
	DataIn.cbData = cbDataInput;


        CRYPTPROTECT_PROMPTSTRUCT prom; 

        prom.cbSize = sizeof(prom);
        prom.dwPromptFlags = 0;


	bool isDecrypt = CryptUnprotectData(
       		&DataIn,
		NULL, 
       		&DataEnt,  /* entropy.c_str(), */
        	NULL,                 // Reserved
        	&prom,                 // Opt. Prompt
        	0,
        	&DataOut);

	if (isDecrypt)
	{
     		//std::cerr << "Decrypted size: " << DataOut.cbData;
		//std::cerr << std::endl;
		if (DataOut.pbData[DataOut.cbData - 1] != '\0')
		{
     			std::cerr << "Error: Decrypted Data not a string...";
			std::cerr << std::endl;
			isDecrypt = false;
		}
		else
		{
     		  //std::cerr << "The decrypted data is: " << DataOut.pbData;
		  //std::cerr << std::endl;
		  config -> passwd = (char *) DataOut.pbData;
		  config -> havePasswd = true;
		}
	}
	else
	{
    		std::cerr << "Decryption error!";
		std::cerr << std::endl;
	}

	/* strings to be freed */
	free(pbDataInput);
	free(pbDataEnt);

	/* generated data space */
	LocalFree(DataOut.pbData);

	return isDecrypt;
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	return false;
}

bool  RsClearAutoLogin(std::string basedir)
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	std::string file = basedir;
	file += "\\help.dta";

	FILE *fp = fopen(file.c_str(), "wb");
	if (fp != NULL)
	{
		fwrite(" ", 1, 1, fp);
		fclose(fp);
		
     		std::cerr << "AutoLogin Data cleared! ";
     		std::cerr << std::endl;
		return true;
	}
	return false;
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return false;
}



