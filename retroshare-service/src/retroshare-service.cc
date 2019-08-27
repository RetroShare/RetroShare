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

CrashStackTrace gCrashStackTrace;

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

class NotifyTxt: public NotifyClient
{
public:
	NotifyTxt(){}
	virtual ~NotifyTxt() {}

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

	RsInit::InitRsConfig();
	RsControl::earlyInitNotificationSystem();

#ifndef __ANDROID__
	NotifyTxt *notify = new NotifyTxt();
	rsNotify->registerNotifyClient(notify);
#endif

    if(RsInit::InitRetroShare(argc, argv, true))
    {
        std::cerr << "Could not properly init Retroshare core." << std::endl;
        return 1;
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
	while(true)
	   sleep(1);
#endif

}
