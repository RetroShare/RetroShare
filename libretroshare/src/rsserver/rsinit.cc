/*
 * libretroshare/src/reserver rsinit.cc
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

/* This is an updated startup class. Class variables are hidden from
 * the GUI / External via a hidden class */




#include <unistd.h>
#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "rsiface/rsinit.h"

#include <list>
#include <string>
#include <sstream>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// for blocking signals
#include <signal.h>

#include "pqi/authssl.h"
#include "pqi/authgpg.h"

class accountId
{
	public:
		std::string pgpId;
		std::string pgpName;
		std::string pgpEmail;

		std::string sslId;
                std::string location;
};


class RsInitConfig 
{
	public:
                /* OS Specifics */
                static char dirSeperator;

                /* Directories (SetupBaseDir) */
                static std::string basedir;
                static std::string homePath;

		static std::list<accountId> accountIds;
		static std::string preferedId;

		/* for certificate creation */
                //static std::string gpgPasswd;

		/* These fields are needed for login */
                static std::string loginId;
                static std::string configDir;
                static std::string load_cert;
                static std::string load_key;
		static std::string ssl_passphrase_file;

		static std::string passwd;

                static bool havePasswd;                 /* for Commandline password */
                static bool autoLogin;                  /* autoLogin allowed */
                static bool startMinimised; 		/* Icon or Full Window */

                /* Key Parameters that must be set before
                 * RetroShare will start up:
                 */

                /* Listening Port */
                static bool forceExtPort;
                static bool forceLocalAddr;
                static unsigned short port;
                static char inet[256];

                /* Logging */
                static bool haveLogFile;
                static bool outStderr;
                static bool haveDebugLevel;
                static int  debugLevel;
                static char logfname[1024];

                static bool firsttime_run;
                static bool load_trustedpeer;
                static std::string load_trustedpeer_file;

                static bool udpListenerOnly;
};


const int p3facestartupzone = 47238;

// initial configuration bootstrapping...
static const std::string configInitFile = "default_cert.txt";
static const std::string configConfFile = "config.rs";
static const std::string configCertDir = "friends";
static const std::string configKeyDir = "keys";
static const std::string configCaFile = "cacerts.pem";
static const std::string configLogFileName = "retro.log";
static const std::string configHelpName = "retro.htm";

std::list<accountId> RsInitConfig::accountIds;
std::string RsInitConfig::preferedId;

std::string RsInitConfig::configDir;
std::string RsInitConfig::load_cert;
std::string RsInitConfig::load_key;
std::string RsInitConfig::ssl_passphrase_file;

std::string RsInitConfig::passwd;
//std::string RsInitConfig::gpgPasswd;

bool RsInitConfig::havePasswd; 		/* for Commandline password */
bool RsInitConfig::autoLogin;  		/* autoLogin allowed */
bool RsInitConfig::startMinimised; /* Icon or Full Window */

/* Win/Unix Differences */
char RsInitConfig::dirSeperator;

/* Directories */
std::string RsInitConfig::basedir;
std::string RsInitConfig::homePath;

/* Listening Port */
bool RsInitConfig::forceExtPort;
bool RsInitConfig::forceLocalAddr;
unsigned short RsInitConfig::port;
char RsInitConfig::inet[256];

/* Logging */
bool RsInitConfig::haveLogFile;
bool RsInitConfig::outStderr;
bool RsInitConfig::haveDebugLevel;
int  RsInitConfig::debugLevel;
char RsInitConfig::logfname[1024];

bool RsInitConfig::firsttime_run;
bool RsInitConfig::load_trustedpeer;
std::string RsInitConfig::load_trustedpeer_file;

bool RsInitConfig::udpListenerOnly;


/* Uses private class - so must be hidden */
static bool getAvailableAccounts(std::list<accountId> &ids);
static bool checkAccount(std::string accountdir, accountId &id);


