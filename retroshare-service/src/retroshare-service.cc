/*
 * RetroShare Service
 * Copyright (C) 2016-2022  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2021-2022  Asociación Civil Altermundi <info@altermundi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */


#include <cmath>
#include <csignal>
#include <iomanip>
#include <atomic>

#include "retroshare/rsinit.h"
#include "retroshare/rstor.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"

#include "util/stacktrace.h"
#include "util/rsprint.h"
#include "util/argstream.h"
#include "util/rskbdinput.h"
#include "util/rsdir.h"
#include "util/rsdebug.h"

#ifdef RS_JSONAPI
#	include "retroshare/rsjsonapi.h"

#	ifdef RS_WEBUI
#		include "retroshare/rswebui.h"
#	endif // def RS_WEBUI
#endif // def RS_JSONAPI

static CrashStackTrace gCrashStackTrace;

// We should move these functions to rsprint in libretroshare

#define COLOR_GREEN  0
#define COLOR_YELLOW 1
#define COLOR_BLUE   2
#define COLOR_PURPLE 3
#define COLOR_RED    4

std::string colored(int color,const std::string& s)
{
    switch(color)
    {
    case COLOR_GREEN : return "\033[0;32m"+s+"\033[0m";
    case COLOR_YELLOW: return "\033[0;33m"+s+"\033[0m";
    case COLOR_BLUE  : return "\033[0;36m"+s+"\033[0m";
    case COLOR_PURPLE: return "\033[0;35m"+s+"\033[0m";
    case COLOR_RED   : return "\033[0;31m"+s+"\033[0m";
    default:
        return s;
    }
}

#ifdef RS_SERVICE_TERMINAL_LOGIN
class RsServiceNotify: public NotifyClient
{
public:
	RsServiceNotify() = default;
	virtual ~RsServiceNotify() = default;

	virtual bool askForPassword(
	        const std::string& title, const std::string& question,
	        bool /*prev_is_bad*/, std::string& password, bool& cancel )
	{
        std::string question1 = title + colored(COLOR_GREEN,"Please enter your PGP password for key:\n    ")  + question + " :";
		password = RsUtil::rs_getpass(question1.c_str()) ;
		cancel = false ;

		return !password.empty();
	}
};
#endif // def RS_SERVICE_TERMINAL_LOGIN

static std::atomic<bool> keepRunning(true);
static int receivedSignal = 0;

void signalHandler(int signal)
{
	if(RsControl::instance()->isReady())
		RsControl::instance()->rsGlobalShutDown();
	receivedSignal = signal;
	keepRunning = false;
}


