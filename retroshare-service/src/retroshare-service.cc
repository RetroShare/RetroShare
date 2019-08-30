/*
 * RetroShare Service
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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
 */

#include "util/stacktrace.h"
#include "util/argstream.h"
#include "retroshare/rsinit.h"
#include "jsonapi/jsonapi.h"

CrashStackTrace gCrashStackTrace;

#include <cmath>
#include <csignal>

#ifndef __ANDROID__
#include <termios.h>
#endif

#ifdef __ANDROID__
#	include <QAndroidService>
#	include <QCoreApplication>
#	include <QObject>
#	include <QStringList>

#	include "util/androiddebug.h"
#endif // def __ANDROID__

#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"

#ifndef RS_JSONAPI
#	error Inconsistent build configuration retroshare_service needs rs_jsonapi
#endif

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

std::string readStringFromKeyboard(const char *prompt, bool show_asterisk=true)
{
  const char BACKSPACE=127;
  const char RETURN=10;

  std::string password;
  unsigned char ch=0;

  std::cout <<prompt; std::cout.flush();

  while((ch=getch())!=RETURN)
    {
       if(ch==BACKSPACE)
         {
            if(password.length()!=0)
              {
                 if(show_asterisk)
                 std::cout <<"\b \b";
                 password.resize(password.length()-1);
              }
         }
       else
         {
             password+=ch;
             if(show_asterisk)
                 std::cout <<'*';
             else
                 std::cout << ch,std::cout.flush();
         }
    }
  std::cout <<std::endl;
  return password;
}

class RsServiceNotify: public NotifyClient
{
public:
	RsServiceNotify(){}
	virtual ~RsServiceNotify() {}

	virtual bool askForPassword(const std::string& title, const std::string& question, bool prev_is_bad, std::string& password,bool& cancel)
	{
		std::string question1=title + "\nPlease enter your PGP password for key:\n    " + question + " :";
		password = readStringFromKeyboard(question1.c_str()) ;
		cancel = false ;

		return !password.empty();
	}
};