void RsInit::InitRsConfig()
{
#ifndef WINDOWS_SYS
	RsInitConfig::dirSeperator = '/'; // For unix.
#else
	RsInitConfig::dirSeperator = '\\'; // For windows.
#endif


	RsInitConfig::load_trustedpeer = false;
	RsInitConfig::firsttime_run = false;
	RsInitConfig::port = 7812; // default port.
	RsInitConfig::forceLocalAddr = false;
	RsInitConfig::haveLogFile    = false;
	RsInitConfig::outStderr      = false;
	RsInitConfig::forceExtPort   = false;

	strcpy(RsInitConfig::inet, "127.0.0.1");
	strcpy(RsInitConfig::logfname, "");

	RsInitConfig::autoLogin      = true; // Always on now.
	RsInitConfig::startMinimised = false;
	RsInitConfig::passwd         = "";
	RsInitConfig::havePasswd     = false;
	RsInitConfig::haveDebugLevel = false;
	RsInitConfig::debugLevel	= PQL_WARNING;
	RsInitConfig::udpListenerOnly = false;

	RsInitConfig::/* setup the homePath (default save location) */

	RsInitConfig::homePath = getHomePath();


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
				RsInitConfig::autoLogin = true;
				RsInitConfig::startMinimised = true;
				std::cerr << "AutoLogin Allowed / Start Minimised On";
				std::cerr << std::endl;
				break;
			case 'm':
				RsInitConfig::startMinimised = true;
				std::cerr << "Start Minimised On";
				std::cerr << std::endl;
				break;
			case 'l':
				strncpy(RsInitConfig::logfname, optarg, 1024);
				std::cerr << "LogFile (" << RsInitConfig::logfname;
				std::cerr << ") Selected" << std::endl;
				RsInitConfig::haveLogFile = true;
				break;
			case 'w':
				RsInitConfig::passwd = optarg;
				std::cerr << "Password Specified(" << RsInitConfig::passwd;
				std::cerr << ") Selected" << std::endl;
				RsInitConfig::havePasswd = true;
				break;
			case 'i':
				strncpy(RsInitConfig::inet, optarg, 256);
				std::cerr << "New Inet Addr(" << RsInitConfig::inet;
				std::cerr << ") Selected" << std::endl;
				RsInitConfig::forceLocalAddr = true;
				break;
			case 'p':
				RsInitConfig::port = atoi(optarg);
				std::cerr << "New Listening Port(" << RsInitConfig::port;
				std::cerr << ") Selected" << std::endl;
				break;
			case 'c':
				RsInitConfig::basedir = optarg;
				std::cerr << "New Base Config Dir(";
				std::cerr << RsInitConfig::basedir;
				std::cerr << ") Selected" << std::endl;
				break;
			case 's':
				RsInitConfig::outStderr = true;
				RsInitConfig::haveLogFile = false;
				std::cerr << "Output to Stderr";
				std::cerr << std::endl;
				break;
			case 'd':
				RsInitConfig::haveDebugLevel = true;
				RsInitConfig::debugLevel = atoi(optarg);
				std::cerr << "Opt for new Debug Level";
				std::cerr << std::endl;
				break;
			case 'u':
				RsInitConfig::udpListenerOnly = true;
				std::cerr << "Opt for only udpListener";
				std::cerr << std::endl;
				break;
			case 'e':
				RsInitConfig::forceExtPort = true;
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
	if (RsInitConfig::haveDebugLevel)
	{
		if ((RsInitConfig::debugLevel > 0) &&
			(RsInitConfig::debugLevel <= PQL_DEBUG_ALL))
		{
			std::cerr << "Setting Debug Level to: ";
			std::cerr << RsInitConfig::debugLevel;
			std::cerr << std::endl;
  			setOutputLevel(RsInitConfig::debugLevel);
		}
		else
		{
			std::cerr << "Ignoring Invalid Debug Level: ";
			std::cerr << RsInitConfig::debugLevel;
			std::cerr << std::endl;
		}
	}

	// set the debug file.
	if (RsInitConfig::haveLogFile)
	{
		setDebugFile(RsInitConfig::logfname);
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
	// SWITCH off the SIGPIPE - kills process on Linux.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	struct sigaction sigact;
	sigact.sa_handler = SIG_IGN;
	sigact.sa_flags = 0;

	sigset_t set;
	sigemptyset(&set);
	//sigaddset(&set, SIGINT); // or whatever other signal
	sigact.sa_mask = set;

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


	/* At this point we want to.
	 * 1) Load up Dase Directory.
	 * 3) Get Prefered Id.
	 * 2) Get List of Available Accounts.
	 * 4) Get List of GPG Accounts.
	 */
        AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL);

	// first check config directories, and set bootstrap values.
	setupBaseDir();
	get_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);
	//std::list<accountId> ids;
	std::list<accountId>::iterator it;
	getAvailableAccounts(RsInitConfig::accountIds);

	/* check that preferedId */
	std::string userName;
	std::string userId;
	bool existingUser = false;
	for(it = RsInitConfig::accountIds.begin(); it != RsInitConfig::accountIds.end(); it++)
	{
		std::cerr << "Checking Account Id: " << it->sslId << std::endl;
		if (RsInitConfig::preferedId == it->sslId)
		{
			std::cerr << " * Preferred * " << std::endl;
			userId = it->sslId;
                        userName = it->pgpName;
			existingUser = true;
		}
	}
	if (!existingUser)
	{
		std::cerr << "No Existing User" << std::endl;
		RsInitConfig::preferedId == "";
	}


	/* if existing user, and havePasswd .... we can skip the login prompt */
	if (existingUser)
	{
		if (RsInitConfig::havePasswd)
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

/**************************** Access Functions for Init Data **************************/

bool     RsInit::getPreferedAccountId(std::string &id)
{
	id = RsInitConfig::preferedId;
	return (RsInitConfig::preferedId != "");
}

bool     RsInit::getAccountIds(std::list<std::string> &ids)
{
	std::list<accountId>::iterator it;
	std::cerr << "getAccountIds:" << std::endl;

	for(it = RsInitConfig::accountIds.begin(); it != RsInitConfig::accountIds.end(); it++)
	{
		std::cerr << "SSL Id: " << it->sslId << " PGP Id " << it->pgpId <<
		std::cerr << " PGP Name: " << it->pgpName;
		std::cerr << " PGP Email: " << it->pgpEmail;
                std::cerr << " Location: " << it->location;
		std::cerr << std::endl;

		ids.push_back(it->sslId);
	}
	return true;
}


bool     RsInit::getAccountDetails(std::string id, 
                                std::string &gpgId, std::string &gpgName, 
                                std::string &gpgEmail, std::string &location)
{
	std::list<accountId>::iterator it;
	for(it = RsInitConfig::accountIds.begin(); it != RsInitConfig::accountIds.end(); it++)
	{
		if (id == it->sslId)
		{
			gpgId = it->pgpId;
			gpgName = it->pgpName;
			gpgEmail = it->pgpEmail;
                        location = it->location;
			return true;
		}
	}
	return false;
}

/**************************** Access Functions for Init Data **************************/
/**************************** Private Functions for InitRetroshare ********************/
/**************************** Private Functions for InitRetroshare ********************/


void RsInit::setupBaseDir()
{
	// get the default configuration location.

	if (RsInitConfig::basedir == "")
	{
		// v0.4.x if unix. homedir + /.pqiPGPrc
		// v0.5.x if unix. homedir + /.retroshare

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
		RsInitConfig::basedir = h;
		RsInitConfig::basedir += "/.retroshare";
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

			RsInitConfig::basedir="C:\\Retro";

		}
		else
		{
			RsInitConfig::basedir = h;
		}

		if (!RsDirUtil::checkCreateDirectory(RsInitConfig::basedir))
		{
			std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
			exit(1);
		}
		RsInitConfig::basedir += "\\RetroShare";
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}

	// fatal if cannot find/create.
	std::cerr << "Creating Root Retroshare Config Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(RsInitConfig::basedir))
	{
		std::cerr << "Cannot Create BaseConfig Dir:" << RsInitConfig::basedir << std::endl;
		exit(1);
	}
}


