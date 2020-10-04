/*******************************************************************************
 * libretroshare/src/retroshare: rsinit.h                                      *
 *                                                                             *
 * Copyright (C) 2004-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@altermundi.net>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

/// @file RetroShare initialization and login API

// Initialize ok, result >= 0
#define RS_INIT_OK              0 // Initialize ok
#define RS_INIT_HAVE_ACCOUNT    1 // Initialize ok, have account
// Initialize failed, result < 0
#define RS_INIT_AUTH_FAILED    -1 // AuthGPG::InitAuth failed
#define RS_INIT_BASE_DIR_ERROR -2 // AuthGPG::InitAuth failed
#define RS_INIT_NO_KEYRING     -3 // Keyring is empty. Need to import it.
#define RS_INIT_NO_EXECUTABLE  -4 // executable path hasn't been set in config options

#include <list>
#include <map>
#include <vector>
#include <cstdint>
#include <system_error>

#include "retroshare/rstypes.h"
#include "retroshare/rsversion.h"


class RsLoginHelper;

/**
 * Pointer to global instance of RsLoginHelper
 * @jsonapi{development}
 */
extern RsLoginHelper* rsLoginHelper;


enum class RsInitErrorNum : int32_t
{
	ALREADY_LOGGED_IN         = 6000,
	CANT_ACQUIRE_LOCK         = 6001,
	INVALID_LOCATION_NAME     = 6002,
	PGP_NAME_OR_ID_NEEDED     = 6003,
	PGP_KEY_CREATION_FAILED   = 6004,
	SSL_KEY_CREATION_FAILED   = 6005,
	INVALID_SSL_ID            = 6006,
	LOGIN_FAILED              = 6007
};

struct RsInitErrorCategory: std::error_category
{
	const char* name() const noexcept override
	{ return "RetroShare init"; }

	std::string message(int ev) const override
	{
		switch (static_cast<RsInitErrorNum>(ev))
		{
		case RsInitErrorNum::ALREADY_LOGGED_IN:
			return "Already logged in";
		case RsInitErrorNum::CANT_ACQUIRE_LOCK:
			return "Cannot aquire lock on location data. Another instance is "
			        "already running with this profile?";
		case RsInitErrorNum::INVALID_LOCATION_NAME:
			return "Invalid location name";
		case RsInitErrorNum::PGP_NAME_OR_ID_NEEDED:
			return "Either PGP name or PGP id is needed";
		case RsInitErrorNum::PGP_KEY_CREATION_FAILED:
			return "Failure creating PGP key";
		case RsInitErrorNum::SSL_KEY_CREATION_FAILED:
			return "Failure creating SSL key";
		case RsInitErrorNum::INVALID_SSL_ID:
			return "Invalid SSL id";
		case RsInitErrorNum::LOGIN_FAILED:
			return "Generic login failure";
		default:
			return rsErrorNotInCategory(ev, name());
		}
	}

	const static RsInitErrorCategory instance;
};


namespace std
{
/** Register RsJsonApiErrorNum as an error condition enum, must be in std
 * namespace */
template<> struct is_error_condition_enum<RsInitErrorNum> : true_type {};
}

/** Provide RsInitErrorNum conversion to std::error_condition, must be in
 * same namespace of RsInitErrorNum */
inline std::error_condition make_error_condition(RsInitErrorNum e) noexcept
{
	return std::error_condition(
	            static_cast<int>(e), RsInitErrorCategory::instance );
};



/**
 * @brief The RsInitConfig struct
 * This class contains common configuration options, that executables using libretroshare may want to
 * set using e.g. commandline options. To be passed to RsInit::InitRetroShare().
 */
struct RsConfigOptions
{
	RsConfigOptions();

    // required

	std::string main_executable_path;/* this should be set to argv[0] */

    // Optional. Only change if needed.

	bool autoLogin;                  /* try auto-login */

	bool udpListenerOnly;			 /* only listen to udp */
    std::string forcedInetAddress; 	 /* inet address to use.*/
    uint16_t    forcedPort; 	     /* port to listen to */

	bool outStderr;
	int  debugLevel;
    std::string logfname;			/* output filename for log */

