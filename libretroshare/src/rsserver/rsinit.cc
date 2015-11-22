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
#include "rsserver/rsaccounts.h"

#include <list>
#include <string>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/rand.h>
#include <fcntl.h>

#define ENABLE_GROUTER

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

// for blocking signals
#include <signal.h>

#include <openssl/ssl.h>

#include "pqi/authssl.h"
#include "pqi/sslfns.h"
#include "pqi/authgpg.h"

#ifdef ENABLE_GROUTER
#include "grouter/p3grouter.h"
#endif

#include "tcponudp/udpstunner.h"

// #define GPG_DEBUG
// #define AUTHSSL_DEBUG
// #define FIM_DEBUG

//std::map<std::string,std::vector<std::string> > RsInit::unsupported_keys ;

class RsInitConfig 
{
	public:

		RsFileHash main_executable_hash;

#ifdef WINDOWS_SYS
		bool portable;
		bool isWindowsXP;
#endif
		rs_lock_handle_t lockHandle;

		std::string passwd;
		std::string gxs_passwd;

		bool autoLogin;                  /* autoLogin allowed */
		bool startMinimised; 		/* Icon or Full Window */

		/* Key Parameters that must be set before
		 * RetroShare will start up:
		 */

		/* Listening Port */
		bool forceExtPort;
		bool forceLocalAddr;
		unsigned short port;
		std::string inet ;

		/* v0.6 features */
		bool        hiddenNodeSet;
		std::string hiddenNodeAddress;
		uint16_t    hiddenNodePort;

		/* Logging */
		bool haveLogFile;
		bool outStderr;
		int  debugLevel;
		std::string logfname;

		bool load_trustedpeer;
		std::string load_trustedpeer_file;

		bool udpListenerOnly;

		std::string RetroShareLink;
};

static RsInitConfig *rsInitConfig = NULL;

const int p3facestartupzone = 47238;

// initial configuration bootstrapping...
//static const std::string configInitFile = "default_cert.txt";
//static const std::string configConfFile = "config.rs";
//static const std::string configCertDir = "friends";
//static const std::string configKeyDir = "keys";
//static const std::string configCaFile = "cacerts.pem";
//static const std::string configHelpName = "retro.htm";

static const std::string configLogFileName = "retro.log";
static const int SSLPWD_LEN = 64;

