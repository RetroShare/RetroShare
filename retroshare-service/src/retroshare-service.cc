/*
 * RetroShare Service
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include "util/stacktrace.h"
#include "util/argstream.h"
#include "util/rskbdinput.h"
#include "retroshare/rsinit.h"

#ifdef RS_JSONAPI
#include "retroshare/rsjsonapi.h"

#ifdef RS_WEBUI
#include "retroshare/rswebui.h"
#endif
#endif

static CrashStackTrace gCrashStackTrace;

#include <cmath>
#include <csignal>
#include <iomanip>
#include <atomic>

#ifdef __ANDROID__
#	include <QAndroidService>
#	include <QCoreApplication>
#	include <QObject>
#	include <QStringList>

#	include "util/androiddebug.h"
#endif // def __ANDROID__

#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"
#include "util/rsdebug.h"

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
		std::string question1 = title +
		        "\nPlease enter your PGP password for key:\n    " +
		        question + " :";
		password = RsUtil::rs_getpass(question1.c_str()) ;
		cancel = false ;

		return !password.empty();
	}
};
#endif // def RS_SERVICE_TERMINAL_LOGIN

#ifdef __ANDROID__
void signalHandler(int /*signal*/) { QCoreApplication::exit(0); }
#else
static std::atomic<bool> keepRunning(true);
static int receivedSignal = 0;

void signalHandler(int signal)
{
	if(RsControl::instance()->isReady())
		RsControl::instance()->rsGlobalShutDown();
	receivedSignal = signal;
	keepRunning = false;
}
#endif // def __ANDROID__


int main(int argc, char* argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
	QAndroidService app(argc, argv);
#endif // def __ANDROID__

	signal(SIGINT,   signalHandler);
	signal(SIGTERM,  signalHandler);
#ifdef SIGBREAK
	signal(SIGBREAK, signalHandler);
#endif // ifdef SIGBREAK

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
	conf.jsonApiPort = RsJsonApi::DEFAULT_PORT;	// enable JSonAPI by default
#ifdef RS_WEBUI
	std::string webui_base_directory = RsWebUi::DEFAULT_BASE_DIRECTORY;
#endif
#endif

	argstream as(argc,argv);
	as >> option( 's', "stderr", conf.outStderr,
	              "output to stderr instead of log file." )
	   >> option( 'u',"udp", conf.udpListenerOnly,
	              "Only listen to UDP." )
	   >> parameter( 'c',"base-dir", conf.optBaseDir, "directory",
	                 "Set base directory.", false )
	   >> parameter( 'l', "log-file", conf.logfname, "logfile",
	                 "Set Log filename.", false )
	   >> parameter( 'd', "debug-level", conf.debugLevel, "level",
	                 "Set debug level.", false )
	   >> parameter( 'i', "ip-address", conf.forcedInetAddress, "IP",
	                 "Force IP address to use (if cannot be detected).", false )
	   >> parameter( 'o', "opmode", conf.opModeStr, "opmode",
	                 "Set Operating mode (Full, NoTurtle, Gaming, Minimal).",
	                 false )
	   >> parameter( 'p', "port", conf.forcedPort, "port",
	                 "Set listenning port to use.", false );

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

#if (defined(RS_JSONAPI) && defined(RS_WEBUI)) && defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	std::string webui_pass1 = "Y";
	if(askWebUiPassword)
	{
		std::string webui_pass2 = "N";

		while(keepRunning)
		{
			webui_pass1 = RsUtil::rs_getpass(
			            "Please register a password for the web interface: " );
			webui_pass2 = RsUtil::rs_getpass(
			            "Please enter the same password again            : " );

			if(webui_pass1 != webui_pass2)
			{
				std::cout << "Passwords do not match!" << std::endl;
				continue;
			}
			if(webui_pass1.empty())
			{
				std::cout << "Password cannot be empty!" << std::endl;
				continue;
			}

			break;
		}
	}
#endif /* defined(RS_JSONAPI) && defined(RS_WEBUI)
	&& defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD) */

	conf.main_executable_path = argv[0];

	int initResult = RsInit::InitRetroShare(conf);
	if(initResult != RS_INIT_OK)
	{
		RsErr() << "Retroshare core initalization failed with: " << initResult
		        << std::endl;
		return -initResult;
	}

#ifdef RS_SERVICE_TERMINAL_LOGIN
	if(!prefUserString.empty()) // Login from terminal requested
	{
		if(prefUserString == "list")
		{
			std::cout << std::endl << std::endl
			          << "Available accounts:" << std::endl;

			std::vector<RsLoginHelper::Location> locations;
			rsLoginHelper->getLocations(locations);

			int accountCountDigits = static_cast<int>(
			            ceil(log(locations.size())/log(10.0)) );

			for( uint32_t i=0; i<locations.size(); ++i )
				std::cout << "[" << std::setw(accountCountDigits)
				          << std::setfill('0') << i+1 << "] "
				          << locations[i].mLocationId << " ("
				          << locations[i].mPgpId << "): "
				          << locations[i].mPgpName
				          << " \t (" << locations[i].mLocationName << ")"
				          << std::endl;

			uint32_t nacc = 0;
			while(keepRunning && (nacc < 1 || nacc >= locations.size()))
			{
				std::cout << "Please enter account number: ";
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
			RsErr() << "Invalid User location id: a hexadecimal ID is expected."
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
	}
#endif // def RS_SERVICE_TERMINAL_LOGIN

#if (defined(RS_JSONAPI) && defined(RS_WEBUI)) && defined(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	if(rsJsonApi && !webui_pass1.empty())
    {
		rsWebUi->setHtmlFilesDirectory(webui_base_directory);
		rsWebUi->setUserPassword(webui_pass1);
		rsWebUi->restart();
    }
#endif

#ifdef __ANDROID__
	rsControl->setShutdownCallback(QCoreApplication::exit);

	QObject::connect(
	            &app, &QCoreApplication::aboutToQuit,
	            [](){
		if(RsControl::instance()->isReady())
			RsControl::instance()->rsGlobalShutDown(); } );

	return app.exec();
#else // def __ANDROID__
	rsControl->setShutdownCallback([&](int){keepRunning = false;});

	while(keepRunning)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	return 0;
#endif
}
