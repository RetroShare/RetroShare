
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
#include "ft/ftserver.h"
#include "ft/ftcontroller.h"
#include "rsiface/rsturtle.h"

/* global variable now points straight to
 * ft/ code so variable defined here.
 */

RsFiles *rsFiles = NULL;
RsTurtle *rsTurtle = NULL ;

#include "pqi/pqipersongrp.h"
#include "pqi/pqisslpersongrp.h"
#include "pqi/pqiloopback.h"
#include "pqi/p3cfgmgr.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"

#include "rsiface/rsinit.h"
#include "upnp/upnphandler.h"
#include "dht/opendhtmgr.h"

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3gamelauncher.h"
#include "services/p3ranking.h"
#include "services/p3photoservice.h"
#include "services/p3forums.h"
#include "services/p3channels.h"
#include "services/p3status.h"
#include "services/p3Qblog.h"
#include "turtle/p3turtle.h"

#include <list>
#include <string>
#include <sstream>

// for blocking signals
#include <signal.h>

/* Implemented Rs Interfaces */
#include "rsserver/p3face.h"
#include "rsserver/p3peers.h"
#include "rsserver/p3rank.h"
#include "rsserver/p3msgs.h"
#include "rsserver/p3discovery.h"
#include "rsserver/p3photo.h"
#include "rsserver/p3Blog.h"
#include "rsiface/rsgame.h"

#include "rsserver/p3files.h"

#include "pqi/p3notify.h" // HACK - moved to pqi for compilation order.


// COMMENT THIS FOR UNFINISHED SERVICES
/****
#define RS_RELEASE 1
****/

#define RS_RELEASE 1

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	#include "pqi/authxpgp.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
/**************** PQI_USE_SSLONLY ***************/
  #if defined(PQI_USE_SSLONLY)
	#include "pqi/authssl.h"
  #else /* X509 Certificates */
  /**************** PQI_USE_SSLONLY ***************/
  /**************** SSL + OPENPGP *****************/
	#include "pqi/authgpg.h"
	#include "pqi/authssl.h"
  #endif /* X509 Certificates */
  /**************** SSL + OPENPGP *****************/
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

std::string RsInit::load_cert;
std::string RsInit::load_key;
std::string RsInit::passwd;

bool RsInit::havePasswd; 		/* for Commandline password */
bool RsInit::autoLogin;  		/* autoLogin allowed */
bool RsInit::startMinimised; /* Icon or Full Window */

/* Win/Unix Differences */
char RsInit::dirSeperator;

/* Directories */
std::string RsInit::basedir;
std::string RsInit::homePath;

/* Listening Port */
bool RsInit::forceExtPort;
bool RsInit::forceLocalAddr;
unsigned short RsInit::port;
char RsInit::inet[256];

/* Logging */
bool RsInit::haveLogFile;
bool RsInit::outStderr;
bool RsInit::haveDebugLevel;
int  RsInit::debugLevel;
char RsInit::logfname[1024];

bool RsInit::firsttime_run;
bool RsInit::load_trustedpeer;
std::string RsInit::load_trustedpeer_file;

bool RsInit::udpListenerOnly;


/* Helper Functions */
//void load_check_basedir(RsInit *config);
//int  create_configinit(RsInit *config);

RsControl *createRsControl(RsIface &iface, NotifyBase &notify)
{
	RsServer *srv = new RsServer(iface, notify);
	return srv;
}

//void    CleanupRsConfig(RsInit *config)
//{
//	delete config;
//}

void RsInit::InitRsConfig()
{
	load_trustedpeer = false;
	firsttime_run = false;
	port = 7812; // default port.
	forceLocalAddr = false;
	haveLogFile    = false;
	outStderr      = false;
	forceExtPort   = false;

	strcpy(inet, "127.0.0.1");
	strcpy(logfname, "");

	autoLogin      = true; // Always on now.
	startMinimised = false;
	passwd         = "";
	havePasswd     = false;
	haveDebugLevel = false;
	debugLevel	= PQL_WARNING;
	udpListenerOnly = false;

#ifndef WINDOWS_SYS
	dirSeperator = '/'; // For unix.
#else
	dirSeperator = '\\'; // For windows.
#endif

	/* setup the homePath (default save location) */

	homePath = getHomePath();

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
}

const char *RsInit::RsConfigDirectory()
{
	return basedir.c_str();
}