void RsInit::InitRsConfig()
{
	rsInitConfig = new RsInitConfig ;

	/* Directories */
#ifdef WINDOWS_SYS
	rsInitConfig->portable = false;
	rsInitConfig->isWindowsXP = false;
#endif
	/* v0.6 features */
	rsInitConfig->hiddenNodeSet = false;


#ifndef WINDOWS_SYS
	rsInitConfig->lockHandle = -1;
#else
	rsInitConfig->lockHandle = NULL;
#endif


	rsInitConfig->load_trustedpeer = false;
	rsInitConfig->port = 0 ;
	rsInitConfig->forceLocalAddr = false;
	rsInitConfig->haveLogFile    = false;
	rsInitConfig->outStderr      = false;
	rsInitConfig->forceExtPort   = false;

	rsInitConfig->inet = std::string("127.0.0.1");

	rsInitConfig->autoLogin      = false; // .
	rsInitConfig->startMinimised = false;
	rsInitConfig->passwd         = "";
	rsInitConfig->debugLevel	= PQL_WARNING;
	rsInitConfig->udpListenerOnly = false;

	/* setup the homePath (default save location) */
	//	rsInitConfig->homePath = getHomePath();

#ifdef WINDOWS_SYS
	// test for portable version
	if (GetFileAttributes(L"portable") != (DWORD) -1) {
		// use portable version
		rsInitConfig->portable = true;
	}

	// test for Windows XP
	OSVERSIONINFOEX osvi;
	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (GetVersionEx((OSVERSIONINFO*) &osvi)) {
		if (osvi.dwMajorVersion == 5) {
			if (osvi.dwMinorVersion == 1) {
				/* Windows XP */
				rsInitConfig->isWindowsXP = true;
			} else if (osvi.dwMinorVersion == 2) {
				SYSTEM_INFO si;
				memset(&si, 0, sizeof(si));
				GetSystemInfo(&si);
				if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
					/* Windows XP Professional x64 Edition */
					rsInitConfig->isWindowsXP = true;
				}
			}
		}
	}

	if (rsInitConfig->isWindowsXP) {
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

	std::string prefUserString = "";
	std::string opt_base_dir;

         /* getopt info: every availiable option is listed here. if it is followed by a ':' it
            needs an argument. If it is followed by a '::' the argument is optional.
         */
			//rsInitConfig->logfname = "" ;
			//rsInitConfig->inet = "" ;

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


			as >> option('a',"auto-login"       ,rsInitConfig->autoLogin      ,"AutoLogin (Windows Only) + StartMinimised")
			   >> option('m',"minimized"        ,rsInitConfig->startMinimised ,"Start minimized."                         )
			   >> option('s',"stderr"           ,rsInitConfig->outStderr      ,"output to stderr instead of log file."    )
			   >> option('u',"udp"              ,rsInitConfig->udpListenerOnly,"Only listen to UDP."                      )
			   >> option('e',"external-port"    ,rsInitConfig->forceExtPort   ,"Use a forwarded external port."           )

			   >> parameter('l',"log-file"      ,rsInitConfig->logfname       ,"logfile"   ,"Set Log filename."                        ,false)
			   >> parameter('d',"debug-level"   ,rsInitConfig->debugLevel     ,"level"     ,"Set debug level."                         ,false)
			   >> parameter('w',"password"      ,rsInitConfig->passwd         ,"password"  ,"Set Login Password."                      ,false)
			   >> parameter('i',"ip-address"    ,rsInitConfig->inet           ,"nnn.nnn.nnn.nnn", "Set IP address to use."                   ,false)
			   >> parameter('p',"port"          ,rsInitConfig->port           ,"port", "Set listenning port to use."              ,false)
			   >> parameter('c',"base-dir"      ,opt_base_dir                 ,"directory", "Set base directory."                      ,false)
			   >> parameter('U',"user-id"       ,prefUserString               ,"ID", "[User Name/GPG id/SSL id] Sets Account to Use, Useful when Autologin is enabled",false)
			   >> parameter('r',"link"          ,rsInitConfig->RetroShareLink ,"retroshare://...", "Use a given Retroshare Link"              ,false)
#ifdef LOCALNET_TESTING
			   >> parameter('R',"restrict-port" ,portRestrictions             ,"port1-port2","Apply port restriction"                   ,false)
#endif
#ifdef __APPLE__
 				>> help('h',"help","Display this Help") ;
#else
				>> help() ;
#endif

			as.defaultErrorHandling(true) ;

			if(rsInitConfig->autoLogin)         rsInitConfig->startMinimised = true ;
			if(rsInitConfig->outStderr)         rsInitConfig->haveLogFile    = false ;
			if(!rsInitConfig->logfname.empty()) rsInitConfig->haveLogFile    = true;
			if(rsInitConfig->inet != "127.0.0.1") rsInitConfig->forceLocalAddr = true;
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

			setOutputLevel(rsInitConfig->debugLevel);

//	// set the default Debug Level...
//	if (rsInitConfig->haveDebugLevel)
//	{
//		if ((rsInitConfig->debugLevel > 0) &&
//			(rsInitConfig->debugLevel <= PQL_DEBUG_ALL))
//		{
//			std::cerr << "Setting Debug Level to: ";
//			std::cerr << rsInitConfig->debugLevel;
//			std::cerr << std::endl;
//		}
//		else
//		{
//			std::cerr << "Ignoring Invalid Debug Level: ";
//			std::cerr << rsInitConfig->debugLevel;
//			std::cerr << std::endl;
//		}
//	}

	// set the debug file.
	if (rsInitConfig->haveLogFile)
		setDebugFile(rsInitConfig->logfname.c_str());

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
		std::cerr << "RetroShare:: Successfully installed the SIGPIPE Block" << std::endl;
	}
	else
	{
		std::cerr << "RetroShare:: Failed to install the SIGPIPE Block" << std::endl;
	}
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// Hash the main executable.
	
	uint64_t tmp_size ;

	if(!RsDirUtil::getFileHash(argv[0],rsInitConfig->main_executable_hash,tmp_size,NULL))
		std::cerr << "Cannot hash executable! Plugins will not be loaded correctly." << std::endl;
	else
		std::cerr << "Hashed main executable: " << rsInitConfig->main_executable_hash << std::endl;

	/* At this point we want to.
	 * 1) Load up Dase Directory.
	 * 3) Get Prefered Id.
	 * 2) Get List of Available Accounts.
	 * 4) Get List of GPG Accounts.
	 */
	/* create singletons */
	AuthSSL::AuthSSLInit();
    AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL, "");

	rsAccounts = new RsAccountsDetail() ;

	// first check config directories, and set bootstrap values.
	if(!rsAccounts->setupBaseDirectory(opt_base_dir))
		return RS_INIT_BASE_DIR_ERROR ; 

	// Setup PGP stuff.
	std::string pgp_dir = rsAccounts->PathPGPDirectory();

	if(!RsDirUtil::checkCreateDirectory(pgp_dir))
		throw std::runtime_error("Cannot create pgp directory " + pgp_dir) ;

	AuthGPG::init(	pgp_dir + "/retroshare_public_keyring.gpg",
						pgp_dir + "/retroshare_secret_keyring.gpg",
						pgp_dir + "/retroshare_trustdb.gpg",
						pgp_dir + "/lock");

	// load Accounts.
	if (!rsAccounts->loadAccounts())
	{
		return RS_INIT_NO_KEYRING ;
	}

	// choose alternative account.
	if(prefUserString != "")
	{
		if (!rsAccounts->selectAccountByString(prefUserString))
		{
			std::cerr << "Invalid User name/GPG id/SSL id: not found in list";
			std::cerr << std::endl;
			return RS_INIT_AUTH_FAILED ;
		}
	}

	/* check that we have selected someone */
	RsPeerId preferredId;
	bool existingUser = rsAccounts->getPreferredAccountId(preferredId);

	if (existingUser)
	{
		if (rsInitConfig->passwd != "")
		{
			return RS_INIT_HAVE_ACCOUNT;
		}

		if(RsLoginHandler::getSSLPassword(preferredId,false,rsInitConfig->passwd))
		{
			RsInit::setAutoLogin(true);
			std::cerr << "Autologin has succeeded" << std::endl;
			return RS_INIT_HAVE_ACCOUNT;
		}
	}
	return RS_INIT_OK;
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

	return RsDirUtil::createLockFile(lockFile,rsInitConfig->lockHandle) ;
}