int main(int argc, char* argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
	QAndroidService app(argc, argv);

	signal(SIGINT,   QCoreApplication::exit);
	signal(SIGTERM,  QCoreApplication::exit);
#ifdef SIGBREAK
	signal(SIGBREAK, QCoreApplication::exit);
#endif // ifdef SIGBREAK

#endif // def __ANDROID__
    std::cerr << "========================================================================" << std::endl;
    std::cerr << "==                        Retroshare Service                          ==" << std::endl;
    std::cerr << "========================================================================" << std::endl;

	RsInit::InitRsConfig();
	RsControl::earlyInitNotificationSystem();

#ifdef __APPLE__
	// TODO: is this still needed with argstream?
	/* HACK to avoid stupid OSX Finder behaviour
	 * remove the commandline arguments - if we detect we are launched from Finder,
	 * and we have the unparsable "-psn_0_12332" option.
	 * this is okay, as you cannot pass commandline arguments via Finder anyway
	 */
	if ((argc >= 2) && (0 == strncmp(argv[1], "-psn", 4))) argc = 1;
#endif

    std::string prefUserString;
    RsConfigOptions conf;

	argstream as(argc,argv);
	as      >> option('s',"stderr"           ,conf.outStderr        ,"output to stderr instead of log file."    )
	        >> option('u',"udp"              ,conf.udpListenerOnly  ,"Only listen to UDP."                      )
	        >> parameter('c',"base-dir"      ,conf.optBaseDir       ,"directory", "Set base directory."                                         ,false)
	        >> parameter('l',"log-file"      ,conf.logfname         ,"logfile"   ,"Set Log filename."                                           ,false)
	        >> parameter('d',"debug-level"   ,conf.debugLevel       ,"level"     ,"Set debug level."                                            ,false)
	        >> parameter('i',"ip-address"    ,conf.forcedInetAddress,"nnn.nnn.nnn.nnn", "Force IP address to use (if cannot be detected)."      ,false)
	        >> parameter('o',"opmode"        ,conf.opModeStr        ,"opmode"    ,"Set Operating mode (Full, NoTurtle, Gaming, Minimal)."       ,false)
	        >> parameter('p',"port"          ,conf.forcedPort       ,"port", "Set listenning port to use."                                      ,false)
	        >> parameter('U',"user-id"       ,prefUserString        ,"ID", "[node Id] Selected account to use and asks for passphrase. Use \"-u list\" in order to list available accounts.",false);

#ifdef RS_JSONAPI
	as      >> parameter('J', "jsonApiPort", conf.jsonApiPort, "jsonApiPort", "Enable JSON API on the specified port", false )
	        >> parameter('P', "jsonApiBindAddress", conf.jsonApiBindAddress, "jsonApiBindAddress", "JSON API Bind Address.", false);
#endif // ifdef RS_JSONAPI

#ifdef LOCALNET_TESTING
	as      >> parameter('R',"restrict-port" ,portRestrictions             ,"port1-port2","Apply port restriction"                   ,false);
#endif // ifdef LOCALNET_TESTING

#ifdef RS_AUTOLOGIN
	as      >> option('a',"auto-login"       ,conf.autoLogin      ,"AutoLogin (Windows Only) + StartMinimised");
#endif // ifdef RS_AUTOLOGIN

	as >> help('h',"help","Display this Help");
	as.defaultErrorHandling(true,true);

#ifndef __ANDROID__
	RsServiceNotify *notify = new RsServiceNotify();
	rsNotify->registerNotifyClient(notify);

	std::string webui_pass1 = "Y";
	std::string webui_pass2 = "N";

	for(;;)
	{
		webui_pass1 = readStringFromKeyboard("Please register a password for the web interface: ");
		webui_pass2 = readStringFromKeyboard("Please enter the same password again            : ");

		if(webui_pass1 != webui_pass2)
		{
			std::cerr << "Passwords do not match!" << std::endl;
			continue;
		}
		if(webui_pass1.empty())
		{
			std::cerr << "Password cannot be empty!" << std::endl;
			continue;
		}

		break;
	}
#endif

    conf.main_executable_path = argv[0];

    if(RS_INIT_OK != RsInit::InitRetroShare(conf))
    {
        std::cerr << "Could not properly init Retroshare core." << std::endl;
        return 1;
    }

	// choose alternative account.
	if(prefUserString != "")
	{
		if(prefUserString == "list")
		{
			std::cerr << "Available accounts:" << std::endl;

            std::vector<RsLoginHelper::Location> locations;
            rsLoginHelper->getLocations(locations);

			int account_number_size = (int)ceil(log(locations.size())/log(10.0f)) ;

			for(uint32_t i=0;i<locations.size();++i)
				std::cout << "[" << std::setw(account_number_size) << std::setfill('0')
				          << i+1 << "] " << locations[i].mLocationId << " (" << locations[i].mPgpId << "): " << locations[i].mPgpName
                          << " \t (" << locations[i].mLocationName << ")" << std::endl;

			int nacc=0;

			while(nacc < 1 || nacc >= locations.size())
			{
				std::cout << "Please enter account number: ";
				std::cout.flush();
				std::string str;
				std::getline(std::cin, str);

				nacc = atoi(str.c_str())-1;

                if(nacc >= 0 && nacc < locations.size())
				{
					prefUserString = locations[nacc].mLocationId.toStdString();
					break;
				}
				nacc=0;	// allow to continue if something goes wrong.
			}
        }

		RsPeerId ssl_id(prefUserString);

		if(ssl_id.isNull())
		{
			std::cerr << "Invalid User location id: a hexadecimal ID is expected." << std::endl;
			return 1;
		}

		RsInit::LoadCertificateStatus result = rsLoginHelper->attemptLogin(ssl_id,std::string()); // supply empty passwd so that it is properly asked 3 times on console

        std::string lock_file_path = RsAccounts::AccountDirectory()+"/lock" ;

        switch(result)
        {
				case RsInit::OK:	break;
				case RsInit::ERR_ALREADY_RUNNING:	std::cerr << "Another RetroShare using the same profile is already running on your system. Please close "
					                 							 "that instance first.\nLock file: " << RsInit::lockFilePath() << std::endl;
					return 1;
				case RsInit::ERR_CANT_ACQUIRE_LOCK:	std::cerr << "An unexpected error occurred when Retroshare tried to acquire the single instance lock file. \nLock file: " << RsInit::lockFilePath() << std::endl;
					return 1;
				case RsInit::ERR_UNKNOWN:
				default: std::cerr << "Unknown error." << std::endl;
					return 1;
		}
	}

#ifdef __ANDROID__
	rsControl->setShutdownCallback(QCoreApplication::exit);

	QObject::connect(
	            &app, &QCoreApplication::aboutToQuit,
	            [](){
		if(RsControl::instance()->isReady())
			RsControl::instance()->rsGlobalShutDown(); } );

	return app.exec();
#else
    if(jsonApiServer)
		jsonApiServer->authorizeToken("webui:"+webui_pass1);

	std::atomic<bool> keepRunning(true);
	rsControl->setShutdownCallback([&](int){keepRunning = false;});

	while(keepRunning)
		std::this_thread::sleep_for(std::chrono::seconds(5));
#endif

}