bool	RsInit::setStartMinimised()
{
	return startMinimised;
}

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
int RsInit::InitRetroShare(int argc, char **argv)
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

   /* for static PThreads under windows... we need to init the library...
    */
   #ifdef PTW32_STATIC_LIB
      #include <pthread.h>
   #endif

int RsInit::InitRetroShare(int argcIgnored, char **argvIgnored)
{

  /* THIS IS A HACK TO ALLOW WINDOWS TO ACCEPT COMMANDLINE ARGUMENTS */

  const int MAX_ARGS = 32;
  int i,j;

  int argc;
  char *argv[MAX_ARGS];
  char *wholeline = (char*)GetCommandLine();
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
	/* getopt info: every availiable option is listet here. if it is followed by a ':' it
	   needs an argument. If it is followed by a '::' the argument is optional.
	*/
	while((c = getopt(argc, argv,"hesamui:p:c:w:l:d:")) != -1)
	{
		switch (c)
		{
			case 'a':
				autoLogin = true;
				startMinimised = true;
				std::cerr << "AutoLogin Allowed / Start Minimised On";
				std::cerr << std::endl;
				break;
			case 'm':
				startMinimised = true;
				std::cerr << "Start Minimised On";
				std::cerr << std::endl;
				break;
			case 'l':
				strncpy(logfname, optarg, 1024);
				std::cerr << "LogFile (" << logfname;
				std::cerr << ") Selected" << std::endl;
				haveLogFile = true;
				break;
			case 'w':
				passwd = optarg;
				std::cerr << "Password Specified(" << passwd;
				std::cerr << ") Selected" << std::endl;
				havePasswd = true;
				break;
			case 'i':
				strncpy(inet, optarg, 256);
				std::cerr << "New Inet Addr(" << inet;
				std::cerr << ") Selected" << std::endl;
				forceLocalAddr = true;
				break;
			case 'p':
				port = atoi(optarg);
				std::cerr << "New Listening Port(" << port;
				std::cerr << ") Selected" << std::endl;
				break;
			case 'c':
				basedir = optarg;
				std::cerr << "New Base Config Dir(";
				std::cerr << basedir;
				std::cerr << ") Selected" << std::endl;
				break;
			case 's':
				outStderr = true;
				haveLogFile = false;
				std::cerr << "Output to Stderr";
				std::cerr << std::endl;
				break;
			case 'd':
				haveDebugLevel = true;
				debugLevel = atoi(optarg);
				std::cerr << "Opt for new Debug Level";
				std::cerr << std::endl;
				break;
			case 'u':
				udpListenerOnly = true;
				std::cerr << "Opt for only udpListener";
				std::cerr << std::endl;
				break;
			case 'e':
				forceExtPort = true;
				std::cerr << "Opt for External Port Mode";
				std::cerr << std::endl;
				break;
			case 'h':
				std::cerr << "Help: " << std::endl;
				std::cerr << "The commandline options are for retroshare-nogui, a headless server in a shell, or systems without QT." << std::endl << std::endl;
				std::cerr << "-l [logfile]      Set the logfilename" << std::endl;
				std::cerr << "-w [password]     Set the password" << std::endl;
				std::cerr << "-i [ip_adress]    Set IP Adress to use" << std::endl;
				std::cerr << "-p [port]         Set the Port to listen on" << std::endl;
				std::cerr << "-c [basedir]      Set the config basdir" << std::endl;
				std::cerr << "-s                Output to Stderr" << std::endl;
				std::cerr << "-d [debuglevel]   Set the debuglevel" << std::endl;
				std::cerr << "-a                AutoLogin (Windows Only) + StartMinimised" << std::endl;
				std::cerr << "-m                StartMinimised" << std::endl;
				std::cerr << "-u                Only listen to UDP" << std::endl;
				std::cerr << "-e                Use a forwarded external Port" << std::endl << std::endl;
				std::cerr << "Example" << std::endl;
				std::cerr << "./retroshare-nogui -wmysecretpassword -e" << std::endl;
				exit(1);
				break;
			default:
				std::cerr << "Unknown Option!" << std::endl;
				std::cerr << "Use '-h' for help." << std::endl;
				exit(1);
		}
	}


	// set the default Debug Level...
	if (haveDebugLevel)
	{
		if ((debugLevel > 0) &&
			(debugLevel <= PQL_DEBUG_ALL))
		{
			std::cerr << "Setting Debug Level to: ";
			std::cerr << debugLevel;
			std::cerr << std::endl;
  			setOutputLevel(debugLevel);
		}
		else
		{
			std::cerr << "Ignoring Invalid Debug Level: ";
			std::cerr << debugLevel;
			std::cerr << std::endl;
		}
	}

	// set the debug file.
	if (haveLogFile)
	{
		setDebugFile(logfname);
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
	load_check_basedir();

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
	std::string userId;
	bool existingUser = false;

	/* do a null init to allow the SSL libray to startup! */
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (LoadCheckXPGPandGetName(load_cert.c_str(), userName, userId))
	{
		std::cerr << "Existing Name: " << userName << std::endl;
		std::cerr << "Existing Id: " << userId << std::endl;
		existingUser = true;
	}
	else
	{
		std::cerr << "No Existing User" << std::endl;
	}
#else /* X509 Certificates */
/**************** PQI_USE_SSLONLY ***************/
	/* Initial Certificate load will be X509 for SSL cases.
	 * in the OpenPGP case, this needs to be checked too.
	 */

	if (LoadCheckX509andGetName(load_cert.c_str(), userName, userId))
	{
		std::cerr << "X509 Existing Name: " << userName << std::endl;
		std::cerr << "Existing Id: " << userId << std::endl;
		existingUser = true;
	}
	else
	{
		std::cerr << "No Existing User" << std::endl;
	}

/**************** SSL + OPENPGP *****************/
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	getAuthMgr() -> InitAuth(NULL, NULL, NULL);

	/* if existing user, and havePasswd .... we can skip the login prompt */
	if (existingUser)
	{
		if (havePasswd)
		{
			return 1;
		}
		if (RsTryAutoLogin())
		{
			return 1;
		}
	}
	return 0;
}

#ifdef RS_USE_PGPSSL
int	RsInit::GetLogins(std::list<std::string> &pgpIds)
{
  #ifdef PQI_USE_XPGP
	return 0;
  #else
    #ifdef PQI_USE_SSLONLY
	return 0;
    #else  // PGP+SSL
	GPGAuthMgr *mgr = (GPGAuthMgr *) getAuthMgr();

	mgr->availablePGPCertificates(pgpIds);
	return 1;
    #endif
  #endif
}

int RsInit::GetLoginDetails(std::string id, std::string &name, std::string &email)
{
  #ifdef PQI_USE_XPGP
	return 0;
  #else
    #ifdef PQI_USE_SSLONLY
	return 0;
    #else  // PGP+SSL

	GPGAuthMgr *mgr = (GPGAuthMgr *) getAuthMgr();
	pqiAuthDetails details;
	if (!mgr->getDetails(id, details))
	{
		return 0;
	}

	name = details.name;
	email = details.email;

	return 1;
    #endif
  #endif
}


std::string RsInit::gpgPasswd;

bool RsInit::LoadGPGPassword(std::string id, std::string _passwd)
{
	bool ok = false;
	std::string gpgId = id;
	std::string name = id;
	gpgPasswd = _passwd;


	GPGAuthMgr *gpgAuthMgr = (GPGAuthMgr *) getAuthMgr();
	if (0 < gpgAuthMgr -> GPGInit(gpgId, name, gpgPasswd.c_str()))
	{
		ok = true;
		std::cerr << "PGP Auth Success!";
		std::cerr << "ID: " << id << " NAME: " << name;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "PGP Auth Failed!";
		std::cerr << "ID: " << id << " NAME: " << name;
		std::cerr << std::endl;
	}
	return ok;
}

#endif // RS_USE_PGPSSL





const std::string& RsServer::certificateFileName() { return RsInit::load_cert ; }
/*
 * The Real RetroShare Startup Function.
 */

int RsServer::StartupRetroShare()
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
	if (1 != mAuthMgr -> InitAuth(NULL, NULL, NULL))
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
	if ((!RsInit::haveLogFile) && (!RsInit::outStderr))
	{
		std::string crashfile = RsInit::basedir + RsInit::dirSeperator;
		crashfile += configLogFileName;
		setDebugCrashMode(crashfile.c_str());
	}

	unsigned long flags = 0;
	if (RsInit::udpListenerOnly)
	{
		flags |= PQIPERSON_NO_LISTENER;
	}

	/**************************************************************************/

	// Load up Certificates, and Old Configuration (if present)

	std::string certConfigFile = RsInit::basedir.c_str();
	std::string certNeighDir   = RsInit::basedir.c_str();
	std::string emergencySaveDir = RsInit::basedir.c_str();
	std::string emergencyPartialsDir = RsInit::basedir.c_str();
	if (certConfigFile != "")
	{
		certConfigFile += "/";
		certNeighDir += "/";
		emergencySaveDir += "/";
		emergencyPartialsDir += "/";
	}
	certConfigFile += configConfFile;
	certNeighDir +=   configCertDir;
	emergencySaveDir += "Incoming";
	emergencyPartialsDir += "Partials";

	/* if we've loaded an old format file! */
        bool oldFormat = false;
	std::map<std::string, std::string> oldConfigMap;

	mAuthMgr -> setConfigDirectories(certConfigFile, certNeighDir);

	mAuthMgr -> loadCertificates();

	/**************************************************************************/
	/* setup classes / structures */
	/**************************************************************************/

	/* Setup Notify Early - So we can use it. */
	rsNotify = new p3Notify();

	mConnMgr = new p3ConnectMgr(mAuthMgr);
	pqiNetAssistFirewall *mUpnpMgr = new upnphandler();
	p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr, RsInit::basedir);

	SecurityPolicy *none = secpolicy_create();
	pqih = new pqisslpersongrp(none, flags);
	//pqih = new pqipersongrpDummy(none, flags);

	/****** New Ft Server **** !!! */
        ftserver = new ftServer(mAuthMgr, mConnMgr);
        ftserver->setP3Interface(pqih);
	ftserver->setConfigDirectory(RsInit::basedir);

	ftserver->SetupFtServer(&(getNotify()));
	CacheStrapper *mCacheStrapper = ftserver->getCacheStrapper();
	CacheTransfer *mCacheTransfer = ftserver->getCacheTransfer();

        /* setup any extra bits (Default Paths) */
        ftserver->setPartialsDirectory(emergencyPartialsDir);
        ftserver->setDownloadDirectory(emergencySaveDir);

	/* This should be set by config ... there is no default */
        //ftserver->setSharedDirectories(fileList);
	rsFiles = ftserver;


	mConfigMgr = new p3ConfigMgr(mAuthMgr, RsInit::basedir, "rs-v0.4.cfg", "rs-v0.4.sgn");
	mGeneralConfig = new p3GeneralConfig();

	/* create Services */
	ad = new p3disc(mAuthMgr, mConnMgr);
	msgSrv = new p3MsgService(mConnMgr);
	chatSrv = new p3ChatService(mConnMgr);

	p3turtle *tr = new p3turtle(mConnMgr,ftserver) ;
	rsTurtle = tr ;
	pqih -> addService(tr);
	ftserver->connectToTurtleRouter(tr) ;

	pqih -> addService(ad);
	pqih -> addService(msgSrv);
	pqih -> addService(chatSrv);

	/* create Cache Services */
	std::string config_dir = RsInit::basedir;
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	std::string channelsdir = config_dir + "/channels";


	//mRanking = NULL;
	mRanking = new p3Ranking(mConnMgr, RS_SERVICE_TYPE_RANK,     /* declaration of cache enable service rank */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir, 3600 * 24 * 30 * 6); // 6 Months

        CachePair cp(mRanking, mRanking, CacheId(RS_SERVICE_TYPE_RANK, 0));
	mCacheStrapper -> addCachePair(cp);				/* end of declaration */

	p3Forums *mForums = new p3Forums(RS_SERVICE_TYPE_FORUM,
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir, mAuthMgr);

        CachePair cp4(mForums, mForums, CacheId(RS_SERVICE_TYPE_FORUM, 0));
	mCacheStrapper -> addCachePair(cp4);
	pqih -> addService(mForums);  /* This must be also ticked as a service */

	p3Channels *mChannels = new p3Channels(RS_SERVICE_TYPE_CHANNEL,
			mCacheStrapper, mCacheTransfer, rsFiles,
			localcachedir, remotecachedir, channelsdir, mAuthMgr);

        CachePair cp5(mChannels, mChannels, CacheId(RS_SERVICE_TYPE_CHANNEL, 0));
	mCacheStrapper -> addCachePair(cp5);
	pqih -> addService(mChannels);  /* This must be also ticked as a service */