int main(int argc, char* argv[])
{
	signal(SIGINT,   signalHandler);
	signal(SIGTERM,  signalHandler);
#ifdef SIGBREAK
	signal(SIGBREAK, signalHandler);
#endif // ifdef SIGBREAK

#ifdef WINDOWS_SYS
	// Enable ANSI color support in Windows console
	{
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif

		HANDLE hStdin = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdin) {
			DWORD consoleMode;
			if (GetConsoleMode(hStdin, &consoleMode)) {
				if ((consoleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
					if (SetConsoleMode(hStdin, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
						std::cout << "Enabled ANSI color support in console" << std::endl;
					} else {
						RsErr() << "Error getting console mode" << std::endl;
					}
				}
			} else {
				RsErr() << "Error getting console mode" << std::endl;
			}
		} else {
			RsErr() << "Error getting stdin handle" << std::endl;
		}
	}
#endif

	RsInfo() << "\n" <<
	    "+================================================================+\n"
	    "|     o---o                                             o        |\n"
	    "|      \\ /           - Retroshare Service -            / \\       |\n"
	    "|       o                                             o---o      |\n"
	    "+================================================================+"
	         << std::endl << std::endl;

	RsInit::InitRsConfig();
	RsControl::earlyInitNotificationSystem();

#ifdef __APPLE__
	// TODO: is this still needed with argstream?
	/* HACK to avoid stupid OSX Finder behaviour
	 * remove the commandline arguments - if we detect we are launched from
	 * Finder, and we have the unparsable "-psn_0_12332" option.
	 * this is okay, as you cannot pass commandline arguments via Finder anyway
	 */
	if ((argc >= 2) && (0 == strncmp(argv[1], "-psn", 4))) argc = 1;
#endif

	std::string prefUserString;
	RsConfigOptions conf;

#ifdef RS_JSONAPI
	conf.jsonApiPort = RsJsonApi::DEFAULT_PORT;	// enable JSON API by default
#ifdef RS_WEBUI
	std::string webui_base_directory = RsWebUi::DEFAULT_BASE_DIRECTORY;
#endif
#endif

	argstream as(argc,argv);
	as >> option( 's', "stderr", conf.outStderr,
	              "output to stderr instead of log file." )
	   >> option( 'u',"udp", conf.udpListenerOnly,
	              "Only listen to UDP." )
       >> parameter( 'c',"base-dir", conf.optBaseDir, "directory", "Set base directory.", false )
       >> parameter( 'l', "log-file", conf.logfname, "logfile", "Set Log filename.", false )
       >> parameter( 'd', "debug-level", conf.debugLevel, "level", "Set debug level.", false )
       >> parameter( 'i', "ip-address", conf.forcedInetAddress, "IP", "Force IP address to use (if cannot be detected).", false )
       >> parameter( 'o', "opmode", conf.opModeStr, "opmode", "Set Operating mode (Full, NoTurtle, Gaming, Minimal).", false )
       >> parameter( 'p', "port", conf.forcedPort, "port", "Set listenning port to use.", false )
       >> parameter( 't', "tor", conf.userSuppliedTorExecutable, "tor", "Set Tor executable full path.", false );

#ifdef RS_SERVICE_TERMINAL_LOGIN
	as >> parameter( 'U', "user-id", prefUserString, "ID",
	                 "[node Id] Selected account to use and asks for passphrase"
	                 ". Use \"-U list\" in order to list available accounts.",
	                 false );
#endif // def RS_SERVICE_TERMINAL_LOGIN

#ifdef RS_JSONAPI
	as >> parameter( 'J', "jsonApiPort", conf.jsonApiPort, "TCP Port",
	                 "Enable JSON API on the specified port", false )
	   >> parameter( 'P', "jsonApiBindAddress", conf.jsonApiBindAddress,
	                 "TCP bind address", "JSON API Bind Address default "
	                                     "127.0.0.1.", false );
#endif // def RS_JSONAPI

#if (defined(RS_JSONAPI) && defined(RS_WEBUI)) && defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	bool askWebUiPassword = false;
	as >> parameter( 'B', "webui-directory", webui_base_directory, "Place where to find the html/js files for the webui.",false );
	as >> option( 'W', "webui-password", askWebUiPassword, "Ask WebUI password on the console." );
#endif /* defined(RS_JSONAPI) && defined(RS_WEBUI) \
	        && defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD) */


#ifdef LOCALNET_TESTING
	as >> parameter( 'R', "restrict-port" , portRestrictions, "port1-port2",
	                 "Apply port restriction", false);
#endif

#ifdef RS_AUTOLOGIN
	as >> option( 'a', "auto-login", conf.autoLogin,
	              "enable auto-login." );
#endif

	as >> help( 'h', "help", "Display this Help" );
	as.defaultErrorHandling(true, true);

    if(!conf.userSuppliedTorExecutable.empty())
        RsTor::setTorExecutablePath(conf.userSuppliedTorExecutable);

#if (defined(RS_JSONAPI) && defined(RS_WEBUI)) && defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	std::string webui_pass1;
	if(askWebUiPassword)
	{
		std::string webui_pass2 = "N";

		while(keepRunning)
		{
            webui_pass1 = RsUtil::rs_getpass( colored(COLOR_GREEN,"Please register a password for the web interface: "));
            webui_pass2 = RsUtil::rs_getpass( colored(COLOR_GREEN,"Please enter the same password again            : "));

			if(webui_pass1 != webui_pass2)
			{
                std::cout << colored(COLOR_RED,"Passwords do not match!") << std::endl;
				continue;
			}
			if(webui_pass1.empty())
			{
                std::cout << colored(COLOR_RED,"Password cannot be empty!") << std::endl;
				continue;
			}

			break;
		}
	}
#ifdef RS_SERVICE_TERMINAL_WEBUI_PASSWORD
    if(askWebUiPassword && !webui_pass1.empty())
    {
        rsWebUi->setHtmlFilesDirectory(webui_base_directory);
        conf.webUIPasswd = webui_pass1;	// cannot be set using rsWebUI methods because it calls the still non-existent rsJsonApi
        conf.enableWebUI = true;

        // JsonApi is started below in InitRetroShare(). Not calling restart here avoids multiple restart.
    }
#endif
#endif /* defined(RS_JSONAPI) && defined(RS_WEBUI)
	&& defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD) */

	conf.main_executable_path = argv[0];

	int initResult = RsInit::InitRetroShare(conf);

#ifdef RS_JSONAPI
    RsInit::startupWebServices(conf,true);
    rstime::rs_usleep(1000000); // waits for jas->restart to print stuff
#endif

	if(initResult != RS_INIT_OK)
	{
		RsFatal() << "Retroshare core initalization failed with: " << initResult
		          << std::endl;
		return -initResult;
	}

