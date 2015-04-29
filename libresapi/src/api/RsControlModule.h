#pragma once

#include <util/rsthreads.h>
#include <retroshare/rsnotify.h>
#include "api/ResourceRouter.h"

namespace resource_api{

class StateTokenServer;
class ApiServer;

// resource api module to control accounts, startup and shutdown of retroshare
// - this module handles everything, not things are required from outside
// - exception: users of this module have to create an api server and register this module
// tasks:
// - show, import, export and create private pgp keys
// - show existing and create new locations
// - load certificate, startup retroshare
// - handle password callback
// - confirm plugin loading
// - shutdown retroshare
class RsControlModule: public ResourceRouter, NotifyClient,
        private RsThread
{
public:
    // ApiServer will be called once RS is started, to load additional api modules
    // full_control: set to true if this module should handle rsinit and login
    // set to false if rsinit is handled by the Qt gui
    RsControlModule(int argc, char **argv, StateTokenServer* sts, ApiServer* apiserver, bool full_control);
    ~RsControlModule();

    // returns true if the process should terminate
    bool processShouldExit();

    // from NotifyClient
    virtual bool askForPassword(const std::string& key_details, bool prev_is_bad , std::string& password);

protected:
    // from RsThread
    // wee need a thread to call into things which block like askForPassword()
    virtual void run();

private:
    enum RunState { WAITING_INIT, FATAL_ERROR, WAITING_ACCOUNT_SELECT, WAITING_STARTUP, RUNNING_OK, RUNNING_OK_NO_FULL_CONTROL};
    void handleRunState(Request& req, Response& resp);
    void handleIdentities(Request& req, Response& resp);
    void handleLocations(Request& req, Response& resp);
    void handlePassword(Request& req, Response& resp);
    void handleLogin(Request& req, Response& resp);
    void handleShutdown(Request& req, Response& resp);

    void setRunState(RunState s, std::string errstr = "");
    // for startup
    int argc;
    char **argv;

    StateTokenServer* const mStateTokenServer;
    ApiServer* const mApiServer;

    RsMutex mExitFlagMtx;
    bool mProcessShouldExit;

    RsMutex mDataMtx;

    StateToken mStateToken; // one state token for everything, to make life easier

    RunState mRunState;
    std::string mLastErrorString;

    // id of the account to load
    // null when no account was selected
    RsPeerId mLoadPeerId;
    bool mAutoLoginNextTime;

    // to notify that a password callback is waiting
    // to answer the request, clear the flag and set the password
    bool mWantPassword;
    std::string mKeyName;
    std::string mPassword;
};

} // namespace resource_api
