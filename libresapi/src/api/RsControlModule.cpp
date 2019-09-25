/*******************************************************************************
 * libresapi/api/RsControlModule.cpp                                           *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#include "RsControlModule.h"

#include <sstream>
#include <unistd.h>
#include <cstdio>

#include <retroshare/rsinit.h>
#include <retroshare/rsiface.h>
#include <util/rsdir.h>

#include "api/ApiServer.h"
#include "api/Operators.h"
#include "api/StateTokenServer.h"

#include "GetPluginInterfaces.h"

//#define DEBUG_CONTROL_MODULE 1

namespace resource_api{

RsControlModule::RsControlModule(int argc, char **argv, StateTokenServer* sts, ApiServer *apiserver, bool full_control):
    mStateTokenServer(sts),
    mApiServer(apiserver),
    mExitFlagMtx("RsControlModule::mExitFlagMtx"),
    mProcessShouldExit(false),
    mDataMtx("RsControlModule::mDataMtx"),
    mRunState(WAITING_INIT),
    mAutoLoginNextTime(false),
    mWantPassword(false),
    mPrevIsBad(false),
    mCountAttempts(0),
    mPassword("")
{
    mStateToken = sts->getNewToken();
    this->argc = argc;
    this->argv = argv;
    // start worker thread
    if(full_control)
        start("resapi ctrl mod");
    else
        mRunState = RUNNING_OK_NO_FULL_CONTROL;

    addResourceHandler("runstate", this, &RsControlModule::handleRunState);
    addResourceHandler("identities", this, &RsControlModule::handleIdentities);
    addResourceHandler("locations", this, &RsControlModule::handleLocations);
    addResourceHandler("password", this, &RsControlModule::handlePassword);
    addResourceHandler("login", this, &RsControlModule::handleLogin);
    addResourceHandler("shutdown", this, &RsControlModule::handleShutdown);
    addResourceHandler("import_pgp", this, &RsControlModule::handleImportPgp);
    addResourceHandler("create_location", this, &RsControlModule::handleCreateLocation);
}

RsControlModule::~RsControlModule()
{
//        join();
}

bool RsControlModule::processShouldExit()
{
	RS_STACK_MUTEX(mExitFlagMtx); // ********** LOCKED **********
    return mProcessShouldExit;
}

bool RsControlModule::askForPassword(const std::string &title, const std::string &key_details, bool prev_is_bad, std::string &password, bool& cancelled)
{
#ifdef DEBUG_CONTROL_MODULE
	std::cerr << "RsControlModule::askForPassword(): current passwd is \"" << mPassword << "\"" << std::endl;
#endif
	cancelled = false ;
    {
		RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********

		mCountAttempts++;
		if(mCountAttempts == 3)
		{
			mPrevIsBad = prev_is_bad;
			mCountAttempts = 0;
		}
		else
			mPrevIsBad = false;

        if(mFixedPassword != "")
		{
            password = mFixedPassword;
            return true;
        }

        mWantPassword = true;
        mTitle = title;
        mKeyName = key_details;

		if(mPassword != "")
		{
			password = mPassword;
			mWantPassword = false;
			return true;
		}

        mStateTokenServer->replaceToken(mStateToken);
    }

	int i = 0;
	while(i<100)
    {
        usleep(5*1000);
		RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********

		if(mPassword != "")
        {
			password = mPassword;
            mWantPassword = false;
            mStateTokenServer->replaceToken(mStateToken);
            return true;
        }
		i++;
    }
    return false;
}

void RsControlModule::run()
{
#ifdef DEBUG_CONTROL_MODULE
    std::cerr << "RsControlModule: initialising libretroshare..." << std::endl;
#endif

    RsInit::InitRsConfig();
    RsConfigOptions opt;
    int initResult = RsInit::InitRetroShare(opt);

    if (initResult < 0) {
        std::cerr << "RsControlModule: FATAL ERROR, initialising libretroshare FAILED." << std::endl;
        /* Error occured */
        std::stringstream ss;
        switch (initResult) {
        case RS_INIT_AUTH_FAILED:
            ss << "RsControlModule::run() AuthGPG::InitAuth failed" << std::endl;
            break;
        default:
            /* Unexpected return code */
            ss << "ControlModule::run() unexpected return code " << initResult << std::endl;
            break;
        }
        // FATAL ERROR, we can't recover from this. Just send the message to the user.
        setRunState(FATAL_ERROR, ss.str());
        return;
    }

    // This is needed to allocate rsNotify, so that it can be used to ask for PGP passphrase
    RsControl::earlyInitNotificationSystem();
    rsNotify->registerNotifyClient(this);

#ifdef DEBUG_CONTROL_MODULE
	std::cerr << "RsControlModule::run() Entering login wait loop." << std::endl;