/*
 * Unlock the currently locked profile, if there is one.
 * For Unix systems we simply close the handle of the lock file.
 */
void	RsInit::UnlockConfigDirectory()
{
	RsDirUtil::releaseLockFile(rsInitConfig->lockHandle) ;
}




bool RsInit::collectEntropy(uint32_t n)
{
	RAND_seed(&n,4) ;

	return true ;
}

/***************************** FINAL LOADING OF SETUP *************************/


                /* Login SSL */
bool     RsInit::LoadPassword(const std::string& inPwd)
{
	rsInitConfig->passwd = inPwd;
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
	if (!rsAccounts->lockPreferredAccount())
	{
		return 3; // invalid PreferredAccount.
	}

	int retVal = 0;

	// Logic that used to be external to RsInit...
	RsPeerId accountId;
	if (!rsAccounts->getPreferredAccountId(accountId))
	{
		retVal = 3; // invalid PreferredAccount;
	}
	
	RsPgpId pgpId;
	std::string pgpName, pgpEmail, location;

	if (retVal == 0 && !rsAccounts->getAccountDetails(accountId, pgpId, pgpName, pgpEmail, location))
		retVal = 3; // invalid PreferredAccount;
		
	if (retVal == 0 && !rsAccounts->SelectPGPAccount(pgpId))
		retVal = 3; // PGP Error.
	
	if(retVal == 0)
		retVal = LockConfigDirectory(rsAccounts->PathAccountDirectory(), lockFilePath);

	if(retVal == 0 && LoadCertificates(autoLoginNT) != 1)
	{
		UnlockConfigDirectory();
		retVal = 3;
	}

	if(retVal != 0)
	{
		rsAccounts->unlockPreferredAccount();
	}

	return retVal;
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
	RsPeerId preferredId;
	if (!rsAccounts->getPreferredAccountId(preferredId))
	{
		std::cerr << "No Account Selected" << std::endl;
		return 0;
	}
		
	
	if (rsAccounts->PathCertFile() == "")
	{
	  std::cerr << "RetroShare needs a certificate" << std::endl;
	  return 0;
	}

	if (rsAccounts->PathKeyFile() == "")
	{
	  std::cerr << "RetroShare needs a key" << std::endl;
	  return 0;
	}

	//check if password is already in memory
	
	if(rsInitConfig->passwd == "") {
		if (RsLoginHandler::getSSLPassword(preferredId,true,rsInitConfig->passwd) == false) {
			std::cerr << "RsLoginHandler::getSSLPassword() Failed!";
			return 0 ;
		}
	} else {
		if (RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile(preferredId,rsInitConfig->passwd) == false) {
			std::cerr << "RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile() Failed!";
			return 0;
		}
	}

	std::cerr << "rsAccounts->PathKeyFile() : " << rsAccounts->PathKeyFile() << std::endl;

    if(0 == AuthSSL::getAuthSSL() -> InitAuth(rsAccounts->PathCertFile().c_str(), rsAccounts->PathKeyFile().c_str(), rsInitConfig->passwd.c_str(), rsAccounts->LocationName()))
	{
		std::cerr << "SSL Auth Failed!";
		return 0 ;
	}

	if(autoLoginNT)
	{
		std::cerr << "RetroShare will AutoLogin next time";
		std::cerr << std::endl;

		RsLoginHandler::enableAutoLogin(preferredId,rsInitConfig->passwd);
		rsInitConfig->autoLogin = true ;
	}

	/* wipe out password */

	// store pword to allow gxs use it to services' key their databases
	// ideally gxs should have its own password
	rsInitConfig->gxs_passwd = rsInitConfig->passwd;
	rsInitConfig->passwd = "";
	
	rsAccounts->storePreferredAccount();      
	return 1;
}

bool RsInit::RsClearAutoLogin()
{
	RsPeerId preferredId;
	if (!rsAccounts->getPreferredAccountId(preferredId))
	{
		std::cerr << "RsInit::RsClearAutoLogin() No Account Selected" << std::endl;
		return 0;
	}
	return	RsLoginHandler::clearAutoLogin(preferredId);
}


bool RsInit::isPortable()
{
#ifdef WINDOWS_SYS
    return rsInitConfig->portable;
#else
    return false;
#endif
}

bool RsInit::isWindowsXP()
{
#ifdef WINDOWS_SYS
    return rsInitConfig->isWindowsXP;
#else
    return false;
#endif
}
	
bool RsInit::getStartMinimised()
{
	return rsInitConfig->startMinimised;
}

std::string RsInit::getRetroShareLink()
{
	return rsInitConfig->RetroShareLink;
}

int RsInit::getSslPwdLen(){
	return SSLPWD_LEN;
}

bool RsInit::getAutoLogin(){
	return rsInitConfig->autoLogin;
}

void RsInit::setAutoLogin(bool autoLogin){
	rsInitConfig->autoLogin = autoLogin;
}

