#include "RsControlModule.h"

#include <sstream>
#include <unistd.h>

#include <retroshare/rsinit.h>
#include <retroshare/rsiface.h>

#include "api/ApiServer.h"
#include "api/Operators.h"
#include "api/StateTokenServer.h"

#include "GetPluginInterfaces.h"

namespace resource_api{

RsControlModule::RsControlModule(int argc, char **argv, StateTokenServer* sts, ApiServer *apiserver, bool full_control):
    mStateTokenServer(sts),
    mApiServer(apiserver),
    mExitFlagMtx("RsControlModule::mExitFlagMtx"),
    mProcessShouldExit(false),
    mDataMtx("RsControlModule::mDataMtx"),
    mRunState(WAITING_INIT),
    mAutoLoginNextTime(false),
    mWantPassword(false)
{
    mStateToken = sts->getNewToken();
    this->argc = argc;
    this->argv = argv;
    // start worker thread
    if(full_control)
        start();
    else
        mRunState = RUNNING_OK;

    addResourceHandler("runstate", this, &RsControlModule::handleRunState);
    addResourceHandler("identities", this, &RsControlModule::handleIdentities);
    addResourceHandler("locations", this, &RsControlModule::handleLocations);
    addResourceHandler("password", this, &RsControlModule::handlePassword);
    addResourceHandler("login", this, &RsControlModule::handleLogin);
    addResourceHandler("shutdown", this, &RsControlModule::handleShutdown);
}

RsControlModule::~RsControlModule()
{
    join();
}

bool RsControlModule::processShouldExit()
{
    RsStackMutex stack(mExitFlagMtx);
    return mProcessShouldExit;
}

bool RsControlModule::askForPassword(const std::string &key_details, bool prev_is_bad, std::string &password)
{
    {
        RsStackMutex stack(mDataMtx); // ********** LOCKED **********
        mWantPassword = true;
        mKeyName = key_details;
        mPassword = "";
        mStateTokenServer->replaceToken(mStateToken);
    }

    bool wait = true;
    while(wait)
    {
        usleep(5*1000);

        RsStackMutex stack(mDataMtx); // ********** LOCKED **********
        wait = mWantPassword;
        if(!wait && mPassword != "")
        {
            password = mPassword;
            mPassword = "";
            mWantPassword = false;
            mStateTokenServer->replaceToken(mStateToken);
            return true;
        }
    }
    return false;
}

void RsControlModule::run()
{
    std::cerr << "RsControlModule: initialising libretroshare..." << std::endl;

    RsInit::InitRsConfig();
    int initResult = RsInit::InitRetroShare(argc, argv, true);

    if (initResult < 0) {
        std::cerr << "RsControlModule: FATAL ERROR, initialising libretroshare FAILED." << std::endl;
        /* Error occured */
        std::stringstream ss;
        switch (initResult) {
        case RS_INIT_AUTH_FAILED:
            ss << "RsInit::InitRetroShare AuthGPG::InitAuth failed" << std::endl;
            break;
        default:
            /* Unexpected return code */
            ss << "RsInit::InitRetroShare unexpected return code " << initResult << std::endl;
            break;
        }
        // FATAL ERROR, we can't recover from this. Just send the message to the user.
        setRunState(FATAL_ERROR, ss.str());
        return;
    }

    // This is needed to allocate rsNotify, so that it can be used to ask for PGP passphrase
    RsControl::earlyInitNotificationSystem();
    rsNotify->registerNotifyClient(this);

    bool login_ok = false;
    while(!login_ok)
    {
        // skip account selection if autologin is available
        if(initResult != RS_INIT_HAVE_ACCOUNT)
            setRunState(WAITING_ACCOUNT_SELECT);

        // wait for login request
        bool auto_login = false;
        bool wait_for_account_select = (initResult != RS_INIT_HAVE_ACCOUNT);
        while(wait_for_account_select && !processShouldExit())
        {
            usleep(5*1000);
            RsStackMutex stack(mDataMtx); // ********** LOCKED **********
            wait_for_account_select = mLoadPeerId.isNull();
            auto_login = mAutoLoginNextTime;
            if(!wait_for_account_select)
            {
                wait_for_account_select = !RsAccounts::SelectAccount(mLoadPeerId);
                if(wait_for_account_select)
                    setRunState(WAITING_ACCOUNT_SELECT);
            }
        }

        if(processShouldExit())
            return;

        bool autoLogin = (initResult == RS_INIT_HAVE_ACCOUNT) | auto_login;
        std::string lockFile;
        int retVal = RsInit::LockAndLoadCertificates(autoLogin, lockFile);

        std::string error_string;
        switch (retVal) {
        case 0:
            login_ok = true;
            break;
        case 1:
            error_string = "Another RetroShare using the same profile is "
                           "already running on your system. Please close "
                           "that instance first\n Lock file:\n" + lockFile;
            break;
        case 2:
            error_string = "An unexpected error occurred when Retroshare "
                           "tried to acquire the single instance lock\n Lock file:\n"
                           + lockFile;
            break;
        case 3:
            error_string = "Login Failure: Maybe password is wrong";
            break;
        default:
            std::cerr << "RsControlModule::run() LockAndLoadCertificates failed. Unexpected switch value: " << retVal << std::endl;
            break;
        }
    }

    setRunState(WAITING_STARTUP);

    std::cerr << "RsControlModule: login ok, starting Retroshare worker threads..." << std::endl;
    RsControl::instance() -> StartupRetroShare();

    std::cerr << "RsControlModule: loading main resource api modules..." << std::endl;
    RsPlugInInterfaces ifaces;
    getPluginInterfaces(ifaces);
    mApiServer->loadMainModules(ifaces);

    std::cerr << "RsControlModule: Retroshare is up and running. Enjoy!" << std::endl;
    setRunState(RUNNING_OK);

    while(!processShouldExit())
    {
        usleep(5*1000);
    }

    std::cerr << "RsControlModule: stopping Retroshare..." << std::endl;
    RsControl::instance() -> rsGlobalShutDown();
    std::cerr << "RsControlModule: Retroshare stopped. Bye!" << std::endl;
}

void RsControlModule::handleRunState(Request &req, Response &resp)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    std::string state;
    switch(mRunState)
    {
    case WAITING_INIT:
        state = "waiting_init";
        break;
    case FATAL_ERROR:
        state = "fatal_error";
        break;
    case WAITING_ACCOUNT_SELECT:
        state = "waiting_account_select";
        break;
    case WAITING_STARTUP:
        state = "waiting_startup";
        break;
    case RUNNING_OK:
        state = "running_ok";
        break;
    default:
        state = "error_should_not_happen_this_is_a_bug";
    }
    resp.mDataStream << makeKeyValueReference("runstate", state);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handleIdentities(Request &req, Response &resp)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    if(mRunState == WAITING_INIT || mRunState == FATAL_ERROR)
    {
        resp.setFail("Retroshare is not initialised. Operation not possible.");
        return;
    }