/* directories with valid certificates in the expected location */
bool getAvailableAccounts(std::list<accountId> &ids)
{
	/* get the directories */
	std::list<std::string> directories;
	std::list<std::string>::iterator it;

	std::cerr << "getAvailableAccounts()";
	std::cerr << std::endl;

                /* now iterate through the directory...
                 * directories - flags as old,
                 * files checked to see if they have changed. (rehashed)
                 */

        struct dirent *dent;
        struct stat buf;
	
        /* check for the dir existance */
        DIR *dir = opendir(RsInitConfig::basedir.c_str());
	if (!dir)
	{
		std::cerr << "Cannot Open Base Dir - No Available Accounts" << std::endl;
		exit(1);
	}

        while(NULL != (dent = readdir(dir)))
        {
		/* check entry type */
		std::string fname = dent -> d_name;
		std::string fullname = RsInitConfig::basedir + "/" + fname;
		
		if (-1 != stat(fullname.c_str(), &buf))
		{
			#ifdef FIM_DEBUG
			std::cerr << "buf.st_mode: " << buf.st_mode <<std::endl;
			#endif
			if (S_ISDIR(buf.st_mode))
			{
				if ((fname == ".") || (fname == ".."))
				{
#ifdef FIM_DEBUG
					std::cerr << "Skipping:" << fname << std::endl;
#endif
					continue; /* skipping links */
				}
		
#ifdef FIM_DEBUG
			std::cerr << "Is Directory: " << fullname << std::endl;
#endif

			/* */
			directories.push_back(fname);

			}
		}	
	}

	for(it = directories.begin(); it != directories.end(); it++)
	{
		std::string accountdir = RsInitConfig::basedir + RsInitConfig::dirSeperator + *it;
		std::cerr << "getAvailableAccounts() Checking: " << *it;
		std::cerr << std::endl;

		accountId tmpId;
		if (checkAccount(accountdir, tmpId))
		{
			std::cerr << "getAvailableAccounts() Accepted: " << *it;
			std::cerr << std::endl;
			ids.push_back(tmpId);
		}
	}
	return true;
}



