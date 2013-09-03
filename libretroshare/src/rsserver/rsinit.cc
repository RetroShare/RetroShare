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

#ifndef WINDOWS_SYS
// for locking instances
#include <errno.h>
#else
#include "util/rswin.h"
#endif

#include "util/argstream.h"
#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsrandom.h"
#include "util/folderiterator.h"
#include "util/rsstring.h"
#include "retroshare/rsinit.h"
#include "plugins/pluginmanager.h"
#include "rsserver/rsloginhandler.h"

#include <list>
#include <string>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

// for blocking signals
#include <signal.h>

#include "pqi/authssl.h"
#include "pqi/sslfns.h"
#include "pqi/authgpg.h"

#include "tcponudp/udpstunner.h"

// #define GPG_DEBUG
// #define AUTHSSL_DEBUG
// #define FIM_DEBUG

class accountId
{
	public:
		std::string pgpId;
		std::string pgpName;
		std::string pgpEmail;

		std::string sslId;
                std::string location;
};

std::map<std::string,std::vector<std::string> > RsInit::unsupported_keys ;

class RsInitConfig 
{
	public:
                /* Directories (SetupBaseDir) */
                static std::string basedir;
                static std::string homePath;
                static std::string main_executable_hash;
#ifdef WINDOWS_SYS
                static bool portable;
                static bool isWindowsXP;
#endif

		static std::list<accountId> accountIds;
		static std::string preferedId;

		/* for certificate creation */
                //static std::string gpgPasswd;

		static rs_lock_handle_t lockHandle;

		/* These fields are needed for login */
                static std::string loginId;
                static std::string configDir;
                static std::string load_cert;
                static std::string load_key;

		static std::string passwd;
		static std::string gxs_passwd;

                static bool autoLogin;                  /* autoLogin allowed */
                static bool startMinimised; 		/* Icon or Full Window */

                /* Key Parameters that must be set before
                 * RetroShare will start up:
                 */

                /* Listening Port */
                static bool forceExtPort;
                static bool forceLocalAddr;
                static unsigned short port;
                static std::string inet ;

                /* Logging */
                static bool haveLogFile;
                static bool outStderr;
                static int  debugLevel;
                static std::string logfname;

                static bool firsttime_run;
                static bool load_trustedpeer;
                static std::string load_trustedpeer_file;

                static bool udpListenerOnly;