/* Setup Hidden Location; */
bool RsInit::SetHiddenLocation(const std::string& hiddenaddress, uint16_t port)
{
	/* parse the bugger (todo) */
	rsInitConfig->hiddenNodeSet = true;
	rsInitConfig->hiddenNodeAddress = hiddenaddress;
	rsInitConfig->hiddenNodePort = port;
	return true;
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

RsFiles *rsFiles = NULL;
RsTurtle *rsTurtle = NULL ;
RsReputations *rsReputations = NULL ;
#ifdef ENABLE_GROUTER
RsGRouter *rsGRouter = NULL ;
#endif

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
	
#include "services/p3gxsreputation.h"
#include "services/p3serviceinfo.h"
#include "services/p3heartbeat.h"
#include "services/p3discovery2.h"
#include "services/p3msgservice.h"
#include "services/p3statusservice.h"

#include "turtle/p3turtle.h"
#include "chat/p3chatservice.h"

#ifdef RS_ENABLE_GXS
// NEW GXS SYSTEMS.
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsnetservice.h"
#include "retroshare/rsgxsflags.h"

#include "pgp/pgpauxutils.h"
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
 * #define RS_RTT           1
****/

#define RS_RELEASE      1
#define RS_RTT          1


#ifdef RS_RTT
#include "services/p3rtt.h"
#endif


#include "services/p3banlist.h"
#include "services/p3bwctrl.h"

#ifdef SERVICES_DSDV
#include "services/p3dsdv.h"
#endif

RsControl *RsControl::instance()
{
	static RsServer *rsicontrol = NULL ;

	if(rsicontrol == NULL) 
		rsicontrol = new RsServer();
	
	return rsicontrol;
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

    if (1 != AuthSSL::getAuthSSL() -> InitAuth(NULL, NULL, NULL, ""))
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Invalid Certificate configuration!" << std::endl;
		std::cerr << std::endl;
		return false ;
	}

	RsPeerId ownId = AuthSSL::getAuthSSL()->OwnId();

	/**************************************************************************/
	/* Any Initial Configuration (Commandline Options)  */
	/**************************************************************************/

	/* set the debugging to crashMode */
	std::cerr << "set the debugging to crashMode." << std::endl;
	if ((!rsInitConfig->haveLogFile) && (!rsInitConfig->outStderr))
	{
		std::string crashfile = rsAccounts->PathAccountDirectory();
		crashfile +=  "/" + configLogFileName;
		setDebugCrashMode(crashfile.c_str());
	}

	unsigned long flags = 0;
	if (rsInitConfig->udpListenerOnly)
	{
		flags |= PQIPERSON_NO_LISTENER;
	}

	/* check account directory */
	if (!rsAccounts->checkAccountDirectory())
	{
		std::cerr << "RsServer::StartupRetroShare() - Fatal Error....." << std::endl;
		std::cerr << "checkAccount failed!" << std::endl;
		std::cerr << std::endl;
		return 0;
	}

	/**************************************************************************/
	// Load up Certificates, and Old Configuration (if present)
	std::cerr << "Load up Certificates, and Old Configuration (if present)." << std::endl;

	std::string emergencySaveDir = rsAccounts->PathAccountDirectory();
	std::string emergencyPartialsDir = rsAccounts->PathAccountDirectory();
	if (emergencySaveDir != "")
	{
		emergencySaveDir += "/";
		emergencyPartialsDir += "/";
	}
	emergencySaveDir += "Downloads";
	emergencyPartialsDir += "Partials";

	/**************************************************************************/
	/* setup Configuration */
	/**************************************************************************/
	std::cerr << "Load Configuration" << std::endl;

	mConfigMgr = new p3ConfigMgr(rsAccounts->PathAccountDirectory());
	mGeneralConfig = new p3GeneralConfig();

	// Get configuration options from rsAccounts.
	bool isHiddenNode   = false;
	bool isFirstTimeRun = false;
	rsAccounts->getAccountOptions(isHiddenNode, isFirstTimeRun);

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
	rsPeers = new p3Peers(mLinkMgr, mPeerMgr, mNetMgr);

	mPeerMgr->setManagers(mLinkMgr, mNetMgr);
	mNetMgr->setManagers(mPeerMgr, mLinkMgr);
		
		
	//load all the SSL certs as friends
	//        std::list<std::string> sslIds;
	//        AuthSSL::getAuthSSL()->getAuthenticatedList(sslIds);
	//        for (std::list<std::string>::iterator sslIdsIt = sslIds.begin(); sslIdsIt != sslIds.end(); ++sslIdsIt) {
	//            mConnMgr->addFriend(*sslIdsIt);
	//        }
	//p3DhtMgr  *mDhtMgr  = new OpenDHTMgr(ownId, mConnMgr, rsInitConfig->configDir);
	/**************************** BITDHT ***********************************/

	// Make up an address. XXX

	struct sockaddr_in tmpladdr;
	sockaddr_clear(&tmpladdr);
	tmpladdr.sin_port = htons(rsInitConfig->port);


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
#define BITDHT_FILTERED_IP_FILENAME  	"bdfilter.txt"


	std::string bootstrapfile = rsAccounts->PathAccountDirectory();
	if (bootstrapfile != "")
		bootstrapfile += "/";
	bootstrapfile += BITDHT_BOOTSTRAP_FILENAME;

    std::string filteredipfile = rsAccounts->PathAccountDirectory();
    if (filteredipfile != "")
        filteredipfile += "/";
    filteredipfile += BITDHT_FILTERED_IP_FILENAME;

    std::cerr << "Checking for DHT bootstrap file: " << bootstrapfile << std::endl;

	/* check if bootstrap file exists...
	 * if not... copy from dataDirectory
	 */

	uint64_t tmp_size ;
	if (!RsDirUtil::checkFile(bootstrapfile,tmp_size,true))
	{
		std::cerr << "DHT bootstrap file not in ConfigDir: " << bootstrapfile << std::endl;
		std::string installfile = rsAccounts->PathDataDirectory();
		installfile += "/";
		installfile += BITDHT_BOOTSTRAP_FILENAME;

		std::cerr << "Checking for Installation DHT bootstrap file " << installfile << std::endl;
		if ((installfile != "") && (RsDirUtil::checkFile(installfile,tmp_size)))
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
    p3BitDht *mBitDht = new p3BitDht(ownId, mLinkMgr, mNetMgr, mDhtStack, bootstrapfile, filteredipfile);

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
	// We do this by binding to xx.xx.xx.xx:0 which which gives us a random port.

	struct sockaddr_in sndladdr;
	sockaddr_clear(&sndladdr);

#ifdef LOCALNET_TESTING

	// 	// HACK Proxy Port near Dht Port - For Relay Testing.
	// 	uint16_t rndport = rsInitConfig->port + 3;
	// 	sndladdr.sin_port = htons(rndport);

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

	p3ServiceControl *serviceCtrl = new p3ServiceControl(mLinkMgr);
	rsServiceControl = serviceCtrl;

    pqih = new pqisslpersongrp(serviceCtrl, flags, mPeerMgr);
	//pqih = new pqipersongrpDummy(none, flags);

	/****** New Ft Server **** !!! */
	ftServer *ftserver = new ftServer(mPeerMgr, serviceCtrl);
	ftserver->setConfigDirectory(rsAccounts->PathAccountDirectory());

	ftserver->SetupFtServer() ;
	CacheStrapper *mCacheStrapper = ftserver->getCacheStrapper();
	//CacheTransfer *mCacheTransfer = ftserver->getCacheTransfer();

	/* setup any extra bits (Default Paths) */
	ftserver->setPartialsDirectory(emergencyPartialsDir);
	ftserver->setDownloadDirectory(emergencySaveDir);

	/* This should be set by config ... there is no default */
	//ftserver->setSharedDirectories(fileList);

	rsFiles = ftserver;


	/* create Cache Services */
	std::string config_dir = rsAccounts->PathAccountDirectory();
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";

	std::vector<std::string> plugins_directories ;

#ifndef WINDOWS_SYS
	plugins_directories.push_back(std::string(PLUGIN_DIR)) ;
#endif
	std::string extensions_dir = rsAccounts->PathBaseDirectory() + "/extensions6/" ;
	plugins_directories.push_back(extensions_dir) ;

	if(!RsDirUtil::checkCreateDirectory(extensions_dir))
		std::cerr << "(EE) Cannot create extensions directory " + extensions_dir + ". This is not mandatory, but you probably have a permission problem." << std::endl;

#ifdef DEBUG_PLUGIN_SYSTEM
	plugins_directories.push_back(".") ;	// this list should be saved/set to some correct value.
	// possible entries include: /usr/lib/retroshare, ~/.retroshare/extensions/, etc.
#endif

	mPluginsManager = new RsPluginManager(rsInitConfig->main_executable_hash) ;
	rsPlugins  = mPluginsManager ;
	mConfigMgr->addConfiguration("plugins.cfg", mPluginsManager);
	mPluginsManager->loadConfiguration() ;

	// These are needed to load plugins: plugin devs might want to know the place of
	// cache directories, get pointers to cache strapper, or access ownId()
	//
	mPluginsManager->setCacheDirectories(localcachedir,remotecachedir) ;
	mPluginsManager->setServiceControl(serviceCtrl) ;

//	std::cerr << "rsinitconf (core 1) = " << (void*)rsInitConfig<<std::endl;
//	std::cerr << "gxs_passwd (core 1) = " << (void*)&rsInitConfig->gxs_passwd<<" \"" <<  rsInitConfig->gxs_passwd << "\""<< std::endl;

	// Now load the plugins. This parses the available SO/DLL files for known symbols.
	//
	mPluginsManager->loadPlugins(plugins_directories) ;

//	std::cerr << "rsinitconf (core 1) = " << (void*)rsInitConfig<<std::endl;
//	std::cerr << "gxs_passwd (core 2) = " << (void*)&rsInitConfig->gxs_passwd<< " \"" << rsInitConfig->gxs_passwd << "\""<< std::endl;

	// Also load some plugins explicitly. This is helpful for
	// - developping plugins 
	//
	std::vector<RsPlugin *> programatically_inserted_plugins ;		

	// Push your own plugins into this list, before the call:
	//
	// 	programatically_inserted_plugins.push_back(myCoolPlugin) ;
	//
	mPluginsManager->loadPlugins(programatically_inserted_plugins) ;

#ifdef RS_ENABLE_GXS

	std::string currGxsDir = rsAccounts->PathAccountDirectory() + "/gxs";
        RsDirUtil::checkCreateDirectory(currGxsDir);

        RsNxsNetMgr* nxsMgr =  new RsNxsNetMgrImpl(serviceCtrl);

        /**** Identity service ****/

        RsGeneralDataService* gxsid_ds = new RsDataService(currGxsDir + "/", "gxsid_db",
                        RS_SERVICE_GXS_TYPE_GXSID, NULL, rsInitConfig->gxs_passwd);

        // init gxs services
	PgpAuxUtils *pgpAuxUtils = new PgpAuxUtilsImpl();
        p3IdService *mGxsIdService = new p3IdService(gxsid_ds, NULL, pgpAuxUtils);

        // circles created here, as needed by Ids.
        RsGeneralDataService* gxscircles_ds = new RsDataService(currGxsDir + "/", "gxscircles_db",
                        RS_SERVICE_GXS_TYPE_GXSCIRCLE, NULL, rsInitConfig->gxs_passwd);

	// create GxsCircles - early, as IDs need it.
        p3GxsCircles *mGxsCircles = new p3GxsCircles(gxscircles_ds, NULL, mGxsIdService, pgpAuxUtils);

        // create GXS ID service
        RsGxsNetService* gxsid_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_GXSID, gxsid_ds, nxsMgr,
			mGxsIdService, mGxsIdService->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils,
            false,false); // don't synchronise group automatic (need explicit group request)
                        // don't sync messages at all.

        mGxsIdService->setNes(gxsid_ns);
        /**** GxsCircle service ****/

        // create GXS Circle service
        RsGxsNetService* gxscircles_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_GXSCIRCLE, gxscircles_ds, nxsMgr,
                        mGxsCircles, mGxsCircles->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);

        /**** Posted GXS service ****/



        RsGeneralDataService* posted_ds = new RsDataService(currGxsDir + "/", "posted_db",
                        RS_SERVICE_GXS_TYPE_POSTED, 
			NULL, rsInitConfig->gxs_passwd);

        p3Posted *mPosted = new p3Posted(posted_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* posted_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_POSTED, posted_ds, nxsMgr, 
			mPosted, mPosted->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);

    mPosted->setNetworkExchangeService(posted_ns) ;

    	/**** Reputation system ****/
    
    	p3GxsReputation *mReputations = new p3GxsReputation(mLinkMgr) ;
    	rsReputations = mReputations ;
        
        /**** Wiki GXS service ****/


        RsGeneralDataService* wiki_ds = new RsDataService(currGxsDir + "/", "wiki_db",
                        RS_SERVICE_GXS_TYPE_WIKI,
                        NULL, rsInitConfig->gxs_passwd);