static bool checkAccount(std::string accountdir, accountId &id)
{
	/* check if the cert/key file exists */

	std::string subdir1 = accountdir + RsInitConfig::dirSeperator;
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	// Create the filename.
	std::string basename = accountdir + RsInitConfig::dirSeperator;
	basename += configKeyDir + RsInitConfig::dirSeperator;
	basename += "user";

	std::string cert_name = basename + "_cert.pem";
	std::string userName, userId;

	std::cerr << "checkAccount() dir: " << accountdir << std::endl;

	bool ret = false;

	/* check against authmanagers private keys */
        LoadCheckX509andGetLocation(cert_name.c_str(), id.location, id.sslId);
        std::cerr << "location: " << id.location << " id: " << id.sslId << std::endl;

		std::string tmpid;
		if (LoadCheckX509andGetIssuerName(cert_name.c_str(), id.pgpId, tmpid))
		{
			std::cerr << "issuerName: " << id.pgpId << " id: " << tmpid << std::endl;
			RsInit::GetPGPLoginDetails(id.pgpId, id.pgpName, id.pgpEmail);
			std::cerr << "PGPLoginDetails: " << id.pgpId << " name: " << id.pgpName;
			std::cerr << " email: " << id.pgpEmail << std::endl;
			ret = true;
		}
		else
		{
			std::cerr << "GetIssuerName FAILED!" << std::endl;
			ret = false;
		}

	return ret;
}




/*****************************************************************************/
/*****************************************************************************/
/************************* Generating Certificates ***************************/
/*****************************************************************************/
/*****************************************************************************/


                /* Generating GPGme Account */
int      RsInit::GetPGPLogins(std::list<std::string> &pgpIds) {
        AuthGPG::getAuthGPG()->availableGPGCertificatesWithPrivateKeys(pgpIds);
	return 1;
}

int      RsInit::GetPGPLoginDetails(std::string id, std::string &name, std::string &email)
{
        std::cerr << "RsInit::GetPGPLoginDetails for \"" << id << "\"";
        std::cerr << std::endl;

        name = AuthGPG::getAuthGPG()->getGPGName(id);
        email = AuthGPG::getAuthGPG()->getGPGEmail(id);
        if (name != "") {
            return 1;
        } else {
            return 0;
        }
}

/* Before any SSL stuff can be loaded, the correct PGP must be selected / generated:
 **/

bool RsInit::SelectGPGAccount(std::string id)
{
	bool ok = false;
	std::string gpgId = id;
	std::string name = id;

        if (0 < AuthGPG::getAuthGPG() -> GPGInit(gpgId))
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


//bool RsInit::LoadGPGPassword(std::string inPGPpasswd)
//{
//
//	bool ok = false;
//        if (0 < AuthGPG::getAuthGPG() -> LoadGPGPassword(inPGPpasswd))
//	{
//		ok = true;
//		std::cerr << "PGP LoadPwd Success!";
//		std::cerr << std::endl;
//	}
//	else
//	{
//		std::cerr << "PGP LoadPwd Failed!";
//		std::cerr << std::endl;
//	}
//	return ok;
//}


bool     RsInit::GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString) {
        return AuthGPG::getAuthGPG()->GeneratePGPCertificate(name, email, passwd, pgpId, errString);
}


                /* Create SSL Certificates */
bool     RsInit::GenerateSSLCertificate(std::string gpg_id, std::string org, std::string loc, std::string country, std::string passwd, std::string &sslId, std::string &errString)
{
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

	int nbits = 2048;

        std::string name = AuthGPG::getAuthGPG()->getGPGName(gpg_id);

	// Create the filename .....
	// Temporary Directory for creating files....
	std::string tmpdir = "TMPCFG";

	std::string tmpbase = RsInitConfig::basedir + RsInitConfig::dirSeperator + tmpdir + RsInitConfig::dirSeperator;
	RsInit::setupAccount(tmpbase);

	/* create directory structure */

	std::string basename = tmpbase + configKeyDir + RsInitConfig::dirSeperator;
	basename += "user";

	std::string key_name = basename + "_pk.pem";
	std::string cert_name = basename + "_cert.pem";

	bool gen_ok = false;

	/* Extra step required for SSL + PGP, user must have selected
	 * or generated a suitable key so the signing can happen.
	 */

	X509_REQ *req = GenerateX509Req(
			key_name.c_str(),
			passwd.c_str(),
			name.c_str(),
			"", //ui -> gen_email -> value(),
			org.c_str(),
			loc.c_str(),
			"", //ui -> gen_state -> value(),
			country.c_str(),
			nbits, errString);

	long days = 3000;
        X509 *x509 = AuthSSL::getAuthSSL()->SignX509Req(req, days);

	X509_REQ_free(req);
	if (x509 == NULL) {
		fprintf(stderr,"RsGenerateCert() Couldn't sign ssl certificate. Probably PGP password is wrong.\n");
		return false;
	}

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

	if (!gen_ok)
	{
		errString = "Generation of Certificate Failed";
		return false;
	}

	/* try to load it, and get Id */

        std::string location;
        if (LoadCheckX509andGetLocation(cert_name.c_str(), location, sslId) == 0) {
            std::cerr << "RsInit::GenerateSSLCertificate() Cannot check own signature, maybe the files are corrupted." << std::endl;
            return false;
        }

        /* Move directory to correct id */
	std::string finalbase = RsInitConfig::basedir + RsInitConfig::dirSeperator + sslId + RsInitConfig::dirSeperator;
	/* Rename Directory */

	std::cerr << "Mv Config Dir from: " << tmpbase << " to: " << finalbase;
	std::cerr << std::endl;

	if (0 > rename(tmpbase.c_str(), finalbase.c_str()))
	{
		std::cerr << "rename FAILED" << std::endl;
	}


	/* Flag as first time run */
	RsInitConfig::firsttime_run = true;

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


/******************* PRIVATE FNS TO HELP with GEN **************/
bool RsInit::setupAccount(std::string accountdir)
{
	/* actual config directory isd */

	std::string subdir1 = accountdir + RsInitConfig::dirSeperator;
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	std::string subdir3 = accountdir + RsInitConfig::dirSeperator;
	subdir3 += "cache";

	std::string subdir4 = subdir3 + RsInitConfig::dirSeperator;
	std::string subdir5 = subdir3 + RsInitConfig::dirSeperator;
	subdir4 += "local";
	subdir5 += "remote";

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(accountdir))
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

	return true;
}






