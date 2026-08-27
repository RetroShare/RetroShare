/*
 * RetroShare Service
 * Copyright (C) 2016-2022  Gioacchino Mazzurco <gio@eigenlab.org>
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
#include <cstdlib>
#include <cstdio>

#ifdef WINDOWS_SYS
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

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

/** A terminal that dies mid-prompt -- an ssh session dropping, a container
 * losing its tty -- turns every following read into an immediate EOF. Every
 * prompt below re-asks on empty or mismatched input, so without a bound that
 * becomes a loop nobody can interrupt. Kept outside the terminal-login guard:
 * the web interface password prompt has its own build option. */
static constexpr int MAX_PROMPT_ATTEMPTS = 3;

#ifdef RS_SERVICE_TERMINAL_LOGIN
/** On POSIX rs_getpass() reads stdin, so this tests the very channel it uses.
 * On Windows it reads the console directly through _getch(), so this is a
 * conservative proxy: a service with no console has no interactive stdin
 * either. */
static bool hasInteractiveStdin()
{
#ifdef WINDOWS_SYS
	return _isatty(_fileno(stdin)) != 0;
#else
	return isatty(fileno(stdin)) != 0;
#endif
}

static std::string trimmed(const std::string& s)
{
	const std::string blanks = " \t\r\n";
	const auto first = s.find_first_not_of(blanks);
	if(first == std::string::npos) return std::string();
	return s.substr(first, s.find_last_not_of(blanks) - first + 1);
}
#endif

static void eventHandler(std::shared_ptr<const RsEvent> e)
{
    auto fe = dynamic_cast<const RsSystemEvent*>(e.get());

    if(!fe)
        return;

#ifdef RS_SERVICE_TERMINAL_LOGIN
    if(fe->mEventCode == RsSystemEventCode::PASSWORD_REQUESTED)
    {
        // The core asks for the passphrase through this event on every login,
        // including -U <hexid> from systemd or docker where there is nothing to
        // ask. Answering nothing lets attemptLogin() fail with a proper status;
        // prompting an absent terminal cannot succeed and only hides the cause.
        if(!hasInteractiveStdin())
        {
            RsErr() << "A passphrase is required but stdin is not a terminal. "
                       "Run retroshare-service interactively, or unlock the "
                       "profile through the JSON API." << std::endl;
            return;
        }

        std::string question1 = fe->passwd_request_title + colored(COLOR_GREEN,"Please enter your PGP password for key:\n    ") + fe->passwd_request_key_details + " :";
        std::string password = RsUtil::rs_getpass(question1.c_str()) ;

        if(!password.empty())
            RsLoginHelper::cachePgpPassphrase(password);
    }
#endif

    // We should also handle plugin loading
}


#ifdef TO_REMOVE
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



#ifdef RS_SERVICE_TERMINAL_LOGIN
enum class CreateAccountResult { Created, Cancelled, Failed };

/** Ask the user for a PGP profile to sign the new node with.
 * Returns false when the user cancelled. A null pgpId on return means "make a
 * new profile", which is what createLocationV2() does with a null id. */
