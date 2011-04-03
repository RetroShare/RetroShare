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

// for locking instances
#ifndef WINDOWS_SYS
#include <errno.h>
#endif

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsrandom.h"
#include "util/folderiterator.h"
#include "retroshare/rsinit.h"
#include "rsserver/rsloginhandler.h"

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
#include "pqi/sslfns.h"
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
#ifdef WINDOWS_SYS
                static bool portable;
                static bool isWindowsXP;
#endif

		static std::list<accountId> accountIds;
		static std::string preferedId;

		/* for certificate creation */
                //static std::string gpgPasswd;

#ifndef WINDOWS_SYS
		static int lockHandle;
#else
		static HANDLE lockHandle;
#endif

		/* These fields are needed for login */
                static std::string loginId;
                static std::string configDir;
                static std::string load_cert;
                static std::string load_key;

		static std::string passwd;

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
                static std::string logfname;

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
static const int SSLPWD_LEN = 64;

std::list<accountId> RsInitConfig::accountIds;
std::string RsInitConfig::preferedId;

#ifndef WINDOWS_SYS
	int RsInitConfig::lockHandle;
#else
	HANDLE RsInitConfig::lockHandle;
#endif

std::string RsInitConfig::configDir;
std::string RsInitConfig::load_cert;
std::string RsInitConfig::load_key;

std::string RsInitConfig::passwd;
//std::string RsInitConfig::gpgPasswd;

bool RsInitConfig::autoLogin;  		/* autoLogin allowed */
bool RsInitConfig::startMinimised; /* Icon or Full Window */

/* Win/Unix Differences */
char RsInitConfig::dirSeperator;

/* Directories */
std::string RsInitConfig::basedir;
std::string RsInitConfig::homePath;
#ifdef WINDOWS_SYS
bool RsInitConfig::portable = false;
bool RsInitConfig::isWindowsXP = false;
#endif

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
std::string RsInitConfig::logfname;

bool RsInitConfig::firsttime_run;
bool RsInitConfig::load_trustedpeer;
std::string RsInitConfig::load_trustedpeer_file;

bool RsInitConfig::udpListenerOnly;


/* Uses private class - so must be hidden */
static bool getAvailableAccounts(std::list<accountId> &ids);
static bool checkAccount(std::string accountdir, accountId &id);

static std::string toUpperCase(const std::string& s)
{
	std::string res(s) ;

	for(uint32_t i=0;i<res.size();++i)
		if(res[i] > 96 && res[i] < 123)
			res[i] -= 97-65 ;

	return res ;
}

static std::string toLowerCase(const std::string& s)
{
	std::string res(s) ;

	for(uint32_t i=0;i<res.size();++i)
		if(res[i] > 64 && res[i] < 91)
			res[i] += 97-65 ;

	return res ;
}

void RsInit::InitRsConfig()
{
#ifndef WINDOWS_SYS
	RsInitConfig::dirSeperator = '/'; // For unix.
	RsInitConfig::lockHandle = -1;
#else
	RsInitConfig::dirSeperator = '\\'; // For windows.
	RsInitConfig::lockHandle = NULL;
#endif


	RsInitConfig::load_trustedpeer = false;
	RsInitConfig::firsttime_run = false;
	RsInitConfig::port = 0 ;
	RsInitConfig::forceLocalAddr = false;
	RsInitConfig::haveLogFile    = false;
	RsInitConfig::outStderr      = false;
	RsInitConfig::forceExtPort   = false;

	strcpy(RsInitConfig::inet, "127.0.0.1");

	RsInitConfig::autoLogin      = false; // .
	RsInitConfig::startMinimised = false;
	RsInitConfig::passwd         = "";
	RsInitConfig::haveDebugLevel = false;
	RsInitConfig::debugLevel	= PQL_WARNING;
	RsInitConfig::udpListenerOnly = false;

	/* setup the homePath (default save location) */
	RsInitConfig::homePath = getHomePath();

#ifdef WINDOWS_SYS
	// test for portable version
	if (GetFileAttributes (L"gpg.exe") != (DWORD) -1 && GetFileAttributes (L"gpgme-w32spawn.exe") != (DWORD) -1) {
		// use portable version
		RsInitConfig::portable = true;
	}

	// test for Windows XP
	OSVERSIONINFOEX osvi;
	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (GetVersionEx((OSVERSIONINFO*) &osvi)) {
		if (osvi.dwMajorVersion == 5) {
			if (osvi.dwMinorVersion == 1) {
				/* Windows XP */
				RsInitConfig::isWindowsXP = true;
			} else if (osvi.dwMinorVersion == 2) {
				SYSTEM_INFO si;
				memset(&si, 0, sizeof(si));
				GetSystemInfo(&si);
				if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
					/* Windows XP Professional x64 Edition */
					RsInitConfig::isWindowsXP = true;
				}
			}
		}
	}

	if (RsInitConfig::isWindowsXP) {
		std::cerr << "Running Windows XP" << std::endl;
	} else {
		std::cerr << "Not running Windows XP" << std::endl;
	}