/***************************** FINAL LOADING OF SETUP *************************/
                /* Login SSL */
bool     RsInit::LoadPassword(std::string id, std::string inPwd)
{
	/* select configDir */

	RsInitConfig::preferedId = id;
	RsInitConfig::configDir = RsInitConfig::basedir + RsInitConfig::dirSeperator + id;
	RsInitConfig::passwd = inPwd;
	RsInitConfig::havePasswd = true;

        // Create the filename.
        std::string basename = RsInitConfig::configDir + RsInitConfig::dirSeperator;
        basename += configKeyDir + RsInitConfig::dirSeperator;
	RsInitConfig::ssl_passphrase_file  = basename + "ssl_passphrase.pgp";
	basename += "user";

        RsInitConfig::load_key  = basename + "_pk.pem";
        RsInitConfig::load_cert = basename + "_cert.pem";

	return true;
}



/***************************** FINAL LOADING OF SETUP *************************
 * Requires:
 *     PGPid to be selected (Password not required).
 *     CertId to be selected (Password Required).
 */

int RsInit::LoadCertificates(bool autoLoginNT)
{
	if (RsInitConfig::load_cert == "")
	{
	  std::cerr << "RetroShare needs a certificate" << std::endl;
	  return 0;
	}

	if (RsInitConfig::load_key == "")
	{
	  std::cerr << "RetroShare needs a key" << std::endl;
	  return 0;
	}

	bool ok = false;

	/* The SSL / SSL + PGP version requires, SSL init + PGP init.  */
	const char* sslPassword;
	sslPassword = RsInitConfig::passwd.c_str();
	//check if password is already in memory
	if ((RsInitConfig::havePasswd) && (RsInitConfig::passwd != ""))
	{
		std::cerr << "RetroShare have a ssl Password" << std::endl;
		sslPassword = RsInitConfig::passwd.c_str();

		std::cerr << "let's store the ssl Password into a pgp ecrypted file" << std::endl;
		FILE *sslPassphraseFile = fopen(RsInitConfig::ssl_passphrase_file.c_str(), "w");
                std::cerr << "opening sslPassphraseFile : " << RsInitConfig::ssl_passphrase_file.c_str() << std::endl;
		gpgme_data_t cipher;
		gpgme_data_t plain;
                gpgme_data_new_from_mem(&plain, sslPassword, strlen(sslPassword), 1);
                gpgme_data_new_from_stream (&cipher, sslPassphraseFile);
                if (0 < AuthGPG::getAuthGPG()->encryptText(plain, cipher)) {
		    std::cerr << "Encrypting went ok !" << std::endl;
                } else {
                    std::cerr << "Encrypting went wrong !" << std::endl;
                }
                gpgme_data_release (cipher);
		gpgme_data_release (plain);
		fclose(sslPassphraseFile);

	} else {
		//let's read the password from an encrypted file
		//let's check if there's a ssl_passpharese_file that we can decrypt with PGP
		FILE *sslPassphraseFile = fopen(RsInitConfig::ssl_passphrase_file.c_str(), "r");
		if (sslPassphraseFile == NULL)
		{
                        std::cerr << "No password povided, and no sslPassphraseFile : " << RsInitConfig::ssl_passphrase_file.c_str() << std::endl;
			return 0;
		} else {
                        std::cerr << "opening sslPassphraseFile : " << RsInitConfig::ssl_passphrase_file.c_str() << std::endl;
			gpgme_data_t cipher;
			gpgme_data_t plain;
			gpgme_data_new (&plain);
			gpgme_error_t error_reading_file = gpgme_data_new_from_stream (&cipher, sslPassphraseFile);
                        if (0 < AuthGPG::getAuthGPG()->decryptText(cipher, plain)) {
			    std::cerr << "Decrypting went ok !" << std::endl;
                            gpgme_data_write (plain, "", 1);
			    sslPassword = gpgme_data_release_and_get_mem(plain, NULL);
			} else {
			    gpgme_data_release (plain);
                            std::cerr << "Error : decrypting went wrong !" << std::endl;
			    return 0;
			}
			gpgme_data_release (cipher);
			fclose(sslPassphraseFile);
		}
	}

	std::cerr << "RsInitConfig::load_key.c_str() : " << RsInitConfig::load_key.c_str() << std::endl;
        std::cerr << "sslPassword : " << sslPassword << std::endl;;
        if (0 < AuthSSL::getAuthSSL() -> InitAuth(RsInitConfig::load_cert.c_str(), RsInitConfig::load_key.c_str(), sslPassword))
	{
		ok = true;
	}
	else
	{
		std::cerr << "SSL Auth Failed!";
		std::cerr << std::endl;
	}

	if (ok)
	{
		if (autoLoginNT)
		{
			std::cerr << "RetroShare will AutoLogin next time";
			std::cerr << std::endl;

			RsStoreAutoLogin();
		}
		/* wipe password */
		RsInitConfig::passwd = "";
		create_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);
		return 1;
	}

	std::cerr << "RetroShare Failed To Start!" << std::endl;
	std::cerr << "Please Check File Names/Password" << std::endl;

	return 0;
}