#ifdef RS_USE_WIKI
        p3Wiki *mWiki = new p3Wiki(wiki_ds, NULL, mGxsIdService);
        // create GXS wiki service
        RsGxsNetService* wiki_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_WIKI, wiki_ds, nxsMgr, 
			mWiki, mWiki->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);

    mWiki->setNetworkExchangeService(wiki_ns) ;
#endif

        /**** Forum GXS service ****/

        RsGeneralDataService* gxsforums_ds = new RsDataService(currGxsDir + "/", "gxsforums_db",
                                                            RS_SERVICE_GXS_TYPE_FORUMS, NULL, rsInitConfig->gxs_passwd);


        p3GxsForums *mGxsForums = new p3GxsForums(gxsforums_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* gxsforums_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_FORUMS, gxsforums_ds, nxsMgr,
                        mGxsForums, mGxsForums->getServiceInfo(),
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);

    mGxsForums->setNetworkExchangeService(gxsforums_ns) ;

        /**** Channel GXS service ****/

        RsGeneralDataService* gxschannels_ds = new RsDataService(currGxsDir + "/", "gxschannels_db",
                                                            RS_SERVICE_GXS_TYPE_CHANNELS, NULL, rsInitConfig->gxs_passwd);

        p3GxsChannels *mGxsChannels = new p3GxsChannels(gxschannels_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* gxschannels_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_CHANNELS, gxschannels_ds, nxsMgr,
                        mGxsChannels, mGxsChannels->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);

    mGxsChannels->setNetworkExchangeService(gxschannels_ns) ;

