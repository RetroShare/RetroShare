/*******************************************************************************
 * retroshare-nogui/src/TerminalApiClient.cpp                                  *
 *                                                                             *
 * retroshare-nogui: headless version of retroshare                            *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare.project@gmail.com>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "TerminalApiClient.h"

#include <unistd.h>
#include <sstream>
#include <cmath>
#include <iomanip>

#include <api/JsonStream.h>

// windows has _kbhit() and _getch() fr non-blocking keaboard read.
// linux does not have these functions.
// the TerminalInput class provides both for win and linux
// only use a single instance of this class in the whole program!
// or destroy the classes in the inverse order how the where created
// else terminal echo will not be restored
// the point of this class is:
// - it configures the terminal in the constructor
// - it restores the terminal in the destructor
#ifdef _WIN32
    #include <conio.h>
#else // LINUX
    #include <termios.h>
    #include <stdio.h>
    #include <sys/ioctl.h>
#endif
#define TERMINALINPUT_DEBUG
class TerminalInput
{
public:
    TerminalInput()
    {
#ifndef _WIN32
        /*
        Q: Is there a getch() (from conio) equivalent on Linux/UNIX?

         A: No. But it's easy to emulate:

        This code sets the terminal into non-canonical mode, thus disabling line buffering, reads a character from stdin and then restores the old terminal status. For more info on what else you can do with termios, see ``man termios''.
         There's also a ``getch()'' function in the curses library, but it is /not/ equivalent to the DOS ``getch()'' and may only be used within real curses applications (ie: it only works in curses ``WINDOW''s).

        http://cboard.cprogramming.com/faq-board/27714-faq-there-getch-conio-equivalent-linux-unix.html
        */
        tcgetattr(STDIN_FILENO, &mOldTermSettings);
        termios term = mOldTermSettings;
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO; // disable echo
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
        setbuf(stdin, NULL);
#endif
    }
    ~TerminalInput()
    {
#ifndef _WIN32
        // restore terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &mOldTermSettings);
#ifdef TERMINALINPUT_DEBUG
        std::cerr << "Terminal killed" << std::endl;
#endif
#endif
    }

    // returns a non zero value if a key was pressed
    int kbhit()
    {
#ifdef _WIN32
        return _kbhit();
#else // LINUX
        int bytesWaiting;
        ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
        return bytesWaiting;
#endif
    }
    // return the pressed key
    int getch()
    {
#ifdef _WIN32
        return _getch();
#else // LINUX
        return getchar();
#endif
    }
private:
#ifndef _WIN32
    struct termios mOldTermSettings;
#endif
};