#ifndef RS_RELEASE

	p3GameLauncher *gameLauncher = new p3GameLauncher(mConnMgr);
	pqih -> addService(gameLauncher);

	p3PhotoService *photoService = new p3PhotoService(RS_SERVICE_TYPE_PHOTO,   /* .... for photo service */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir);

        CachePair cp2(photoService, photoService, CacheId(RS_SERVICE_TYPE_PHOTO, 0));
	mCacheStrapper -> addCachePair(cp2);

	mQblog = new p3Qblog(mConnMgr, RS_SERVICE_TYPE_QBLOG, 			/* ...then for Qblog */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir, 3600 * 24 * 30 * 6); // 6 Months

	CachePair cp3(mQblog, mQblog, CacheId(RS_SERVICE_TYPE_QBLOG, 0));
	mCacheStrapper -> addCachePair(cp3);


#else
	mQblog = NULL;
#endif

	/**************************************************************************/

	mConnMgr->addNetAssistConnect(1, mDhtMgr);
	mConnMgr->addNetAssistFirewall(1, mUpnpMgr);

	/**************************************************************************/
	/* need to Monitor too! */

	mConnMgr->addMonitor(pqih);
	mConnMgr->addMonitor(mCacheStrapper);
	mConnMgr->addMonitor(ad);
	mConnMgr->addMonitor(msgSrv);

	/* must also add the controller as a Monitor...
	 * a little hack to get it to work.
	 */
	mConnMgr->addMonitor(((ftController *) mCacheTransfer));


	/**************************************************************************/

	//mConfigMgr->addConfiguration("ftserver.cfg", ftserver);
	//
	mConfigMgr->addConfiguration("peers.cfg", mConnMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("chat.cfg", chatSrv);
	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);

	mConfigMgr->addConfiguration("ranklink.cfg", mRanking);
	mConfigMgr->addConfiguration("forums.cfg", mForums);
	mConfigMgr->addConfiguration("channels.cfg", mChannels);
	mConfigMgr->addConfiguration("turtle.cfg", tr);