static bool askPgpProfile(RsPgpId& pgpId)
{
	pgpId.clear();

	std::list<RsPgpId> pgpIds;
	RsAccounts::GetPGPLogins(pgpIds);

	if(pgpIds.empty()) return true; // nothing to reuse, nothing to ask

	std::vector<RsPgpId> choices(pgpIds.begin(), pgpIds.end());

	std::cout << std::endl
	          << colored(COLOR_GREEN, "Existing profiles on this machine:")
	          << std::endl << std::endl;

	for(size_t i = 0; i < choices.size(); ++i)
	{
		std::string name, email;
		RsAccounts::GetPGPLoginDetails(choices[i], name, email);
		std::cout << colored(COLOR_GREEN, "  [" + RsUtil::NumberToString(i+1) + "]") << " "
		          << colored(COLOR_BLUE, choices[i].toStdString()) << ": "
		          << colored(COLOR_PURPLE, name) << std::endl;
	}

	std::cout << colored(COLOR_GREEN, "  [n]") << " "
	          << colored(COLOR_YELLOW, "Create a new profile") << std::endl << std::endl
	          << colored(COLOR_YELLOW,
	                     "A new profile is a new identity: your existing friends "
	                     "will not recognise it,\nand you will have to exchange "
	                     "certificates with them again. Reuse a profile above\n"
	                     "to simply add this machine as another node of it.")
	          << std::endl << std::endl;

	for(int attempt = 0; keepRunning && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
	{
		std::cout << colored(COLOR_GREEN, "Profile to use, or 'n' for a new one: ");
		std::cout.flush();

		std::string inputStr;
		if(!std::getline(std::cin, inputStr))
		{
			RsErr() << "Unable to read the profile selection from the terminal." << std::endl;
			return false;
		}

		inputStr = trimmed(inputStr);

		if(inputStr == "n" || inputStr == "N") return true;

		char* inputEnd = nullptr;
		unsigned long selection = std::strtoul(inputStr.c_str(), &inputEnd, 10);
		if(inputEnd != inputStr.c_str() && *inputEnd == '\0' &&
		        selection >= 1 && selection <= choices.size())
		{
			pgpId = choices[selection - 1];
			return true;
		}

		std::cout << colored(COLOR_RED, "Invalid selection. Please try again.") << std::endl;
	}

	if(keepRunning)
		RsErr() << "Too many invalid selections, giving up." << std::endl;

	return false;
}

static CreateAccountResult doTerminalCreateAccount()
{
	if(!hasInteractiveStdin())
	{
		RsErr() << "Account creation requires an interactive terminal." << std::endl;
		return CreateAccountResult::Failed;
	}

	std::cout << std::endl
	          << colored(COLOR_GREEN, "=== Create New RetroShare Account ===") << std::endl << std::endl;

	RsPgpId pgpId;
	if(!askPgpProfile(pgpId))
		return keepRunning ? CreateAccountResult::Failed : CreateAccountResult::Cancelled;

	const bool reusingProfile = !pgpId.isNull();

	// Only asked when a profile is created: reusing one keeps its name, and
	// createLocationV2() ignores pgpName as soon as pgpId is not null.
	std::string pgpName;
	if(!reusingProfile)
	{
		for(int attempt = 0; keepRunning && pgpName.empty() && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
		{
			std::cout << colored(COLOR_GREEN, "Please enter your Username: ");
			std::cout.flush();
			if(!std::getline(std::cin, pgpName))
			{
				RsErr() << "Unable to read the account name from the terminal." << std::endl;
				return CreateAccountResult::Failed;
			}
			pgpName = trimmed(pgpName);
			if (pgpName.empty())
				std::cout << colored(COLOR_RED, "Name cannot be empty!") << std::endl;
		}
		if (!keepRunning) return CreateAccountResult::Cancelled;
		if (pgpName.empty())
		{
			RsErr() << "No account name given, giving up." << std::endl;
			return CreateAccountResult::Failed;
		}
	}

	std::string locationName;
	for(int attempt = 0; keepRunning && locationName.empty() && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
	{
		std::cout << colored(COLOR_GREEN, "Please enter Node/Location Name (e.g. Laptop, Home): ");
		std::cout.flush();
		if(!std::getline(std::cin, locationName))
		{
			RsErr() << "Unable to read the location name from the terminal." << std::endl;
			return CreateAccountResult::Failed;
		}
		locationName = trimmed(locationName);
		if (locationName.empty())
			std::cout << colored(COLOR_RED, "Location name cannot be empty!") << std::endl;
	}
	if (!keepRunning) return CreateAccountResult::Cancelled;
	if (locationName.empty())
	{
		RsErr() << "No location name given, giving up." << std::endl;
		return CreateAccountResult::Failed;
	}

	std::string pass1, pass2;
	bool passphraseAccepted = false;
	for(int attempt = 0; keepRunning && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
	{
		pass1 = RsUtil::rs_getpass(colored(COLOR_GREEN,
		            reusingProfile ? "Please enter the passphrase of that profile: "
		                           : "Please enter passphrase for new account: "));

		if(reusingProfile)
		{
			// Nothing to confirm: the passphrase already exists and a typo is
			// caught by the key itself rather than by a second prompt.
			if(!pass1.empty()) { passphraseAccepted = true; break; }
			std::cout << colored(COLOR_RED, "Passphrase cannot be empty! Please try again.") << std::endl;
			continue;
		}

		pass2 = RsUtil::rs_getpass(colored(COLOR_GREEN, "Please enter the same passphrase again: "));

		if (pass1 != pass2)
		{
			std::cout << colored(COLOR_RED, "Passphrases do not match! Please try again.") << std::endl;
			continue;
		}
		if (pass1.empty())
		{
			std::cout << colored(COLOR_RED, "Passphrase cannot be empty! Please try again.") << std::endl;
			continue;
		}
		passphraseAccepted = true;
		break;
	}
	if (!keepRunning) return CreateAccountResult::Cancelled;
	if (!passphraseAccepted)
	{
		RsErr() << "No usable passphrase given, giving up." << std::endl;
		return CreateAccountResult::Failed;
	}

	if(reusingProfile)
		std::cout << colored(COLOR_YELLOW, "Generating SSL certificate for the new node...") << std::endl;
	else
		std::cout << colored(COLOR_YELLOW, "Generating 4096-bit PGP key & SSL certificate (this may take a few seconds)...") << std::endl;

	RsPeerId locationId;
	std::error_condition err = rsLoginHelper->createLocationV2(locationId, pgpId, locationName, pgpName, pass1);

	if (err)
	{
		RsErr() << colored(COLOR_RED, "Account creation failed: " + err.message()) << std::endl;
		return CreateAccountResult::Failed;
	}

	std::cout << std::endl
	          << colored(COLOR_GREEN, "Account successfully created and logged in!") << std::endl;
	std::cout << colored(COLOR_GREEN, "  Location ID : ") << colored(COLOR_YELLOW, locationId.toStdString()) << std::endl;
	std::cout << colored(COLOR_GREEN, "  PGP ID      : ") << colored(COLOR_BLUE, pgpId.toStdString()) << std::endl << std::endl;

	return CreateAccountResult::Created;
}
#endif



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

    RsEventsHandlerId_t EventHandlerId = 0;
    rsEvents->registerEventsHandler(eventHandler,EventHandlerId, RsEventType::SYSTEM);

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
	                 ". Use \"-U list\" to list accounts, or \"-U create\" to create a new account.",
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

		// Same bound as the account prompts: -W on a service with no terminal
		// re-asks a question that can never be answered.
		for(int attempt = 0; keepRunning && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
		{
            webui_pass1 = RsUtil::rs_getpass( colored(COLOR_GREEN,"Please register a password for the web interface: "));
            webui_pass2 = RsUtil::rs_getpass( colored(COLOR_GREEN,"Please enter the same password again            : "));

			if(webui_pass1 != webui_pass2)
			{
                std::cout << colored(COLOR_RED,"Passwords do not match!") << std::endl;
				webui_pass1.clear();
				continue;
			}
			if(webui_pass1.empty())
			{
                std::cout << colored(COLOR_RED,"Password cannot be empty!") << std::endl;
				continue;
			}

			break;
		}

		if(askWebUiPassword && webui_pass1.empty())
			RsErr() << "No web interface password given, the web interface will "
			           "not be started." << std::endl;
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
		bool alreadyLoggedIn = false;

		if(prefUserString == "create")
		{
			switch(doTerminalCreateAccount())
			{
			case CreateAccountResult::Created:   alreadyLoggedIn = true; break;
			case CreateAccountResult::Cancelled: return 0;
			case CreateAccountResult::Failed:    return -RsInit::ERR_UNKNOWN;
			}
		}
		else if(prefUserString == "list")
		{
			std::vector<RsLoginHelper::Location> locations;
			rsLoginHelper->getLocations(locations);

			std::cout << std::endl << std::endl;
			if(locations.empty())
				std::cout << colored(COLOR_YELLOW,"No existing RetroShare accounts found.")
				          << std::endl << std::endl;
			else
			{
				std::cout << colored(COLOR_GREEN,"Available accounts:") << std::endl<<std::endl;

				int accountCountDigits = static_cast<int>( ceil(log(locations.size() + 1)/log(10.0)) );

				for( uint32_t i=0; i<locations.size(); ++i )
					std::cout << colored(COLOR_GREEN,"  [" + RsUtil::NumberToString(i+1,false,'0',accountCountDigits)+"]") << " "
					          << colored(COLOR_YELLOW,locations[i].mLocationId.toStdString())<< " "
					          << colored(COLOR_BLUE,"(" + locations[i].mPgpId.toStdString()+ "): ")
					          << colored(COLOR_PURPLE,locations[i].mPgpName + " (" + locations[i].mLocationName + ")" )
					          << std::endl;

			}

			// "-U list" is documented as a way to list accounts, so on a
			// non-interactive stdin print the list and stop there rather than
			// advertising a [c] entry nobody can type. The error is kept for
			// the case it actually describes: no account to list.
			if(!hasInteractiveStdin())
			{
				if(locations.empty())
				{
					RsErr() << "No available account, and stdin is not a terminal "
					           "to create one." << std::endl;
					return -RsInit::ERR_NO_AVAILABLE_ACCOUNT;
				}
				return 0;
			}

			std::cout << colored(COLOR_GREEN,"  [c]") << " "
			          << colored(COLOR_YELLOW,"Create new account") << std::endl
			          << std::endl;

			bool selectionMade = false;
			for(int attempt = 0; keepRunning && attempt < MAX_PROMPT_ATTEMPTS; ++attempt)
			{
				std::cout << colored(COLOR_GREEN,"Please enter account number or 'c' to create: ");
				std::cout.flush();

				std::string inputStr;
				if(!std::getline(std::cin, inputStr))
				{
					RsErr() << "Unable to read an account selection from the terminal." << std::endl;
					return -RsInit::ERR_NO_AVAILABLE_ACCOUNT;
				}

				inputStr = trimmed(inputStr);

				if(inputStr == "c" || inputStr == "C")
				{
					switch(doTerminalCreateAccount())
					{
					case CreateAccountResult::Created:   alreadyLoggedIn = true; break;
					case CreateAccountResult::Cancelled: return 0;
					case CreateAccountResult::Failed:    return -RsInit::ERR_UNKNOWN;
					}
					break;
				}

				char* inputEnd = nullptr;
				unsigned long selection = std::strtoul(inputStr.c_str(), &inputEnd, 10);
				if(inputEnd != inputStr.c_str() && *inputEnd == '\0' &&
				        selection >= 1 && selection <= locations.size())
				{
					prefUserString = locations[selection - 1].mLocationId.toStdString();
					selectionMade = true;
					break;
				}

				std::cout << colored(COLOR_RED,"Invalid selection. Please try again.") << std::endl;
			}

			// Ctrl-C at a prompt: on glibc signal() installs the handler with
			// SA_RESTART, so the blocked read is restarted and keepRunning is
			// only seen once the user also presses Enter. Without this, control
			// fell through with prefUserString still "list" and the user who
			// just cancelled was told their location id was invalid.
			if(!keepRunning) return 0;

			if(!alreadyLoggedIn && !selectionMade)
			{
				RsErr() << "No account selected, giving up." << std::endl;
				return -RsInit::ERR_NO_AVAILABLE_ACCOUNT;
			}
		}

		if(!alreadyLoggedIn)
		{
			RsPeerId ssl_id(prefUserString);
			if(ssl_id.isNull())
			{
				RsErr() << colored(COLOR_RED,"Invalid User location id: a hexadecimal ID, 'list', or 'create' is expected.")
				        << std::endl;
				return -EINVAL;
			}

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
			case RsInit::ERR_UNKNOWN: // Fall-through
			default:
				RsErr() << "Cannot login. Check your passphrase." << std::endl
				        << std::endl;
				return -result;
			}
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
