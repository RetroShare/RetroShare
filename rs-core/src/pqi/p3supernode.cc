/*
 * "$Id: p3supernode.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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





#include "pqi/pqisupernode.h"

#include <list>
#include <string>

// Includes for directory creation.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// Conflicts with FLTK def - hope they are the same.
//#include <dirent.h>

// for blocking signals
#include <signal.h>

#include "pqi/pqidebug.h"

void load_check_basedir();

// Global Pointers for the callback functions.

// initial configuration bootstrapping...
static const std::string config_init_file = "default_cert.txt";
static const std::string config_file = "config.rs";
static const std::string cert_dir = "friends";
static const std::string key_dir = "keys";
static const std::string ca_file = "cacerts.pem";

static std::string config_basedir;
static std::string load_cert;
static std::string load_key;
static std::string load_cacert;

void 	load_check_basedir();

static const char dirSeperator = '/'; // For unix.

// standard start for unix.
int main(int argc, char **argv)
{
	// setup debugging for desired zones.
	setOutputLevel(PQL_ALERT); // to show the others.
	setZoneLevel(PQL_ALERT, 38422); // pqipacket.
	setZoneLevel(PQL_ALERT, 96184); // pqinetwork;
	setZoneLevel(PQL_ALERT, 82371); // pqiperson.
	setZoneLevel(PQL_ALERT, 60478); // pqitunnel.
	setZoneLevel(PQL_ALERT, 34283); // pqihandler.
	setZoneLevel(PQL_ALERT, 44863); // discItems.
	setZoneLevel(PQL_DEBUG_BASIC, 2482); // p3disc
	setZoneLevel(PQL_ALERT, 1728); // proxy
	setZoneLevel(PQL_ALERT, 1211); // sslroot.
	setZoneLevel(PQL_ALERT, 37714); // pqissl.
	setZoneLevel(PQL_ALERT, 8221); // pqistreamer.
	setZoneLevel(PQL_ALERT, 354); // pqipersongrp.

	std::list<std::string> dirs;

	short port = 7812; // default port.
	bool forceLocalAddr = false;

	char inet[256] = "127.0.0.1";
	char passwd[256] = "";
	char logfname[1024] = "";

	int c;
	bool havePasswd     = false;
	bool haveLogFile    = false;

	while((c = getopt(argc, argv,"i:p:c:s:w:l:")) != -1)
	{
		switch (c)
		{
			case 'l':
				strncpy(logfname, optarg, 1024);
				std::cerr << "LogFile (" << logfname;
				std::cerr << ") Selected" << std::endl;
				haveLogFile = true;
				break;
			case 'w':
				strncpy(passwd, optarg, 256);
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
				config_basedir = optarg;
				std::cerr << "New Base Config Dir(";
				std::cerr << config_basedir;
				std::cerr << ") Selected" << std::endl;
				break;
			case 's':
				dirs.push_back(std::string(optarg));
				std::cerr << "Adding Search Directory (" << optarg;
				std::cerr << ")" << std::endl;
				break;
			default:
				std::cerr << "Unknown Option!";
				exit(1);
		}
	}

	// Can only log under linux.
	// set the debug file.
	if (haveLogFile)
	{
		setDebugFile(logfname);
	}

	// first check config directories, and set bootstrap values.
	load_check_basedir();

	// SWITCH off the SIGPIPE - kills process on Linux.
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

	if (!havePasswd)
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Missing Passwd!" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	/* determine the cert/key parameters....
	 */
	load_check_basedir();

	bool sslroot_ok = false;
	sslroot *sr = getSSLRoot();

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (0 < sr -> initssl(load_cert.c_str(), load_key.c_str(), passwd))
	{
		sslroot_ok = true;
	}
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (0 < sr -> initssl(load_cert.c_str(), load_key.c_str(), 
			load_cacert.c_str(), passwd))
	{
		sslroot_ok = true;
	}
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (!sslroot_ok)
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Invalid Certificate configuration!" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	// wait for this window to be finished....
	if(!(sr -> active()))
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "SSLRoot Failed to start" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	// set the directories for full configuration load.
	sr -> setConfigDirs(config_basedir.c_str(), cert_dir.c_str());
	sr -> loadCertificates(config_file.c_str());
	sr -> checkNetAddress();

	SecurityPolicy *none = secpolicy_create();

	if (forceLocalAddr)
	{
		struct sockaddr_in laddr;

		laddr.sin_family = AF_INET;
		laddr.sin_port = htons(port);

		// universal
		laddr.sin_addr.s_addr = inet_addr(inet);
		// unix specific
		//inet_aton(inet, &(laddr.sin_addr));

		cert *own = sr -> getOwnCert();
		if (own != NULL)
		{
			own -> localaddr = laddr;
		}
	}


/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	/* can only do this in XPGP */
	sr -> superNodeMode();
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	pqisupernode psn(none, sr);
        psn.load_config();

	psn.run();

	return 1;
}


void load_check_basedir()
{
	// get the default configuration location.
	
	if (config_basedir == "")
	{
		std::cerr << "load_check_basedir() - Fatal Error....." << std::endl;
		std::cerr << "Missing config_basedir" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	std::string key_subdir = config_basedir + dirSeperator;
	key_subdir += key_dir;

	// will catch bad files later.
	
	// Check for config file.
	std::string initfile = config_basedir + dirSeperator;
	initfile += config_init_file;

	/* setup the ca cert file */
	load_cacert = key_dir + dirSeperator;
	load_cacert += ca_file;

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
			load_cert = path;
		}
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++);
			path[i] = '\0';
			load_key = path;
		}
		fclose(ifd);
	}
	else
	{
		std::cerr << "load_check_basedir() - Fatal Error....." << std::endl;
		std::cerr << "Cannot open initfile." << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	return;
}