bool	RsInit::get_configinit(std::string dir, std::string &id)
{
	// have a config directories.

	// Check for config file.
	std::string initfile = dir + RsInitConfig::dirSeperator;
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
			id = path;
		}
		fclose(ifd);
		return true;
	}

	// we have now
	// 1) checked or created the config dirs.
	// 2) loaded the config_init file - if possible.
	return false;
}


bool	RsInit::create_configinit(std::string dir, std::string id)
{
	// Check for config file.
	std::string initfile = dir + RsInitConfig::dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "w");

	if (ifd != NULL)
	{
		fprintf(ifd, "%s\n", id.c_str());
		fclose(ifd);

		std::cerr << "Creating Init File: " << initfile << std::endl;
		std::cerr << "\tId: " << id << std::endl;

		return true;
	}
	std::cerr << "Failed To Create Init File: " << initfile << std::endl;
	return false;
}

std::string make_path_unix(std::string path);

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
	std::cerr << out.str();

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
#if defined(__CYGWIN__)

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
	std::string entropy = RsInitConfig::load_cert;

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;
	BYTE *pbDataInput = (BYTE *) strdup(RsInitConfig::passwd.c_str());
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
		std::string passwdfile = RsInitConfig::configDir;
		passwdfile += RsInitConfig::dirSeperator;
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
	if (!RsInitConfig::autoLogin)
	{
		return false;
	}

	/* try to load from file */
	std::string entropy = RsInitConfig::load_cert;
	/* get the data out */

	/* open the data to the file */
	std::string passwdfile = RsInitConfig::configDir;
	passwdfile += RsInitConfig::dirSeperator;
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
		  RsInitConfig::passwd = (char *) DataOut.pbData;
		  RsInitConfig::havePasswd = true;
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

bool  RsInit::RsClearAutoLogin()
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
	return false;
#else
	std::string passwdfile = RsInitConfig::configDir;
	passwdfile += RsInitConfig::dirSeperator;
	passwdfile += "help.dta";

	FILE *fp = fopen(passwdfile.c_str(), "wb");
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
















std::string RsInit::RsConfigDirectory()
{
	return RsInitConfig::basedir;
}

std::string RsInit::RsProfileConfigDirectory()
{
    std::string dir = RsInitConfig::basedir + RsInitConfig::dirSeperator + RsInitConfig::preferedId;
    std::cerr << "RsInit::RsProfileConfigDirectory() returning : " << dir << std::endl;
    return dir;
}

bool	RsInit::setStartMinimised()
{
	return RsInitConfig::startMinimised;
}


/*
 *
 * Init Part of RsServer...  needs the private
 * variables so in the same file.
 *
 */

#include <unistd.h>
//#include <getopt.h>

#include "dbase/cachestrapper.h"
#include "ft/ftserver.h"
#include "ft/ftcontroller.h"

#include "rsiface/rsiface.h"
#include "rsiface/rsturtle.h"

/* global variable now points straight to 
 * ft/ code so variable defined here.
 */

RsControl *rsicontrol = NULL;
RsFiles *rsFiles = NULL;
RsTurtle *rsTurtle = NULL ;

#include "pqi/pqipersongrp.h"
#include "pqi/pqisslpersongrp.h"
#include "pqi/pqiloopback.h"
#include "pqi/p3cfgmgr.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"

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
#include "services/p3blogs.h"
#include "turtle/p3turtle.h"
#include "services/p3tunnel.h"

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

#include "pqi/p3notify.h" // HACK - moved to pqi for compilation order.


/****
#define RS_RELEASE 1
****/

#define RS_RELEASE 1