#endif
    bool login_ok = false;
    while(!login_ok)
    {
#ifdef DEBUG_CONTROL_MODULE
		std::cerr << "RsControlModule::run() reseting passwd." << std::endl;
#endif
		{
			RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
			mPassword = "";
		}

        // skip account selection if autologin is available
		bool wait_for_account_select = (initResult != RS_INIT_HAVE_ACCOUNT);

        // wait for login request
        bool auto_login = false;

		if(wait_for_account_select)
		{
#ifdef DEBUG_CONTROL_MODULE
			std::cerr << "RsControlModule::run() wait_for_account_select=true => setting run state to WAITING_ACCOUNT_SELECT." << std::endl;
#endif
			setRunState(WAITING_ACCOUNT_SELECT);
		}

        while(wait_for_account_select && !processShouldExit())
        {
#ifdef DEBUG_CONTROL_MODULE
			std::cerr << "RsControlModule::run() while(wait_for_account_select) mLoadPeerId=" << mLoadPeerId << std::endl;
#endif
            usleep(500*1000);
			RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********

			if(!mLoadPeerId.isNull())
			{
                wait_for_account_select = wait_for_account_select && !RsAccounts::SelectAccount(mLoadPeerId);
#ifdef DEBUG_CONTROL_MODULE
				std::cerr << "RsControlModule::run() mLoadPeerId != NULL, account selection result: " << !wait_for_account_select << std::endl;
#endif
			}

            auto_login = mAutoLoginNextTime;

            //if(!wait_for_account_select)
            //{
            //    if(wait_for_account_select)
            //        setRunState(WAITING_ACCOUNT_SELECT);
            //}
        }

        if(processShouldExit())
            return;

        bool autoLogin = (initResult == RS_INIT_HAVE_ACCOUNT) | auto_login;
        std::string lockFile;
#ifdef DEBUG_CONTROL_MODULE
		std::cerr << "RsControlModule::run() trying to load certificate..." << std::endl;
#endif
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
#ifdef DEBUG_CONTROL_MODULE
		std::cerr << "RsControlModule::run() Error string: \"" << error_string << "\"" << std::endl;
#endif

		{
			RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
			mLoadPeerId.clear();
		}
    }
#ifdef DEBUG_CONTROL_MODULE
	std::cerr << "RsControlModule::run() login is ok. Starting up..." << std::endl;