#if 0 // PHOTO IS DISABLED FOR THE MOMENT
        /**** Photo service ****/
        RsGeneralDataService* photo_ds = new RsDataService(currGxsDir + "/", "photoV2_db",
                        RS_SERVICE_GXS_TYPE_PHOTO, NULL, rsInitConfig->gxs_passwd);

        // init gxs services
        mPhoto = new p3PhotoService(photo_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* photo_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_PHOTO, photo_ds, nxsMgr, 
			mPhoto, mPhoto->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);
#endif

#if 0 // WIRE IS DISABLED FOR THE MOMENT
        /**** Wire GXS service ****/
        RsGeneralDataService* wire_ds = new RsDataService(currGxsDir + "/", "wire_db",
                        RS_SERVICE_GXS_TYPE_WIRE, 
			NULL, rsInitConfig->gxs_passwd);

        mWire = new p3Wire(wire_ds, NULL, mGxsIdService);

        // create GXS photo service
        RsGxsNetService* wire_ns = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_WIRE, wire_ds, nxsMgr, 
			mWire, mWire->getServiceInfo(), 
			mGxsIdService, mGxsCircles,
			pgpAuxUtils);
#endif
        // now add to p3service
        pqih->addService(gxsid_ns, true);
        pqih->addService(gxscircles_ns, true);
        pqih->addService(posted_ns, true);
#ifdef RS_USE_WIKI
        pqih->addService(wiki_ns, true);
#endif
        pqih->addService(gxsforums_ns, true);
        pqih->addService(gxschannels_ns, true);
        //pqih->addService(photo_ns, true);

        // remove pword from memory
        rsInitConfig->gxs_passwd = "";

#endif // RS_ENABLE_GXS.

	/* create Services */
	p3ServiceInfo *serviceInfo = new p3ServiceInfo(serviceCtrl);
	mDisc = new p3discovery2(mPeerMgr, mLinkMgr, mNetMgr, serviceCtrl);
	mHeart = new p3heartbeat(serviceCtrl, pqih);
	msgSrv = new p3MsgService(serviceCtrl,mGxsIdService);
	chatSrv = new p3ChatService(serviceCtrl,mGxsIdService, mLinkMgr, mHistoryMgr);
	mStatusSrv = new p3StatusService(serviceCtrl);