#endif

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
int RsInit::InitRetroShare(int argc, char **argv, bool strictCheck)
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

   /* for static PThreads under windows... we need to init the library...
    */
   #ifdef PTW32_STATIC_LIB
      #include <pthread.h>
   #endif

int RsInit::InitRetroShare(int argcIgnored, char **argvIgnored, bool strictCheck)
{

  /* THIS IS A HACK TO ALLOW WINDOWS TO ACCEPT COMMANDLINE ARGUMENTS */




  int argc;
  int i;
#ifdef USE_CMD_ARGS
  char** argv = argvIgnored;
  argc = argcIgnored;


#else

  const int MAX_ARGS = 32;
  int j;
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

#endif

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
         std::string prefUserString = "";
         /* getopt info: every availiable option is listed here. if it is followed by a ':' it
            needs an argument. If it is followed by a '::' the argument is optional.
         */
         while((c = getopt(argc, argv,"hesamui:p:c:w:l:d:U:")) != -1)
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
                                 RsInitConfig::logfname = optarg;
                                 std::cerr << "LogFile (" << RsInitConfig::logfname;
                                 std::cerr << ") Selected" << std::endl;
                                 RsInitConfig::haveLogFile = true;
                                 break;
                         case 'w':
                                 RsInitConfig::passwd = optarg;
                                 std::cerr << "Password Specified(********" ; //<< RsInitConfig::passwd;
                                 std::cerr << ") Selected" << std::endl;
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
                         case 'U':
                                 prefUserString = optarg;
                                 std::cerr << "Opt for User Id ";
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
                                 std::cerr << "-e                Use a forwarded external Port" << std::endl ;
                                 std::cerr << "-U [User Name/GPG id/SSL id]  Sets Account to Use, Useful when Autologin is enabled." << std::endl;
                                 exit(1);
                                 break;
                         default:
				if (strictCheck) {
                                   std::cerr << "Unknown Option!" << std::endl;
                                   std::cerr << "Use '-h' for help." << std::endl;
                                   exit(1);
				}
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
		setDebugFile(RsInitConfig::logfname.c_str());
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
	/* create singletons */
	AuthSSLInit();
	AuthGPGInit();

        AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL);

	// first check config directories, and set bootstrap values.
	setupBaseDir();
	get_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);

	/* Initialize AuthGPG */
	if (AuthGPG::getAuthGPG()->InitAuth() == false) {
		std::cerr << "AuthGPG::InitAuth failed" << std::endl;
		return RS_INIT_AUTH_FAILED;
	}

	//std::list<accountId> ids;
	std::list<accountId>::iterator it;
	getAvailableAccounts(RsInitConfig::accountIds);

        // if a different user id has been passed to cmd line check for that instead

	std::string lower_case_user_string = toLowerCase(prefUserString) ;
	std::string upper_case_user_string = toUpperCase(prefUserString) ;

	bool pgpNameFound = false;
	if(prefUserString != "")
	{

		for(it = RsInitConfig::accountIds.begin() ; it!= RsInitConfig::accountIds.end() ; it++)
		{
			std::cerr << "Checking account (gpgid = " << it->pgpId << ", name=" << it->pgpName << ", sslId=" << it->sslId << ")" << std::endl ;

			if(prefUserString == it->pgpName || upper_case_user_string == it->pgpId || lower_case_user_string == it->sslId)
			{
				RsInitConfig::preferedId = it->sslId;
				pgpNameFound = true;
			}
		}
		if(!pgpNameFound){
			std::cerr << "Invalid User name/GPG id/SSL id: not found in list" << std::endl;
			exit(1);
		}
	}



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
		if (RsInitConfig::passwd != "")
		{
			return RS_INIT_HAVE_ACCOUNT;
		}

		RsInit::LoadPassword(RsInitConfig::preferedId, "");

		if(RsLoginHandler::getSSLPassword(RsInitConfig::preferedId,false,RsInitConfig::passwd))
		{
			RsInit::setAutoLogin(true);
			std::cerr << "Autologin has succeeded" << std::endl;
			return RS_INIT_HAVE_ACCOUNT;
		}
	}
	return RS_INIT_OK;
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
        #ifdef AUTHSSL_DEBUG
	std::cerr << "getAccountIds:" << std::endl;
        #endif

	for(it = RsInitConfig::accountIds.begin(); it != RsInitConfig::accountIds.end(); it++)
	{
                #ifdef AUTHSSL_DEBUG
                std::cerr << "SSL Id: " << it->sslId << " PGP Id " << it->pgpId;
		std::cerr << " PGP Name: " << it->pgpName;
		std::cerr << " PGP Email: " << it->pgpEmail;
                std::cerr << " Location: " << it->location;
		std::cerr << std::endl;
                #endif

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
		if (RsInitConfig::portable) {
			// use directory "Data" in portable version
			RsInitConfig::basedir = "Data";
		} else {
			wchar_t *wh = _wgetenv(L"APPDATA");
			std::string h;
			librs::util::ConvertUtf16ToUtf8(std::wstring(wh), h);
			std::cerr << "retroShare::basedir() -> $APPDATA = ";
			std::cerr << h << std::endl;
			char *h2 = getenv("HOMEDRIVE");
			std::cerr << "retroShare::basedir() -> $HOMEDRIVE = ";
			std::cerr << h2 << std::endl;
			wchar_t *wh3 = _wgetenv(L"HOMEPATH");
			std::string h3;
			librs::util::ConvertUtf16ToUtf8(std::wstring(wh3), h3);
			std::cerr << "retroShare::basedir() -> $HOMEPATH = ";
			std::cerr << h3 << std::endl;
			if (h.empty())
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
		}
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


/***********************************************************
 * This Directory is used to store data and "template" file that Retroshare requires.
 * These files will either be copied into Retroshare's configuration directory, 
 * if they are to be modified. Or used directly, if read-only.
 *
 * This will initially be used for the DHT bootstrap file.
 *
 * Please modify the code below to suit your platform!
 * 
 * WINDOWS: 
 * WINDOWS PORTABLE:
 * Linux:
 * OSX:

 ***********/

#ifdef __APPLE__
	/* needs CoreFoundation Framework */
	#include <CoreFoundation/CoreFoundation.h>
	//#include <CFURL.h>
	//#include <CFBundle.h>
#endif

std::string RsInit::getRetroshareDataDirectory()
{
	std::string dataDirectory;

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

  #ifdef __APPLE__
	/* For OSX, applications are Bundled in a directory...
	 * need to get the path to the executable Bundle.
	 * 
	 * Code nicely supplied by Qt!
	 */

	CFURLRef pluginRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef,
       	                                    kCFURLPOSIXPathStyle);
	const char *pathPtr = CFStringGetCStringPtr(macPath,
                                           CFStringGetSystemEncoding());
	dataDirectory = pathPtr;
	CFRelease(pluginRef);
	CFRelease(macPath);

    	dataDirectory += "/Contents/Resources";
	std::cerr << "getRetroshareDataDirectory() OSX: " << dataDirectory;

  #else
	/* For Linux, we have a fixed standard data directory  */
	dataDirectory = "/usr/share/RetroShare";
	std::cerr << "getRetroshareDataDirectory() Linux: " << dataDirectory;

  #endif
#else
//	if (RsInitConfig::portable)
//	{
//		/* For Windows Portable, files must be in the data directory */
//		dataDirectory = "Data";
//		std::cerr << "getRetroshareDataDirectory() WINDOWS PORTABLE: " << dataDirectory;
//		std::cerr << std::endl;
//	}
//	else
//	{
//		/* For Windows: environment variable APPDATA should be suitable */
//		dataDirectory = getenv("APPDATA");
//		dataDirectory += "\\RetroShare";
//
//		std::cerr << "getRetroshareDataDirectory() WINDOWS: " << dataDirectory;
//		std::cerr << std::endl;
//	}

	/* Use RetroShare's exe dir */
	dataDirectory = ".";
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	/* Make sure the directory exists, else return emptyString */
	if (!RsDirUtil::checkDirectory(dataDirectory))
	{
		std::cerr << "Data Directory not Found: " << dataDirectory << std::endl;
		dataDirectory = "";
	}
	else
	{
		std::cerr << "Data Directory Found: " << dataDirectory << std::endl;
	}

	return dataDirectory;
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

	/* check for the dir existance */
	librs::util::FolderIterator dirIt(RsInitConfig::basedir);
	if (!dirIt.isValid())
	{
		std::cerr << "Cannot Open Base Dir - No Available Accounts" << std::endl;
		exit(1);
	}

	struct stat64 buf;

	while (dirIt.readdir())
	{
		/* check entry type */
		std::string fname;
		dirIt.d_name(fname);
		std::string fullname = RsInitConfig::basedir + "/" + fname;
#ifdef FIM_DEBUG
		std::cerr << "calling stats on " << fullname <<std::endl;
#endif

#ifdef WINDOWS_SYS
		std::wstring wfullname;
		librs::util::ConvertUtf8ToUtf16(fullname, wfullname);
		if (-1 != _wstati64(wfullname.c_str(), &buf))
#else
		if (-1 != stat64(fullname.c_str(), &buf))
#endif

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
	/* close directory */
	dirIt.closedir();

	for(it = directories.begin(); it != directories.end(); it++)
	{
		std::string accountdir = RsInitConfig::basedir + RsInitConfig::dirSeperator + *it;
#ifdef GPG_DEBUG
		std::cerr << "getAvailableAccounts() Checking: " << *it << std::endl;
#endif

		accountId tmpId;
		if (checkAccount(accountdir, tmpId))
		{
#ifdef GPG_DEBUG
			std::cerr << "getAvailableAccounts() Accepted: " << *it << std::endl;
#endif
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

        #ifdef AUTHSSL_DEBUG
	std::cerr << "checkAccount() dir: " << accountdir << std::endl;
        #endif

	bool ret = false;

	/* check against authmanagers private keys */
        if (LoadCheckX509(cert_name.c_str(), id.pgpId, id.location, id.sslId))
        {
        	#ifdef AUTHSSL_DEBUG
        	std::cerr << "location: " << id.location << " id: " << id.sslId << std::endl;
        	#endif


                #ifdef GPG_DEBUG
                std::cerr << "issuerName: " << id.pgpId << " id: " << id.sslId << std::endl;
                #endif
                RsInit::GetPGPLoginDetails(id.pgpId, id.pgpName, id.pgpEmail);
                #ifdef GPG_DEBUG
                std::cerr << "PGPLoginDetails: " << id.pgpId << " name: " << id.pgpName;
                std::cerr << " email: " << id.pgpEmail << std::endl;
                #endif
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
        #ifdef GPG_DEBUG
        std::cerr << "RsInit::GetPGPLoginDetails for \"" << id << "\"" << std::endl;
        #endif

        name = AuthGPG::getAuthGPG()->getGPGName(id);
        email = AuthGPG::getAuthGPG()->getGPGEmail(id);
        if (name != "") {
            return 1;
        } else {
            return 0;
        }
}

/*
 * To prevent several running instances from using the same directory
 * simultaneously we have to use a global lock.
 * We use a lock file on Unix systems.
 *
 * Return value:
 * 0 : Success
 * 1 : Another instance already has the lock
 * 2 : Unexpected error
 */
int RsInit::LockConfigDirectory(const std::string& accountDir, std::string& lockFilePath)
{
	const std::string lockFile = accountDir + RsInitConfig::dirSeperator + "lock";

        lockFilePath = lockFile;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	if(RsInitConfig::lockHandle != -1)
		close(RsInitConfig::lockHandle);

	// open the file in write mode, create it if necessary, truncate it (it should be empty)
	RsInitConfig::lockHandle = open(lockFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
						S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if(RsInitConfig::lockHandle == -1)
	{
		std::cerr << "Could not open lock file " << lockFile.c_str() << std::flush;
		perror(NULL);
		return 2;
	}

	// see "man fcntl" for the details, in short: non blocking lock creation on the whole file contents
	struct flock lockDetails;
	lockDetails.l_type = F_WRLCK;
	lockDetails.l_whence = SEEK_SET;
	lockDetails.l_start = 0;
	lockDetails.l_len = 0;

	if(fcntl(RsInitConfig::lockHandle, F_SETLK, &lockDetails) == -1)
	{
		int fcntlErr = errno;
		std::cerr << "Could not request lock on file " << lockFile.c_str() << std::flush;
		perror(NULL);

		// there's no lock so let's release the file handle immediately
		close(RsInitConfig::lockHandle);
		RsInitConfig::lockHandle = -1;

		if(fcntlErr == EACCES || fcntlErr == EAGAIN)
			return 1;
		else
			return 2;
	}

	return 0;
#else
	if (RsInitConfig::lockHandle) {
		CloseHandle(RsInitConfig::lockHandle);
	}

	std::wstring wlockFile;
	librs::util::ConvertUtf8ToUtf16(lockFile, wlockFile);

	// open the file in write mode, create it if necessary
	RsInitConfig::lockHandle = CreateFile(wlockFile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	if (RsInitConfig::lockHandle == INVALID_HANDLE_VALUE) {
		DWORD lasterror = GetLastError();

		std::cerr << "Could not open lock file " << lockFile.c_str() << std::endl;
		std::cerr << "Last error: " << lasterror << std::endl << std::flush;
		perror(NULL);

		if (lasterror == ERROR_SHARING_VIOLATION || lasterror == ERROR_ACCESS_DENIED) {
			return 1;
		}
		return 2;
	}

	return 0;
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
}

/*
 * Unlock the currently locked profile, if there is one.
 * For Unix systems we simply close the handle of the lock file.
 */
void	RsInit::UnlockConfigDirectory()
{
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	if(RsInitConfig::lockHandle != -1)
	{
		close(RsInitConfig::lockHandle);
		RsInitConfig::lockHandle = -1;
	}
#else
	if(RsInitConfig::lockHandle)
	{
		CloseHandle(RsInitConfig::lockHandle);
		RsInitConfig::lockHandle = NULL;
	}
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
}


/* Before any SSL stuff can be loaded, the correct PGP must be selected / generated:
 **/

bool RsInit::SelectGPGAccount(const std::string& gpgId)
{
	bool retVal = false;

	if (0 < AuthGPG::getAuthGPG() -> GPGInit(gpgId))
	{
		retVal = true;
		std::cerr << "PGP Auth Success!";
	}
	else
		std::cerr << "PGP Auth Failed!";

	std::cerr << " ID: " << gpgId << std::endl;

	return retVal;
}


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
        X509 *x509 = AuthSSL::getAuthSSL()->SignX509ReqWithGPG(req, days);

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
        	if (NULL == (out = RsDirUtil::rs_fopen(cert_name.c_str(), "w")))
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
        std::string gpgid;
        if (LoadCheckX509(cert_name.c_str(), gpgid, location, sslId) == 0) {
            std::cerr << "RsInit::GenerateSSLCertificate() Cannot check own signature, maybe the files are corrupted." << std::endl;
            return false;
        }

        /* Move directory to correct id */
	std::string finalbase = RsInitConfig::basedir + RsInitConfig::dirSeperator + sslId + RsInitConfig::dirSeperator;
	/* Rename Directory */

	std::cerr << "Mv Config Dir from: " << tmpbase << " to: " << finalbase;
	std::cerr << std::endl;

	if (!RsDirUtil::renameFile(tmpbase, finalbase))
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

	//	if(inPwd != "")
	//		RsInitConfig::havePasswd = true;

	// Create the filename.
	std::string basename = RsInitConfig::configDir + RsInitConfig::dirSeperator;
	basename += configKeyDir + RsInitConfig::dirSeperator;
	basename += "user";

	RsInitConfig::load_key  = basename + "_pk.pem";
	RsInitConfig::load_cert = basename + "_cert.pem";

	return true;
}


/**
 * Locks the profile directory and tries to finalize the login procedure
 *
 * Return value:
 * 0 : success
 * 1 : another instance is already running
 * 2 : unexpected error while locking
 * 3 : unexpected error while loading certificates
 */
int 	RsInit::LockAndLoadCertificates(bool autoLoginNT, std::string& lockFilePath)
{
        int retVal = LockConfigDirectory(RsInitConfig::configDir, lockFilePath);
	if(retVal != 0)
		return retVal;

	retVal = LoadCertificates(autoLoginNT);
	if(retVal != 1) {
		UnlockConfigDirectory();
		return 3;
	}

	return 0;
}


/** *************************** FINAL LOADING OF SETUP *************************
 * Requires:
 *     PGPid to be selected (Password not required).
 *     CertId to be selected (Password Required).
 *
 * Return value:
 * 0 : unexpected error
 * 1 : success
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

	//check if password is already in memory
	
	if(RsInitConfig::passwd == "")
		RsLoginHandler::getSSLPassword(RsInitConfig::preferedId,true,RsInitConfig::passwd) ;
	else
		RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile(RsInitConfig::preferedId,RsInitConfig::passwd) ;

	std::cerr << "RsInitConfig::load_key.c_str() : " << RsInitConfig::load_key.c_str() << std::endl;

	if(0 == AuthSSL::getAuthSSL() -> InitAuth(RsInitConfig::load_cert.c_str(), RsInitConfig::load_key.c_str(), RsInitConfig::passwd.c_str()))
	{
		std::cerr << "SSL Auth Failed!";
		return 0 ;
	}

	if(autoLoginNT)
	{
		std::cerr << "RetroShare will AutoLogin next time";
		std::cerr << std::endl;

		RsLoginHandler::enableAutoLogin(RsInitConfig::preferedId,RsInitConfig::passwd);
		RsInitConfig::autoLogin = true ;
	}

	/* wipe out password */
	RsInitConfig::passwd = "";
	create_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);
      
	return 1;
}
bool RsInit::RsClearAutoLogin()
{
	return	RsLoginHandler::clearAutoLogin(RsInitConfig::preferedId);
}
bool	RsInit::get_configinit(std::string dir, std::string &id)
{
	// have a config directories.

	// Check for config file.
	std::string initfile = dir + RsInitConfig::dirSeperator;
	initfile += configInitFile;

	// open and read in the lines.
	FILE *ifd = RsDirUtil::rs_fopen(initfile.c_str(), "r");
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
	FILE *ifd = RsDirUtil::rs_fopen(initfile.c_str(), "w");

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

char RsInit::dirSeperator()
{
	return RsInitConfig::dirSeperator;
}

bool RsInit::isPortable()
{
#ifdef WINDOWS_SYS
    return RsInitConfig::portable;
#else
    return false;
#endif
}

bool RsInit::isWindowsXP()
{
#ifdef WINDOWS_SYS
    return RsInitConfig::isWindowsXP;
#else
    return false;
#endif
}
std::string RsInit::RsConfigKeysDirectory()
{
    return RsInitConfig::basedir + RsInitConfig::dirSeperator + RsInitConfig::preferedId + RsInitConfig::dirSeperator + configKeyDir ;
}
std::string RsInit::RsConfigDirectory()
{
	return RsInitConfig::basedir;
}

std::string RsInit::RsProfileConfigDirectory()
{
    std::string dir = RsInitConfig::basedir + RsInitConfig::dirSeperator + RsInitConfig::preferedId;
    //std::cerr << "RsInit::RsProfileConfigDirectory() returning : " << dir << std::endl;
    return dir;
}

bool	RsInit::setStartMinimised()
{
	return RsInitConfig::startMinimised;
}

int RsInit::getSslPwdLen(){
	return SSLPWD_LEN;
}

bool RsInit::getAutoLogin(){
	return RsInitConfig::autoLogin;
}

void RsInit::setAutoLogin(bool autoLogin){
	RsInitConfig::autoLogin = autoLogin;
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

#include "retroshare/rsiface.h"
#include "retroshare/rsturtle.h"

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
//#include "dht/opendhtmgr.h"

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3gamelauncher.h"
#include "services/p3ranking.h"
#include "services/p3photoservice.h"
#include "services/p3forums.h"
#include "services/p3channels.h"
#include "services/p3statusservice.h"
#include "services/p3blogs.h"
#include "turtle/p3turtle.h"

#ifndef PQI_DISABLE_TUNNEL
#include "services/p3tunnel.h"
#endif

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
#include "rsserver/p3status.h"
#include "retroshare/rsgame.h"

#include "pqi/p3notify.h" // HACK - moved to pqi for compilation order.

#include "tcponudp/tou.h"
#include "tcponudp/rsudpstack.h"

#ifdef RS_USE_BITDHT
#include "dht/p3bitdht.h"
#include "udp/udpstack.h"
#endif

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
		crashfile += ownId + RsInitConfig::dirSeperator + configLogFileName;
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

	std::string emergencySaveDir = RsInitConfig::configDir.c_str();
	std::string emergencyPartialsDir = RsInitConfig::configDir.c_str();
	if (emergencySaveDir != "")
	{
		emergencySaveDir += "/";
		emergencyPartialsDir += "/";
	}
	emergencySaveDir += "Downloads";
	emergencyPartialsDir += "Partials";

	/**************************************************************************/
	/* setup classes / structures */
	/**************************************************************************/
        std::cerr << "setup classes / structures" << std::endl;

	/* Setup Notify Early - So we can use it. */
	rsNotify = new p3Notify();

        mConnMgr = new p3ConnectMgr();

        //load all the SSL certs as friends
//        std::list<std::string> sslIds;
//        AuthSSL::getAuthSSL()->getAuthenticatedList(sslIds);
//        for (std::list<std::string>::iterator sslIdsIt = sslIds.begin(); sslIdsIt != sslIds.end(); sslIdsIt++) {
//            mConnMgr->addFriend(*sslIdsIt);
//        }
	pqiNetAssistFirewall *mUpnpMgr = new upnphandler();
        //p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr, RsInitConfig::configDir);
/**************************** BITDHT ***********************************/

	// Make up an address. XXX

	struct sockaddr_in tmpladdr;
	sockaddr_clear(&tmpladdr);
	tmpladdr.sin_port = htons(RsInitConfig::port);
	rsUdpStack *mUdpStack = new rsUdpStack(tmpladdr);

#ifdef RS_USE_BITDHT

#define BITDHT_BOOTSTRAP_FILENAME  	"bdboot.txt"


	std::string bootstrapfile = RsInitConfig::configDir.c_str();
	if (bootstrapfile != "")
	{
		bootstrapfile += "/";
	}
	bootstrapfile += BITDHT_BOOTSTRAP_FILENAME;

	std::cerr << "Checking for DHT bootstrap file: " << bootstrapfile << std::endl;

	/* check if bootstrap file exists...
	 * if not... copy from dataDirectory
	 */

	if (!RsDirUtil::checkFile(bootstrapfile))
	{
		std::cerr << "DHT bootstrap file not in ConfigDir: " << bootstrapfile << std::endl;
		std::string installfile = RsInit::getRetroshareDataDirectory();
		installfile += RsInitConfig::dirSeperator;
		installfile += BITDHT_BOOTSTRAP_FILENAME;

		std::cerr << "Checking for Installation DHT bootstrap file " << installfile << std::endl;
		if ((installfile != "") && (RsDirUtil::checkFile(installfile)))
		{
			std::cerr << "Copying Installation DHT bootstrap file..." << std::endl;
			if (RsDirUtil::copyFile(installfile, bootstrapfile))
			{
				std::cerr << "Installed DHT bootstrap file in configDir" << std::endl;
			}
			else
			{
				std::cerr << "Failed Installation DHT bootstrap file..." << std::endl;
			}
		}
		else
		{
			std::cerr << "No Installation DHT bootstrap file to copy" << std::endl;
		}
	}

	p3BitDht *mBitDht = new p3BitDht(ownId, mConnMgr, 
				mUdpStack, bootstrapfile);

	/* construct the rest of the stack */
	tou_init(mUdpStack);
#endif


/**************************** BITDHT ***********************************/


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
#ifndef MINIMAL_LIBRS
	msgSrv = new p3MsgService(mConnMgr);
	chatSrv = new p3ChatService(mConnMgr);
	mStatusSrv = new p3StatusService(mConnMgr);
#endif // MINIMAL_LIBRS

#ifndef PQI_DISABLE_TUNNEL
        p3tunnel *tn = new p3tunnel(mConnMgr, pqih);
	pqih -> addService(tn);
	mConnMgr->setP3tunnel(tn);
#endif

	p3turtle *tr = new p3turtle(mConnMgr,ftserver) ;
	rsTurtle = tr ;
	pqih -> addService(tr);
	ftserver->connectToTurtleRouter(tr) ;

	pqih -> addService(ad);
#ifndef MINIMAL_LIBRS
	pqih -> addService(msgSrv);
	pqih -> addService(chatSrv);
	pqih ->addService(mStatusSrv);
#endif // MINIMAL_LIBRS

	/* create Cache Services */
	std::string config_dir = RsInitConfig::configDir;
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	std::string channelsdir = config_dir + "/channels";
	std::string blogsdir = config_dir + "/blogs";
	std::string forumdir = config_dir + "/forums";


#ifndef MINIMAL_LIBRS
	mRanking = new p3Ranking(mConnMgr, RS_SERVICE_TYPE_RANK,     /* declaration of cache enable service rank */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir, 3600 * 24 * 30 * 6); // 6 Months

        CachePair cp(mRanking, mRanking, CacheId(RS_SERVICE_TYPE_RANK, 0));
	mCacheStrapper -> addCachePair(cp);				/* end of declaration */

	mForums = new p3Forums(RS_SERVICE_TYPE_FORUM,
			mCacheStrapper, mCacheTransfer,
                        localcachedir, remotecachedir, forumdir);

        CachePair cp4(mForums, mForums, CacheId(RS_SERVICE_TYPE_FORUM, 0));
	mCacheStrapper -> addCachePair(cp4);
	pqih -> addService(mForums);  /* This must be also ticked as a service */

	mChannels = new p3Channels(RS_SERVICE_TYPE_CHANNEL,
			mCacheStrapper, mCacheTransfer, rsFiles,
                        localcachedir, remotecachedir, channelsdir);

        CachePair cp5(mChannels, mChannels, CacheId(RS_SERVICE_TYPE_CHANNEL, 0));
	mCacheStrapper -> addCachePair(cp5);
	pqih -> addService(mChannels);  /* This must be also ticked as a service */
#ifdef RS_USE_BLOGS	
			p3Blogs *mBlogs = new p3Blogs(RS_SERVICE_TYPE_QBLOG,
			mCacheStrapper, mCacheTransfer, rsFiles,
                        localcachedir, remotecachedir, blogsdir);

        CachePair cp6(mBlogs, mBlogs, CacheId(RS_SERVICE_TYPE_QBLOG, 0));
	mCacheStrapper -> addCachePair(cp6);
	pqih -> addService(mBlogs);  /* This must be also ticked as a service */

#endif


#ifndef RS_RELEASE
	p3GameLauncher *gameLauncher = new p3GameLauncher(mConnMgr);
	pqih -> addService(gameLauncher);

	p3PhotoService *photoService = new p3PhotoService(RS_SERVICE_TYPE_PHOTO,   /* .... for photo service */
			mCacheStrapper, mCacheTransfer,
			localcachedir, remotecachedir);

        CachePair cp2(photoService, photoService, CacheId(RS_SERVICE_TYPE_PHOTO, 0));
	mCacheStrapper -> addCachePair(cp2);
#endif
#endif // MINIMAL_LIBRS

	/**************************************************************************/

#ifdef RS_USE_BITDHT
        mConnMgr->addNetAssistConnect(1, mBitDht);
	mConnMgr->addNetListener(mUdpStack); 
#endif
	mConnMgr->addNetAssistFirewall(1, mUpnpMgr);

	/**************************************************************************/
	/* need to Monitor too! */
	mConnMgr->addMonitor(pqih);
	mConnMgr->addMonitor(mCacheStrapper);
	mConnMgr->addMonitor(ad);
#ifndef MINIMAL_LIBRS
	mConnMgr->addMonitor(msgSrv);
	mConnMgr->addMonitor(mStatusSrv);
	mConnMgr->addMonitor(chatSrv);
#endif // MINIMAL_LIBRS

	/* must also add the controller as a Monitor...
	 * a little hack to get it to work.
	 */
	mConnMgr->addMonitor(((ftController *) mCacheTransfer));


	/**************************************************************************/

	//mConfigMgr->addConfiguration("ftserver.cfg", ftserver);
	//
        mConfigMgr->addConfiguration("gpg_prefs.cfg", (AuthGPGimpl *) AuthGPG::getAuthGPG());
        mConfigMgr->loadConfiguration();

        //mConfigMgr->addConfiguration("sslcerts.cfg", AuthSSL::getAuthSSL());
        mConfigMgr->addConfiguration("peers.cfg", mConnMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);
#ifndef MINIMAL_LIBRS
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("chat.cfg", chatSrv);
#ifdef RS_USE_BLOGS	
        mConfigMgr->addConfiguration("blogs.cfg", mBlogs);
#endif
	mConfigMgr->addConfiguration("ranklink.cfg", mRanking);
	mConfigMgr->addConfiguration("forums.cfg", mForums);
	mConfigMgr->addConfiguration("channels.cfg", mChannels);
	mConfigMgr->addConfiguration("p3Status.cfg", mStatusSrv);
#endif // MINIMAL_LIBRS
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

	// Clear the News Feeds that are generated by Initial Cache Loading.

	/* Peer stuff is up to date */

	/* Channel/Forum/Blog stuff will all come from Caches */
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_CHAN_NEW);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_CHAN_UPDATE);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_CHAN_MSG);

	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FORUM_NEW);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FORUM_UPDATE);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FORUM_MSG);

	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_BLOG_NEW);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_BLOG_UPDATE);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_BLOG_MSG);

	//getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_CHAT_NEW);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_MESSAGE);
	//getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FILES_NEW);

	/* flag that the basic Caches are now in the pending Queues */