	std::string opModeStr;          /* operating mode. Acceptable values: "Full", "NoTurtle", "Gaming", "Minimal" */
	std::string optBaseDir;			/* base directory where to find profiles, etc */

	uint16_t    jsonApiPort;		/* port to use fo Json API */
	std::string jsonApiBindAddress; /* bind address for Json API */
};


/*!
 * Initialisation Class (not publicly disclosed to RsIFace)
 */
class RsInit
{
public:
	enum RS_DEPRECATED_FOR(RsInitErrorNum) LoadCertificateStatus : uint8_t
	{
		OK,                     /// Everything go as expected, no error occurred
		ERR_ALREADY_RUNNING,    /// Another istance is running already
		ERR_CANT_ACQUIRE_LOCK,  /// Another istance is already running?
		ERR_UNKNOWN              /// Unkown error, maybe password is wrong?
	};

	/* reorganised RsInit system */

	/*!
	 * PreLogin
	 * Call before init retroshare, initialises rsinitconfig's public attributes
	 */
	static void InitRsConfig();

	/*!
	 * Should be called to load up ssl cert and private key, and intialises gpg
	 * this must be called before accessing rsserver (e.g. ::startupretroshare)
	 * @param argc passed from executable
	 * @param argv commandline arguments passed to executable
	 * @param strictCheck set to true if you want rs to continue executing if
	 *	invalid argument passed and vice versa
	 * @return RS_INIT_...
	 */
	static int InitRetroShare(const RsConfigOptions&);

	static bool isPortable();
	static bool isWindowsXP();
	static bool collectEntropy(uint32_t bytes) ;

    /*!
     * \brief lockFilePath
     * \return
     * 		full path for the lock file. Can be used to warn the user about a non deleted lock that would prevent to start.
     */
	static std::string lockFilePath();

	/*
	 * Setup Hidden Location;
	 */
	static void SetHiddenLocation(const std::string& hiddenaddress, uint16_t port, bool useBob);

	static bool LoadPassword(const std::string& passwd) ;

	/*
	 * Final Certificate load. This can be called if:
	 * a) InitRetroshare() returns RS_INIT_HAVE_ACCOUNT -> autoLoad/password Set.
	 * b) or LoadPassword()
	 *
	 * This uses the preferredId from RsAccounts.
	 * This wrapper also locks the profile before finalising the login
	 */
	static LoadCertificateStatus LockAndLoadCertificates(
	        bool autoLoginNT, std::string& lockFilePath );

	// Post Login Options
	static bool getStartMinimised();

	static int getSslPwdLen();
	static bool getAutoLogin();
	static void setAutoLogin(bool autoLogin);
	static bool RsClearAutoLogin() ;

private:
	/** @brief Lock profile directory
	 * param[in] accountDir account directory to lock
	 * param[out] lockFilePath path of the created lock-file
	 */
	static LoadCertificateStatus LockConfigDirectory(
	        const std::string& accountDir, std::string& lockFilePath);

	/// @brief Unlock profile directory
	static void UnlockConfigDirectory();

	static int LoadCertificates(bool autoLoginNT);
};



/* Seperate static Class for dealing with Accounts */

class RsAccountsDetail;

class RsAccounts
{
public:
	/// Should be called once before everything else.
	static bool init(const std::string &opt_base_dir, int& error_code);

	/**
	 * @brief ConfigDirectory (usually ~/.retroshare) you can call this method
	 * even before initialisation (you can't with some other methods)
	 * @see RsAccountsDetail::PathBaseDirectory()
	 */
	static std::string ConfigDirectory();

	/**
	 * @brief Get current account id. Beware that an account may be selected
	 *	without actually logging in.
	 * @jsonapi{development,unauthenticated}
	 * @param[out] id storage for current account id
	 * @return false if account hasn't been selected yet, true otherwise
	 */
	static bool getCurrentAccountId(RsPeerId &id);

	/**
	 * @brief DataDirectory
	 * you can call this method even before initialisation (you can't with some other methods)
	 * @param check if set to true and directory does not exist, return empty string
	 * @return path where global platform independent files are stored, like bdboot.txt or webinterface files
	 */
	static std::string systemDataDirectory(bool check = true);
	static std::string PGPDirectory();