#ifdef ENABLE_GROUTER
    p3GRouter *gr = new p3GRouter(serviceCtrl,mGxsIdService) ;
	rsGRouter = gr ;
	pqih->addService(gr,true) ;
#endif

	p3turtle *tr = new p3turtle(serviceCtrl,mLinkMgr) ;
	rsTurtle = tr ;
	pqih -> addService(tr,true);
	pqih -> addService(ftserver,true);

	rsDisc  = mDisc;
	rsMsgs  = new p3Msgs(msgSrv, chatSrv);

	// connect components to turtle router.

	ftserver->connectToTurtleRouter(tr) ;
	chatSrv->connectToTurtleRouter(tr) ;
    gr->connectToTurtleRouter(tr) ;
#ifdef ENABLE_GROUTER
	msgSrv->connectToGlobalRouter(gr) ;
#endif

	pqih -> addService(serviceInfo,true);
	pqih -> addService(mHeart,true);
	pqih -> addService(mDisc,true);
	pqih -> addService(msgSrv,true);
	pqih -> addService(chatSrv,true);
	pqih -> addService(mStatusSrv,true);
	pqih -> addService(mReputations,true);

	// set interfaces for plugins
	//
	RsPlugInInterfaces interfaces;
	interfaces.mFiles  = rsFiles;
	interfaces.mPeers  = rsPeers;
	interfaces.mMsgs   = rsMsgs;
	interfaces.mTurtle = rsTurtle;
	interfaces.mDisc   = rsDisc;
	interfaces.mDht    = rsDht;
	interfaces.mNotify = mNotify;
    // gxs
    interfaces.mGxsDir          = currGxsDir;
    interfaces.mIdentity        = mGxsIdService;
    interfaces.mRsNxsNetMgr     = nxsMgr;
    interfaces.mGxsIdService    = mGxsIdService;
    interfaces.mGxsCirlces      = mGxsCircles;
    interfaces.mPgpAuxUtils     = pgpAuxUtils;
    interfaces.mGxsForums       = mGxsForums;
    interfaces.mGxsChannels     = mGxsChannels;
    interfaces.mReputations     = mReputations;
    
	mPluginsManager->setInterfaces(interfaces);

	// now add plugin objects inside the loop:
	// 	- client services provided by plugins.
	// 	- cache services provided by plugins.
	//
	mPluginsManager->registerClientServices(pqih) ;
	mPluginsManager->registerCacheServices() ;



#ifdef RS_RTT
	p3rtt *mRtt = new p3rtt(serviceCtrl);
	pqih -> addService(mRtt, true);
	rsRtt = mRtt;
#endif

	// new services to test.
    p3BanList *mBanList = new p3BanList(serviceCtrl, mNetMgr);
    rsBanList = mBanList ;
	pqih -> addService(mBanList, true);
	mBitDht->setupPeerSharer(mBanList);

	p3BandwidthControl *mBwCtrl = new p3BandwidthControl(pqih);
	pqih -> addService(mBwCtrl, true); 

#ifdef SERVICES_DSDV
	p3Dsdv *mDsdv = new p3Dsdv(serviceCtrl);
	pqih -> addService(mDsdv, true);
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
	mLinkMgr->addMonitor(serviceCtrl);
	mLinkMgr->addMonitor(serviceInfo);

	// NOTE these were added in ftServer (was added twice).
	//mLinkMgr->addMonitor(mCacheStrapper);
	//mLinkMgr->addMonitor(((ftController *) mCacheTransfer));

	// Services that have been changed to pqiServiceMonitor
	serviceCtrl->registerServiceMonitor(msgSrv, msgSrv->getServiceInfo().mServiceType);
	serviceCtrl->registerServiceMonitor(mDisc, mDisc->getServiceInfo().mServiceType);
	serviceCtrl->registerServiceMonitor(mStatusSrv, mStatusSrv->getServiceInfo().mServiceType);
	serviceCtrl->registerServiceMonitor(chatSrv, chatSrv->getServiceInfo().mServiceType);
	serviceCtrl->registerServiceMonitor(mBwCtrl, mDisc->getServiceInfo().mServiceType);

	/**************************************************************************/

	//mConfigMgr->addConfiguration("ftserver.cfg", ftserver);
	//
	mConfigMgr->addConfiguration("gpg_prefs.cfg", AuthGPG::getAuthGPG());
	mConfigMgr->loadConfiguration();

	mConfigMgr->addConfiguration("peers.cfg", mPeerMgr);
	mConfigMgr->addConfiguration("general.cfg", mGeneralConfig);
	mConfigMgr->addConfiguration("msgs.cfg", msgSrv);
	mConfigMgr->addConfiguration("chat.cfg", chatSrv);
	mConfigMgr->addConfiguration("p3History.cfg", mHistoryMgr);
	mConfigMgr->addConfiguration("p3Status.cfg", mStatusSrv);
	mConfigMgr->addConfiguration("turtle.cfg", tr);
	mConfigMgr->addConfiguration("banlist.cfg", mBanList);
	mConfigMgr->addConfiguration("servicecontrol.cfg", serviceCtrl);
	mConfigMgr->addConfiguration("reputations.cfg", mReputations);