RsControl *createRsControl(RsIface &iface, NotifyBase &notify)
{
	RsServer *srv = new RsServer(iface, notify);
	rsicontrol = srv;
	return srv;
}

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

        if (1 != AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL))
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Invalid Certificate configuration!" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

        std::string ownId = AuthSSL::getAuthSSL()->OwnId();

	/**************************************************************************/
	/* Any Initial Configuration (Commandline Options)  */
	/**************************************************************************/

	/* set the debugging to crashMode */
        std::cerr << "set the debugging to crashMode." << std::endl;
        if ((!RsInitConfig::haveLogFile) && (!RsInitConfig::outStderr))
	{
		std::string crashfile = RsInitConfig::basedir + RsInitConfig::dirSeperator;
		crashfile += configLogFileName;
		setDebugCrashMode(crashfile.c_str());
	}

	unsigned long flags = 0;
	if (RsInitConfig::udpListenerOnly)
	{
		flags |= PQIPERSON_NO_LISTENER;
	}

	/**************************************************************************/
	// Load up Certificates, and Old Configuration (if present)
        std::cerr << "Load up Certificates, and Old Configuration (if present)." << std::endl;

	std::string certConfigFile = RsInitConfig::configDir.c_str();
	std::string certNeighDir   = RsInitConfig::configDir.c_str();
	std::string emergencySaveDir = RsInitConfig::configDir.c_str();
	std::string emergencyPartialsDir = RsInitConfig::configDir.c_str();
	if (certConfigFile != "")
	{
		certConfigFile += "/";
		certNeighDir += "/";
		emergencySaveDir += "/";
		emergencyPartialsDir += "/";
	}
	certConfigFile += configConfFile;
	certNeighDir +=   configCertDir;
	emergencySaveDir += "Downloads";
	emergencyPartialsDir += "Partials";

	/* if we've loaded an old format file! */
        bool oldFormat = false;
	std::map<std::string, std::string> oldConfigMap;

        AuthSSL::getAuthSSL() -> setConfigDirectories(certConfigFile, certNeighDir);

        //AuthSSL::getAuthSSL() -> loadCertificates();

	/**************************************************************************/
	/* setup classes / structures */
	/**************************************************************************/
        std::cerr << "setup classes / structures" << std::endl;

	/* Setup Notify Early - So we can use it. */
	rsNotify = new p3Notify();

        mConnMgr = new p3ConnectMgr();
        AuthSSL::getAuthSSL()->mConnMgr = mConnMgr;
        //load all the SSL certs as friends
