#include "TerminalApiClient.h"

#include <unistd.h>
#include <sstream>

#include <api/JsonStream.h>

// need two functions for non blocking read from stdin:
// int _kbhit()     (returns a non zero value if a key was pressed)
// int _getch()     (return the pressed key)
// these function are available on windows in conio.h
// they are not available on linux
#ifdef _WIN32
    #include <conio.h>
#else // LINUX
/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 (modified to disable echo)
 */
#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stropts.h>

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO; // disable echo
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
/*
Q: Is there a getch() (from conio) equivalent on Linux/UNIX?

 A: No. But it's easy to emulate:

This code sets the terminal into non-canonical mode, thus disabling line buffering, reads a character from stdin and then restores the old terminal status. For more info on what else you can do with termios, see ``man termios''.
 There's also a ``getch()'' function in the curses library, but it is /not/ equivalent to the DOS ``getch()'' and may only be used within real curses applications (ie: it only works in curses ``WINDOW''s).

http://cboard.cprogramming.com/faq-board/27714-faq-there-getch-conio-equivalent-linux-unix.html
*/
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int _getch( ) {
    struct termios oldt,
                   newt;
    int              ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}
#endif // LINUX


namespace resource_api {

TerminalApiClient::TerminalApiClient(ApiServer *api):
    mApiServer(api)
{
    start();
}

TerminalApiClient::~TerminalApiClient()
{
    join();
}

void TerminalApiClient::run()
{
    // values in milliseconds
    const int MIN_WAIT_TIME           = 20; // sleep time must be smaller or equal than the smallest period
    const int IO_POLL_PERIOD          = 20;
    const int API_EVENT_POLL_PERIOD   = 1000;

    int last_io_poll  = 0;
    int last_event_api_poll = 0;

    int last_char = 0;
    std::string inbuf;
    bool enter_was_pressed = false;

    StateToken runstate_state_token;
    std::string runstate;

    std::vector<std::string> accounts;

    StateToken password_state_token;
    bool ask_for_password = false;
    std::string key_name;

    while(isRunning())
    {
        // assuming sleep_time >> work_time
        // so we don't have to check the absolute time, just sleep every cycle
        usleep(MIN_WAIT_TIME * 1000);
        last_io_poll          += MIN_WAIT_TIME;
        last_event_api_poll   += MIN_WAIT_TIME;

        if(last_io_poll >= IO_POLL_PERIOD)
        {
            last_io_poll = 0;
            last_char = 0;
            if(_kbhit())
            {
                enter_was_pressed = false;
                last_char = _getch();
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
                    std::cout << "*";
                else
                    std::cout << (char) last_char;

                //std::cout << "you pressed key " << (char) last_char  << " as integer: " << last_char << std::endl;
            }
        }

        if(last_event_api_poll >= API_EVENT_POLL_PERIOD)
        {
            last_event_api_poll = 0;
            if(!runstate_state_token.isNull() && !isTokenValid(runstate_state_token))
                runstate_state_token = StateToken(); // must get new state with new token

            if(!password_state_token.isNull() && !isTokenValid(password_state_token))
                password_state_token = StateToken();
        }

        bool edge = false;
        if(runstate_state_token.isNull())
        {
            edge = true;
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
        }
        if(password_state_token.isNull())
        {
            edge = true;
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
            password_state_token = resp.mStateToken;
        }

        if(!ask_for_password && edge && runstate == "waiting_account_select")
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
            std::cout << "Type a number to select an account" << std::endl;
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
                std::cout << "[" << i << "] " << name << "(" << location << ")" << std::endl;
                accounts.push_back(id);
            }
        }

        if(!ask_for_password && runstate == "waiting_account_select"
                && last_char >= '0' && last_char <= '9'
                && (last_char-'0') < accounts.size())
        {
            std::string acc = accounts[last_char-'0'];
            JsonStream reqs;
            JsonStream resps;
            Request req(reqs);
            std::stringstream ss;
            Response resp(resps, ss);

            req.mPath.push("login");
            req.mPath.push("control");
            reqs << makeKeyValueReference("id", acc);
            reqs.switchToDeserialisation();

            ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
            waitForResponse(id);

            inbuf.clear();
        }

        if(edge && ask_for_password)
        {
            std::cout << "Enter the password for key " << key_name << std::endl;
        }

        if(ask_for_password && enter_was_pressed && !inbuf.empty())
        {
            std::cout << "TerminalApiClient: got a password" << std::endl;
            JsonStream reqs;
            JsonStream resps;
            Request req(reqs);
            std::stringstream ss;
            Response resp(resps, ss);

            req.mPath.push("password");
            req.mPath.push("control");
            reqs << makeKeyValueReference("password", inbuf);
            reqs.switchToDeserialisation();

            ApiServer::RequestId id = mApiServer->handleRequest(req, resp);
            waitForResponse(id);

            inbuf.clear();
        }
    }
}

void TerminalApiClient::waitForResponse(ApiServer::RequestId id)
{
    while(!mApiServer->isRequestDone(id))
          usleep(20*1000);
}

bool TerminalApiClient::isTokenValid(StateToken runstate_state_token)
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

} // namespace resource_api