#ifndef RS_RELEASE
#else
#endif

	ftserver->addConfiguration(mConfigMgr);


	/**************************************************************************/


	/**************************************************************************/
	/* (2) Load configuration files */
	/**************************************************************************/

	mConfigMgr->loadConfiguration();

	/* NOTE: CacheStrapper's load causes Cache Files to be
	 * loaded into all the CacheStores/Sources. This happens
	 * after all the other configurations have happened.
	 */

	/**************************************************************************/
	/* Hack Old Configuration into new System (first load only) */
	/**************************************************************************/

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	/**************************************************************************/
	/* trigger generalConfig loading for classes that require it */
	/**************************************************************************/

	pqih->setConfig(mGeneralConfig);

        pqih->load_config();

	/**************************************************************************/
	/* Force Any Configuration before Startup (After Load) */
	/**************************************************************************/

	if (RsInit::forceLocalAddr)
	{
		struct sockaddr_in laddr;

		/* clean sockaddr before setting values (MaxOSX) */
		sockaddr_clear(&laddr);

		laddr.sin_family = AF_INET;
		laddr.sin_port = htons(RsInit::port);

		// universal
		laddr.sin_addr.s_addr = inet_addr(RsInit::inet);

		mConnMgr->setLocalAddress(ownId, laddr);
	}

	if (RsInit::forceExtPort)
	{
		mConnMgr->setOwnNetConfig(RS_NET_MODE_EXT, RS_VIS_STATE_STD);
	}