//        std::list<std::string> sslIds;
//        AuthSSL::getAuthSSL()->getAuthenticatedList(sslIds);
//        for (std::list<std::string>::iterator sslIdsIt = sslIds.begin(); sslIdsIt != sslIds.end(); sslIdsIt++) {
//            mConnMgr->addFriend(*sslIdsIt);
//        }
	pqiNetAssistFirewall *mUpnpMgr = new upnphandler();
        //p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr, RsInitConfig::configDir);

	SecurityPolicy *none = secpolicy_create();
	pqih = new pqisslpersongrp(none, flags);
	//pqih = new pqipersongrpDummy(none, flags);

	/****** New Ft Server **** !!! */
        ftserver = new ftServer(mConnMgr);
        ftserver->setP3Interface(pqih); 
	ftserver->setConfigDirectory(RsInitConfig::configDir);

	ftserver->SetupFtServer(&(getNotify()));
	CacheStrapper *mCacheStrapper = ftserver->getCacheStrapper();
	CacheTransfer *mCacheTransfer = ftserver->getCacheTransfer();

        /* setup any extra bits (Default Paths) */
        ftserver->setPartialsDirectory(emergencyPartialsDir);
        ftserver->setDownloadDirectory(emergencySaveDir);

	/* This should be set by config ... there is no default */
        //ftserver->setSharedDirectories(fileList);
	rsFiles = ftserver;


        mConfigMgr = new p3ConfigMgr(RsInitConfig::configDir, "rs-v0.5.cfg", "rs-v0.5.sgn");
	mGeneralConfig = new p3GeneralConfig();

	/* create Services */
        ad = new p3disc(mConnMgr, pqih);
	msgSrv = new p3MsgService(mConnMgr);
	chatSrv = new p3ChatService(mConnMgr);

        p3tunnel *tn = new p3tunnel(mConnMgr, pqih);
	pqih -> addService(tn);
	mConnMgr->setP3tunnel(tn);

	p3turtle *tr = new p3turtle(mConnMgr,ftserver) ;
	rsTurtle = tr ;
	pqih -> addService(tr);
	ftserver->connectToTurtleRouter(tr) ;

	pqih -> addService(ad);
	pqih -> addService(msgSrv);
	pqih -> addService(chatSrv);

	/* create Cache Services */
	std::string config_dir = RsInitConfig::configDir;
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	std::string channelsdir = config_dir + "/channels";
	std::string blogsdir = config_dir + "/blogs";


	//mRanking = NULL;
	mRanking = new p3Ranking(mConnMgr, RS_SERVICE_TYPE_RANK,     /* declaration of cache enable service rank */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir, 3600 * 24 * 30 * 6); // 6 Months

        CachePair cp(mRanking, mRanking, CacheId(RS_SERVICE_TYPE_RANK, 0));
	mCacheStrapper -> addCachePair(cp);				/* end of declaration */

	p3Forums *mForums = new p3Forums(RS_SERVICE_TYPE_FORUM,
			mCacheStrapper, mCacheTransfer,
                        localcachedir, remotecachedir);

        CachePair cp4(mForums, mForums, CacheId(RS_SERVICE_TYPE_FORUM, 0));
	mCacheStrapper -> addCachePair(cp4);
	pqih -> addService(mForums);  /* This must be also ticked as a service */

	p3Channels *mChannels = new p3Channels(RS_SERVICE_TYPE_CHANNEL,
			mCacheStrapper, mCacheTransfer, rsFiles,
                        localcachedir, remotecachedir, channelsdir);

        CachePair cp5(mChannels, mChannels, CacheId(RS_SERVICE_TYPE_CHANNEL, 0));
	mCacheStrapper -> addCachePair(cp5);
	pqih -> addService(mChannels);  /* This must be also ticked as a service */
	
			p3Blogs *mBlogs = new p3Blogs(RS_SERVICE_TYPE_QBLOG,
			mCacheStrapper, mCacheTransfer, rsFiles,
                        localcachedir, remotecachedir, blogsdir);

        CachePair cp6(mBlogs, mBlogs, CacheId(RS_SERVICE_TYPE_QBLOG, 0));
	mCacheStrapper -> addCachePair(cp6);
	pqih -> addService(mBlogs);  /* This must be also ticked as a service */


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

        //mConnMgr->addNetAssistConnect(1, mDhtMgr);
	mConnMgr->addNetAssistFirewall(1, mUpnpMgr);

	/**************************************************************************/
	/* need to Monitor too! */
        std::cerr << "need to Monitor too!" << std::endl;

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
        mConfigMgr->addConfiguration("gpg_prefs.cfg", AuthGPG::getAuthGPG());
        mConfigMgr->loadConfiguration();

        mConfigMgr->addConfiguration("peers.cfg", mConnMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("chat.cfg", chatSrv);
	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);
	mConfigMgr->addConfiguration("blogs.cfg", mBlogs);
	mConfigMgr->addConfiguration("ranklink.cfg", mRanking);
	mConfigMgr->addConfiguration("forums.cfg", mForums);
	mConfigMgr->addConfiguration("channels.cfg", mChannels);
	mConfigMgr->addConfiguration("turtle.cfg", tr);
        mConfigMgr->addConfiguration("p3disc.cfg", ad);

	ftserver->addConfiguration(mConfigMgr);

	/**************************************************************************/
	/* (2) Load configuration files */
	/**************************************************************************/
        std::cerr << "(2) Load configuration files" << std::endl;

	mConfigMgr->loadConfiguration();

	/* NOTE: CacheStrapper's load causes Cache Files to be
	 * loaded into all the CacheStores/Sources. This happens
	 * after all the other configurations have happened.
	 */

	/**************************************************************************/
	/* trigger generalConfig loading for classes that require it */
	/**************************************************************************/
	pqih->setConfig(mGeneralConfig);

        pqih->load_config();

	/**************************************************************************/
	/* Force Any Configuration before Startup (After Load) */
	/**************************************************************************/
        std::cerr << "Force Any Configuration before Startup (After Load)" << std::endl;

	if (RsInitConfig::forceLocalAddr)
	{
		struct sockaddr_in laddr;

		/* clean sockaddr before setting values (MaxOSX) */
		sockaddr_clear(&laddr);
		
		laddr.sin_family = AF_INET;
		laddr.sin_port = htons(RsInitConfig::port);

		// universal
		laddr.sin_addr.s_addr = inet_addr(RsInitConfig::inet);

		mConnMgr->setLocalAddress(ownId, laddr);
	}

	if (RsInitConfig::forceExtPort)
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
	mConnMgr->addNetListener(pqih); /* add listener so we can reset all sockets later */




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

        //mDhtMgr->start();

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback(ownId);

	mod -> peerid = ownId;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

        rsPeers = new p3Peers(mConnMgr);
        rsMsgs  = new p3Msgs(msgSrv, chatSrv);
	rsDisc  = new p3Discovery(ad);

	rsForums = mForums;
	rsChannels = mChannels;
	rsRanks = new p3Rank(mRanking);
	rsBlogs = mBlogs;

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
	if (RsInitConfig::firsttime_run)
	{
		msgSrv->loadWelcomeMsg();
	}

	// load up the help page
	std::string helppage = RsInitConfig::basedir + RsInitConfig::dirSeperator;
	helppage += configHelpName;

	/* Startup this thread! */
        createThread(*this);


	return 1;
}