#ifndef MINIMAL_LIBRS
	mForums->HistoricalCachesDone();
	mChannels->HistoricalCachesDone();
	
#ifdef RS_USE_BLOGS	
	mBlogs->HistoricalCachesDone();
#endif
#endif // MINIMAL_LIBRS	
	
	
	/**************************************************************************/
	/* Add AuthGPG services */
	/**************************************************************************/

	AuthGPG::getAuthGPG()->addService(ad);

	/**************************************************************************/
	/* Force Any Last Configuration Options */
	/**************************************************************************/

	/**************************************************************************/
	/* Start up Threads */
	/**************************************************************************/

	ftserver->StartupThreads();
	ftserver->ResumeTransfers();

        //mDhtMgr->start();
#ifdef RS_USE_BITDHT
        mBitDht->start();
#endif

	// startup the p3distrib threads (for cache loading).
#ifndef MINIMAL_LIBRS
	mForums->start();
	mChannels->start();

#ifdef RS_USE_BLOGS	
	mBlogs->start();
#endif
#endif // MINIMAL_LIBRS

	/**************************************************************************/

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback(ownId);

	mod -> peerid = ownId;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

	rsPeers = new p3Peers(mConnMgr);
	rsDisc  = new p3Discovery(ad);

#ifndef MINIMAL_LIBRS
        rsMsgs  = new p3Msgs(msgSrv, chatSrv);
	rsForums = mForums;
	rsChannels = mChannels;
	rsRanks = new p3Rank(mRanking);
#ifdef RS_USE_BLOGS	
	rsBlogs = mBlogs;
#endif
	rsStatus = new p3Status(mStatusSrv);

#ifndef RS_RELEASE
	rsGameLauncher = gameLauncher;
	rsPhoto = new p3Photo(photoService);
#else
	rsGameLauncher = NULL;
	rsPhoto = NULL;
#endif
#endif // MINIMAL_LIBRS


#ifndef MINIMAL_LIBRS
	/* put a welcome message in! */
	if (RsInitConfig::firsttime_run)
	{
		msgSrv->loadWelcomeMsg();
		ftserver->shareDownloadDirectory(true);
	}
#endif // MINIMAL_LIBRS

	// load up the help page
	std::string helppage = RsInitConfig::basedir + RsInitConfig::dirSeperator;
	helppage += configHelpName;

	/* Startup this thread! */
	createThread(*this);

	return 1;
}