#if 0
	/* must load the trusted_peer before setting up the pqipersongrp */
	if (firsttime_run)
	{
		/* at this point we want to load and start the trusted peer -> if selected */
		if (load_trustedpeer)
		{
			/* sslroot does further checks */
        		sslr -> loadInitialTrustedPeer(load_trustedpeer_file);
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

        ftserver->StartupThreads();
	ftserver->ResumeTransfers();

	mDhtMgr->start();

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback(ownId);

	mod -> peerid = ownId;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

	rsPeers = new p3Peers(mConnMgr, mAuthMgr);
	rsMsgs  = new p3Msgs(mAuthMgr, msgSrv, chatSrv);
	rsDisc  = new p3Discovery(ad);

	rsForums = mForums;
	rsChannels = mChannels;
	rsRanks = new p3Rank(mRanking);

#ifndef RS_RELEASE
	rsGameLauncher = gameLauncher;
	rsPhoto = new p3Photo(photoService);
	rsStatus = new p3Status();
	rsQblog = new p3Blog(mQblog);

#else
	rsGameLauncher = NULL;
	rsPhoto = NULL;
	rsStatus = NULL;
	rsQblog = NULL;
#endif


	/* put a welcome message in! */
	if (RsInit::firsttime_run)
	{
		msgSrv->loadWelcomeMsg();
	}

	// load up the help page
	std::string helppage = RsInit::basedir + RsInit::dirSeperator;
	helppage += configHelpName;

	/* for DHT/UPnP stuff */
	//InitNetworking(basedir + "/kadc.ini");

	/* Startup this thread! */
        createThread(*this);


	return 1;
}



int RsInit::LoadCertificates(bool autoLoginNT)
{
	if (load_cert == "")
	{
	  std::cerr << "RetroShare needs a certificate" << std::endl;
	  return 0;
	}

	if (load_key == "")
	{
	  std::cerr << "RetroShare needs a key" << std::endl;
	  return 0;
	}

	if ((!havePasswd) || (passwd == ""))
	{
	  std::cerr << "RetroShare needs a Password" << std::endl;
	  return 0;
	}

	std::string ca_loc = basedir + dirSeperator;
	ca_loc += configCaFile;

	p3AuthMgr *authMgr = getAuthMgr();

	bool ok = false;

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (0 < authMgr -> InitAuth(load_cert.c_str(), load_key.c_str(),passwd.c_str()))
	{
		ok = true;
	}
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	/* The SSL / SSL + PGP version requires, SSL init + PGP init.  */
  /**************** PQI_USE_XPGP ******************/
  #if defined(PQI_USE_SSLONLY)
	if (0 < authMgr -> InitAuth(load_cert.c_str(), load_key.c_str(),passwd.c_str()))
	{
		ok = true;
	}
	else
	{
		std::cerr << "AuthSSL::InitAuth Failed" << std::endl;
	}

  #else /* X509 Certificates */
  /**************** PQI_USE_XPGP ******************/
	/* The SSL / SSL + PGP version requires, SSL init + PGP init.  */
	if (0 < authMgr -> InitAuth(load_cert.c_str(), load_key.c_str(),passwd.c_str()))
	{
		ok = true;
	}
	else
	{
		std::cerr << "SSL Auth Failed!";
		std::cerr << std::endl;
	}
  #endif /* X509 Certificates */
  /**************** PQI_USE_XPGP ******************/
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (ok)
	{
		if (autoLoginNT)
		{
			std::cerr << "RetroShare will AutoLogin next time";
			std::cerr << std::endl;

			RsStoreAutoLogin();
		}
		/* wipe password */
		passwd = "";
		create_configinit();
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
bool RsInit::ValidateCertificate(std::string &userName)
{
	std::string fname = load_cert;
	std::string userId;
	if (fname != "")
	{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
		return LoadCheckXPGPandGetName(fname.c_str(), userName, userId);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
		/* check against authmanagers private keys */
		return LoadCheckX509andGetName(fname.c_str(), userName, userId);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	}
	return false;
}

bool RsInit::ValidateTrustedUser(std::string fname, std::string &userName)
{
	std::string userId;
	bool valid = false;
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	valid = LoadCheckXPGPandGetName(fname.c_str(), userName, userId);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	valid = LoadCheckX509andGetName(fname.c_str(), userName, userId);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (valid)
	{
		load_trustedpeer = true;
		load_trustedpeer_file = fname;
	}
	else
	{
		load_trustedpeer = false;
	}
	return valid;
}

bool RsInit::LoadPassword(std::string _passwd)
{
	passwd = _passwd;
	havePasswd = true;
	return true;
}


/* A little nasty fn....
 * (1) returns true, if successful, and updates config.
 * (2) returns false if fails, with error msg to errString.
 */

bool RsInit::RsGenerateCertificate(
			std::string name,
			std::string org,
			std::string loc,
			std::string country,
			std::string password,
			std::string &errString)
{
	// In the XPGP world this is easy...
	// generate the private_key / certificate.
	// save to file.
	//
	// then load as if they had entered a passwd.

	// check password.
	if (password.length() < 4)
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
	std::string basename = basedir + dirSeperator;
	basename += configKeyDir + dirSeperator;
	basename += "user";

	std::string key_name = basename + "_pk.pem";
	std::string cert_name = basename + "_cert.pem";

	bool gen_ok = false;

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
        if (generate_xpgp(cert_name.c_str(), key_name.c_str(),
			password.c_str(),
			name.c_str(),
			"", //ui -> gen_email -> value(),
			org.c_str(),
			loc.c_str(),
			"", //ui -> gen_state -> value(),
			country.c_str(),
			nbits))
	{
		gen_ok = true;
	}

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
/**************** PQI_USE_XPGP ******************/
   #if defined(PQI_USE_SSLONLY)
	X509_REQ *req = GenerateX509Req(
			key_name.c_str(),
			password.c_str(),
			name.c_str(),
			"", //ui -> gen_email -> value(),
			org.c_str(),
			loc.c_str(),
			"", //ui -> gen_state -> value(),
			country.c_str(),
			nbits, errString);

	/* load private key */
	/* now convert to a self-signed certificate */
	EVP_PKEY *privkey = NULL;
	long days = 3000;

	gen_ok = true;
        /********** Test Loading the private Key.... ************/
        FILE *tst_in = NULL;
        if (NULL == (tst_in = fopen(key_name.c_str(), "rb")))
        {
                fprintf(stderr,"RsGenerateCert() Couldn't Open Private Key");
                fprintf(stderr," : %s\n", key_name.c_str());
                gen_ok = false;
        }

        if ((gen_ok) && (NULL == (privkey =
                PEM_read_PrivateKey(tst_in,NULL,NULL,(void *) password.c_str()))))
        {
                fprintf(stderr,"RsGenerateCert() Couldn't Read Private Key");
                fprintf(stderr," : %s\n", key_name.c_str());
		gen_ok = false;
        }


	X509 *cert = NULL;
	if (gen_ok)
	{
		cert = SignX509Certificate(X509_REQ_get_subject_name(req),
						privkey,req,days);

		/* Print the signed Certificate! */
       		BIO *bio_out = NULL;
        	bio_out = BIO_new(BIO_s_file());
        	BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

        	/* Print it out */
        	int nmflag = 0;
        	int reqflag = 0;

        	X509_print_ex(bio_out, cert, nmflag, reqflag);

        	BIO_flush(bio_out);
        	BIO_free(bio_out);

	}
	else
        {
                fprintf(stderr,"RsGenerateCert() Didn't Sign Certificate\n");
		gen_ok = false;
        }

	/* Save cert to file */
        // open the file.
        FILE *out = NULL;
        if (NULL == (out = fopen(cert_name.c_str(), "w")))
        {
                fprintf(stderr,"RsGenerateCert() Couldn't create Cert File");
                fprintf(stderr," : %s\n", cert_name.c_str());
                return 0;
        }

        if (!PEM_write_X509(out,cert))
        {
                fprintf(stderr,"RsGenerateCert() Couldn't Save Cert");
                fprintf(stderr," : %s\n", cert_name.c_str());
                return 0;
        }

	if (cert)
	{
		gen_ok = true;
	}

	X509_free(cert);
	X509_REQ_free(req);
        fclose(tst_in);
	fclose(out);
        EVP_PKEY_free(privkey);


  #else /* X509 Certificates */
  /**************** PQI_USE_XPGP ******************/


	/* Extra step required for SSL + PGP, user must have selected
	 * or generated a suitable key so the signing can happen.
	 */

	X509_REQ *req = GenerateX509Req(
			key_name.c_str(),
			password.c_str(),
			name.c_str(),
			"", //ui -> gen_email -> value(),
			org.c_str(),
			loc.c_str(),
			"", //ui -> gen_state -> value(),
			country.c_str(),
			nbits, errString);

	GPGAuthMgr *mgr = (GPGAuthMgr *) getAuthMgr();
	long days = 3000;
	X509 *x509 = mgr->SignX509Req(req, days, "dummypassword");

	X509_REQ_free(req);

	/* save to file */
	if (x509)
	{
		gen_ok = true;

		/* Print the signed Certificate! */
       		BIO *bio_out = NULL;
        	bio_out = BIO_new(BIO_s_file());
        	BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

        	/* Print it out */
        	int nmflag = 0;
        	int reqflag = 0;

        	X509_print_ex(bio_out, x509, nmflag, reqflag);

        	BIO_flush(bio_out);
        	BIO_free(bio_out);

	}
	else
        {
		gen_ok = false;
        }

	if (gen_ok)
	{
		/* Save cert to file */
        	// open the file.
        	FILE *out = NULL;
        	if (NULL == (out = fopen(cert_name.c_str(), "w")))
        	{
               		fprintf(stderr,"RsGenerateCert() Couldn't create Cert File");
                	fprintf(stderr," : %s\n", cert_name.c_str());
			gen_ok = false;
        	}

	        if (!PEM_write_X509(out,x509))
	        {
	                fprintf(stderr,"RsGenerateCert() Couldn't Save Cert");
	                fprintf(stderr," : %s\n", cert_name.c_str());
			gen_ok = false;
	        }

		fclose(out);
		X509_free(x509);
	}


  #endif /* X509 Certificates */
  /**************** PQI_USE_XPGP ******************/
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (!gen_ok)
	{
		errString = "Generation of Certificate Failed";
		return false;
	}

	/* set the load passwd to the gen version
	 * and try to load it!
	 */

	/* if we get here .... then save details to the configuration class */
	load_cert = cert_name;
	load_key  = key_name;
	passwd = password;
	havePasswd = true;
	firsttime_run = true;

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



void RsInit::load_check_basedir()
{
	// get the default configuration location.

	if (basedir == "")
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
		basedir = h;
		basedir += "/.pqiPGPrc";
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

			basedir="C:\\Retro";

		}
		else
		{
			basedir = h;
		}

		if (!RsDirUtil::checkCreateDirectory(basedir))
		{
			std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
			exit(1);
		}
		basedir += "\\RetroShare";
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}


	std::string subdir1 = basedir + dirSeperator;
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	std::string subdir3 = basedir + dirSeperator;
	subdir3 += "cache";

	std::string subdir4 = subdir3 + dirSeperator;
	std::string subdir5 = subdir3 + dirSeperator;
	subdir4 += "local";
	subdir5 += "remote";

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(basedir))
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
	std::string initfile = basedir + dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "r");
	char path[1024];
	int i;

	if (ifd != NULL)
	{
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++) {}
			path[i] = '\0';
			load_cert = path;
		}
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++) {}
			path[i] = '\0';
			load_key = path;
		}
		fclose(ifd);
	}

	// we have now
	// 1) checked or created the config dirs.
	// 2) loaded the config_init file - if possible.
	return;
}