#endif

	{
		RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
		mFixedPassword = mPassword;

		std::cerr << "***Reseting mPasswd " << std::endl;
		mPassword = "";
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

void RsControlModule::handleRunState(Request &, Response &resp)
{
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
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
    case RUNNING_OK_NO_FULL_CONTROL:
        state = "running_ok_no_full_control";
        break;
    default:
        state = "error_should_not_happen_this_is_a_bug";
    }
    resp.mDataStream << makeKeyValueReference("runstate", state);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handleIdentities(Request &, Response &resp)
{
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
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

void RsControlModule::handleLocations(Request &, Response &resp)
{
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
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
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
    std::string passwd;
    req.mStream << makeKeyValueReference("password", passwd);
    if(passwd != "")// && mWantPassword)
    {
        // client sends password
        mPassword = passwd;
        mWantPassword = false;
        mStateTokenServer->replaceToken(mStateToken);
#ifdef DEBUG_CONTROL_MODULE
		std::cerr << "RsControlModule::handlePassword(): setting mPasswd=\"" << mPassword <<  "\"" << std::endl;
#endif
    }
#ifdef DEBUG_CONTROL_MODULE
	else
		std::cerr << "RsControlModule::handlePassword(): not setting mPasswd=\"" << mPassword <<  "\"!!!" << std::endl;
#endif

    resp.mDataStream
            << makeKeyValueReference("want_password", mWantPassword)
	        << makeKeyValueReference("key_name", mKeyName)
	        << makeKeyValueReference("prev_is_bad", mPrevIsBad);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void RsControlModule::handleLogin(Request &req, Response &resp)
{
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
    if(mRunState != WAITING_ACCOUNT_SELECT)
    {
        resp.setFail("Operation not allowed in this runstate. Login is only allowed rigth after initialisation.");
        return;
    }
    req.mStream << makeKeyValueReference("id", mLoadPeerId)
                << makeKeyValueReference("autologin", mAutoLoginNextTime);
    resp.setOk();
}

void RsControlModule::handleShutdown(Request &, Response &resp)
{
	requestShutdown();
	resp.setOk();
}

void RsControlModule::handleImportPgp(Request &req, Response &resp)
{
    std::string key_string;
    req.mStream << makeKeyValueReference("key_string", key_string);

    if(key_string.empty())
    {
        resp.setFail("required field key_string is empty");
        return;
    }

    RsPgpId pgp_id;
    std::string error_string;
    if(RsAccounts::importIdentityFromString(key_string, pgp_id, error_string))
    {
        resp.mDataStream << makeKeyValueReference("pgp_id", pgp_id);
        resp.setOk();
        return;
    }

    resp.setFail("Failed to import key: " + error_string);
}

void RsControlModule::handleCreateLocation(Request &req, Response &resp)
{
    std::string hidden_address;
    std::string hidden_port_str;
    req.mStream << makeKeyValueReference("hidden_adress", hidden_address)
                << makeKeyValueReference("hidden_port", hidden_port_str);
    uint16_t hidden_port = 0;
    bool auto_tor = false ;		// to be set by API, so disabled until then.

    if(hidden_address.empty() != hidden_port_str.empty())
    {
        resp.setFail("you must both specify string hidden_adress and string hidden_port to create a hidden node.");
        return;
    }
    if(!hidden_address.empty())
    {
        int p;
        if(sscanf(hidden_port_str.c_str(), "%i", &p) != 1)
        {
            resp.setFail("failed to parse hidden_port, not a number. Must be a dec or hex number.");
            return;
        }
        if(p < 0 || p > 0xFFFF)
        {
            resp.setFail("hidden_port out of range. It must fit into uint16!");
            return;
        }
        hidden_port = static_cast<uint16_t>(p);
    }

    RsPgpId pgp_id;
    std::string pgp_name;
    std::string pgp_password;
    std::string ssl_name;

    req.mStream << makeKeyValueReference("pgp_id", pgp_id)
                << makeKeyValueReference("pgp_name", pgp_name)
                << makeKeyValueReference("pgp_password", pgp_password)
                << makeKeyValueReference("ssl_name", ssl_name);

    if(pgp_password.empty())
    {
        resp.setFail("Error: pgp_password is empty.");
        return;
    }

    // pgp_id is set: use existing pgp key
    // pgp_name is set: attempt to create a new key
    if(pgp_id.isNull() && (pgp_name.empty()||pgp_password.empty()))
    {
        resp.setFail("You have to set pgp_id to use an existing key, or pgp_name and pgp_password to create a new pgp key.");
        return;
    }
    if(pgp_id.isNull())
    {
        std::string err_string;
        if(!RsAccounts::GeneratePGPCertificate(pgp_name, "", pgp_password, pgp_id, 2048, err_string))
        {
            resp.setFail("could not create pgp key: "+err_string);
            return;
        }
    }

	if(hidden_port) {
		/// TODO add bob to webui
		RsInit::SetHiddenLocation(hidden_address, hidden_port, false);
	}

    std::string ssl_password = RSRandom::random_alphaNumericString(static_cast<uint32_t>(RsInit::getSslPwdLen())) ;

    /* GenerateSSLCertificate - selects the PGP Account */
    //RsInit::SelectGPGAccount(PGPId);

    RsPeerId ssl_id;
    std::string err_string;
    // give the password to the password callback
	{
		RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
		mPassword = pgp_password;
		mFixedPassword = pgp_password;
	}
    bool ssl_ok = RsAccounts::createNewAccount(pgp_id, "", ssl_name, "", hidden_port!=0, auto_tor!=0, ssl_password, ssl_id, err_string);

    // clear fixed password to restore normal password operation
//    {
//        RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
//        mFixedPassword = "";
//    }

    if (ssl_ok)
    {
        // load ssl password and load account
        RsInit::LoadPassword(ssl_password);
        // trigger login in init thread
        {
			RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
            mLoadPeerId = ssl_id;
        }
        resp.mDataStream << makeKeyValueReference("pgp_id", pgp_id)
                         << makeKeyValueReference("pgp_name", pgp_name)
                         << makeKeyValueReference("ssl_name", ssl_name)
                         << makeKeyValueReference("ssl_id", ssl_id);
        resp.setOk();
        return;
    }
    resp.setFail("could not create a new location. Error: "+err_string);
}

bool RsControlModule::askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result, std::string reason /*=""*/)
{
	if(rsPeers->gpgSignData(data,len,sign,signlen,reason))
	{
		signature_result = SELF_SIGNATURE_RESULT_SUCCESS;
		return true;
	}
	else
	{
		signature_result = SELF_SIGNATURE_RESULT_FAILED;
		return false;
	}
}

void RsControlModule::requestShutdown()
{
	RS_STACK_MUTEX(mExitFlagMtx);
	mProcessShouldExit = true;
}

void RsControlModule::setRunState(RunState s, std::string errstr)
{
	RS_STACK_MUTEX(mDataMtx); // ********** LOCKED **********
    mRunState = s;
    mLastErrorString = errstr;
    mStateTokenServer->replaceToken(mStateToken);
}


} // namespace resource_api