	/**
	 * @brief Get available PGP identities id list
	 * @jsonapi{development,unauthenticated}
	 * @param[out] pgpIds storage for PGP id list
	 * @return true on success, false otherwise
	 */
	static int GetPGPLogins(std::list<RsPgpId> &pgpIds);
	static int     GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email);
	static bool    GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString);

	/**
	 * @brief Export full encrypted PGP identity to file
	 * @jsonapi{development}
	 * @param[in] filePath path of certificate file
	 * @param[in] pgpId PGP id to export
	 * @return true on success, false otherwise
	 */
	static bool ExportIdentity( const std::string& filePath,
	                            const RsPgpId& pgpId );

	/**
	 * @brief Import full encrypted PGP identity from file
	 * @jsonapi{development,unauthenticated}
	 * @param[in] filePath path of certificate file
	 * @param[out] pgpId storage for the PGP fingerprint of the imported key
	 * @param[out] errorMsg storage for eventual human readable error message
	 * @return true on success, false otherwise
	 */
	static bool ImportIdentity(
	        const std::string& filePath, RsPgpId& pgpId, std::string& errorMsg );

	/**
	 * @brief Import full encrypted PGP identity from string
	 * @jsonapi{development,unauthenticated}
	 * @param[in] data certificate string
	 * @param[out] pgpId storage for the PGP fingerprint of the imported key
	 * @param[out] errorMsg storage for eventual human readable error message
	 * @return true on success, false otherwise
	 */
	static bool importIdentityFromString(
	        const std::string& data, RsPgpId& pgpId,
	        std::string& errorMsg );

	/**
	 * @brief Export full encrypted PGP identity to string
	 * @jsonapi{development}
	 * @param[out] data storage for certificate string
	 * @param[in] pgpId PGP id to export
	 * @param[in] includeSignatures true to include signatures
	 * @param[out] errorMsg storage for eventual human readable error message
	 * @return true on success, false otherwise
	 */
	static bool exportIdentityToString(
	        std::string& data, const RsPgpId& pgpId, std::string& errorMsg,
	        bool includeSignatures = true );

	static void    GetUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys);
	static bool    CopyGnuPGKeyrings() ;

	// Rs Accounts
	static bool SelectAccount(const RsPeerId& id);
	static bool	GetPreferredAccountId(RsPeerId &id);
	static bool GetAccountIds(std::list<RsPeerId> &ids);

	static bool	GetAccountDetails(const RsPeerId &id, RsPgpId &gpgId, std::string &gpgName, std::string &gpgEmail, std::string &location);

	static bool createNewAccount(
	        const RsPgpId& pgp_id, const std::string& org,
	        const std::string& loc, const std::string& country,
	        bool ishiddenloc, bool is_auto_tor, const std::string& passwd,
	        RsPeerId &sslId, std::string &errString );

    static void storeSelectedAccount() ;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                 All methods bellow can only be called ones SelectAccount() as been called.                                       //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool getCurrentAccountOptions(bool& is_hidden,bool& is_tor_auto,bool& is_first_time) ;

	static bool checkCreateAccountDirectory();		// Generate the hierarchy of directories below ~/.retroshare/[SSL dir]/
    static bool isHiddenNode() ;                    // true if the running node is a hidden node. Used to choose which services to start.
    static bool isTorAuto() ;                       // true if the running node is a hidden node using automated Tor management

	static std::string AccountDirectory();			// linux: ~/.retroshare/[SSL dir]/
	static std::string AccountKeysDirectory();		// linux: ~/.retroshare/[SSL dir]/keys/
	static std::string AccountPathCertFile();		// linux: ~/.retroshare/[SSL dir]/keys/user_cert.pem
	static std::string AccountPathKeyFile();		// linux: ~/.retroshare/[SSL dir]/keys/user_pk.pem
	static std::string AccountLocationName();

    static bool lockPreferredAccount() ;	// are these methods any useful??
    static void unlockPreferredAccount() ;

private:
	static RsAccountsDetail* rsAccountsDetails;
};