#ifdef RS_SERVICE_TERMINAL_LOGIN
	if(!prefUserString.empty()) // Login from terminal requested
	{
		if(prefUserString == "list")
		{
			std::vector<RsLoginHelper::Location> locations;
			rsLoginHelper->getLocations(locations);

            if(locations.size() == 0)
            {
                RsErr() << colored(COLOR_RED,"No available accounts. You cannot use option -U list") << std::endl;
                return -RsInit::ERR_NO_AVAILABLE_ACCOUNT;
            }

            std::cout << std::endl << std::endl
                      << colored(COLOR_GREEN,"Available accounts:") << std::endl<<std::endl;

            int accountCountDigits = static_cast<int>( ceil(log(locations.size())/log(10.0)) );

			for( uint32_t i=0; i<locations.size(); ++i )
                std::cout << colored(COLOR_GREEN,"  [" + RsUtil::NumberToString(i+1,false,'0',accountCountDigits)+"]") << " "
                          << colored(COLOR_YELLOW,locations[i].mLocationId.toStdString())<< " "
                          << colored(COLOR_BLUE,"(" + locations[i].mPgpId.toStdString()+ "): ")
                          << colored(COLOR_PURPLE,locations[i].mPgpName + " (" + locations[i].mLocationName + ")" )
				          << std::endl;

            std::cout << std::endl;
            uint32_t nacc = 0;
			while(keepRunning && (nacc < 1 || nacc >= locations.size()))
			{
                std::cout << colored(COLOR_GREEN,"Please enter account number: ");
				std::cout.flush();

				std::string inputStr;
				std::getline(std::cin, inputStr);

				nacc = static_cast<uint32_t>(atoi(inputStr.c_str())-1);
				if(nacc < locations.size())
				{
					prefUserString = locations[nacc].mLocationId.toStdString();
					break;
				}
				nacc=0; // allow to continue if something goes wrong.
			}
		}


		RsPeerId ssl_id(prefUserString);
		if(ssl_id.isNull())
		{
            RsErr() << colored(COLOR_RED,"Invalid User location id: a hexadecimal ID is expected.")
			        << std::endl;
			return -EINVAL;
		}

		RsServiceNotify* notify = new RsServiceNotify();
		rsNotify->registerNotifyClient(notify);

		// supply empty passwd so that it is properly asked 3 times on console
		RsInit::LoadCertificateStatus result = rsLoginHelper->attemptLogin(ssl_id, "");

		switch(result)
		{
		case RsInit::OK: break;
		case RsInit::ERR_ALREADY_RUNNING:
			RsErr() << "Another RetroShare using the same profile is already "
			           "running on your system. Please close that instance "
			           "first." << std::endl << "Lock file: "
			        << RsInit::lockFilePath() << std::endl;
			return -RsInit::ERR_ALREADY_RUNNING;
		case RsInit::ERR_CANT_ACQUIRE_LOCK:
			RsErr() << "An unexpected error occurred when Retroshare tried to "
			           "acquire the single instance lock file." << std::endl
			        << "Lock file: " << RsInit::lockFilePath() << std::endl;
			return -RsInit::ERR_CANT_ACQUIRE_LOCK;
		case RsInit::ERR_UNKNOWN: // Fall-throug
		default:
			RsErr() << "Cannot login. Check your passphrase." << std::endl
			        << std::endl;
			return -result;
		}

        if(RsAccounts::isTorAuto())
        {

            std::cerr << colored(COLOR_GREEN,"(II) Hidden service is ready:") << std::endl;

            std::string service_id ;
            std::string onion_address ;
            uint16_t service_port ;
            uint16_t service_target_port ;
            uint16_t proxy_server_port ;
            std::string service_target_address ;
            std::string proxy_server_address ;

            RsTor::getHiddenServiceInfo(service_id,onion_address,service_port,service_target_address,service_target_port);
            RsTor::getProxyServerInfo(proxy_server_address,proxy_server_port) ;

            std::cerr << colored(COLOR_GREEN,"  onion address  : ") << onion_address << std::endl;
            std::cerr << colored(COLOR_GREEN,"  service_id     : ") << service_id << std::endl;
            std::cerr << colored(COLOR_GREEN,"  service port   : ") << service_port << std::endl;
            std::cerr << colored(COLOR_GREEN,"  target port    : ") << service_target_port << std::endl;
            std::cerr << colored(COLOR_GREEN,"  target address : ") << service_target_address << std::endl;

            std::cerr << colored(COLOR_GREEN,"Setting proxy server to ") << service_target_address << ":" << service_target_port << std::endl;

            rsPeers->setLocalAddress(rsPeers->getOwnId(), service_target_address, service_target_port);
            rsPeers->setHiddenNode(rsPeers->getOwnId(), onion_address, service_port);
            rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, proxy_server_address,proxy_server_port) ;
        }
	}
#endif // def RS_SERVICE_TERMINAL_LOGIN

	rsControl->setShutdownCallback([&](int){keepRunning = false;});

	while(keepRunning)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	return 0;
}