    std::list<RsPgpId> pgp_ids;
    RsAccounts::GetPGPLogins(pgp_ids);
    resp.mDataStream.getStreamToMember();
    for(std::list<RsPgpId>::iterator lit = pgp_ids.begin(); lit != pgp_ids.end(); ++lit)
    {
        std::string name;
        std::string email;
        if(RsAccounts::GetPGPLoginDetails(*lit, name, email))
            resp.mDataStream.getStreamToMember()
                    << makeKeyValueReference("id", *lit)
                    << makeKeyValueReference("pgp_id", *lit)
                    << makeKeyValueReference("name", name);
    }
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handleLocations(Request &req, Response &resp)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    if(mRunState == WAITING_INIT || mRunState == FATAL_ERROR)
    {
        resp.setFail("Retroshare is not initialised. Operation not possible.");
        return;
    }

    RsPeerId preferedId;
    RsAccounts::GetPreferredAccountId(preferedId);

    std::list<RsPeerId> peer_ids;
    RsAccounts::GetAccountIds(peer_ids);
    resp.mDataStream.getStreamToMember();
    for(std::list<RsPeerId>::iterator lit = peer_ids.begin(); lit != peer_ids.end(); ++lit)
    {
        bool preferred = preferedId==*lit;
        RsPgpId pgp_id;
        std::string pgp_name, pgp_mail, location_name;
        if(RsAccounts::GetAccountDetails(*lit, pgp_id, pgp_name, pgp_mail, location_name))
            resp.mDataStream.getStreamToMember()
                    << makeKeyValueReference("id", *lit)
                    << makeKeyValueReference("pgp_id", pgp_id)
                    << makeKeyValueReference("peer_id", *lit)
                    << makeKeyValueReference("name", pgp_name)
                    << makeKeyValueReference("location", location_name)
                    << makeKeyValueReference("preferred", preferred);
    }
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handlePassword(Request &req, Response &resp)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    std::string passwd;
    req.mStream << makeKeyValueReference("password", passwd);
    if(passwd != "" && mWantPassword)
    {
        // client sends password
        mPassword = passwd;
        mWantPassword = false;
        mStateTokenServer->replaceToken(mStateToken);
    }

    resp.mDataStream
            << makeKeyValueReference("want_password", mWantPassword)
            << makeKeyValueReference("key_name", mKeyName);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handleLogin(Request &req, Response &resp)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    if(mRunState != WAITING_ACCOUNT_SELECT)
    {
        resp.setFail("Operation not allowed in this runstate. Login is only allowed rigth after initialisation.");
        return;
    }
    req.mStream << makeKeyValueReference("id", mLoadPeerId)
                << makeKeyValueReference("autologin", mAutoLoginNextTime);
    resp.setOk();
}

void RsControlModule::handleShutdown(Request &req, Response &resp)
{
    RsStackMutex stack(mExitFlagMtx); // ********** LOCKED **********
    mProcessShouldExit = true;
    resp.setOk();
}

void RsControlModule::setRunState(RunState s, std::string errstr)
{
    RsStackMutex stack(mDataMtx); // ********** LOCKED **********
    mRunState = s;
    mLastErrorString = errstr;
    mStateTokenServer->replaceToken(mStateToken);
}


} // namespace resource_api