/**
 * Pointer to global instance of RsAccounts needed to expose JSON API, as all
 * the members of this class are static you should call them directly without
 * using this pointer in the other parts of the code
 * @jsonapi{development}
 */
extern RsAccounts* rsAccounts;


/**
 * This helper class have been implemented because there was not reasonable way
 * to login in the API that could be exposed via JSON API
 */
class RsLoginHelper
{
public:
	RsLoginHelper() = default;

	/**
	 * @brief Normal way to attempt login
	 * @jsonapi{development,unauthenticated}
	 * @param[in] account Id of the account to which attempt login
	 * @param[in] password Password for the given account
	 * @return RsInit::OK if login attempt success, error code otherwhise
	 */
	RsInit::LoadCertificateStatus attemptLogin(
	        const RsPeerId& account, const std::string& password );

	/**
	 * @brief Feed extra entropy to the crypto libraries
	 * @jsonapi{development,unauthenticated}
	 * @param[in] bytes number to feed to the entropy pool
	 * @return false if error occurred, true otherwise
	 */
	static bool collectEntropy(uint32_t bytes);

	struct Location : RsSerializable
	{
		RsPeerId mLocationId;
		RsPgpId mPgpId;
		std::string mLocationName;
		std::string mPgpName;

		/// @see RsSerializable::serial_process
		void serial_process( RsGenericSerializer::SerializeJob j,
		                     RsGenericSerializer::SerializeContext& ctx );
	};

	/**
	 * @brief Get locations and associated information
	 * @jsonapi{development,unauthenticated}
	 * @param[out] locations storage for the retrived locations
	 */
	void getLocations(std::vector<RsLoginHelper::Location>& locations);

	/**
	 * @brief Creates a new RetroShare location, and log in once is created
	 * @jsonapi{development,manualwrapper}
	 * @param[out] locationId storage for generated location SSL id
	 * @param[inout] pgpId specify PGP id to use to sign the location, if a null
	 *	id is passed the PGP key is created too and this param is used as
	 *	storage for its id.
	 * @param[in] password to protect and unlock the associated PGP key
	 * param[in] apiUser (JSON API only) string containing username for JSON API
	 *	so it can be later used to authenticate JSON API calls. It is passed
	 *	down to @see RsJsonApi::authorizeUser under the hood.
	 * param[in] apiPass (JSON API only) string containing password for JSON API
	 *	so it can be later used to authenticate JSON API calls. It is passed
	 *	down to @see RsJsonApi::authorizeUser under the hood.
	 *	To improve security we strongly advise to not use the same as the
	 *	password used for the PGP key.
	 * @return Success or error information
	 */
	std::error_condition createLocationV2(
	        RsPeerId& locationId,
	        RsPgpId& pgpId,
	        const std::string& locationName,
	        const std::string& pgpName,
	        const std::string& password
	        /* JSON API only
	         * const std::string& apiUser
	         * const std::string& apiPass */ );

	/**
	 * @brief Check if RetroShare is already logged in, this usually return true
	 *	after a successfull attemptLogin() and before closeSession()
	 * @jsonapi{development,unauthenticated}
	 * @return true if already logged in, false otherwise
	 */
	bool isLoggedIn();

#if !RS_VERSION_AT_LEAST(0,6,6)
	/**
	 * @deprecated Use @see createLocationV2 instead
	 * @brief Creates a new RetroShare location, and log in once is created
	 * @jsonapi{development,manualwrapper}
	 * @param[inout] location provide input information to generate the location
	 *	and storage to output the data of the generated location
	 * @param[in] password to protect and unlock the associated PGP key
	 * @param[out] errorMessage if some error occurred human readable error
	 *	message
	 * @param[in] makeHidden pass true to create an hidden location. UNTESTED!
	 * @param[in] makeAutoTor pass true to create an automatically configured
	 *	Tor hidden location. UNTESTED!
	 * @return true if success, false otherwise
	 */
	RS_DEPRECATED_FOR(createLocationV2)
	bool createLocation( RsLoginHelper::Location& location,
	                     const std::string& password, std::string& errorMessage,
	                     bool makeHidden = false, bool makeAutoTor = false );
#endif // !RS_VERSION_AT_LEAST(0,6,6)
};