#ifdef ENABLE_GROUTER
	mConfigMgr->addConfiguration("grouter.cfg", gr);
#endif
    mConfigMgr->addConfiguration("p3identity.cfg", mGxsIdService);

#ifdef RS_USE_BITDHT
    mConfigMgr->addConfiguration("bitdht.cfg", mBitDht);
#endif

#ifdef RS_ENABLE_GXS
	mConfigMgr->addConfiguration("identity.cfg", gxsid_ns);
	mConfigMgr->addConfiguration("gxsforums.cfg", gxsforums_ns);
	mConfigMgr->addConfiguration("gxschannels.cfg", gxschannels_ns);
	mConfigMgr->addConfiguration("gxscircles.cfg", gxscircles_ns);
	mConfigMgr->addConfiguration("posted.cfg", posted_ns);
#ifdef RS_USE_WIKI
	mConfigMgr->addConfiguration("wiki.cfg", wiki_ns);
#endif
	//mConfigMgr->addConfiguration("photo.cfg", photo_ns);
	//mConfigMgr->addConfiguration("wire.cfg", wire_ns);
#endif

	mPluginsManager->addConfigurations(mConfigMgr) ;

	ftserver->addConfiguration(mConfigMgr);

	/**************************************************************************/
	/* (2) Load configuration files */
	/**************************************************************************/
	std::cerr << "(2) Load configuration files" << std::endl;

	/* NOTE: CacheStrapper's load causes Cache Files to be
	 * loaded into all the CacheStores/Sources. This happens
	 * after all the other configurations have happened.
	 */

	mConfigMgr->addConfiguration("cache.cfg", mCacheStrapper);
	mConfigMgr->loadConfiguration();

	/**************************************************************************/
	/* trigger generalConfig loading for classes that require it */
	/**************************************************************************/
	p3ServerConfig *serverConfig = new p3ServerConfig(mPeerMgr, mLinkMgr, mNetMgr, pqih, mGeneralConfig);
	serverConfig->load_config();

	/**************************************************************************/
	/* Force Any Configuration before Startup (After Load) */
	/**************************************************************************/
	std::cerr << "Force Any Configuration before Startup (After Load)" << std::endl;

	if (rsInitConfig->forceLocalAddr)
	{
		struct sockaddr_storage laddr;

		/* clean sockaddr before setting values (MaxOSX) */
		sockaddr_storage_clear(laddr);

		struct sockaddr_in *lap = (struct sockaddr_in *) &laddr;
		
		lap->sin_family = AF_INET;
		lap->sin_port = htons(rsInitConfig->port);

		// universal
		lap->sin_addr.s_addr = inet_addr(rsInitConfig->inet.c_str());

		mPeerMgr->setLocalAddress(ownId, laddr);
	}

	if (rsInitConfig->forceExtPort)
	{
		mPeerMgr->setOwnNetworkMode(RS_NET_MODE_EXT);
		mPeerMgr->setOwnVisState(RS_VS_DISC_FULL, RS_VS_DHT_FULL);

	}

	if (rsInitConfig->hiddenNodeSet)
	{
		mPeerMgr->setupHiddenNode(rsInitConfig->hiddenNodeAddress, rsInitConfig->hiddenNodePort);
	}
	else if (isHiddenNode)
	{
		mPeerMgr->forceHiddenNode();
	}

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
	mNotify->ClearFeedItems(RS_FEED_ITEM_MESSAGE);
	//getPqiNotify()->ClearFeedItems(RS_FEED_ITEM_FILES_NEW);

	/**************************************************************************/
	/* Add AuthGPG services */
	/**************************************************************************/

	AuthGPG::getAuthGPG()->addService(mDisc);

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
#if RS_USE_WIKI
    rsWiki = mWiki;
#endif
    rsPosted = mPosted;
    rsGxsForums = mGxsForums;
    rsGxsChannels = mGxsChannels;
    //rsPhoto = mPhoto;
    //rsWire = mWire;

	/*** start up GXS core runner ***/
    startServiceThread(mGxsIdService);
    startServiceThread(mGxsCircles);
    startServiceThread(mPosted);
#if RS_USE_WIKI
    startServiceThread(mWiki);
#endif
    startServiceThread(mGxsForums);
    startServiceThread(mGxsChannels);

	//createThread(*mPhoto);
	//createThread(*mWire);

	// cores ready start up GXS net servers
    startServiceThread(gxsid_ns);
    startServiceThread(gxscircles_ns);
    startServiceThread(posted_ns);
#if RS_USE_WIKI
    startServiceThread(wiki_ns);
#endif
    startServiceThread(gxsforums_ns);
    startServiceThread(gxschannels_ns);

	//createThread(*photo_ns);
	//createThread(*wire_ns);

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

	pqih->AddSearchModule(mod);

	/* Setup GUI Interfaces. */

	// rsDisc & RsMsgs done already.
	rsBandwidthControl = mBwCtrl;
	rsConfig = serverConfig;

	
	rsStatus = new p3Status(mStatusSrv);
	rsHistory = new p3History(mHistoryMgr);

	/* put a welcome message in! */
	if (isFirstTimeRun)
	{
		msgSrv->loadWelcomeMsg();
		ftserver->shareDownloadDirectory(true);
		mGeneralConfig->saveConfiguration();
	}

	/* Startup this thread! */
    start() ;

	return 1;
}