namespace resource_api {

TerminalApiClient::TerminalApiClient(ApiServer *api): mApiServer(api)
{
}

TerminalApiClient::~TerminalApiClient()
{
    fullstop();
}

static std::string readStringFromKeyboard(bool passwd_mode)
{
	int c ;
	std::string s;

	while((c=getchar()) != '\n')
	{
		// handle backspace
		if (c == 127) {
			if(s.length()!=0) {
				std::cout << "\b \b";
				s.resize(s.length()-1);
			}
			continue;
		}

		if(passwd_mode)
			putchar('*') ;
		else
			putchar(c) ;

		s += c ;
	}
	putchar('\n');
	return s ;
}

void TerminalApiClient::data_tick()
{
    // values in milliseconds
    const int MIN_WAIT_TIME           = 20; // sleep time must be smaller or equal than the smallest period
    const int API_EVENT_POLL_PERIOD   = 1000;

#ifdef TO_REMOVE
    const int IO_POLL_PERIOD          = 20;
    int last_io_poll  = 0;
    int last_char = 0;
    bool enter_was_pressed = false;
    std::string inbuf;
#endif

	int last_event_api_poll = 0;
	bool prev_is_bad = false ;
	int account_number_size = 1 ;
	size_t selected_account_number = 0 ;
	//int account_number_typed = 0 ;

    StateToken runstate_state_token;
    std::string runstate;

    std::vector<AccountInfo> accounts;

    StateToken password_state_token;
    bool ask_for_password = false;
    std::string key_name;

	// This is only used to remove echo from the input and allow us to replace it by what we want.
    TerminalInput term;

    while(!shouldStop())
    {
        // assuming sleep_time >> work_time
        // so we don't have to check the absolute time, just sleep every cycle
        usleep(MIN_WAIT_TIME * 1000);

        last_event_api_poll   += MIN_WAIT_TIME;

#ifdef TO_REMOVE
        last_io_poll          += MIN_WAIT_TIME;

        if(last_io_poll >= IO_POLL_PERIOD)
        {
            last_io_poll = 0;
            last_char = 0;
            if(term.kbhit())
            {
                enter_was_pressed = false;
                last_char = term.getch();
                if(last_char > 127)
                    std::cout << "Warning: non ASCII characters probably won't work." << std::endl;
                if(last_char >= ' ')// space is the first printable ascii character
                    inbuf += (char) last_char;
                if(last_char == '\r' || last_char == '\n')
                    enter_was_pressed = true;
                else
                    enter_was_pressed = false;
                // send echo
                if(ask_for_password)
				{
                    std::cout << "*";
                    std::cout.flush();
				}
                else
				{
                    std::cout << (char) last_char;
                    std::cout.flush();
				}

                //std::cout << "you pressed key " << (char) last_char  << " as integer: " << last_char << std::endl;
            }
        }
#endif

        if(last_event_api_poll >= API_EVENT_POLL_PERIOD)
        {
            last_event_api_poll = 0;
            if(!runstate_state_token.isNull() && !isTokenValid(runstate_state_token))
                runstate_state_token = StateToken(); // must get new state with new token

            if(!password_state_token.isNull() && !isTokenValid(password_state_token))
                password_state_token = StateToken();
        }

		// If the core has started, we leave. Maybe we should not use this in the future if we want to allow to
		// log out and then log in again?

		if(runstate == "running_ok")
		{
			std::cerr << "Terminating terminal thread because the runstate says that the core is running." << std::endl;
			shutdown();
		}

        bool edge = false;
        if(runstate_state_token.isNull())
        {
            edge = true;
			readRunState(runstate_state_token,runstate) ;
		}
        if(password_state_token.isNull())
		{
            edge = true;
			readPasswordState(password_state_token,ask_for_password,key_name,prev_is_bad) ;
		}

        if(!ask_for_password && edge && runstate == "waiting_account_select")
        {
			readAvailableAccounts(accounts) ;
			account_number_size = (int)ceil(log(accounts.size())/log(10.0f)) ;

			for(uint32_t i=0;i<accounts.size();++i)
				std::cout << "[" << std::setw(account_number_size) << std::setfill('0') << i << "] Location Id: " << accounts[i].ssl_id << " \"" << accounts[i].name << "\" (" << accounts[i].location << ")" << std::endl;

			selected_account_number = accounts.size() ;
			//account_number_typed = 0 ;

			while(selected_account_number >= accounts.size())
			{
				std::cout << std::endl << "Type account number: " ;
				std::cout.flush() ;

				std::string s = readStringFromKeyboard(false) ;

				if(sscanf(s.c_str(),"%lu",&selected_account_number) != 1)
					continue ;

				if(selected_account_number >= accounts.size())
				{
					std::cerr << ": invalid account number (should be between " << std::setw(account_number_size) << std::setfill('0')
					          << 0 << " and " << std::setw(account_number_size) << std::setfill('0') << accounts.size()-1 << ")" << std::endl;
					std::cout << std::endl << "Type account number: " ;
					std::cout.flush() ;

					selected_account_number = accounts.size();
				}

				std::cout << std::endl << "Selected account: " << accounts[selected_account_number].name << " (" << accounts[selected_account_number].location << ") SSL id: " << accounts[selected_account_number].ssl_id << std::endl;
			}
			// now ask for passphrase

            std::string prompt = "Enter the password for key " + key_name + " : " ;
			std::cout << prompt ;
			std::cout.flush();
			std::string passwd = readStringFromKeyboard(true);

			// now we have passwd and account number, so send it to the core.

			std::string acc_ssl_id = accounts[selected_account_number].ssl_id.toStdString();

			sendPassword(passwd) ;
			sendSelectedAccount(acc_ssl_id) ;
        }
		else if(ask_for_password)
		{
            std::string prompt = "Enter the password for key " + key_name + " : " ;
			std::cout << prompt ;
			std::cout.flush();
			std::string passwd = readStringFromKeyboard(true);

			// now we have passwd and account number, so send it to the core.

			sendPassword(passwd) ;
		}
    }
}

void TerminalApiClient::waitForResponse(ApiServer::RequestId id) const
{
    while(!mApiServer->isRequestDone(id))
          usleep(20*1000);
}

void TerminalApiClient::sendPassword(const std::string& passwd) const
{
	JsonStream reqs;
	JsonStream resps;
	Request req(reqs);
	std::stringstream ss;
	Response resp(resps, ss);

	req.mPath.push("password");
	req.mPath.push("control");

	std::string pass(passwd) ;

	reqs << makeKeyValueReference("password", pass);
	reqs.switchToDeserialisation();

	ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
	waitForResponse(id);
}

void TerminalApiClient::sendSelectedAccount(const std::string& ssl_id) const
{
	JsonStream reqs;
	JsonStream resps;
	Request req(reqs);
	std::stringstream ss;
	Response resp(resps, ss);

	std::string acc_ssl_id(ssl_id) ;
	req.mPath.push("login");
	req.mPath.push("control");
	reqs << makeKeyValueReference("id", acc_ssl_id);
	reqs.switchToDeserialisation();

	ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
	waitForResponse(id);
}

void TerminalApiClient::readAvailableAccounts(std::vector<AccountInfo>& accounts) const
{
	JsonStream reqs;
	JsonStream resps;
	Request req(reqs);
	std::stringstream ss;
	Response resp(resps, ss);

	req.mPath.push("locations");
	req.mPath.push("control");
	reqs.switchToDeserialisation();

	ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
	waitForResponse(id);

	resps.switchToDeserialisation();

	if(!resps.hasMore())
		std::cout << "Error: No Accounts. Use the Qt-GUI or the webinterface to create an account." << std::endl;

	int i = 0;
	accounts.clear();

	while(resps.hasMore())
	{
		std::string id;
		std::string name;
		std::string location;

		resps.getStreamToMember()
		        << makeKeyValueReference("id", id)
		        << makeKeyValueReference("name", name)
		        << makeKeyValueReference("location", location);

		AccountInfo info ;
		info.location = location ;
		info.name = name ;
		info.ssl_id = RsPeerId(id) ;

		accounts.push_back(info);
		i++;
	}
}


bool TerminalApiClient::isTokenValid(StateToken runstate_state_token) const
{
    JsonStream reqs;
    JsonStream resps;
    Request req(reqs);
    std::stringstream ss;
    Response resp(resps, ss);

    req.mPath.push("statetokenservice");
    req.mStream << runstate_state_token;
    reqs.switchToDeserialisation();

    ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
    waitForResponse(id);

    resps.switchToDeserialisation();
    if(resps.hasMore())
        return false;
    else
        return true;
}

void TerminalApiClient::readPasswordState(StateToken& password_state_token,bool& ask_for_password,std::string& key_name,bool& prev_is_bad) const
{
	JsonStream reqs;
	JsonStream resps;
	Request req(reqs);
	std::stringstream ss;
	Response resp(resps, ss);

	req.mPath.push("password");
	req.mPath.push("control");
	reqs.switchToDeserialisation();

	ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
	waitForResponse(id);

	resps.switchToDeserialisation();
	resps << makeKeyValueReference("want_password", ask_for_password);
	resps << makeKeyValueReference("key_name", key_name);
	resps << makeKeyValueReference("prev_is_bad", prev_is_bad);
	password_state_token = resp.mStateToken;

	std::cerr << "****** Passwd state changed: want_passwd=" << ask_for_password << " key_name=" << key_name << " prev_is_bad=" << prev_is_bad << std::endl;
}

void TerminalApiClient::readRunState(StateToken& runstate_state_token,std::string& runstate) const
{
	JsonStream reqs;
	JsonStream resps;
	Request req(reqs);
	std::stringstream ss;
	Response resp(resps, ss);

	req.mPath.push("runstate");
	req.mPath.push("control");
	reqs.switchToDeserialisation();

	ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
	waitForResponse(id);

	resps.switchToDeserialisation();
	resps << makeKeyValueReference("runstate", runstate);
	runstate_state_token = resp.mStateToken;

	std::cerr << "****** Run State changed to \"" << runstate << "\"" << std::endl;
}

} // namespace resource_api