int	RsInit::create_configinit()
{
	// Check for config file.
	std::string initfile = basedir + dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "w");

	if (ifd != NULL)
	{
		fprintf(ifd, "%s\n", load_cert.c_str());
		fprintf(ifd, "%s\n", load_key.c_str());
		fclose(ifd);

		std::cerr << "Creating Init File: " << initfile << std::endl;
		std::cerr << "\tLoad Cert: " << load_cert << std::endl;
		std::cerr << "\tLoad Key: " <<  load_key << std::endl;

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


std::string RsInit::getHomePath()
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
#include <wincrypt.h>
#include <iomanip>

/*
class CRYPTPROTECT_PROMPTSTRUCT;
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS_SYS
#ifndef WIN_CROSS_UBUNTU

typedef struct _CRYPTPROTECT_PROMPTSTRUCT {
  DWORD cbSize;
  DWORD dwPromptFlags;
  HWND hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT,
 *PCRYPTPROTECT_PROMPTSTRUCT;

#endif
#endif

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



bool  RsInit::RsStoreAutoLogin()
{
	std::cerr << "RsStoreAutoLogin()" << std::endl;
	/* Windows only */
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	/* store password encrypted in a file */
	std::string entropy = load_cert;

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;
	BYTE *pbDataInput = (BYTE *) strdup(passwd.c_str());
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
		std::string passwdfile = basedir;
		passwdfile += dirSeperator;
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



bool  RsInit::RsTryAutoLogin()
{
	std::cerr << "RsTryAutoLogin()" << std::endl;
	/* Windows only */
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	/* Require a AutoLogin flag in the config to do this */
	if (!autoLogin)
	{
		return false;
	}

	/* try to load from file */
	std::string entropy = load_cert;
	/* get the data out */

	/* open the data to the file */
	std::string passwdfile = basedir;
	passwdfile += dirSeperator;
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
		  passwd = (char *) DataOut.pbData;
		  havePasswd = true;
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

bool  RsInit::RsClearAutoLogin(std::string basedir)
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