                static std::string RetroShareLink;
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
std::string RsInitConfig::main_executable_hash;

rs_lock_handle_t RsInitConfig::lockHandle;

std::string RsInitConfig::configDir;
std::string RsInitConfig::load_cert;
std::string RsInitConfig::load_key;

std::string RsInitConfig::passwd;
std::string RsInitConfig::gxs_passwd;
//std::string RsInitConfig::gpgPasswd;

bool RsInitConfig::autoLogin;  		/* autoLogin allowed */
bool RsInitConfig::startMinimised; /* Icon or Full Window */
std::string RsInitConfig::RetroShareLink;

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
std::string RsInitConfig::inet;

/* Logging */
bool RsInitConfig::haveLogFile;
bool RsInitConfig::outStderr;
int  RsInitConfig::debugLevel;
std::string RsInitConfig::logfname;

bool RsInitConfig::firsttime_run;
bool RsInitConfig::load_trustedpeer;
std::string RsInitConfig::load_trustedpeer_file;

bool RsInitConfig::udpListenerOnly;


/* Uses private class - so must be hidden */
static bool getAvailableAccounts(std::list<accountId> &ids,int& failing_accounts,std::map<std::string,std::vector<std::string> >& unsupported_keys);
static bool checkAccount(std::string accountdir, accountId &id,std::map<std::string,std::vector<std::string> >& unsupported_keys);

void RsInit::InitRsConfig()
{
#ifndef WINDOWS_SYS
	RsInitConfig::lockHandle = -1;
#else
	RsInitConfig::lockHandle = NULL;
#endif


	RsInitConfig::load_trustedpeer = false;
	RsInitConfig::firsttime_run = false;
	RsInitConfig::port = 0 ;
	RsInitConfig::forceLocalAddr = false;
	RsInitConfig::haveLogFile    = false;
	RsInitConfig::outStderr      = false;
	RsInitConfig::forceExtPort   = false;

	RsInitConfig::inet = std::string("127.0.0.1");

	RsInitConfig::autoLogin      = false; // .
	RsInitConfig::startMinimised = false;
	RsInitConfig::passwd         = "";
	RsInitConfig::debugLevel	= PQL_WARNING;
	RsInitConfig::udpListenerOnly = false;

	/* setup the homePath (default save location) */
	RsInitConfig::homePath = getHomePath();

#ifdef WINDOWS_SYS
	// test for portable version
	if (GetFileAttributes(L"portable") != (DWORD) -1) {
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

/********
 * LOCALNET_TESTING - allows port restrictions
 *
 * #define LOCALNET_TESTING	1
 *
 ********/


#ifdef LOCALNET_TESTING

std::string portRestrictions;
bool doPortRestrictions = false;

#endif


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
    printf("%d: %s\n", i, argv[i]);

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
			//RsInitConfig::logfname = "" ;
			//RsInitConfig::inet = "" ;

#ifdef __APPLE__
	/* HACK to avoid stupid OSX Finder behaviour
	 * remove the commandline arguments - if we detect we are launched from Finder, 
	 * and we have the unparsable "-psn_0_12332" option.
	 * this is okay, as you cannot pass commandline arguments via Finder anyway
	 */
	if ((argc >= 2) && (0 == strncmp(argv[1], "-psn", 4)))
	{
		argc = 1;
	}
#endif


			argstream as(argc,argv) ;


			as >> option('a',"auto-login"       ,RsInitConfig::autoLogin      ,"AutoLogin (Windows Only) + StartMinimised")
			   >> option('m',"minimized"        ,RsInitConfig::startMinimised ,"Start minimized."                         )
			   >> option('s',"stderr"           ,RsInitConfig::outStderr      ,"output to stderr instead of log file."    )
			   >> option('u',"udp"              ,RsInitConfig::udpListenerOnly,"Only listen to UDP."                      )
			   >> option('e',"external-port"    ,RsInitConfig::forceExtPort   ,"Use a forwarded external port."           )

			   >> parameter('l',"log-file"      ,RsInitConfig::logfname       ,"logfile"   ,"Set Log filename."                        ,false)
			   >> parameter('d',"debug-level"   ,RsInitConfig::debugLevel     ,"level"     ,"Set debug level."                         ,false)
			   >> parameter('w',"password"      ,RsInitConfig::passwd         ,"password"  ,"Set Login Password."                      ,false)
			   >> parameter('i',"ip-address"    ,RsInitConfig::inet           ,"nnn.nnn.nnn.nnn", "Set IP address to use."                   ,false)
			   >> parameter('p',"port"          ,RsInitConfig::port           ,"port", "Set listenning port to use."              ,false)
			   >> parameter('c',"base-dir"      ,RsInitConfig::basedir        ,"directory", "Set base directory."                      ,false)
			   >> parameter('U',"user-id"       ,prefUserString               ,"ID", "[User Name/GPG id/SSL id] Sets Account to Use, Useful when Autologin is enabled",false)
			   >> parameter('r',"link"          ,RsInitConfig::RetroShareLink ,"retroshare://...", "Use a given Retroshare Link"              ,false)
#ifdef LOCALNET_TESTING
			   >> parameter('R',"restrict-port" ,portRestrictions             ,"port1-port2","Apply port restriction"                   ,false)
#endif
				>> help() ;

			as.defaultErrorHandling(true) ;

			if(RsInitConfig::autoLogin)         RsInitConfig::startMinimised = true ;
			if(RsInitConfig::outStderr)         RsInitConfig::haveLogFile    = false ;
			if(!RsInitConfig::logfname.empty()) RsInitConfig::haveLogFile    = true;
			if(RsInitConfig::inet != "127.0.0.1") RsInitConfig::forceLocalAddr = true;
#ifdef LOCALNET_TESTING
			if(!portRestrictions.empty())       doPortRestrictions           = true;
#endif

#ifdef SUSPENDED_CODE
#ifdef LOCALNET_TESTING
         while((c = getopt(argc, argv,"hesamui:p:c:w:l:d:U:r:R:")) != -1)
#else
         while((c = getopt(argc, argv,"hesamui:p:c:w:l:d:U:r:")) != -1)
#endif
         {
                 switch (c)
                 {
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
                                 std::cerr << "-r link           Use RetroShare link." << std::endl;
#ifdef LOCALNET_TESTING
                                 std::cerr << "-R <lport-uport>  Port Restrictions." << std::endl;
#endif
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
#endif

			setOutputLevel(RsInitConfig::debugLevel);

//	// set the default Debug Level...
//	if (RsInitConfig::haveDebugLevel)
//	{
//		if ((RsInitConfig::debugLevel > 0) &&
//			(RsInitConfig::debugLevel <= PQL_DEBUG_ALL))
//		{
//			std::cerr << "Setting Debug Level to: ";
//			std::cerr << RsInitConfig::debugLevel;
//			std::cerr << std::endl;
//		}
//		else
//		{
//			std::cerr << "Ignoring Invalid Debug Level: ";
//			std::cerr << RsInitConfig::debugLevel;
//			std::cerr << std::endl;
//		}
//	}

	// set the debug file.
	if (RsInitConfig::haveLogFile)
		setDebugFile(RsInitConfig::logfname.c_str());

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

	// Hash the main executable.
	
	uint64_t tmp_size ;
	std::string tmp_name ;

	if(!RsDirUtil::getFileHash(argv[0],RsInitConfig::main_executable_hash,tmp_size,NULL))
		std::cerr << "Cannot hash executable! Plugins will not be loaded correctly." << std::endl;
	else
		std::cerr << "Hashed main executable: " << RsInitConfig::main_executable_hash << std::endl;

	/* At this point we want to.
	 * 1) Load up Dase Directory.
	 * 3) Get Prefered Id.
	 * 2) Get List of Available Accounts.
	 * 4) Get List of GPG Accounts.
	 */
	/* create singletons */
	AuthSSL::AuthSSLInit();
	AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL);

	// first check config directories, and set bootstrap values.
	if(!setupBaseDir())
		return RS_INIT_BASE_DIR_ERROR ; 

	get_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);

	std::string pgp_dir = RsPGPDirectory() ;

	if(!RsDirUtil::checkCreateDirectory(pgp_dir))
		throw std::runtime_error("Cannot create pgp directory " + pgp_dir) ;

	AuthGPG::init(	pgp_dir + "/retroshare_public_keyring.gpg",
						pgp_dir + "/retroshare_secret_keyring.gpg",
						pgp_dir + "/retroshare_trustdb.gpg",
						pgp_dir + "/lock");

	/* Initialize AuthGPG */
	// if (AuthGPG::getAuthGPG()->InitAuth() == false) {
	// 	std::cerr << "AuthGPG::InitAuth failed" << std::endl;
	// 	return RS_INIT_AUTH_FAILED;
	// }

	//std::list<accountId> ids;
	std::list<accountId>::iterator it;
	int failing_accounts ;

	getAvailableAccounts(RsInitConfig::accountIds,failing_accounts,unsupported_keys);

	if(failing_accounts > 0 && RsInitConfig::accountIds.empty())
		return RS_INIT_NO_KEYRING ;

	// if a different user id has been passed to cmd line check for that instead

	std::string lower_case_user_string;
	stringToLowerCase(prefUserString, lower_case_user_string) ;
	std::string upper_case_user_string;
	stringToUpperCase(prefUserString, upper_case_user_string) ;

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
			return RS_INIT_AUTH_FAILED ;
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
			break;
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

bool RsInit::exportIdentity(const std::string& fname,const std::string& id)
{
	return AuthGPG::getAuthGPG()->exportProfile(fname,id);
}

bool RsInit::importIdentity(const std::string& fname,std::string& id,std::string& import_error)
{
	return AuthGPG::getAuthGPG()->importProfile(fname,id,import_error);
}

bool RsInit::copyGnuPGKeyrings()
{
	std::string pgp_dir = RsPGPDirectory() ;

	if(!RsDirUtil::checkCreateDirectory(pgp_dir))
		throw std::runtime_error("Cannot create pgp directory " + pgp_dir) ;

	std::string source_public_keyring;
	std::string source_secret_keyring;

#ifdef WINDOWS_SYS
	if (RsInit::isPortable())
	{
		source_public_keyring = RsInit::RsConfigDirectory() + "/gnupg/pubring.gpg";
		source_secret_keyring = RsInit::RsConfigDirectory() + "/gnupg/secring.gpg" ;
	} else {
		source_public_keyring = RsInitConfig::basedir + "/../gnupg/pubring.gpg" ;
		source_secret_keyring = RsInitConfig::basedir + "/../gnupg/secring.gpg" ;
	}
#else
	char *env_gnupghome = getenv("GNUPGHOME") ;

	if(env_gnupghome != NULL)
	{
		std::cerr << "looking into $GNUPGHOME/" << std::endl;

		source_public_keyring = std::string(env_gnupghome) + "/pubring.gpg" ;
		source_secret_keyring = std::string(env_gnupghome) + "/secring.gpg" ;
	}
	else
	{
		char *env_homedir = getenv("HOME") ;

		if(env_homedir != NULL)
		{
			std::cerr << "looking into $HOME/.gnupg/" << std::endl;
			std::string home_dir(env_homedir) ;

			// We need a specific part for MacOS and Linux as well
			source_public_keyring = home_dir + "/.gnupg/pubring.gpg" ;
			source_secret_keyring = home_dir + "/.gnupg/secring.gpg" ;
		}
		else
			return false ;
	}
#endif

	if(!RsDirUtil::copyFile(source_public_keyring,pgp_dir + "/retroshare_public_keyring.gpg"))
	{
		std::cerr << "Cannot copy pub keyring " << source_public_keyring << " to destination file " << pgp_dir + "/retroshare_public_keyring.gpg. If you believe your keyring is in a different place, please make the copy yourself." << std::endl;
		return false ;
	}
	if(!RsDirUtil::copyFile(source_secret_keyring,pgp_dir + "/retroshare_secret_keyring.gpg"))
	{
		std::cerr << "Cannot copy sec keyring " << source_secret_keyring << " to destination file " << pgp_dir + "/retroshare_secret_keyring.gpg. your keyring is in a different place, please make the copy yourself." << std::endl;
		return false ;
	}

	return true ;
}

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


bool     RsInit::getAccountDetails(const std::string &id,
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


bool RsInit::setupBaseDir()
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
			return false ;
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
				return false ;
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
		return false ;
	}
	return true ;
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
	/* NOTE: OSX also qualifies as BSD... so this #ifdef must be before the BSD check. */

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

  #elif (defined(BSD) && (BSD >= 199103))
	/* For BSD, the default is LOCALBASE which will be set
	 * before compilation via the ports/pkg-src mechanisms.
	 * For compilation without ports/pkg-src it is set to
	 * /usr/local (default on Open and Free; Net has /usr/pkg)
	 */
	dataDirectory = "/usr/local/share/retroshare";
	std::cerr << "getRetroshareDataDirectory() BSD: " << dataDirectory;

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


static bool isHexaString(const std::string& s)
{
	for(uint32_t i=0;i<s.length();++i)
		if(!((s[i] >= 'A' && s[i] <= 'F') || (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
			return false ;

	return true ;
}

/* directories with valid certificates in the expected location */
bool getAvailableAccounts(std::list<accountId> &ids,int& failing_accounts,std::map<std::string,std::vector<std::string> >& unsupported_keys)
{
	failing_accounts = 0 ;
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
		return false ;
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
		if(isHexaString(*it) && (*it).length() == 32)
		{
			std::string accountdir = RsInitConfig::basedir + "/" + *it;
#ifdef GPG_DEBUG
			std::cerr << "getAvailableAccounts() Checking: " << *it << std::endl;
#endif

			accountId tmpId;
			if (checkAccount(accountdir, tmpId,unsupported_keys))
			{
#ifdef GPG_DEBUG
				std::cerr << "getAvailableAccounts() Accepted: " << *it << std::endl;
#endif
				ids.push_back(tmpId);
			}
			else
				++failing_accounts ;
		}
#ifdef GPG_DEBUG
		else
			std::cerr << "Skipped non SSLid directory " << *it << std::endl;
#endif
	return true;
}



static bool checkAccount(std::string accountdir, accountId &id,std::map<std::string,std::vector<std::string> >& unsupported_keys)
{
	/* check if the cert/key file exists */

	std::string subdir1 = accountdir + "/";
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	// Create the filename.
	std::string basename = accountdir + "/";
	basename += configKeyDir + "/";
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
		std::cerr << "issuerName: " << id.pgpId << " id: " << id.sslId << std::endl;
#endif

		if(! RsInit::GetPGPLoginDetails(id.pgpId, id.pgpName, id.pgpEmail))
			return false ;

		if(!AuthGPG::getAuthGPG()->haveSecretKey(id.pgpId))
			return false ;

		if(!AuthGPG::getAuthGPG()->isKeySupported(id.pgpId))
		{
			std::string keystring = id.pgpId + " " + id.pgpName + "&#60;" + id.pgpEmail ;
			unsupported_keys[keystring].push_back("Location: " + id.location + "&nbsp;&nbsp;(" + id.sslId + ")") ;
			return false ;
		}

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

int      RsInit::GetPGPLoginDetails(const std::string& id, std::string &name, std::string &email)
{
        #ifdef GPG_DEBUG
        std::cerr << "RsInit::GetPGPLoginDetails for \"" << id << "\"" << std::endl;
        #endif

		  bool ok = true ;
        name = AuthGPG::getAuthGPG()->getGPGName(id,&ok);
		  if(!ok)
			  return 0 ;
        email = AuthGPG::getAuthGPG()->getGPGEmail(id,&ok);
		  if(!ok)
			  return 0 ;

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
	const std::string lockFile = accountDir + "/" + "lock";
	lockFilePath = lockFile;

	return RsDirUtil::createLockFile(lockFile,RsInitConfig::lockHandle) ;
}

/*
 * Unlock the currently locked profile, if there is one.
 * For Unix systems we simply close the handle of the lock file.
 */
void	RsInit::UnlockConfigDirectory()
{
	RsDirUtil::releaseLockFile(RsInitConfig::lockHandle) ;
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


bool     RsInit::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, std::string &pgpId, std::string &errString)
{
    return AuthGPG::getAuthGPG()->GeneratePGPCertificate(name, email, passwd, pgpId, errString);
}


                /* Create SSL Certificates */
bool     RsInit::GenerateSSLCertificate(const std::string& gpg_id, const std::string& org, const std::string& loc, const std::string& country, const std::string& passwd, std::string &sslId, std::string &errString)
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

	std::string tmpbase = RsInitConfig::basedir + "/" + tmpdir + "/";
	
	if(!RsInit::setupAccount(tmpbase))
		return false ;

	/* create directory structure */

	std::string basename = tmpbase + configKeyDir + "/";
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

	if (req == NULL)
	{
		fprintf(stderr,"RsGenerateCert() Couldn't create Request. Reason: %s\n", errString.c_str());
		return false;
	}

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
	std::string finalbase = RsInitConfig::basedir + "/" + sslId + "/";
	/* Rename Directory */

	std::cerr << "Mv Config Dir from: " << tmpbase << " to: " << finalbase;
	std::cerr << std::endl;

	if (!RsDirUtil::renameFile(tmpbase, finalbase))
	{
		std::cerr << "rename FAILED" << std::endl;
	}

	/* Flag as first time run */
	RsInitConfig::firsttime_run = true;

	std::cerr << "RetroShare has Successfully generated a Certficate/Key" << std::endl;
	std::cerr << "\tCert Located: " << cert_name << std::endl;
	std::cerr << "\tLocated: " << key_name << std::endl;

	return true;
}


/******************* PRIVATE FNS TO HELP with GEN **************/
bool RsInit::setupAccount(const std::string& accountdir)
{
	/* actual config directory isd */

	std::string subdir1 = accountdir + "/";
	std::string subdir2 = subdir1;
	subdir1 += configKeyDir;
	subdir2 += configCertDir;

	std::string subdir3 = accountdir + "/";
	subdir3 += "cache";

	std::string subdir4 = subdir3 + "/";
	std::string subdir5 = subdir3 + "/";
	subdir4 += "local";
	subdir5 += "remote";

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(accountdir))
	{
		std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir1))
	{
		std::cerr << "Cannot Create Config/Key Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir2))
	{
		std::cerr << "Cannot Create Config/Cert Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir3))
	{
		std::cerr << "Cannot Create Config/Cache Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir4))
	{
		std::cerr << "Cannot Create Config/Cache/local Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir5))
	{
		std::cerr << "Cannot Create Config/Cache/remote Dir" << std::endl;
		return false ;
	}

	return true;
}






/***************************** FINAL LOADING OF SETUP *************************/
                /* Login SSL */
bool     RsInit::LoadPassword(const std::string& id, const std::string& inPwd)
{
	/* select configDir */

	RsInitConfig::preferedId = id;
	RsInitConfig::configDir = RsInitConfig::basedir + "/" + id;
	RsInitConfig::passwd = inPwd;

	//	if(inPwd != "")
	//		RsInitConfig::havePasswd = true;

	// Create the filename.
	std::string basename = RsInitConfig::configDir + "/";
	basename += configKeyDir + "/";
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
	
	if(RsInitConfig::passwd == "") {
		if (RsLoginHandler::getSSLPassword(RsInitConfig::preferedId,true,RsInitConfig::passwd) == false) {
			std::cerr << "RsLoginHandler::getSSLPassword() Failed!";
			return 0 ;
		}
	} else {
		if (RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile(RsInitConfig::preferedId,RsInitConfig::passwd) == false) {
			std::cerr << "RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile() Failed!";
			return 0;
		}
	}

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

	// store pword to allow gxs use it to services' key their databases
	// ideally gxs should have its own password
	RsInitConfig::gxs_passwd = RsInitConfig::passwd;
	RsInitConfig::passwd = "";
	create_configinit(RsInitConfig::basedir, RsInitConfig::preferedId);
      
	return 1;
}

bool RsInit::RsClearAutoLogin()
{
	return	RsLoginHandler::clearAutoLogin(RsInitConfig::preferedId);
}

bool	RsInit::get_configinit(const std::string& dir, std::string &id)
{
	// have a config directories.

	// Check for config file.
	std::string initfile = dir + "/";
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

bool	RsInit::create_configinit(const std::string& dir, const std::string& id)
{
	// Check for config file.
	std::string initfile = dir + "/";
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

std::string RsInit::getHomePath()
{
	std::string home;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */

	home = getenv("HOME");

#else /* Windows */

	char *h2 = getenv("HOMEDRIVE");
	std::cerr << "getHomePath() -> $HOMEDRIVE = " << h2 << std::endl;
	char *h3 = getenv("HOMEPATH");
	std::cerr << "getHomePath() -> $HOMEPATH = " << h3 << std::endl;

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

	std::cerr << "fltkserver::getHomePath() -> " << home << std::endl;

	// convert to FLTK desired format.
	home = RsDirUtil::convertPathToUnix(home);
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return home;
}

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

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
    return RsInitConfig::basedir + "/" + RsInitConfig::preferedId + "/" + configKeyDir ;
}
std::string RsInit::RsConfigDirectory()
{
	return RsInitConfig::basedir;
}
std::string RsInit::RsPGPDirectory()
{
	return RsInitConfig::basedir + "/pgp" ;
}

std::string RsInit::RsProfileConfigDirectory()
{
    std::string dir = RsInitConfig::basedir + "/" + RsInitConfig::preferedId;
    //std::cerr << "RsInit::RsProfileConfigDirectory() returning : " << dir << std::endl;
    return dir;
}

bool RsInit::getStartMinimised()
{
	return RsInitConfig::startMinimised;
}

std::string RsInit::getRetroShareLink()
{
	return RsInitConfig::RetroShareLink;
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
#include "pqi/p3historymgr.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsrandom.h"

#ifdef RS_ENABLE_ZEROCONF
	#include "zeroconf/p3zeroconf.h"
#endif

#ifdef RS_ENABLE_ZCNATASSIST
	#include "zeroconf/p3zcnatassist.h"
#else
        #ifdef RS_USE_LIBUPNP
		#include "upnp/upnphandler_linux.h"
	#else
		#include "upnp/upnphandler_miniupnp.h"
        #endif
#endif

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3statusservice.h"
#include "turtle/p3turtle.h"

#ifdef RS_ENABLE_GXS
// NEW GXS SYSTEMS.
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsnetservice.h"
#include "retroshare/rsgxsflags.h"

#include "services/p3idservice.h"
#include "services/p3gxscircles.h"
#include "services/p3wiki.h"
#include "services/p3posted.h"
#include "services/p3photoservice.h"
#include "services/p3gxsforums.h"
#include "services/p3gxschannels.h"
#include "services/p3wire.h"

#endif // RS_ENABLE_GXS


#include <list>
#include <string>

// for blocking signals
#include <signal.h>

/* Implemented Rs Interfaces */
#include "rsserver/p3face.h"
#include "rsserver/p3peers.h"
#include "rsserver/p3msgs.h"
#include "rsserver/p3discovery.h"
#include "rsserver/p3status.h"
#include "rsserver/p3history.h"
#include "rsserver/p3serverconfig.h"


#include "pqi/p3notify.h" // HACK - moved to pqi for compilation order.

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"
	
	
#include "tcponudp/tou.h"
#include "tcponudp/rsudpstack.h"

	
#ifdef RS_USE_BITDHT
#include "dht/p3bitdht.h"
#include "dht/stunaddrassist.h"

#include "udp/udpstack.h"
#include "tcponudp/udppeer.h"
#include "tcponudp/udprelay.h"
#endif

/****
 * #define RS_RELEASE 		1
****/

#define RS_RELEASE 	1

#include "services/p3banlist.h"
#include "services/p3bwctrl.h"
#include "services/p3dsdv.h"


RsControl *createRsControl(NotifyBase &notify)
{
	RsServer *srv = new RsServer(notify);
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
		return false ;
	}

	std::string ownId = AuthSSL::getAuthSSL()->OwnId();

	/**************************************************************************/
	/* Any Initial Configuration (Commandline Options)  */
	/**************************************************************************/

	/* set the debugging to crashMode */
	std::cerr << "set the debugging to crashMode." << std::endl;
	if ((!RsInitConfig::haveLogFile) && (!RsInitConfig::outStderr))
	{
		std::string crashfile = RsInitConfig::basedir + "/";
		crashfile += ownId + "/" + configLogFileName;
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



	/* History Manager */
	mHistoryMgr = new p3HistoryMgr();
	mPeerMgr = new p3PeerMgrIMPL( AuthSSL::getAuthSSL()->OwnId(),
											AuthGPG::getAuthGPG()->getGPGOwnId(),
											AuthGPG::getAuthGPG()->getGPGOwnName(),
											AuthSSL::getAuthSSL()->getOwnLocation());
	mNetMgr = new p3NetMgrIMPL();
	mLinkMgr = new p3LinkMgrIMPL(mPeerMgr, mNetMgr);

	/* Setup Notify Early - So we can use it. */
	rsNotify = new p3Notify();
	rsPeers = new p3Peers(mLinkMgr, mPeerMgr, mNetMgr);

	mPeerMgr->setManagers(mLinkMgr, mNetMgr);
	mNetMgr->setManagers(mPeerMgr, mLinkMgr);

	//load all the SSL certs as friends
	//        std::list<std::string> sslIds;
	//        AuthSSL::getAuthSSL()->getAuthenticatedList(sslIds);
	//        for (std::list<std::string>::iterator sslIdsIt = sslIds.begin(); sslIdsIt != sslIds.end(); sslIdsIt++) {
	//            mConnMgr->addFriend(*sslIdsIt);
	//        }
	//p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr, RsInitConfig::configDir);
	/**************************** BITDHT ***********************************/

	// Make up an address. XXX

	struct sockaddr_in tmpladdr;
	sockaddr_clear(&tmpladdr);
	tmpladdr.sin_port = htons(RsInitConfig::port);


#ifdef LOCALNET_TESTING

	rsUdpStack *mDhtStack = new rsUdpStack(UDP_TEST_RESTRICTED_LAYER, tmpladdr);

	/* parse portRestrictions */
	unsigned int lport, uport;

	if (doPortRestrictions)
	{
		if (2 == sscanf(portRestrictions.c_str(), "%u-%u", &lport, &uport))
		{
			std::cerr << "Adding Port Restriction (" << lport << "-" << uport << ")";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "Failed to parse Port Restrictions ... exiting";
			std::cerr << std::endl;
			exit(1);
		}

		RestrictedUdpLayer *url = (RestrictedUdpLayer *) mDhtStack->getUdpLayer();
		url->addRestrictedPortRange(lport, uport);
	}
#else
	rsUdpStack *mDhtStack = new rsUdpStack(tmpladdr);
#endif

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

	if (!RsDirUtil::checkFile(bootstrapfile,true))
	{
		std::cerr << "DHT bootstrap file not in ConfigDir: " << bootstrapfile << std::endl;
		std::string installfile = RsInit::getRetroshareDataDirectory();
		installfile += "/";
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

	/* construct the rest of the stack, important to build them in the correct order! */
	/* MOST OF THIS IS COMMENTED OUT UNTIL THE REST OF libretroshare IS READY FOR IT! */

	UdpSubReceiver *udpReceivers[RSUDP_NUM_TOU_RECVERS];
	int udpTypes[RSUDP_NUM_TOU_RECVERS];

	// FIRST DHT STUNNER.
	UdpStunner *mDhtStunner = new UdpStunner(mDhtStack);
	mDhtStunner->setTargetStunPeriod(300); /* slow (5mins) */
	mDhtStack->addReceiver(mDhtStunner);

#ifdef LOCALNET_TESTING
	mDhtStunner->SetAcceptLocalNet();
#endif

	// NEXT BITDHT.
	p3BitDht *mBitDht = new p3BitDht(ownId, mLinkMgr, mNetMgr, mDhtStack, bootstrapfile);
	/* install external Pointer for Interface */
	rsDht = mBitDht;

	// NEXT THE RELAY (NEED to keep a reference for installing RELAYS)
	UdpRelayReceiver *mRelay = new UdpRelayReceiver(mDhtStack); 
	udpReceivers[RSUDP_TOU_RECVER_RELAY_IDX] = mRelay; /* RELAY Connections (DHT Port) */
	udpTypes[RSUDP_TOU_RECVER_RELAY_IDX] = TOU_RECEIVER_TYPE_UDPRELAY;
	mDhtStack->addReceiver(udpReceivers[RSUDP_TOU_RECVER_RELAY_IDX]);

	// LAST ON THIS STACK IS STANDARD DIRECT TOU
	udpReceivers[RSUDP_TOU_RECVER_DIRECT_IDX] = new UdpPeerReceiver(mDhtStack);  /* standard DIRECT Connections (DHT Port) */
	udpTypes[RSUDP_TOU_RECVER_DIRECT_IDX] = TOU_RECEIVER_TYPE_UDPPEER;
	mDhtStack->addReceiver(udpReceivers[RSUDP_TOU_RECVER_DIRECT_IDX]);

	// NOW WE BUILD THE SECOND STACK.
	// Create the Second UdpStack... Port should be random (but openable!).

	//#define MIN_RANDOM_PORT 30000
	//#define MAX_RANDOM_PORT 50000

	struct sockaddr_in sndladdr;
	sockaddr_clear(&sndladdr);

	// #ifdef LOCALNET_TESTING
	// 	// HACK Proxy Port near Dht Port - For Relay Testing.
	// 	uint16_t rndport = RsInitConfig::port + 3;
	// 	sndladdr.sin_port = htons(rndport);
	// #else
	// 	uint16_t rndport = MIN_RANDOM_PORT + RSRandom::random_u32() % (MAX_RANDOM_PORT - MIN_RANDOM_PORT);
	// #endif

#ifdef LOCALNET_TESTING

	rsFixedUdpStack *mProxyStack = new rsFixedUdpStack(UDP_TEST_RESTRICTED_LAYER, sndladdr);

	/* portRestrictions already parsed */
	if (doPortRestrictions)
	{
		RestrictedUdpLayer *url = (RestrictedUdpLayer *) mProxyStack->getUdpLayer();
		url->addRestrictedPortRange(lport, uport);
	}
#else
	rsFixedUdpStack *mProxyStack = new rsFixedUdpStack(sndladdr);
#endif

	// FIRSTLY THE PROXY STUNNER.
	UdpStunner *mProxyStunner = new UdpStunner(mProxyStack);
	mProxyStunner->setTargetStunPeriod(300); /* slow (5mins) */
	mProxyStack->addReceiver(mProxyStunner);

#ifdef LOCALNET_TESTING
	mProxyStunner->SetAcceptLocalNet();
#endif


	// FINALLY THE PROXY UDP CONNECTIONS
	udpReceivers[RSUDP_TOU_RECVER_PROXY_IDX] = new UdpPeerReceiver(mProxyStack); /* PROXY Connections (Alt UDP Port) */	
	udpTypes[RSUDP_TOU_RECVER_PROXY_IDX] = TOU_RECEIVER_TYPE_UDPPEER;	
	mProxyStack->addReceiver(udpReceivers[RSUDP_TOU_RECVER_PROXY_IDX]);

	// REAL INITIALISATION - WITH THREE MODES
	tou_init((void **) udpReceivers, udpTypes, RSUDP_NUM_TOU_RECVERS);

	mBitDht->setupConnectBits(mDhtStunner, mProxyStunner, mRelay);

	mNetMgr->setAddrAssist(new stunAddrAssist(mDhtStunner), new stunAddrAssist(mProxyStunner));
#else
	/* install NULL Pointer for rsDht Interface */
	rsDht = NULL;
#endif


	/**************************** BITDHT ***********************************/


	SecurityPolicy *none = secpolicy_create();
	pqih = new pqisslpersongrp(none, flags, mPeerMgr);
	//pqih = new pqipersongrpDummy(none, flags);

	/****** New Ft Server **** !!! */
	ftserver = new ftServer(mPeerMgr, mLinkMgr);
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

	/* create Cache Services */
	std::string config_dir = RsInitConfig::configDir;
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";

	std::vector<std::string> plugins_directories ;

#ifndef WINDOWS_SYS
	plugins_directories.push_back(std::string("/usr/lib/retroshare/extensions/")) ;
#endif
	plugins_directories.push_back(RsInitConfig::basedir + "/extensions/") ;
#ifdef DEBUG_PLUGIN_SYSTEM
	plugins_directories.push_back(".") ;	// this list should be saved/set to some correct value.
	// possible entries include: /usr/lib/retroshare, ~/.retroshare/extensions/, etc.
#endif

	mPluginsManager = new RsPluginManager(RsInitConfig::main_executable_hash) ;
	rsPlugins  = mPluginsManager ;
	mConfigMgr->addConfiguration("plugins.cfg", mPluginsManager);

	mPluginsManager->loadConfiguration() ;

	// These are needed to load plugins: plugin devs might want to know the place of
	// cache directories, get pointers to cache strapper, or access ownId()
	//
	mPluginsManager->setCacheDirectories(localcachedir,remotecachedir) ;
	mPluginsManager->setFileServer(ftserver) ;
	mPluginsManager->setLinkMgr(mLinkMgr) ;

	// Now load the plugins. This parses the available SO/DLL files for known symbols.
	//
	mPluginsManager->loadPlugins(plugins_directories) ;

	// Also load some plugins explicitly. This is helpful for
	// - developping plugins 
	//
	std::vector<RsPlugin *> programatically_inserted_plugins ;		

	// Push your own plugins into this list, before the call:
	//
	// 	programatically_inserted_plugins.push_back(myCoolPlugin) ;
	//
	mPluginsManager->loadPlugins(programatically_inserted_plugins) ;

	// set interfaces for plugins
	RsPlugInInterfaces interfaces;
	interfaces.mFiles = rsFiles;
	interfaces.mPeers = rsPeers;
	mPluginsManager->setInterfaces(interfaces);

	/* create Services */
	ad = new p3disc(mPeerMgr, mLinkMgr, mNetMgr, pqih);
	msgSrv = new p3MsgService(mLinkMgr);
	chatSrv = new p3ChatService(mLinkMgr, mHistoryMgr);
	mStatusSrv = new p3StatusService(mLinkMgr);

	p3turtle *tr = new p3turtle(mLinkMgr) ;
	rsTurtle = tr ;
	pqih -> addService(tr);

	ftserver->connectToTurtleRouter(tr) ;
	chatSrv->connectToTurtleRouter(tr) ;
	msgSrv->connectToTurtleRouter(tr) ;

	pqih -> addService(ad);
	pqih -> addService(msgSrv);
	pqih -> addService(chatSrv);
	pqih ->addService(mStatusSrv);

	// now add plugin objects inside the loop:
	// 	- client services provided by plugins.
	// 	- cache services provided by plugins.
	//
	mPluginsManager->registerClientServices(pqih) ;
	mPluginsManager->registerCacheServices() ;

#ifdef RS_ENABLE_GXS

        // The idea is that if priorGxsDir is non
        // empty and matches an exist directory location
        // the given ssl user id then this directory is cleaned
        // and deleted
        std::string priorGxsDir = "./" + mLinkMgr->getOwnId() + "/";
	std::string currGxsDir = RsInitConfig::configDir + "/GXS_phase2";

#ifdef GXS_DEV_TESTNET // Different Directory for testing.
	currGxsDir += "_TESTNET5";
#endif

        bool cleanUpGxsDir = false;

        if(!priorGxsDir.empty())
            cleanUpGxsDir = RsDirUtil::checkDirectory(priorGxsDir);

        std::list<std::string> filesToKeep;
        bool cleanUpSuccess = RsDirUtil::cleanupDirectory(priorGxsDir, filesToKeep);

        if(!cleanUpSuccess)
            std::cerr << "RsInit::StartupRetroShare() Clean up of Old Gxs Dir Failed!";
        else
            rmdir(priorGxsDir.c_str());

        // TODO: temporary to store GXS service data, remove
        RsDirUtil::checkCreateDirectory(currGxsDir);

        RsNxsNetMgr* nxsMgr =  new RsNxsNetMgrImpl(mLinkMgr);

        /**** Identity service ****/

        RsGeneralDataService* gxsid_ds = new RsDataService(currGxsDir + "/", "gxsid_db",
                        RS_SERVICE_GXSV2_TYPE_GXSID, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        gxsid_ds->resetDataStore(); 
#endif

        // init gxs services
        mGxsIdService = new p3IdService(gxsid_ds, NULL);

        RsGeneralDataService* gxscircles_ds = new RsDataService(currGxsDir + "/", "gxscircles_db",
                        RS_SERVICE_GXSV2_TYPE_GXSCIRCLE, NULL, RsInitConfig::gxs_passwd);

        mGxsCircles = new p3GxsCircles(gxscircles_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* gxsid_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_GXSID, gxsid_ds, nxsMgr,
                        mGxsIdService, mGxsIdService, mGxsCircles);

        /**** GxsCircle service ****/




#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        gxscircles_ds->resetDataStore(); 
#endif

        // init gxs services
        mGxsCircles = new p3GxsCircles(gxscircles_ds, NULL, mGxsIdService);

        // create GXS Circle service
        RsGxsNetService* gxscircles_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_GXSCIRCLE, gxscircles_ds, nxsMgr,
                        mGxsCircles, mGxsIdService, mGxsCircles);


        /**** Photo service ****/
        RsGeneralDataService* photo_ds = new RsDataService(currGxsDir + "/", "photoV2_db",
                        RS_SERVICE_GXSV2_TYPE_PHOTO, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        photo_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        // init gxs services
        mPhoto = new p3PhotoService(photo_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* photo_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_PHOTO, photo_ds, nxsMgr, mPhoto, mGxsIdService, mGxsCircles);

        /**** Posted GXS service ****/



        RsGeneralDataService* posted_ds = new RsDataService(currGxsDir + "/", "posted_db",
                                                            RS_SERVICE_GXSV2_TYPE_POSTED, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        posted_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        mPosted = new p3Posted(posted_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* posted_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_POSTED, posted_ds, nxsMgr, mPosted, mGxsIdService, mGxsCircles);


        /**** Wiki GXS service ****/



        RsGeneralDataService* wiki_ds = new RsDataService(currGxsDir + "/", "wiki_db",
                                                            RS_SERVICE_GXSV2_TYPE_WIKI,
                                                            NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        wiki_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        mWiki = new p3Wiki(wiki_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* wiki_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_WIKI, wiki_ds, nxsMgr, mWiki, mGxsIdService, mGxsCircles);


        /**** Wire GXS service ****/


        RsGeneralDataService* wire_ds = new RsDataService(currGxsDir + "/", "wire_db",
                                                            RS_SERVICE_GXSV2_TYPE_WIRE, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        wire_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        mWire = new p3Wire(wire_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* wire_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_WIRE, wire_ds, nxsMgr, mWire, mGxsIdService, mGxsCircles);


        /**** Forum GXS service ****/

        RsGeneralDataService* gxsforums_ds = new RsDataService(currGxsDir + "/", "gxsforums_db",
                                                            RS_SERVICE_GXSV2_TYPE_FORUMS, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
        gxsforums_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        mGxsForums = new p3GxsForums(gxsforums_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* gxsforums_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_FORUMS, gxsforums_ds, nxsMgr,
                        mGxsForums, mGxsIdService, mGxsCircles);


        /**** Channel GXS service ****/

        RsGeneralDataService* gxschannels_ds = new RsDataService(currGxsDir + "/", "gxschannels_db",
                                                            RS_SERVICE_GXSV2_TYPE_CHANNELS, NULL, RsInitConfig::gxs_passwd);

#ifndef GXS_DEV_TESTNET // NO RESET, OR DUMMYDATA for TESTNET
       gxschannels_ds->resetDataStore(); //TODO: remove, new service data per RS session, for testing
#endif

        mGxsChannels = new p3GxsChannels(gxschannels_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* gxschannels_ns = new RsGxsNetService(
                        RS_SERVICE_GXSV2_TYPE_CHANNELS, gxschannels_ds, nxsMgr,
                        mGxsChannels, mGxsIdService, mGxsCircles);

        // now add to p3service
        pqih->addService(gxsid_ns);
        pqih->addService(gxscircles_ns);
        pqih->addService(photo_ns);
        pqih->addService(posted_ns);
        pqih->addService(wiki_ns);
        pqih->addService(gxsforums_ns);
        pqih->addService(gxschannels_ns);

        // remove pword from memory
        RsInitConfig::gxs_passwd = "";

#endif // RS_ENABLE_GXS.


#ifdef RS_VOIPTEST
	p3VoRS *mVoipTest = new p3VoRS(mLinkMgr);
	pqih -> addService(mVoipTest);
	rsVoip = mVoipTest;
#endif

	// new services to test.
	p3BanList *mBanList = new p3BanList(mLinkMgr, mNetMgr);
	pqih -> addService(mBanList);
	mBitDht->setupPeerSharer(mBanList);

	p3BandwidthControl *mBwCtrl = new p3BandwidthControl(pqih);
	pqih -> addService(mBwCtrl); 

#ifdef RS_DSDVTEST
	p3Dsdv *mDsdv = new p3Dsdv(mLinkMgr);
	pqih -> addService(mDsdv);
	rsDsdv = mDsdv;
	mDsdv->addTestService();
#endif

	/**************************************************************************/

#ifdef RS_USE_BITDHT
	mNetMgr->addNetAssistConnect(1, mBitDht);
	mNetMgr->addNetListener(mDhtStack); 
	mNetMgr->addNetListener(mProxyStack); 

#endif

#ifdef RS_ENABLE_ZEROCONF
	p3ZeroConf *mZeroConf = new p3ZeroConf(
			AuthGPG::getAuthGPG()->getGPGOwnId(), ownId, 
			mLinkMgr, mNetMgr, mPeerMgr);
	mNetMgr->addNetAssistConnect(2, mZeroConf);
	mNetMgr->addNetListener(mZeroConf); 
#endif

#ifdef RS_ENABLE_ZCNATASSIST
	// Apple's UPnP & NAT-PMP assistance.
	p3zcNatAssist *mZcNatAssist = new p3zcNatAssist();
	mNetMgr->addNetAssistFirewall(1, mZcNatAssist);
#else
	// Original UPnP Interface.
	pqiNetAssistFirewall *mUpnpMgr = new upnphandler();
	mNetMgr->addNetAssistFirewall(1, mUpnpMgr);
#endif

	/**************************************************************************/
	/* need to Monitor too! */
	mLinkMgr->addMonitor(pqih);
	mLinkMgr->addMonitor(mCacheStrapper);
	//mLinkMgr->addMonitor(ad);
	mLinkMgr->addMonitor(msgSrv);
	mLinkMgr->addMonitor(mStatusSrv);
	mLinkMgr->addMonitor(chatSrv);
	mLinkMgr->addMonitor(mBwCtrl);

	/* must also add the controller as a Monitor...
	 * a little hack to get it to work.
	 */
	mLinkMgr->addMonitor(((ftController *) mCacheTransfer));


	/**************************************************************************/

	//mConfigMgr->addConfiguration("ftserver.cfg", ftserver);
	//
	mConfigMgr->addConfiguration("gpg_prefs.cfg", AuthGPG::getAuthGPG());
	mConfigMgr->loadConfiguration();

	mConfigMgr->addConfiguration("peers.cfg", mPeerMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("chat.cfg", chatSrv);
	mConfigMgr->addConfiguration("p3History.cfg", mHistoryMgr);
	mConfigMgr->addConfiguration("p3Status.cfg", mStatusSrv);
	mConfigMgr->addConfiguration("turtle.cfg", tr);
	//mConfigMgr->addConfiguration("p3disc.cfg", ad);

#ifdef RS_USE_BITDHT
	mConfigMgr->addConfiguration("bitdht.cfg", mBitDht);
#endif

	mPluginsManager->addConfigurations(mConfigMgr) ;

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
	p3ServerConfig *serverConfig = new p3ServerConfig(mPeerMgr, mLinkMgr, mNetMgr, pqih, mGeneralConfig);
	serverConfig->load_config();

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
		laddr.sin_addr.s_addr = inet_addr(RsInitConfig::inet.c_str());

		mPeerMgr->setLocalAddress(ownId, laddr);
	}

	if (RsInitConfig::forceExtPort)
	{
		mPeerMgr->setOwnNetworkMode(RS_NET_MODE_EXT);
		mPeerMgr->setOwnVisState(RS_VIS_STATE_STD);

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

	mNetMgr -> checkNetAddress();

	/**************************************************************************/
	/* startup (stuff dependent on Ids/peers is after this point) */
	/**************************************************************************/

	pqih->init_listener();
	mNetMgr->addNetListener(pqih); /* add listener so we can reset all sockets later */



	/**************************************************************************/
	/* load caches and secondary data */
	/**************************************************************************/

	// Clear the News Feeds that are generated by Initial Cache Loading.

	/* Peer stuff is up to date */

	//getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_CHAT_NEW);
	getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_MESSAGE);
	//getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FILES_NEW);

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

#ifdef RS_ENABLE_GXS

	// Must Set the GXS pointers before starting threads.
	rsIdentity = mGxsIdService;
	rsGxsCircles = mGxsCircles;
	rsWiki = mWiki;
	rsPosted = mPosted;
	rsPhoto = mPhoto;
	rsGxsForums = mGxsForums;
	rsGxsChannels = mGxsChannels;
	rsWire = mWire;

	/*** start up GXS core runner ***/
	createThread(*mGxsIdService);
	createThread(*mGxsCircles);
	createThread(*mPhoto);
	createThread(*mPosted);
	createThread(*mWiki);
	createThread(*mWire);
	createThread(*mGxsForums);
	createThread(*mGxsChannels);

	// cores ready start up GXS net servers
	createThread(*gxsid_ns);
	createThread(*gxscircles_ns);
	createThread(*photo_ns);
	createThread(*posted_ns);
	createThread(*wiki_ns);
	createThread(*wire_ns);
	createThread(*gxsforums_ns);
	createThread(*gxschannels_ns);


#endif // RS_ENABLE_GXS

	ftserver->StartupThreads();
	ftserver->ResumeTransfers();

	//mDhtMgr->start();
#ifdef RS_USE_BITDHT
	mBitDht->start();
#endif

	/**************************************************************************/

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback(ownId);

	mod -> peerid = ownId;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

	rsDisc  = new p3Discovery(ad);
	rsBandwidthControl = mBwCtrl;
	rsConfig = serverConfig;

	rsMsgs  = new p3Msgs(msgSrv, chatSrv);
	
	rsStatus = new p3Status(mStatusSrv);
	rsHistory = new p3History(mHistoryMgr);

	/* put a welcome message in! */
	if (RsInitConfig::firsttime_run)
	{
		msgSrv->loadWelcomeMsg();
		ftserver->shareDownloadDirectory(true);
	}

	// load up the help page
	std::string helppage = RsInitConfig::basedir + "/";
	helppage += configHelpName;

	/* Startup this thread! */
	createThread(*this);

	return 1;
}
